// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       BinaryFormatterSinks.cs
//
//  Summary:    Binary formatter client and server sinks.
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;
using System.Runtime.Remoting.Configuration;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Metadata;
using System.Security;
using System.Security.Permissions;
using System.Globalization;


namespace System.Runtime.Remoting.Channels
{

    //
    // CLIENT-SIDE BINARY FORMATTER SINKS
    //

    public class BinaryClientFormatterSinkProvider : IClientFormatterSinkProvider
    {
        private IClientChannelSinkProvider _next;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;
        

        public BinaryClientFormatterSinkProvider()
        {
        } // BinaryClientFormatterSinkProvider


        public BinaryClientFormatterSinkProvider(IDictionary properties, ICollection providerData)
        {
            // look at properties
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    String keyStr = entry.Key.ToString();
                    switch (keyStr)
                    {
                    case "includeVersions": _includeVersioning = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "strictBinding": _strictBinding = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;

                    default:                        
                        break;
                    }
                }
            }
        
            // not expecting any provider data
            CoreChannel.VerifyNoProviderData(this.GetType().Name, providerData);
        } // BinaryClientFormatterSinkProvider

   
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IClientChannelSink CreateSink(IChannelSender channel, String url, 
                                             Object remoteChannelData)
        {
            IClientChannelSink nextSink = null;
            if (_next != null)
            {
                nextSink = _next.CreateSink(channel, url, remoteChannelData);
                if (nextSink == null)
                    return null;
            }

            SinkChannelProtocol protocol = CoreChannel.DetermineChannelProtocol(channel);

            BinaryClientFormatterSink sink = new BinaryClientFormatterSink(nextSink);
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            sink.ChannelProtocol = protocol;
            return sink;
        } // CreateSink

        public IClientChannelSinkProvider Next
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _next; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _next = value; }
        }
    } // class BinaryClientFormatterSinkProvider

    
    public class BinaryClientFormatterSink : IClientFormatterSink
    {
        private IClientChannelSink _nextSink = null;

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false; // strict binding should be used
        
        private SinkChannelProtocol _channelProtocol = SinkChannelProtocol.Other;
        
    
        public BinaryClientFormatterSink(IClientChannelSink nextSink)
        {
            _nextSink = nextSink;
        } // BinaryClientFormatterSink

        internal bool IncludeVersioning
        {
            set { _includeVersioning = value; }
        } // IncludeVersioning

        internal bool StrictBinding
        {
            set { _strictBinding = value; }
        } // StrictBinding

        internal SinkChannelProtocol ChannelProtocol
        {
            set { _channelProtocol = value; }
        } // ChannelProtocol



        public IMessageSink NextSink { 
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { throw new NotSupportedException(); }
        }

        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IMessage SyncProcessMessage(IMessage msg)
        {
            IMethodCallMessage mcm = msg as IMethodCallMessage;
            IMessage retMsg;
        
            try 
            {
                // serialize message
                ITransportHeaders headers;
                Stream requestStream;
                SerializeMessage(msg, out headers, out requestStream);

                // process message
                Stream returnStream;
                ITransportHeaders returnHeaders;
                _nextSink.ProcessMessage(msg, headers, requestStream,
                                         out returnHeaders, out returnStream);
                if (returnHeaders == null)
                    throw new ArgumentNullException("returnHeaders");                                         
                                     
                // deserialize stream
                retMsg = DeserializeMessage(mcm, returnHeaders, returnStream);
            }
            catch (Exception e)
            {
                retMsg = new ReturnMessage(e, mcm);
            }
            
            return retMsg;
        } // SyncProcessMessage


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            IMethodCallMessage mcm = (IMethodCallMessage)msg;
            IMessage retMsg;

            try
            {
                // serialize message
                ITransportHeaders headers;
                Stream requestStream;
                SerializeMessage(msg, out headers, out requestStream);
            
                // process message
                ClientChannelSinkStack sinkStack = new ClientChannelSinkStack(replySink);
                sinkStack.Push(this, msg);
                _nextSink.AsyncProcessRequest(sinkStack, msg, headers, requestStream);
            }
            catch (Exception e)
            {
                retMsg = new ReturnMessage(e, mcm);
                if (replySink != null)
                    replySink.SyncProcessMessage(retMsg);
            }
                                          
            return null;
        } // AsyncProcessMessage


        // helper function to serialize the message
        private void SerializeMessage(IMessage msg, 
                                      out ITransportHeaders headers, out Stream stream)
        {
            BaseTransportHeaders requestHeaders = new BaseTransportHeaders();
            headers = requestHeaders;

            // add other http soap headers
            requestHeaders.ContentType = CoreChannel.BinaryMimeType;
            if (_channelProtocol == SinkChannelProtocol.Http)
                headers["__RequestVerb"] = "POST";

            bool bMemStream = false;
            stream = _nextSink.GetRequestStream(msg, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }
            CoreChannel.SerializeBinaryMessage(msg, stream, _includeVersioning);
            if (bMemStream)
                stream.Position = 0;               
        } // SerializeMessage


        // helper function to deserialize the message
        private IMessage DeserializeMessage(IMethodCallMessage mcm, 
                                            ITransportHeaders headers, Stream stream)
        {
            // deserialize the message
            IMessage retMsg = CoreChannel.DeserializeBinaryResponseMessage(stream, mcm, _strictBinding); 
                
            stream.Close();
            return retMsg;
        } // DeserializeMessage
       

        //
        // IClientChannelSink implementation
        //
        
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {
            // should never gets called, since this sink is always first
            throw new NotSupportedException();
        } // ProcessMessage


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            // should never be called, this sink is always first
            throw new NotSupportedException();
        } // AsyncProcessRequest


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, Object state,
                                         ITransportHeaders headers, Stream stream)
        {
            // previously we stored the outgoing message in state
            IMethodCallMessage mcm = (IMethodCallMessage)state;  
            IMessage retMsg = DeserializeMessage(mcm, headers, stream);
            sinkStack.DispatchReplyMessage(retMsg);
        } // AsyncProcessRequest

       
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            // never called on formatter sender sink
            throw new NotSupportedException();
        }
        

        public IClientChannelSink NextChannelSink
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _nextSink; }
        }


        public IDictionary Properties
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return null; }
        } // Properties

        //
        // end of IClientChannelSink implementation
        //
        
    } // class BinaryClientFormatterSink



    //
    // SERVER-SIDE SOAP FORMATTER SINKS
    //

    public class BinaryServerFormatterSinkProvider : IServerFormatterSinkProvider
    {
        private IServerChannelSinkProvider _next = null;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Low;     

        public BinaryServerFormatterSinkProvider()
        {
        } // BinaryServerFormatterSinkProvider


        public BinaryServerFormatterSinkProvider(IDictionary properties, ICollection providerData)
        {       
            // look at properties
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    String keyStr = entry.Key.ToString();
                    switch (keyStr)
                    {
                    case "includeVersions": _includeVersioning = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "strictBinding": _strictBinding = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "typeFilterLevel": 
                        _formatterSecurityLevel = (TypeFilterLevel) Enum.Parse(typeof(TypeFilterLevel), (string)entry.Value); 
                        break;

                    default:
                        break;
                    }
                }
            }
        
            // not expecting any provider data
            CoreChannel.VerifyNoProviderData(this.GetType().Name, providerData);        
        } // BinaryServerFormatterSinkProvider
        

        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void GetChannelData(IChannelDataStore channelData)
        {
        } // GetChannelData
   
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IServerChannelSink CreateSink(IChannelReceiver channel)
        {
            if(null == channel)
            {
                throw new ArgumentNullException("channel");               
            }

            IServerChannelSink nextSink = null;
            if (_next != null)
                nextSink = _next.CreateSink(channel);

            BinaryServerFormatterSink.Protocol protocol = 
                BinaryServerFormatterSink.Protocol.Other;

            // see if this is an http channel
            String uri = channel.GetUrlsForUri("")[0];
            if (String.Compare("http", 0, uri, 0, 4, StringComparison.OrdinalIgnoreCase) == 0)
                protocol = BinaryServerFormatterSink.Protocol.Http;            

            BinaryServerFormatterSink sink = new BinaryServerFormatterSink(protocol, nextSink, channel);
            sink.TypeFilterLevel = _formatterSecurityLevel;
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            return sink;
        } // CreateSink

        public IServerChannelSinkProvider Next
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _next; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _next = value; }
        } // Next
        
        [System.Runtime.InteropServices.ComVisible(false)]
        public TypeFilterLevel TypeFilterLevel {
            get {
                return _formatterSecurityLevel;
            }
            
            set {
                _formatterSecurityLevel = value;
            }
        }
    } // class BinaryServerFormatterSinkProvider
    

    public class BinaryServerFormatterSink : IServerChannelSink
    {
		[Serializable]
        public enum Protocol
        {
            Http, // special processing needed for http
            Other
        }
    
        private IServerChannelSink _nextSink; // If this sink doesn't recognize, the incoming
                                              //   format then it should call the next
                                              //   sink if there is one.

        private Protocol _protocol; // remembers which protocol is being used
        
        private IChannelReceiver _receiver; // transport sink used to parse url

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false; // strict binding should be used        
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Full;                                     
        private string lastUri = null;

        public BinaryServerFormatterSink(Protocol protocol, IServerChannelSink nextSink,
                                         IChannelReceiver receiver)
        {
            if (receiver == null)
                throw new ArgumentNullException("receiver");

            _nextSink = nextSink;
            _protocol = protocol;
            _receiver = receiver;            
        } // BinaryServerFormatterSinkProvider


        internal bool IncludeVersioning
        {
            set { _includeVersioning = value; }
        } // IncludeVersioning

        internal bool StrictBinding
        {
            set { _strictBinding = value; }
        } // StrictBinding

        [System.Runtime.InteropServices.ComVisible(false)]
        public TypeFilterLevel TypeFilterLevel {
            get {
                return _formatterSecurityLevel;
            }
            
            set {
                _formatterSecurityLevel = value;
            }
        }

        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders, 
            out Stream responseStream)
        {
            if (requestMsg != null)
            {
                // The message has already been deserialized so delegate to the next sink.
                return _nextSink.ProcessMessage(
                    sinkStack,
                    requestMsg, requestHeaders, requestStream, 
                    out responseMsg, out responseHeaders, out responseStream);
            }
        
            if (requestHeaders ==  null)
                throw new ArgumentNullException("requestHeaders");

            BaseTransportHeaders wkRequestHeaders = requestHeaders as BaseTransportHeaders;
        
            ServerProcessing processing;
        
            responseHeaders = null;
            responseStream = null;

            String verb = null;
            String contentType = null;

            bool bCanServiceRequest = true;

            // determine the content type
            String contentTypeHeader = null;
            if (wkRequestHeaders != null)
                contentTypeHeader = wkRequestHeaders.ContentType;
            else
                contentTypeHeader = requestHeaders["Content-Type"] as String;
            if (contentTypeHeader != null)
            {
                String charsetValue;
                HttpChannelHelper.ParseContentType(contentTypeHeader,
                                                   out contentType, out charsetValue);
            }

            // check to see if Content-Type matches
            if ((contentType != null) &&
                (String.CompareOrdinal(contentType, CoreChannel.BinaryMimeType) != 0))
            {
                bCanServiceRequest = false;                
            }

            // check for http specific verbs
            if (_protocol == Protocol.Http)
            {
                verb = (String)requestHeaders["__RequestVerb"];    
                if (!verb.Equals("POST") && !verb.Equals("M-POST"))
                    bCanServiceRequest = false;
            }

            // either delegate or return an error message if we can't service the request
            if (!bCanServiceRequest)
            {
                // delegate to next sink if available
                if (_nextSink != null)
                {
                    return _nextSink.ProcessMessage(sinkStack, null, requestHeaders, requestStream,   
                        out responseMsg, out responseHeaders, out responseStream);
                }
                else
                {
                    // send back an error message
                    if (_protocol == Protocol.Http)
                    {
                        // return a client bad request error     
                        responseHeaders = new TransportHeaders();
                        responseHeaders["__HttpStatusCode"] = "400";
                        responseHeaders["__HttpReasonPhrase"] = "Bad Request";
                        responseStream = null;
                        responseMsg = null;
                        return ServerProcessing.Complete;
                    }
                    else
                    {
                        // The transport sink will catch this and do something here.
                        throw new RemotingException(
                            CoreChannel.GetResourceString("Remoting_Channels_InvalidRequestFormat"));
                    }
                }
            }
            

            try
            {
                String objectUri = null;

                bool bIsCustomErrorEnabled = true;
                object oIsCustomErrorEnabled = requestHeaders["__CustomErrorsEnabled"];
                if (oIsCustomErrorEnabled != null && oIsCustomErrorEnabled is bool){
                    bIsCustomErrorEnabled = (bool)oIsCustomErrorEnabled;
                }
                CallContext.SetData("__CustomErrorsEnabled", bIsCustomErrorEnabled);
              
                if (wkRequestHeaders != null)
                    objectUri = wkRequestHeaders.RequestUri;
                else
                    objectUri = (String)requestHeaders[CommonTransportKeys.RequestUri];              
            
                if (objectUri != lastUri && RemotingServices.GetServerTypeForUri(objectUri) == null)
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ChnlSink_UriNotPublished"));
                else
                    lastUri = objectUri;
            
                PermissionSet currentPermissionSet = null;                  
                if (this.TypeFilterLevel != TypeFilterLevel.Full) {                    
                    currentPermissionSet = new PermissionSet(PermissionState.None);                
                    currentPermissionSet.SetPermission(new SecurityPermission(SecurityPermissionFlag.SerializationFormatter));                    
                }
                                    
                try {
                    if (currentPermissionSet != null)
                        currentPermissionSet.PermitOnly();
                        
                    // Deserialize Request - Stream to IMessage
                    requestMsg = CoreChannel.DeserializeBinaryRequestMessage(objectUri, requestStream, _strictBinding, this.TypeFilterLevel);                    
                }
                finally {
                    if (currentPermissionSet != null)
                        CodeAccessPermission.RevertPermitOnly();
                }                                    
                requestStream.Close();

                if(requestMsg == null)
                {
                    throw new RemotingException(CoreChannel.GetResourceString("Remoting_DeserializeMessage"));
                }

                // Transparent proxy and MBRO IMessages are allowed conditionally by AppSettings
                if (requestMsg is MarshalByRefObject && !AppSettings.AllowTransparentProxyMessage)
                {
                    // Null request to prevent calling transparent proxy methods in catch below.
                    // Fwlink is provided to explain why it is not supported.  Inner exceptions propagate back to sender.
                    requestMsg = null;
                    throw new RemotingException(CoreChannel.GetResourceString("Remoting_DeserializeMessage"), 
                                                new NotSupportedException(AppSettings.AllowTransparentProxyMessageFwLink));
                }

                // Dispatch Call
                sinkStack.Push(this, null);
                processing =                    
                    _nextSink.ProcessMessage(sinkStack, requestMsg, requestHeaders, null,
                        out responseMsg, out responseHeaders, out responseStream);
                // make sure that responseStream is null
                if (responseStream != null)
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ChnlSink_WantNullResponseStream"));
                }
                
                switch (processing)
                {

                case ServerProcessing.Complete:
                {
                    if (responseMsg == null)
                        throw new RemotingException(CoreChannel.GetResourceString("Remoting_DispatchMessage"));

                    sinkStack.Pop(this);

                    SerializeResponse(sinkStack, responseMsg,
                                      ref responseHeaders, out responseStream);
                    break;
                } // case ServerProcessing.Complete

                case ServerProcessing.OneWay:
                {
                    sinkStack.Pop(this);
                    break;
                } // case ServerProcessing.OneWay:

                case ServerProcessing.Async:
                {
                    sinkStack.Store(this, null);
                    break;   
                } // case ServerProcessing.Async
                    
                } // switch (processing)                
            }
            catch(Exception e)
            {
                processing = ServerProcessing.Complete;
                responseMsg = new ReturnMessage(e, (IMethodCallMessage)(requestMsg==null?new ErrorMessage():requestMsg));
                //

                CallContext.SetData("__ClientIsClr", true);
                responseStream = (MemoryStream)CoreChannel.SerializeBinaryMessage(responseMsg, _includeVersioning);
                CallContext.FreeNamedDataSlot("__ClientIsClr");                
                responseStream.Position = 0;
                responseHeaders = new TransportHeaders();

                if (_protocol == Protocol.Http)
                {
                    responseHeaders["Content-Type"] = CoreChannel.BinaryMimeType;
                }
            }
            finally{
                CallContext.FreeNamedDataSlot("__CustomErrorsEnabled");
            }

            return processing;
        } // ProcessMessage


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)
        {
            SerializeResponse(sinkStack, msg, ref headers, out stream);
            sinkStack.AsyncProcessResponse(msg, headers, stream);
        } // AsyncProcessResponse


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        private void SerializeResponse(IServerResponseChannelSinkStack sinkStack,
                                       IMessage msg, ref ITransportHeaders headers,
                                       out Stream stream)
        {
            BaseTransportHeaders responseHeaders = new BaseTransportHeaders();
            if (headers != null)
            {
                // copy old headers into new headers
                foreach (DictionaryEntry entry in headers)
                {
                    responseHeaders[entry.Key] = entry.Value;
                }
            }            
            headers = responseHeaders;

            if (_protocol == Protocol.Http)
            {
                responseHeaders.ContentType = CoreChannel.BinaryMimeType;
            }

            bool bMemStream = false;
            stream = sinkStack.GetResponseStream(msg, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }

            bool bBashUrl = CoreChannel.SetupUrlBashingForIisSslIfNecessary(); 
            try
            {
                CallContext.SetData("__ClientIsClr", true);
                CoreChannel.SerializeBinaryMessage(msg, stream, _includeVersioning);
            }
            finally
            {
                CallContext.FreeNamedDataSlot("__ClientIsClr");
                CoreChannel.CleanupUrlBashingForIisSslIfNecessary(bBashUrl);
            }

            if (bMemStream)            
                stream.Position = 0;
        } // SerializeResponse


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            // This should never get called since we're the last in the chain, and never
            //   push ourselves to the sink stack.
            throw new NotSupportedException();
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _nextSink; }
        }


        public IDictionary Properties
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return null; }
        } // Properties
        
        
    } // class BinaryServerFormatterSink



} // namespace System.Runtime.Remoting.Channnels
