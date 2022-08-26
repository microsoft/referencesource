// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       IpcHandler.cs
//  Author:   Microsoft@Microsoft.com
//  Summary:    Class for managing a socket connection.
//
//==========================================================================

using System;
using System.IO;
using System.Runtime.Remoting.Messaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Runtime.Remoting.Channels.Tcp;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Ipc
{

    internal class IpcServerHandler : TcpSocketHandler
    {
        // The stream to manage incoming request data
        private Stream _stream = null;
        protected Stream _requestStream = null;
        // NamePipe associated with this manager
        protected IpcPort _port;
        private RequestQueue _requestQueue;
        bool _bOneWayRequest;
        int _contentLength;

        internal IpcServerHandler(IpcPort port, RequestQueue requestQueue, Stream stream) : base (null, requestQueue, stream)
        {
            _requestQueue = requestQueue;
            _port = port;
            _stream = stream;
        }

        internal Stream GetRequestStream()
        {
                _requestStream =  new TcpFixedLengthReadingStream(this, _contentLength);
                return _requestStream;
        }

        internal IpcPort Port {
            get {
                return _port;
            }
        }

        internal ITransportHeaders ReadHeaders()
        {
            BaseTransportHeaders headers = new BaseTransportHeaders();

            UInt16 operation;
            ReadVersionAndOperation(out operation);

            if (operation == TcpOperations.OneWayRequest)
            {
                _bOneWayRequest = true;
            }

            bool bChunked = false;
            // content length must come next (may be chunked or a specific length)
            ReadContentLength(out bChunked, out _contentLength);

            // read to end of headers  
            ReadToEndOfHeaders(headers);   
                           
            return headers;
        }

        protected new void ReadToEndOfHeaders(BaseTransportHeaders headers)
        {
            bool bError = false;
            String statusPhrase = null; 
        
            UInt16 headerType = ReadUInt16();
            while (headerType != TcpHeaders.EndOfHeaders)
            {
                if (headerType == TcpHeaders.Custom)
                {
                    String headerName = ReadCountedString();
                    String headerValue = ReadCountedString();

                    headers[headerName] = headerValue;
                }
                else
                if (headerType == TcpHeaders.RequestUri)
                {         
                    ReadAndVerifyHeaderFormat("RequestUri", TcpHeaderFormat.CountedString);
                
                    // read uri (and make sure that no channel specific data is present)
                    String uri = ReadCountedString();
                    
                    String channelURI;
                    String objectURI;
                    channelURI = IpcChannelHelper.ParseURL(uri, out objectURI);
                    if (channelURI == null)
                        objectURI = uri;              
            
                    headers.RequestUri = objectURI;
                }
                else
                if (headerType == TcpHeaders.StatusCode)
                {
                    ReadAndVerifyHeaderFormat("StatusCode", TcpHeaderFormat.UInt16);
                    
                    UInt16 statusCode = ReadUInt16();
                    // We'll throw an exception here if there was an error. If an error
                    //   occurs above the transport level, the status code will still be
                    //   success here.
                    if (statusCode != TcpStatusCode.Success)
                        bError = true;
                }
                else
                if (headerType == TcpHeaders.StatusPhrase)
                {
                    ReadAndVerifyHeaderFormat("StatusPhrase", TcpHeaderFormat.CountedString);
                
                    statusPhrase = ReadCountedString();
                }
                else
                if (headerType == TcpHeaders.ContentType)
                {
                    ReadAndVerifyHeaderFormat("Content-Type", TcpHeaderFormat.CountedString);
                
                    String contentType = ReadCountedString();
                    headers.ContentType = contentType;
                }
                else
                {
                    // unknown header: Read header format and ignore rest of data
                    byte headerFormat = (byte)ReadByte();

                    switch (headerFormat)
                    {
                    case TcpHeaderFormat.Void: break;
                    case TcpHeaderFormat.CountedString: ReadCountedString(); break;
                    case TcpHeaderFormat.Byte: ReadByte(); break;
                    case TcpHeaderFormat.UInt16: ReadUInt16(); break;
                    case TcpHeaderFormat.Int32: ReadInt32(); break;

                    default:
                    {
                        // unknown format
                        throw new RemotingException(
                            String.Format(
                                CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_UnknownHeaderType"),
                                headerType, headerFormat));
                    }
                    
                    } // switch (format)
                
                }

                // read next header token
                headerType = ReadUInt16();
            } // loop until end of headers         

            // if an error occurred, throw an exception
            if (bError)
            {
                if (statusPhrase == null)
                    statusPhrase = "";
                    
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_GenericServerError"),
                        statusPhrase));
            }
        } // ReadToEndOfHeaders

        private void ReadAndVerifyHeaderFormat(String headerName, byte expectedFormat)
        {
            byte headerFormat = (byte)ReadByte();

            if (headerFormat != expectedFormat)
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_IncorrectHeaderFormat"),
                        expectedFormat, headerName));
            }
        } // ReadAndVerifyHeaderFormat

        // Prepare for reading a new request off of the same socket
        protected override void PrepareForNewMessage()
        {
        } // PrepareForNewRequest

        protected override void SendErrorMessageIfPossible(Exception e)
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
            WriteCountedString(e.ToString(), headerStream);

            // indicate that we are about to close the connection
            WriteUInt16(TcpHeaders.CloseConnection, headerStream);
            WriteByte(TcpHeaderFormat.Void, headerStream);

            // end of headers
            WriteUInt16(TcpHeaders.EndOfHeaders, headerStream);
            
            headerStream.WriteTo(NetStream);
            headerStream.Close();
        }     

        internal void SendResponse(ITransportHeaders headers, Stream contentStream)
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
        }
    }

}
