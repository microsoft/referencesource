// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SoapFormatterSinks.cs
//
//  Summary:    Soap formatter client and server sinks.
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Metadata;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Serialization.Formatters.Soap;
using System.Text;
using System.Globalization;


namespace System.Runtime.Remoting.Channels
{

    //
    // CLIENT-SIDE SOAP FORMATTER SINKS
    //

    public class SoapClientFormatterSinkProvider : IClientFormatterSinkProvider
    {
        private IClientChannelSinkProvider _next = null;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;


        public SoapClientFormatterSinkProvider()
        {
        }

        public SoapClientFormatterSinkProvider(IDictionary properties, ICollection providerData)
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
        }
    
   
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

            SoapClientFormatterSink sink = new SoapClientFormatterSink(nextSink);
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            sink.ChannelProtocol = protocol;
            return sink;
        }

        public IClientChannelSinkProvider Next
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _next; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _next = value; }
        }
    } // class SoapClientFormatterSinkProvider

    
    public class SoapClientFormatterSink : IClientFormatterSink
    {
        private IClientChannelSink _nextSink = null;

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false; // strict binding should be used
        private SinkChannelProtocol _channelProtocol = SinkChannelProtocol.Other;
        
    
        public SoapClientFormatterSink(IClientChannelSink nextSink)
        {
            _nextSink = nextSink;
        } // SoapClientFormatterSink


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


        //
        // IMessageSink implementation
        //

        // formatter sender sink is always last "IMessageSink"
        public IMessageSink NextSink {
              [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
              get { throw new NotSupportedException(); }
        }


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IMessage SyncProcessMessage(IMessage msg)
        {
            IMethodCallMessage mcm = (IMethodCallMessage)msg;
            IMessage retMsg;
        
            try 
            {               
                // serialize message
                ITransportHeaders headers;
                Stream requestStream;
                SerializeMessage(mcm, out headers, out requestStream);
            
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
                SerializeMessage(mcm, out headers, out requestStream);
            
                // process message
                ClientChannelSinkStack sinkStack = new ClientChannelSinkStack(replySink);
                sinkStack.Push(this, mcm);
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


        //
        // end of IMessageSink implementation
        //


        // helper function to serialize the message
        private void SerializeMessage(IMethodCallMessage mcm, 
                                      out ITransportHeaders headers, out Stream stream)
        {
            BaseTransportHeaders requestHeaders = new BaseTransportHeaders();
            headers = requestHeaders;
        
            // add SOAPAction header
            MethodBase mb = mcm.MethodBase;
            headers["SOAPAction"] = 
                '"' + 
                HttpEncodingHelper.EncodeUriAsXLinkHref(
                    SoapServices.GetSoapActionFromMethodBase(mb)) + 
                '"';

            // add other http soap headers
            requestHeaders.ContentType = CoreChannel.SOAPContentType;
            if (_channelProtocol == SinkChannelProtocol.Http)
                headers["__RequestVerb"] = "POST";

            bool bMemStream = false;
            stream = _nextSink.GetRequestStream(mcm, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }
            CoreChannel.SerializeSoapMessage(mcm, stream, _includeVersioning);
            if (bMemStream)
                stream.Position = 0;
        } // SerializeMessage


        // helper function to deserialize the message
        private IMessage DeserializeMessage(IMethodCallMessage mcm, 
                                            ITransportHeaders headers, Stream stream)
        {
            IMessage retMsg;
        
            Header[] h = new Header[3];
            h[0] = new Header("__TypeName", mcm.TypeName);
            h[1] = new Header("__MethodName", mcm.MethodName);
            h[2] = new Header("__MethodSignature", mcm.MethodSignature);

            String contentTypeHeader = headers["Content-Type"] as String;
            String contentTypeValue, charsetValue;
            HttpChannelHelper.ParseContentType(contentTypeHeader,
                                               out contentTypeValue, out charsetValue);
            
            if (String.Compare(contentTypeValue, CoreChannel.SOAPMimeType, StringComparison.Ordinal) == 0)
            {
                // deserialize the message
                retMsg = CoreChannel.DeserializeSoapResponseMessage(stream, mcm, h, _strictBinding);
            }
            else
            {
                // an error has occurred
                int bufferSize = 1024;
                byte[] buffer = new byte[bufferSize];
                StringBuilder sb = new StringBuilder();

                int readCount = stream.Read(buffer, 0, bufferSize);
                while (readCount > 0)
                {
                    sb.Append(Encoding.ASCII.GetString(buffer, 0, readCount));
                    readCount = stream.Read(buffer, 0, bufferSize);
                }
                
                retMsg = new ReturnMessage(new RemotingException(sb.ToString()), mcm);
            }

            // Close the stream since we're done with it (especially important if this
            //   happened to be a network stream)
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
            // never gets called, this sink is always first
            throw new NotSupportedException();
        } // ProcessMessage


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            // never gets called, this sink is always first
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
            // should never be called on formatter sender sink
            throw new NotSupportedException();
        } // GetRequestStream


        public IClientChannelSink NextChannelSink
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _nextSink; }
        } // Next
        

        public IDictionary Properties
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return null; }
        } // Properties

        //
        // end of IClientChannelSink implementation
        //
        
    } // class SoapClientFormatterSink



    //
    // SERVER-SIDE SOAP FORMATTER SINKS
    //

    public class SoapServerFormatterSinkProvider : IServerFormatterSinkProvider
    {
        private IServerChannelSinkProvider _next = null;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Low; 

        public SoapServerFormatterSinkProvider()
        {
        } // SoapServerFormatterSinkProvider

        public SoapServerFormatterSinkProvider(IDictionary properties, ICollection providerData)
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
        } // SoapServerFormatterSinkProvider
        

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

            SoapServerFormatterSink.Protocol protocol = 
                SoapServerFormatterSink.Protocol.Other;

            // see if this is an http channel
            String uri = channel.GetUrlsForUri("")[0];
            if (String.Compare("http", 0, uri, 0, 4, StringComparison.OrdinalIgnoreCase) == 0)
                protocol = SoapServerFormatterSink.Protocol.Http;  

            SoapServerFormatterSink sink = new SoapServerFormatterSink(protocol, nextSink, channel);
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            sink.TypeFilterLevel = _formatterSecurityLevel;
            return sink;
        }

        public IServerChannelSinkProvider Next
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _next; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _next = value; }
        } 
        
        [System.Runtime.InteropServices.ComVisible(false)]
        public TypeFilterLevel TypeFilterLevel {
            get {
                return _formatterSecurityLevel;
            }
            
            set {
                _formatterSecurityLevel = value;
            }
        }
    } // class SoapServerFormatterSinkProvider
    

    public class SoapServerFormatterSink : IServerChannelSink
    {
        [Serializable]
        public enum Protocol
        {
            Http, // special processing needed for http
            Other
        }
        
    
        private IServerChannelSink _nextSink; // If this sink doesn't recognize, the incoming
                                              //   format then he should call the next
                                              //   sink if there is one.

        private Protocol _protocol; // transport protocol being used
        
        private IChannelReceiver _receiver; // transport sink used to parse url

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false;  // should strict binding be used
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Full;             
    
    
        public SoapServerFormatterSink(Protocol protocol, IServerChannelSink nextSink,
                                       IChannelReceiver receiver)
        {
            if (receiver == null)
                throw new ArgumentNullException("receiver");

            _nextSink = nextSink;
            _protocol = protocol;
            _receiver = receiver;
        } // SoapServerFormatterSinkProvider


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
                (String.Compare(contentType, CoreChannel.SOAPMimeType, StringComparison.Ordinal) != 0))
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
            
            bool bClientIsClr = true;
            try            
            {
                String objectUri = null;
                if (wkRequestHeaders != null)
                    objectUri = wkRequestHeaders.RequestUri;
                else
                    objectUri = (String)requestHeaders[CommonTransportKeys.RequestUri];              
                
                if (RemotingServices.GetServerTypeForUri(objectUri) == null)
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ChnlSink_UriNotPublished"));

                if (_protocol == Protocol.Http)
                {
                    String userAgent = (String)requestHeaders["User-Agent"];
                    if (userAgent != null)
                    {                       
                        if (userAgent.IndexOf("MS .NET Remoting") == -1)
                        {
                            // user agent string did not contain ".NET Remoting", so it is someone else
                            bClientIsClr = false;
                        }
                    }
                    else
                    {
                        bClientIsClr = false;
                    }
                }

                bool bIsCustomErrorEnabled = true;
                object oIsCustomErrorEnabled = requestHeaders["__CustomErrorsEnabled"];
                if (oIsCustomErrorEnabled != null && oIsCustomErrorEnabled is bool){
                    bIsCustomErrorEnabled = (bool)oIsCustomErrorEnabled;
                }
                CallContext.SetData("__CustomErrorsEnabled", bIsCustomErrorEnabled);
                
                String soapActionToVerify;
                Header[] h = GetChannelHeaders(requestHeaders, out soapActionToVerify);

                PermissionSet currentPermissionSet = null;
                if (this.TypeFilterLevel != TypeFilterLevel.Full) {
                    currentPermissionSet = new PermissionSet(PermissionState.None);                
                    currentPermissionSet.SetPermission(new SecurityPermission(SecurityPermissionFlag.SerializationFormatter));                    
                }
                                    
                try {
                    if (currentPermissionSet != null) 
                        currentPermissionSet.PermitOnly();                                                                    

                    // Deserialize Request - Stream to IMessage                                        
                    requestMsg = CoreChannel.DeserializeSoapRequestMessage(requestStream, h, _strictBinding, this.TypeFilterLevel);
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

                // verify soap action if necessary
                if ((soapActionToVerify != null) &&
                    (!SoapServices.IsSoapActionValidForMethodBase(
                        soapActionToVerify, ((IMethodMessage)requestMsg).MethodBase)))
                {
                    throw new RemotingException(
                        String.Format(
                            CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Soap_InvalidSoapAction"),
                            soapActionToVerify)
                        );
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

                    SerializeResponse(sinkStack, responseMsg, bClientIsClr,
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
                CallContext.SetData("__ClientIsClr", bClientIsClr);
                responseStream = (MemoryStream)CoreChannel.SerializeSoapMessage(responseMsg, _includeVersioning);
                CallContext.FreeNamedDataSlot("__ClientIsClr");
                responseStream.Position = 0;
                responseHeaders = new TransportHeaders();

                if (_protocol == Protocol.Http)
                {
                    responseHeaders["__HttpStatusCode"] = "500";
                    responseHeaders["__HttpReasonPhrase"] = "Internal Server Error";
                    responseHeaders["Content-Type"] = CoreChannel.SOAPContentType;
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
            // <

            SerializeResponse(sinkStack, msg, true, ref headers, out stream);
            sinkStack.AsyncProcessResponse(msg, headers, stream);
        } // AsyncProcessResponse


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        private void SerializeResponse(IServerResponseChannelSinkStack sinkStack,
                                       IMessage msg, bool bClientIsClr, ref ITransportHeaders headers,
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
            responseHeaders.ContentType = CoreChannel.SOAPContentType;

            if (_protocol == Protocol.Http)
            {
                // check to see if an exception occurred (requires special status code for HTTP)
                IMethodReturnMessage mrm = msg as IMethodReturnMessage;
                if ((mrm != null) && (mrm.Exception != null))
                {
                    headers["__HttpStatusCode"] = "500";
                    headers["__HttpReasonPhrase"] = "Internal Server Error";
                }                
            }

            bool bMemStream = false;
            stream = sinkStack.GetResponseStream(msg, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }


            bool bBashUrl = CoreChannel.SetupUrlBashingForIisSslIfNecessary();            
            CallContext.SetData("__ClientIsClr", bClientIsClr);
            try
            {
                CoreChannel.SerializeSoapMessage(msg, stream, _includeVersioning);           
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
            // We don't need to implement this because it will never be called.
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


        // Helper method for analyzing headers
        private Header[] GetChannelHeaders(ITransportHeaders requestHeaders, 
                                           out String soapActionToVerify)
        {
            soapActionToVerify = null;

            // transport sink removes any channel specific information
            String objectURI = (String)requestHeaders[CommonTransportKeys.RequestUri];        

            // see if a unique SOAPAction is present (if more than one SOAPAction is present some
            //   scenarios won't work, but one-many soap action to method base relationships are
            //   for interop scenarios only)
            String soapAction = (String) requestHeaders["SOAPAction"];
            if (soapAction == null)
                throw new RemotingException(CoreChannel.GetResourceString("Remoting_SoapActionMissing"));
            soapAction = HttpEncodingHelper.DecodeUri(soapAction);                

            soapActionToVerify = soapAction;
            
            String typeName, methodName;
            if (!SoapServices.GetTypeAndMethodNameFromSoapAction(soapAction, out typeName, out methodName))
            {
                // This means there are multiple methods for this soap action, so we will have to
                // settle for the type based off of the uri.
                Type type = RemotingServices.GetServerTypeForUri(objectURI);
                if (type == null)
                {
                    throw new RemotingException(                        
                        String.Format(
                            CultureInfo.CurrentCulture, CoreChannel.GetResourceString(
                                "Remoting_TypeNotFoundFromUri"), objectURI));                
                }

                // @todo: This throws away the version, culture and public key token
                typeName = "clr:" + type.FullName + ", " + type.Assembly.GetName().Name;
            }
            else
            {
                typeName = "clr:" + typeName;
            }
                         
            // Create a new header array and pass it back.
            int headerLen = 2;
            Header[] h = new Header[headerLen];
            h[0] = new Header("__Uri", objectURI);
            h[1] = new Header("__TypeName", typeName);

            return h;
        } // GetChannelHeaders
        
        
    } // class SoapServerFormatterSink



} // namespace System.Runtime.Remoting.Channnels
