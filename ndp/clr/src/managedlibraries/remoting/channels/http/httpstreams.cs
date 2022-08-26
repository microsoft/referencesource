// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    HttpStreams.cs
//
// Summary: Defines streams used by HTTP channels
//
//============================================================

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting.Channels;
using System.Text;
using System.Threading;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Http
{
    internal abstract class HttpServerResponseStream : Stream
    {
        public override bool CanRead { get { return false; } }
        public override bool CanSeek { get { return false; } }
        public override bool CanWrite { get { return true; } }
        
        public override long Length  { get { throw new NotSupportedException(); } }

        public override long Position
        {
             get { throw new NotSupportedException(); }
             set { throw new NotSupportedException(); }
        }

        public override long Seek(long offset, SeekOrigin origin) { throw new NotSupportedException(); }
        public override void SetLength(long value) { throw new NotSupportedException(); }
        
        public override int Read(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }
    } // HttpServerResponseStream


    internal sealed class HttpFixedLengthResponseStream : HttpServerResponseStream
    {
        private Stream _outputStream = null; // funnel http data into here
        private static int _length;
    
        internal HttpFixedLengthResponseStream(Stream outputStream, int length)
        {
            _outputStream = outputStream;
            _length = length;
        } // HttpFixedLengthResponseStream


        protected override void Dispose(bool disposing)
        {
            try {
                if (disposing)
                    _outputStream.Flush();
            }
            finally {
                base.Dispose(disposing);
            }
        } // Close

        public override void Flush()
        {
            _outputStream.Flush();
        } // Flush

        public override void Write(byte[] buffer, int offset, int count)
        {            
            _outputStream.Write(buffer, offset, count);           
        } // Write

        public override void WriteByte(byte value)
        {
            _outputStream.WriteByte(value);
        } // WriteByte
    
    } // class HttpFixedLengthResponseStream
    

    internal sealed class HttpChunkedResponseStream : HttpServerResponseStream
    {
        private static byte[] _trailer = Encoding.ASCII.GetBytes("0\r\n\r\n"); // 0-length, no trailer, end chunked
        private static byte[] _endChunk = Encoding.ASCII.GetBytes("\r\n");
   
        private Stream _outputStream = null; // funnel chunked http data into here

        private byte[] _chunk;       // chunk of data to write
        private int    _chunkSize;   // size of chunk
        private int    _chunkOffset; // next byte to write in to chunk

        private byte[] _byteBuffer = new byte[1]; // buffer for writing bytes
        

        internal HttpChunkedResponseStream(Stream outputStream)
        {
            _outputStream = outputStream;

            _chunk = CoreChannel.BufferPool.GetBuffer();
            _chunkSize = _chunk.Length - 2; // reserve space for _endChunk directly in buffer
            _chunkOffset = 0;

            // write end chunk bytes at end of buffer (avoids extra socket write)
            _chunk[_chunkSize - 2] = (byte)'\r';
            _chunk[_chunkSize - 1] = (byte)'\n';
        } // HttpChunkedResponseStream
        

        protected override void Dispose(bool disposing)
        {
            try {
                if (disposing) {
                    if (_chunkOffset > 0)
                        FlushChunk();
        
                    _outputStream.Write(_trailer, 0, _trailer.Length);
                    _outputStream.Flush();
                }

                CoreChannel.BufferPool.ReturnBuffer(_chunk);
                _chunk = null;
            }
            finally {
                base.Dispose(disposing);
            }
        } // Close

        public override void Flush()
        {
            if (_chunkOffset > 0)
                FlushChunk();
            _outputStream.Flush();
        } // Flush

        public override void Write(byte[] buffer, int offset, int count)
        {
            while (count > 0)
            {
                if ((_chunkOffset == 0) && (count >= _chunkSize))
                {
                    // just write the rest as a chunk directly to the wire
                    WriteChunk(buffer, offset, count);
                    break;
                }
                else
                {
                    // write bytes to current chunk buffer
                    int writeCount = Math.Min(_chunkSize - _chunkOffset, count);
                    Array.Copy(buffer, offset, _chunk, _chunkOffset, writeCount);
                    _chunkOffset += writeCount;
                    count -= writeCount;
                    offset += writeCount;

                    // see if we need to terminate the chunk
                    if (_chunkOffset == _chunkSize)
                        FlushChunk();
                }
            }
        } // Write

        public override void WriteByte(byte value)
        {
            _byteBuffer[0] = value;
            Write(_byteBuffer, 0, 1);
        } // WriteByte

        private void FlushChunk()
        {
            WriteChunk(_chunk, 0, _chunkOffset);
            _chunkOffset = 0;
        }

        private void WriteChunk(byte[] buffer, int offset, int count)
        {
            byte[] size = IntToHexChars(count);

            _outputStream.Write(size, 0, size.Length);

            if (buffer == _chunk)
            {
                // _chunk already has end chunk encoding at end
                _outputStream.Write(_chunk, offset, count + 2);
            }
            else
            {
                _outputStream.Write(buffer, offset, count);
                _outputStream.Write(_endChunk, 0, _endChunk.Length);
            }
        } // WriteChunk
        
        
        private byte[] IntToHexChars(int i)
        {
            String str = "";

            while (i > 0)
            {
                int val = i % 16;

                switch (val)
                {
                    case 15: str = 'F' + str; break;
                    case 14: str = 'E' + str; break;
                    case 13: str = 'D' + str; break;
                    case 12: str = 'C' + str; break;
                    case 11: str = 'B' + str; break;
                    case 10: str = 'A' + str; break;

                    default: str = (char)(val + (int)'0') + str; break;
                }

                i = i / 16;
            }

            str += "\r\n";
             
            return Encoding.ASCII.GetBytes(str);
        } // IntToHexChars
        
    } // HttpChunkedResponseStream
    
    

    internal abstract class HttpReadingStream : Stream
    {
        public virtual bool ReadToEnd()
        {
            // This will never be called at a point where it is valid
            //   for someone to use the remaining data, so we don't
            //   need to buffer it.
            
            byte[] buffer = new byte[16];
            int bytesRead = 0;
            do
            {
               bytesRead = Read(buffer, 0, 16);
            } while (bytesRead > 0);

            return bytesRead == 0;
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

    } // HttpReadingStream


    internal sealed class HttpFixedLengthReadingStream : HttpReadingStream
    {    
        private HttpSocketHandler _inputStream = null;  // read content data from here     
        private int _bytesLeft;                               // bytes left in current chunk
       
        internal HttpFixedLengthReadingStream(HttpSocketHandler inputStream, int contentLength)
        {
            _inputStream = inputStream;
            _bytesLeft = contentLength;
        } // HttpFixedLengthReadingStream
        

        public override bool FoundEnd { get { return _bytesLeft == 0; } }

        protected override void Dispose(bool disposing)
        {
            try {
                // 
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
                
    } // HttpFixedLengthReadingStream



    // Stream class to read chunked data for HTTP
    //   (assumes that provided outputStream will be positioned for
    //    reading the body)    
    internal sealed class HttpChunkedReadingStream : HttpReadingStream
    {
        private static byte[] _trailer = Encoding.ASCII.GetBytes("0\r\n\r\n\r\n"); // 0-length, null trailer, end chunked
        private static byte[] _endChunk = Encoding.ASCII.GetBytes("\r\n");
   
        private HttpSocketHandler _inputStream = null; // read chunked http data from here
        
        private int    _bytesLeft;          // bytes left in current chunk
        private bool   _bFoundEnd = false;  // has end of stream been reached?

        private byte[] _byteBuffer = new byte[1]; // buffer for reading bytes
        
        
        internal HttpChunkedReadingStream(HttpSocketHandler inputStream)
        {
            _inputStream = inputStream;
            
            _bytesLeft = 0;
        } // HttpChunkedReadingStream

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
                    // this loop stops when the end of line is found
                    for (;;)
                    {
                        byte b = (byte)_inputStream.ReadByte();
                    
                        // see if this is the end of the length
                        if (b == '\r')
                        {          
                            // This had better be '\n'
                            if ((char)_inputStream.ReadByte() != '\n')
                            {
                                throw new RemotingException(
                                    CoreChannel.GetResourceString(
                                        "Remoting_Http_ChunkedEncodingError"));
                            }
                            else
                                break; // we've finished reading the length
                        }
                        else
                        {
                            int value = HttpChannelHelper.CharacterHexDigitToDecimal(b);
                            // make sure value is a hex-digit
                            if ((value < 0) || (value > 15))
                            {
                                throw new RemotingException(
                                    CoreChannel.GetResourceString(
                                        "Remoting_Http_ChunkedEncodingError"));
                            }

                            // update _bytesLeft value to account for new digit on the right
                            _bytesLeft = (_bytesLeft * 16) + value;
                        }
                    }
    
                    if (_bytesLeft == 0)
                    {
                        // read off trailing headers and end-line
                        String trailerHeader;
                        do
                        {
                            trailerHeader = _inputStream.ReadToEndOfLine();
                        } while (!(trailerHeader.Length == 0));
                        
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
                                "Remoting_Http_ChunkedEncodingError"));
                    }
                    
                    _bytesLeft -= bytesReadThisTime;
                    count -= bytesReadThisTime;
                    offset += bytesReadThisTime;
                    bytesRead += bytesReadThisTime;
                
                    // see if the end of the chunk was found
                    if (_bytesLeft == 0)
                    {
                        // read off "\r\n"
                        char ch = (char)_inputStream.ReadByte();
                        if (ch != '\r')
                        {
                            throw new RemotingException(
                                CoreChannel.GetResourceString(
                                    "Remoting_Http_ChunkedEncodingError"));
                        }
                        ch = (char)_inputStream.ReadByte();
                        if (ch != '\n')
                        {                      
                            throw new RemotingException(
                                CoreChannel.GetResourceString(
                                    "Remoting_Http_ChunkedEncodingError"));
                        }
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
        
    } // class HttpChunkedReadingStream

    
	[Serializable]
    internal enum HttpVersion
    {
        V1_0,
        V1_1
    } // HttpVersion


    // Maintains control of a socket connection.
    internal sealed class HttpServerSocketHandler : HttpSocketHandler
    {
        // Used to make sure verb characters are valid
        private static ValidateByteDelegate s_validateVerbDelegate =
            new ValidateByteDelegate(HttpServerSocketHandler.ValidateVerbCharacter);
    
        // Used to keep track of socket connections
        private static Int64 _connectionIdCounter = 0;        

        // primed buffer data
        private static byte[] _bufferhttpContinue = Encoding.ASCII.GetBytes("HTTP/1.1 100 Continue\r\n\r\n");
                
        // stream manager data
        private HttpReadingStream _requestStream = null;  // request stream we handed out.
        private HttpServerResponseStream _responseStream = null; // response stream we handed out.

        private Int64 _connectionId; // id for this connection

        // request state flags
        private HttpVersion _version; // http version used by client
        
        private int  _contentLength = 0;        // Content-Length value if found
        private bool _chunkedEncoding = false;  // does request stream use chunked encoding?
        private bool _keepAlive = false;        // does the client want to keep the connection alive?
        

        internal HttpServerSocketHandler(Socket socket, RequestQueue requestQueue, Stream stream) : base(socket, requestQueue, stream)
        {          
            _connectionId = Interlocked.Increment(ref _connectionIdCounter);
        } // HttpServerSocketHandler


        // 



        public bool AllowChunkedResponse { get { return false; } }


        // Determine if it's possible to service another request
        public bool CanServiceAnotherRequest()
        {
            if (_keepAlive && (_requestStream != null))           
            {
                if (_requestStream.FoundEnd || _requestStream.ReadToEnd())
                    return true;
            }

            return false;
        } // CanServiceAnotherRequest
        

        // Prepare for reading a new request off of the same socket
        protected override void PrepareForNewMessage()
        {
            _requestStream = null;
            _responseStream = null;
            
            _contentLength = 0;
            _chunkedEncoding = false;
            _keepAlive = false; 
        } // PrepareForNewRequest
            
        string GenerateFaultString(Exception e) {
            //If the user has specified it's a development server (versus a production server) in remoting config,
            //then we should just return e.ToString instead of extracting the list of messages.                        
            if (!CustomErrorsEnabled()) 
                return e.ToString();            
            else {                
                return CoreChannel.GetResourceString("Remoting_InternalError");                                                             
            }            
        }
            
        protected override void SendErrorMessageIfPossible(Exception e)
        {
            // If we haven't started sending a response back, we can do the following.
            if ((_responseStream == null) && !(e is SocketException))
            {
                Stream outputStream = new MemoryStream();
                StreamWriter writer = new StreamWriter(outputStream, new UTF8Encoding(false));
                writer.WriteLine(GenerateFaultString(e));
                writer.Flush();            
            
                SendResponse(outputStream, "500", CoreChannel.GetResourceString("Remoting_InternalError"), null);
            }                   
        } // SendErrorMessageIfPossible


        private static bool ValidateVerbCharacter(byte b)
        {
            if (Char.IsLetter((char)b) ||
                (b == '-'))
            {
                return true;
            }

            return false;
        } // ValidateVerbCharacter
            

        // read headers
        public BaseTransportHeaders ReadHeaders()
        {        
            bool bSendContinue = false;
        
            BaseTransportHeaders headers = new BaseTransportHeaders();

            // read first line
            String verb, requestURI, version;
            ReadFirstLine(out verb, out requestURI, out version);

            if ((verb == null) || (requestURI == null) || (version == null))
            {
                throw new RemotingException(
                    CoreChannel.GetResourceString(
                        "Remoting_Http_UnableToReadFirstLine"));
            }

            if (version.Equals("HTTP/1.1")) // most common case
                _version = HttpVersion.V1_1;
            else
            if (version.Equals("HTTP/1.0"))
                _version = HttpVersion.V1_0;
            else
                _version = HttpVersion.V1_1; // (assume it will understand 1.1)

            if (_version == HttpVersion.V1_1)
            {
                _keepAlive = true;
            }   
            else // it's a 1.0 client
            {
                _keepAlive = false;
            }
            

            // update request uri to be sure that it has no channel data
            String channelURI;
            String objectURI;
            channelURI = HttpChannelHelper.ParseURL(requestURI, out objectURI);
            if (channelURI == null)
            {
                objectURI = requestURI;
            }                        

            headers["__RequestVerb"] = verb;
            headers.RequestUri = objectURI;
            headers["__HttpVersion"] = version;

            // check to see if we must send continue
            if ((_version == HttpVersion.V1_1) &&
                (verb.Equals("POST") || verb.Equals("PUT")))
            {
                bSendContinue = true;
            }            

            ReadToEndOfHeaders(headers, out _chunkedEncoding, out _contentLength,
                               ref _keepAlive, ref bSendContinue);       

            if (bSendContinue && (_version != HttpVersion.V1_0))
                SendContinue();

            // add IP address and Connection Id to headers
            headers[CommonTransportKeys.IPAddress] = ((IPEndPoint)NetSocket.RemoteEndPoint).Address;
            headers[CommonTransportKeys.ConnectionId] = _connectionId;
            
            return headers;
        } // ReadHeaders


        public Stream GetRequestStream()
        {
            if (_chunkedEncoding)
                _requestStream = new HttpChunkedReadingStream(this);
            else
                _requestStream = new HttpFixedLengthReadingStream(this, _contentLength);
            return _requestStream;
        } // GetRequestStream


        public Stream GetResponseStream(String statusCode, String reasonPhrase,
                                        ITransportHeaders headers)
        {
            bool contentLengthPresent = false;
            bool useChunkedEncoding = false;
            int contentLength = 0;

            // check for custom user status code and reason phrase
            Object userStatusCode = headers["__HttpStatusCode"]; // someone might have stored an int
            String userReasonPhrase = headers["__HttpReasonPhrase"] as String;

            if (userStatusCode != null)
                statusCode = userStatusCode.ToString();
            if (userReasonPhrase != null)
                reasonPhrase = userReasonPhrase;

            // see if we can handle any more requests on this socket
            if (!CanServiceAnotherRequest())
            {
                headers["Connection"] = "Close";
            }                

            // check for content length
            Object contentLengthEntry = headers["Content-Length"];
            if (contentLengthEntry != null)
            {
                contentLengthPresent = true;
                if (contentLengthEntry is int)
                    contentLength = (int)contentLengthEntry;
                else                
                    contentLength = Convert.ToInt32(contentLengthEntry, CultureInfo.InvariantCulture);
            }

            // see if we are going to use chunked-encoding
            useChunkedEncoding = AllowChunkedResponse && !contentLengthPresent;
            if (useChunkedEncoding)
                headers["Transfer-Encoding"] = "chunked";


            // write headers to stream
            ChunkedMemoryStream headerStream = new ChunkedMemoryStream(CoreChannel.BufferPool);
            WriteResponseFirstLine(statusCode, reasonPhrase, headerStream);
            WriteHeaders(headers, headerStream);

            headerStream.WriteTo(NetStream);
            headerStream.Close();
          
           
            // return stream ready for content
            if (useChunkedEncoding)
                _responseStream = new HttpChunkedResponseStream(NetStream);
            else
                _responseStream = new HttpFixedLengthResponseStream(NetStream, contentLength);

            return _responseStream;
        } // GetResponseStream       


        private bool ReadFirstLine(out String verb, out String requestURI, out String version)
        {
            verb = null;
            requestURI = null;
            version = null;

            verb = ReadToChar(' ', s_validateVerbDelegate);

            byte[] requestUriBytes = ReadToByte((byte)' ');
            int decodedUriLength;
            HttpChannelHelper.DecodeUriInPlace(requestUriBytes, out decodedUriLength);
            requestURI = Encoding.UTF8.GetString(requestUriBytes, 0, decodedUriLength);
            
            version = ReadToEndOfLine();

            return true;
        } // ReadFirstLine
        

        private void SendContinue()
        {
            // Output:
            // HTTP/1.1 100 Continue
            
            // Send the continue response back to the client
            NetStream.Write(_bufferhttpContinue, 0, _bufferhttpContinue.Length);
        } // SendContinue


        public void SendResponse(Stream httpContentStream, 
                                 String statusCode, String reasonPhrase,
                                 ITransportHeaders headers)
        {
            if (_responseStream != null)
            {
                _responseStream.Close();
                if (_responseStream != httpContentStream)
                {
                    throw new RemotingException(
                                 CoreChannel.GetResourceString("Remoting_Http_WrongResponseStream"));
                }

                // we are done with the response stream
                _responseStream = null;
            }
            else
            {
                if (headers == null)
                    headers = new TransportHeaders();

                String serverHeader = (String)headers["Server"];
                if (serverHeader != null)
                    serverHeader = HttpServerTransportSink.ServerHeader + ", " + serverHeader;
                else
                    serverHeader = HttpServerTransportSink.ServerHeader;
                headers["Server"] = serverHeader;
            
                // Add length to response headers if necessary
                if (!AllowChunkedResponse && (httpContentStream != null))
                    headers["Content-Length"] = httpContentStream.Length.ToString(CultureInfo.InvariantCulture);
                else
                if (httpContentStream == null)
                    headers["Content-Length"] = "0";
            
                GetResponseStream(statusCode, reasonPhrase, headers);
        
                // write HTTP content
                if(httpContentStream != null)
                {
                    StreamHelper.CopyStream(httpContentStream, _responseStream);                    

                    _responseStream.Close();
                    httpContentStream.Close();
                }

                // we are done with the response stream
                _responseStream = null;
            }
        } // SendResponse
       
        

    } // class HttpServerSocketHandler



} // namespace System.Runtime.Remoting.Channels.Http


