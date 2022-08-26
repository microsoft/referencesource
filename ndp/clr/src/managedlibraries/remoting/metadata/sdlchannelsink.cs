// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SdlChannelSink.cs
//
//  Summary:    Sdl channel sink for generating sdl dynamically on the server.
//
//==========================================================================


using System;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;
using System.Runtime.Remoting.Messaging;
using System.Text;
#if !FEATURE_PAL
using System.Web;
using System.Security.Permissions;
#endif


namespace System.Runtime.Remoting.MetadataServices
{

    public class SdlChannelSinkProvider : IServerChannelSinkProvider
    {
        private IServerChannelSinkProvider _next = null;
        private bool _bRemoteApplicationMetadataEnabled = false;
        private bool _bMetadataEnabled = true;        
                                                         
        public SdlChannelSinkProvider()
        {
        }

        public SdlChannelSinkProvider(IDictionary properties, ICollection providerData)
        {        
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "remoteApplicationMetadataEnabled": _bRemoteApplicationMetadataEnabled = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "metadataEnabled": _bMetadataEnabled = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    default:
                        CoreChannel.ReportUnknownProviderConfigProperty(
                            this.GetType().Name, (String)entry.Key);
                        break;                    
                    }
                }
            }                                            
        }

        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void GetChannelData(IChannelDataStore localChannelData)
        {
        }
   
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public IServerChannelSink CreateSink(IChannelReceiver channel)
        {
            IServerChannelSink nextSink = null;
            if (_next != null)
                nextSink = _next.CreateSink(channel);
                
            SdlChannelSink channelSink = new SdlChannelSink(channel, nextSink);                
            channelSink.RemoteApplicationMetadataEnabled = _bRemoteApplicationMetadataEnabled; 
            channelSink.MetadataEnabled = _bMetadataEnabled;
            return channelSink;
        }

        public IServerChannelSinkProvider Next
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _next; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _next = value; }
        }
    } // class SdlChannelSinkProvider

   
    

    public class SdlChannelSink : IServerChannelSink
    {
        private IChannelReceiver _receiver;
        private IServerChannelSink _nextSink; 
        private bool _bRemoteApplicationMetadataEnabled = false;
        private bool _bMetadataEnabled = false;        

    
        public SdlChannelSink(IChannelReceiver receiver, IServerChannelSink nextSink)
        {
            _receiver = receiver;
            _nextSink = nextSink;
        } // SdlChannelSink

        internal bool RemoteApplicationMetadataEnabled 
        {
            get { return _bRemoteApplicationMetadataEnabled; }            
            set { _bRemoteApplicationMetadataEnabled = value; }
        }
        
        internal bool MetadataEnabled 
        {
            get { return _bMetadataEnabled; }            
            set { _bMetadataEnabled = value; }
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
        
            SdlType sdlType;
            if (!ShouldIntercept(requestHeaders, out sdlType))
                return _nextSink.ProcessMessage(sinkStack, null, requestHeaders, requestStream,
                                                out responseMsg, out responseHeaders, out responseStream);            
                        
            // generate sdl and return it
            responseHeaders = new TransportHeaders();
            GenerateSdl(sdlType, sinkStack, requestHeaders, responseHeaders, out responseStream);
            responseMsg = null;

            return ServerProcessing.Complete;            
        } // ProcessMessage


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)
        {
            // We don't need to implement this because we never push ourselves to the sink
            //   stack.
        } // AsyncProcessResponse


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            // We don't need to implement this because we never push ourselves
            //   to the sink stack.
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


        // Should we intercept the call and return some SDL
        private bool ShouldIntercept(ITransportHeaders requestHeaders, out SdlType sdlType)
        {
            sdlType = SdlType.Sdl;
            
            String requestVerb = requestHeaders["__RequestVerb"] as String;
            String requestURI = requestHeaders["__RequestUri"] as String;
    
            // http verb must be "GET" to return sdl (and request uri must be set)
            if ((requestURI == null) ||
                (requestVerb == null) || !requestVerb.Equals("GET"))              
                return false;

            // find last index of ? and look for "sdl" or "sdlx"
            int index = requestURI.LastIndexOf('?');
            if (index == -1)
                return false; // no query string

            String queryString = requestURI.Substring(index).ToLower(CultureInfo.InvariantCulture);

            // sdl?
            if ((String.CompareOrdinal(queryString, "?sdl") == 0) ||
                (String.CompareOrdinal(queryString, "?sdlx") == 0))
            { 
                sdlType = SdlType.Sdl;
                return true;
            }

            // wsdl?
            if (String.CompareOrdinal(queryString, "?wsdl") == 0)
            { 
                sdlType = SdlType.Wsdl;
                return true;
            }            
            
            return false;            
        } // ShouldIntercept


        private void GenerateSdl(SdlType sdlType,
                                 IServerResponseChannelSinkStack sinkStack,
                                 ITransportHeaders requestHeaders,
                                 ITransportHeaders responseHeaders,
                                 out Stream outputStream)
        {
            if (!MetadataEnabled)
                throw new RemotingException(CoreChannel.GetResourceString("Remoting_MetadataNotEnabled"));
            
            String requestUri = requestHeaders[CommonTransportKeys.RequestUri] as String;           
            String objectUri = HttpChannelHelper.GetObjectUriFromRequestUri(requestUri);            
            
            if (!RemoteApplicationMetadataEnabled && 
                (String.Compare(objectUri, "RemoteApplicationMetadata.rem", StringComparison.OrdinalIgnoreCase) == 0))
                throw new RemotingException(CoreChannel.GetResourceString("Remoting_RemoteApplicationMetadataNotEnabled"));

            // If the host header is present, we will use this in the generated uri's
            String hostName = (String)requestHeaders["Host"];
            if (hostName != null)
            {
                // filter out port number if present
                int index = hostName.IndexOf(':');
                if (index != -1)
                    hostName = hostName.Substring(0, index);
            }

#if !FEATURE_PAL
            // For IIS, we will Microsoft the scheme://hostname:port with the incoming value
            String iisHostOverride = SetupUrlBashingForIisIfNecessary(hostName);
#endif
            

            ServiceType[] types = null;

            if (String.Compare(objectUri, "RemoteApplicationMetadata.rem", StringComparison.OrdinalIgnoreCase) == 0)
            {
                // get the service description for all registered service types
                
                ActivatedServiceTypeEntry[] activatedTypes = 
                    RemotingConfiguration.GetRegisteredActivatedServiceTypes();

                WellKnownServiceTypeEntry[] wellKnownTypes = 
                    RemotingConfiguration.GetRegisteredWellKnownServiceTypes();

                // determine total number of types
                int typeCount = 0;
                
                if (activatedTypes != null)
                    typeCount += activatedTypes.Length;
                    
                if (wellKnownTypes != null)
                    typeCount += wellKnownTypes.Length;

                types = new ServiceType[typeCount];

                // collect data
                int co = 0;
                if (activatedTypes != null)
                {
                    foreach (ActivatedServiceTypeEntry entry in activatedTypes)
                    {
                        types[co++] = new ServiceType(entry.ObjectType, null);
                    }                    
                }

                if (wellKnownTypes != null)
                {
                    foreach (WellKnownServiceTypeEntry entry in wellKnownTypes)
                    {   
                        String[] urls = _receiver.GetUrlsForUri(entry.ObjectUri);
                        String url = urls[0];
#if !FEATURE_PAL
                        if (iisHostOverride != null)
                            url = HttpChannelHelper.ReplaceChannelUriWithThisString(url, iisHostOverride);
                        else
#endif
                        if (hostName != null)
                            url = HttpChannelHelper.ReplaceMachineNameWithThisString(url, hostName);

                        types[co++] = new ServiceType(entry.ObjectType, url);
                    } 
                }

                InternalRemotingServices.RemotingAssert(co == typeCount, "Not all types were processed.");                
            }
            else
            {    
                // get the service description for a particular object
                Type objectType = RemotingServices.GetServerTypeForUri(objectUri);
                if (objectType == null)
                {
                    throw new RemotingException(
                        String.Format(
                            CultureInfo.CurrentCulture, "Object with uri '{0}' does not exist at server.",
                            objectUri));
                }
                
                String[] urls = _receiver.GetUrlsForUri(objectUri);
                String url = urls[0];
#if !FEATURE_PAL
                if (iisHostOverride != null)
                    url = HttpChannelHelper.ReplaceChannelUriWithThisString(url, iisHostOverride);
                else
#endif
                if (hostName != null)
                    url = HttpChannelHelper.ReplaceMachineNameWithThisString(url, hostName);

                types = new ServiceType[1];
                types[0] = new ServiceType(objectType, url);
            }

            responseHeaders["Content-Type"] = "text/xml";

            bool bMemStream = false;

            outputStream = sinkStack.GetResponseStream(null, responseHeaders);
            if (outputStream == null)
            {
                outputStream = new MemoryStream(1024);
                bMemStream = true;
            }        

            MetaData.ConvertTypesToSchemaToStream(types, sdlType, outputStream);

            if (bMemStream)
                outputStream.Position = 0;
        } // GenerateXmlForUri               

        // SetupUrlBashingForIisIfNecessaryWorker wrapper.
        // Prevents System.Web type load for client sku installations.
        internal static string SetupUrlBashingForIisIfNecessary(string hostName)
        {
            String iisHostOverride = null;

            if (!CoreChannel.IsClientSKUInstallation)
            {
                iisHostOverride = SetupUrlBashingForIisIfNecessaryWorker(hostName);
            }

            return iisHostOverride;
        } // SetupUrlBashingForIisIfNecessary

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        private static string SetupUrlBashingForIisIfNecessaryWorker(string hostName)
        {
            // For IIS, we will Microsoft the scheme://hostname:port with the incoming value
            String iisHostOverride = null;
            HttpContext context = HttpContext.Current;
            if (context != null)
            {
                HttpRequest request = context.Request;
                String scheme = null;
                if (request.IsSecureConnection)
                    scheme = "https";
                else
                    scheme = "http";

                int port = context.Request.Url.Port;

                StringBuilder sb = new StringBuilder(100);
                sb.Append(scheme);
                sb.Append("://");
                if (hostName != null)
                    sb.Append(hostName);
                else
                    sb.Append(CoreChannel.GetMachineName());
                sb.Append(":");
                sb.Append(port.ToString(CultureInfo.InvariantCulture));

                iisHostOverride = sb.ToString();
            }

            return iisHostOverride;
        } // SetupUrlBashingForIisIfNecessaryWorker
    } // class SdlChannelSink



} // namespace System.Runtime.Remoting.Channnels

