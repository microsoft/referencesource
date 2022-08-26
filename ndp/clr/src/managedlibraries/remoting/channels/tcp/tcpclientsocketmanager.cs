// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       TcpClientSocketManager.cs
//
//  Summary:    Class for managing a socket connection.
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Tcp
{

    // A client socket manager instance should encapsulate the socket
    //   for the purpose of reading a response
    internal class TcpClientSocketHandler : TcpSocketHandler
    {    
        // prebaked bytes
        private static byte[] s_endOfLineBytes = Encoding.ASCII.GetBytes("\r\n");
        
    
        private String _machinePortAndSid; // "machineName:port:Sid"                     
    
        // connection state information
        private bool _bOneWayRequest = false;  // was the request made OneWay?
        private bool _bChunked;    
        private int  _contentLength;

        private Stream _requestStream; // the request stream that we return from GetRequestStream()
        private TcpReadingStream _responseStream; // the stream that we returned from GetResponseStream()
        private TcpClientTransportSink _sink = null;

        public TcpClientSocketHandler(Socket socket, String machinePortAndSid, Stream stream, TcpClientTransportSink sink) :
            base(socket, stream)            
        {          
            _machinePortAndSid = machinePortAndSid;
            _sink = sink;
        } // TcpClientSocketHandler
 
        

        // Prepare for reading a new request off of the same socket
        protected override void PrepareForNewMessage()
        {
            _requestStream = null;
            _responseStream = null;
        } // PrepareForNewRequest


        public override void OnInputStreamClosed()
        {       
            // make sure we read to the end of the response stream
            if (_responseStream != null)
            {
                _responseStream.ReadToEnd();
                _responseStream = null;                
            }
        
            // return socket to the cache
            ReturnToCache();
        } // OnInputStreamClosed



        public BaseTransportHeaders ReadHeaders()
        {           
            BaseTransportHeaders headers = new BaseTransportHeaders();

            UInt16 operation;
            ReadVersionAndOperation(out operation);

            // At this point, we're always expecting a Reply, so check for that.
            if (operation != TcpOperations.Reply)
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_ExpectingReplyOp"),
                        operation.ToString(CultureInfo.CurrentCulture)));
            }                        
                   
            // content length must come next (may be chunked or a specific length)
            ReadContentLength(out _bChunked, out _contentLength);
            
            // read to end of headers  
            ReadToEndOfHeaders(headers); 
                               
            return headers;
        } // ReadHeaders  


        public Stream GetRequestStream(IMessage msg, int contentLength,
                                       ITransportHeaders headers)
        {
            IMethodCallMessage mcm = (IMethodCallMessage)msg;        
            String uri = mcm.Uri;
            _bOneWayRequest = RemotingServices.IsOneWay(mcm.MethodBase);

            ChunkedMemoryStream headerStream = new ChunkedMemoryStream(CoreChannel.BufferPool);

            // output preamble and version
            WritePreambleAndVersion(headerStream);
            // output opcode 
            if (!_bOneWayRequest)
                WriteUInt16(TcpOperations.Request, headerStream);
            else
                WriteUInt16(TcpOperations.OneWayRequest, headerStream);            
            // output content delimiter style
            WriteUInt16(TcpContentDelimiter.ContentLength, headerStream);
            WriteInt32(contentLength, headerStream);
            
            // output request uri
            WriteUInt16(TcpHeaders.RequestUri, headerStream);
            WriteByte(TcpHeaderFormat.CountedString, headerStream);
            WriteCountedString(uri, headerStream);         

            // output rest of headers
            WriteHeaders(headers, headerStream);   
            
            headerStream.WriteTo(NetStream);
            headerStream.Close();

            _requestStream = NetStream;

            return _requestStream;
        } // GetRequestStream

        public void SendRequest(IMessage msg, ITransportHeaders headers, Stream contentStream)
        {
            int requestLength = (int)contentStream.Length;
            GetRequestStream(msg, requestLength, headers);

            StreamHelper.CopyStream(contentStream, NetStream);           

            contentStream.Close();
        } // SendRequest


        public Stream GetResponseStream()
        {
            if (!_bChunked)
                _responseStream = new TcpFixedLengthReadingStream(this, _contentLength);
            else
                _responseStream = new TcpChunkedReadingStream(this);
            
            return _responseStream;
        } // GetResponseStream


        public bool OneWayRequest
        {
            get { return _bOneWayRequest; }
        }

        public void ReturnToCache()
        {
            _sink.ClientSocketCache.ReleaseSocket(
                _machinePortAndSid, this);
        }
    
    } // TcpClientSocketHandler

} // namespace System.Runtime.Remoting.Channels
