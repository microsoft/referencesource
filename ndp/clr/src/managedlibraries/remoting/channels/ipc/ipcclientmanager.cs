// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       IpcClientManager.cs
//
//  Summary:    Class for managing a client pipe connection.
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;
using System.Runtime.Remoting.Channels.Tcp;
using System.Globalization;

namespace System.Runtime.Remoting.Channels.Ipc
{

    // A client manager instance should encapsulate the port
    //   for the purpose of reading a response
    internal class IpcClientHandler : IpcServerHandler
    {
        private bool _bOneWayRequest = false;  // was the request made OneWay?
        private TcpReadingStream _responseStream = null;
        private int _contentLength;
        private bool _bChunked;
        private IpcClientTransportSink _sink = null;

        internal IpcClientHandler(IpcPort port, Stream stream, IpcClientTransportSink sink) : base (port, null, stream)
        {
            _sink = sink;
        }

        internal Stream GetResponseStream()
        {
            _responseStream = new TcpFixedLengthReadingStream(this, _contentLength);
            return _responseStream;
        }

        public new BaseTransportHeaders ReadHeaders()
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

            base.ReturnBufferToPool();
        } // OnInputStreamClosed

        internal void ReturnToCache() {
            _sink.Cache.ReleaseConnection(_port);
        }

        internal void SendRequest(IMessage msg, ITransportHeaders headers, Stream contentStream)
        {
            // Request is written just like a response 
            // we can reuse the same code
            IMethodCallMessage mcm = (IMethodCallMessage)msg;        
            int contentLength = (int)contentStream.Length;

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
            StreamHelper.CopyStream(contentStream, NetStream);           

            contentStream.Close();
        }
    }
} // namespace System.Runtime.Remoting.Channels
