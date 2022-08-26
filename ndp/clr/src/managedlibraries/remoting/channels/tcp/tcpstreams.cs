// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    TcpStreams.cs
//
// Summary: Defines streams used by TCP channel.
//
//============================================================



using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Tcp
{

    internal abstract class TcpReadingStream : Stream
    {
        public void ReadToEnd()
        {
            // This method should never be called where it would be valid
            //   to use this data, so it is ok to throw the excess bytes
            //   away.
            byte[] buffer = new byte[64];
            int readCount;
            do
            {
                readCount = Read(buffer, 0, 64);
            } while (readCount > 0);
        }
    
        public virtual bool FoundEnd { get { return false; } }
        
        public override bool CanRead {  get { return true; } }
        public override bool CanSeek { get { return false; } }
        public override bool CanWrite { get { return false; } }
       
        public override long Length {  get { throw new NotSupportedException(); } }

        public override long Position
        {
             get{ throw new NotSupportedException(); }
             set{ throw new NotSupportedException(); }
        }

        public override  void Flush() { throw new NotSupportedException(); }
        
        public override  long Seek(long offset, SeekOrigin origin) { throw new NotSupportedException(); }
        public override  void SetLength(long value) { throw new NotSupportedException(); }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }        

    } // TcpReadingStream


    internal sealed class TcpFixedLengthReadingStream : TcpReadingStream
    { 
        private SocketHandler _inputStream; 
        private int _bytesLeft;   // bytes left to read

        internal TcpFixedLengthReadingStream(SocketHandler inputStream, 
                                             int contentLength)
        {
            _inputStream = inputStream;
            _bytesLeft = contentLength;
        } // TcpFixedLengthReadingStream

        public override bool FoundEnd { get { return _bytesLeft == 0; } }

        protected override void Dispose(bool disposing)
        {
            try {
                if (disposing)
                    _inputStream.OnInputStreamClosed();
            }
            finally {
                base.Dispose(disposing);
            }
        }
        
        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_bytesLeft == 0)
                return 0;
        
            int readCount = _inputStream.Read(buffer, offset, Math.Min(_bytesLeft, count));
            if (readCount > 0)
                _bytesLeft -= readCount;
            
            return readCount;
        } // Read

        public override int ReadByte()
        {
            if (_bytesLeft == 0)
                return -1;

            _bytesLeft -= 1;
            return _inputStream.ReadByte();
        } // ReadByte
                
    } // TcpFixedLengthReadingStream


    internal sealed class TcpChunkedReadingStream : TcpReadingStream
    {   
        private SocketHandler _inputStream = null; // read chunked tcp data from here
        
        private int    _bytesLeft;          // bytes left in current chunk
        private bool   _bFoundEnd = false;  // has end of stream been reached?

        private byte[] _byteBuffer = new byte[1]; // buffer for reading bytes
        
        
        internal TcpChunkedReadingStream(SocketHandler inputStream)
        {
            _inputStream = inputStream;
            
            _bytesLeft = 0;
        } // HttpChunkedRequestStream

        public override bool FoundEnd { get { return _bFoundEnd; } }

        protected override void Dispose(bool disposing)
        {
            try {
                // 
            }
            finally {
                base.Dispose(disposing);
            }
        } // Close
        
        public override int Read(byte[] buffer, int offset, int count)
        {
            int bytesRead = 0;
        
            while (!_bFoundEnd && (count > 0))
            {
                // see if we need to start reading a new chunk
                if (_bytesLeft == 0)
                {
                    _bytesLeft = _inputStream.ReadInt32();
                                        
                    if (_bytesLeft == 0)
                    {
                        ReadTrailer();                        
                        _bFoundEnd = true;
                    }
                }

                if (!_bFoundEnd)
                {
                    int readCount = Math.Min(_bytesLeft, count);
                    int bytesReadThisTime = _inputStream.Read(buffer, offset, readCount);
                    if (bytesReadThisTime <= 0)
                    {
                        throw new RemotingException(
                            CoreChannel.GetResourceString(
                                "Remoting_Tcp_ChunkedEncodingError"));
                    }
                    
                    _bytesLeft -= bytesReadThisTime;
                    count -= bytesReadThisTime;
                    offset += bytesReadThisTime;
                    bytesRead += bytesReadThisTime;
                
                    // see if the end of the chunk was found
                    if (_bytesLeft == 0)
                    {
                        ReadTrailer();
                    }
                }
            } // while (count > 0)  

            return bytesRead;
        } // Read        

        public override int ReadByte()
        {
            int readCount = Read(_byteBuffer, 0, 1);
            if (readCount == 0)
                return -1;
                
            return _byteBuffer[0];         
        } // ReadByte


        private void ReadTrailer()
        {
            // read trailer bytes "\r\n" and throw an exception if they aren't correct.
            int ch = _inputStream.ReadByte();
            if (ch != '\r')
            {
                throw new RemotingException(
                    CoreChannel.GetResourceString(
                        "Remoting_Tcp_ChunkedEncodingError"));
            }

            ch = _inputStream.ReadByte();
            if (ch != '\n')
            {
                throw new RemotingException(
                    CoreChannel.GetResourceString(
                        "Remoting_Tcp_ChunkedEncodingError"));
            }
        }
        
    } // TcpChunkedReadingStream

    


     // Maintains control of a socket connection.
    internal sealed class TcpServerSocketHandler : TcpSocketHandler
    {
        // prebaked bytes
        private static byte[] s_endOfLineBytes = Encoding.ASCII.GetBytes("\r\n");
        
    
        // Used to keep track of socket connections
        private static Int64 _connectionIdCounter = 0;        
        
        private Int64 _connectionId;   // id for this connection

        private bool _bOneWayRequest;  // is the incoming request one way?
        private bool _bChunked;        // is the incoming request chunked?
        private int  _contentLength;   // content length of incoming request

        TcpReadingStream _requestStream; // the request stream

        
        internal TcpServerSocketHandler(Socket socket, RequestQueue requestQueue, Stream stream) : 
                        base(socket, requestQueue, stream)
        {                 
            _connectionId = Interlocked.Increment(ref _connectionIdCounter);
        } // TcpServerSocketHandler


        // Determine if it's possible to service another request
        public bool CanServiceAnotherRequest()
        {
            return true;
        } // CanServiceAnotherRequest
        

        // Prepare for reading a new request off of the same socket
        protected override void PrepareForNewMessage()
        {
            if (_requestStream != null)
            {
                if (!_requestStream.FoundEnd)
                    _requestStream.ReadToEnd();
                _requestStream = null;
            }
        } // PrepareForNewRequest
            
        protected override void SendErrorMessageIfPossible(Exception e)
        {        
            // A fatal exception occurred. We communicate this error by
            // writing an error message and empty message body.
            try
            {
                SendErrorResponse(e, true);
            }
            catch
            {
                // the connection must be dead, so it doesn't really matter.
            }
        } // SendErrorMessageIfPossible
            

        // read headers
        public ITransportHeaders ReadHeaders()
        {        
            BaseTransportHeaders headers = new BaseTransportHeaders();

            UInt16 operation;
            ReadVersionAndOperation(out operation);

            // make sure the operation is Request or OneWayRequest.
            if (operation == TcpOperations.Request)
            {
                _bOneWayRequest = false;
            }
            else
            if (operation == TcpOperations.OneWayRequest)
            {
                _bOneWayRequest = true;
            }
            else
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_ExpectingRequestOp"),
                        operation.ToString(CultureInfo.CurrentCulture)));
            }            

            // content length must come next (may be chunked or a specific length)
            ReadContentLength(out _bChunked, out _contentLength);

            // read to end of headers  
            ReadToEndOfHeaders(headers);   
                           
            // add IP address and Connection Id to headers
            headers.IPAddress = ((IPEndPoint)NetSocket.RemoteEndPoint).Address;
            headers.ConnectionId = _connectionId;
            
            return headers;
        } // ReadHeaders


        public Stream GetRequestStream()
        {
            if (!_bChunked)
                _requestStream =  new TcpFixedLengthReadingStream(this, _contentLength);
            else
                _requestStream =  new TcpChunkedReadingStream(this);
            return _requestStream;
        } // GetRequestStream
      

        public void SendResponse(ITransportHeaders headers, Stream contentStream)
        {           
            // bail out if the original request was OneWay (means the client doesn't even
            //   want or expect to receive responses or error messages)
            if (_bOneWayRequest)
                return;            
        
            // build up headers and send      
            ChunkedMemoryStream headerStream = new ChunkedMemoryStream(CoreChannel.BufferPool);

            // output preamble and version
            WritePreambleAndVersion(headerStream);
            // output opcode
            WriteUInt16(TcpOperations.Reply, headerStream);
            // output content length delimiter
            WriteUInt16(TcpContentDelimiter.ContentLength, headerStream);
            WriteInt32((int)contentStream.Length, headerStream);

            // No status code header is needed because if we're in this code path
            //   the data transfer succeeded as far as the transport protocol is
            //   concerned (and the success status code is optional).

            WriteHeaders(headers, headerStream);
            
            headerStream.WriteTo(NetStream);
            headerStream.Close();

            StreamHelper.CopyStream(contentStream, NetStream);          
                         
            contentStream.Close();            
        } // SendResponse

        string GenerateFaultString(Exception e) {
            //If the user has specified it's a development server (versus a production server) in remoting config,
            //then we should just return e.ToString instead of extracting the list of messages.                        
            if (!CustomErrorsEnabled()) 
                return e.ToString();            
            else {                
                return CoreChannel.GetResourceString("Remoting_InternalError");                                                             
            }            
        }

        public void SendErrorResponse(Exception e, bool bCloseConnection)
        {
            SendErrorResponse(GenerateFaultString(e), bCloseConnection);
        }

        public void SendErrorResponse(string e, bool bCloseConnection)
        {
            // bail out if the original request was OneWay (means the client doesn't even
            //   want or expect to receive responses or error messages)
            if (_bOneWayRequest)
                return;
        
            // build up headers and send      
            ChunkedMemoryStream headerStream = new ChunkedMemoryStream(CoreChannel.BufferPool);

            // output preamble and version
            WritePreambleAndVersion(headerStream);
            // output opcode
            WriteUInt16(TcpOperations.Reply, headerStream);
            // output content length delimiter (0-length stream)
            WriteUInt16(TcpContentDelimiter.ContentLength, headerStream);
            WriteInt32(0, headerStream);

            // output status code and reason
            WriteUInt16(TcpHeaders.StatusCode, headerStream);
            WriteByte(TcpHeaderFormat.UInt16, headerStream);
            WriteUInt16(TcpStatusCode.GenericError, headerStream);
            // we purposely don't include the stack trace to avoid giving
            //   out too much information for security purposes.
            WriteUInt16(TcpHeaders.StatusPhrase, headerStream);
            WriteByte(TcpHeaderFormat.CountedString, headerStream);
            WriteCountedString(e, headerStream);

            // indicate that we are about to close the connection
            WriteUInt16(TcpHeaders.CloseConnection, headerStream);
            WriteByte(TcpHeaderFormat.Void, headerStream);

            // end of headers
            WriteUInt16(TcpHeaders.EndOfHeaders, headerStream);
            
            headerStream.WriteTo(NetStream);
            headerStream.Close();
        } // SendErrorResponse
               

    } // class TcpServerSocketHandler

    
    

} // namespace System.Runtime.Remoting.Channels.Tcp
