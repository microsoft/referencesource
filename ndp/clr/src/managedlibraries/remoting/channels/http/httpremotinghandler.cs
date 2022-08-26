// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       HTTPRemotingHandler.cs
//
//  Summary:    Implements an ASP+ handler that forwards requests to the
//              the remoting HTTP Channel.
//
//  Classes:    Derived from IHttpHandler
//
//
//==========================================================================

using System;
using System.DirectoryServices;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
using System.Reflection;
using System.Collections;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Diagnostics;
using System.Web;
using System.Web.UI;
using System.Runtime.Remoting.MetadataServices;
using System.Globalization;
using System.Collections.Specialized;

namespace System.Runtime.Remoting.Channels.Http
{

    public class HttpRemotingHandler : IHttpHandler
    {
        //Handler Specific

        private static String ApplicationConfigurationFile = "web.config";
        private static bool bLoadedConfiguration = false;
        
        private static HttpHandlerTransportSink s_transportSink = null; // transport sink


        // If an exception occurs while we are configuring the app domain, it is not possible
        // to recover since remoting is in an indeterminate state, so we will return that 
        // exception every time.
        private static Exception s_fatalException = null;         


        public HttpRemotingHandler()
        {
        }

        /// <internalonly/>
        public HttpRemotingHandler(Type type, Object srvID)
        {            
        }

        //
        // Process the ASP+ Request
        //
        public void ProcessRequest(HttpContext context)
        {
            InternalProcessRequest(context);
        }

        //
        // Internal
        //
        // Transform the ASP+ Request and Response Structures in
        // Channel Structures:
        // ** Request.ServerVariables
        // ** Request.InputStream
        // ** Response.Headers
        //
        // This is needed to reduce the between dependency COR Channels
        // and ASP+
        //

        private void InternalProcessRequest(HttpContext context)
        {
            try
            {          
                HttpRequest httpRequest = context.Request;
            
                // check if have previously loaded configuration
                if (!bLoadedConfiguration)
                {
                    // locking a random static variable, so we can lock the class
                    lock(HttpRemotingHandler.ApplicationConfigurationFile)
                    {
                        if (!bLoadedConfiguration)
                        {                                                   
                            // Initialize IIS information
                            IisHelper.Initialize();

                            // set application name
                            if (RemotingConfiguration.ApplicationName == null)
                                RemotingConfiguration.ApplicationName = httpRequest.ApplicationPath;
                    
                            String filename = String.Concat(httpRequest.PhysicalApplicationPath, 
                                                            ApplicationConfigurationFile);

                            if (File.Exists(filename))
                            {
                                try
                                {
                                    RemotingConfiguration.Configure(filename, false/*enableSecurity*/);
                                }
                                catch (Exception e)
                                {
                                    s_fatalException = e;
                                    WriteException(context, e); 
                                    return;
                                }    
                            }

                            try
                            {
                                // do a search for a registered channel that wants to listen
                                IChannelReceiverHook httpChannel = null;
                                IChannel[] channels = ChannelServices.RegisteredChannels;
                                foreach (IChannel channel in channels)
                                {
                                    IChannelReceiverHook hook = channel as IChannelReceiverHook;
                                    if (hook != null)
                                    {
                                        if (String.Compare(hook.ChannelScheme, "http", StringComparison.OrdinalIgnoreCase) == 0)
                                        {
                                            if (hook.WantsToListen)
                                            {
                                                httpChannel = hook;
                                                break;
                                            }
                                        }
                                    }
                                }
                            
                                if (httpChannel == null)
                                {
                                    // No http channel that was listening found.
                                    // Create a new channel.
                                    HttpChannel newHttpChannel = new HttpChannel();
                                    ChannelServices.RegisterChannel(newHttpChannel, false/*enableSecurity*/);
                                    httpChannel = newHttpChannel;
                                }

                                String scheme = null;
                                if (IisHelper.IsSslRequired)
                                    scheme = "https";
                                else
                                    scheme = "http";
    
                                String hookChannelUri =
                                    scheme + "://" + CoreChannel.GetMachineIp();

                                int port = context.Request.Url.Port;
                                String restOfUri = ":" + port + "/" + RemotingConfiguration.ApplicationName;
                                hookChannelUri += restOfUri;                                   

                                // add hook uri for this channel
                                httpChannel.AddHookChannelUri(hookChannelUri);
                                
                                // If it uses ChannelDataStore, re-retrieve updated url in case it was updated.
                                ChannelDataStore cds = ((IChannelReceiver)httpChannel).ChannelData as ChannelDataStore;
                                if (cds != null)
                                    hookChannelUri = cds.ChannelUris[0];

                                IisHelper.ApplicationUrl = hookChannelUri;

                                // This is a hack to refresh the channel data.
                                //   In V-Next, we will add a ChannelServices.RefreshChannelData() api.
                                ChannelServices.UnregisterChannel(null);
                                                            
                                s_transportSink = new HttpHandlerTransportSink(httpChannel.ChannelSinkChain);
                            }
                            catch (Exception e)
                            {
                                s_fatalException = e;
                                WriteException(context, e);                            
                                return;
                            }
                            bLoadedConfiguration = true;
                        }
                    }
                }  

                if (s_fatalException == null) 
                {
                    if (!CanServiceRequest(context))
                        WriteException(context, new RemotingException(CoreChannel.GetResourceString("Remoting_ChnlSink_UriNotPublished")));
                    else                        
                        s_transportSink.HandleRequest(context);       
                }
                else                
                    WriteException(context, s_fatalException);                 
            }
            catch (Exception e)
            {
                WriteException(context, e);
            }
        } // InternalProcessRequest
        
        public bool IsReusable { get { return true; } }        

        string ComposeContentType(string contentType, Encoding encoding) {
            if (encoding != null) {
                StringBuilder sb = new StringBuilder(contentType);
                sb.Append("; charset=");
                sb.Append(encoding.WebName);
                return sb.ToString();
            }
            else
                return contentType;
        }

        bool CanServiceRequest(HttpContext context) {                        
            //Need to get the object uri first (cannot have query string)
            string requestUri = GetRequestUriForCurrentRequest(context);            
            string objectUri = HttpChannelHelper.GetObjectUriFromRequestUri(requestUri);              
            context.Items["__requestUri"] = requestUri;                                                  
            
            if (String.Compare(context.Request.HttpMethod, "GET", StringComparison.OrdinalIgnoreCase) != 0) {
                //If the request backed by an existing object
                if (RemotingServices.GetServerTypeForUri(requestUri) != null)
                    return true;                                                                                       
            } 
            else {
                if (context.Request.QueryString.Count != 1) 
                    return false;
            
                string[] values =  context.Request.QueryString.GetValues(0);                       
                if (values.Length != 1 || String.Compare(values[0], "wsdl", StringComparison.OrdinalIgnoreCase) != 0)
                    return false;                                                        
            
                //If the request specifically asks for the wildcard                
                if (String.Compare(objectUri, "RemoteApplicationMetadata.rem", StringComparison.OrdinalIgnoreCase) == 0)
                    return true;
            
                // find last index of ?            
                int index = requestUri.LastIndexOf('?');
                if (index != -1) 
                    requestUri =  requestUri.Substring(0, index);

                //If the request backed by an existing object
                if (RemotingServices.GetServerTypeForUri(requestUri) != null)
                    return true;                                                                                                                                               
            }
            
            //If the request is backed by an existing file on disk it should be serviced
            if (File.Exists(context.Request.PhysicalPath))
                return true;                                            
            
            return false;      
        }
                
        string GetRequestUriForCurrentRequest(HttpContext context) {
            // we need to pull off any http specific data plus the application v-dir name
            String rawUrl = context.Request.RawUrl;
            // here's where we pull off channel info
            String channelUri;
            String requestUri;
            channelUri = HttpChannelHelper.ParseURL(rawUrl, out requestUri);
            if (channelUri == null)
                requestUri = rawUrl;
    
            // here's where we pull off the application v-dir name
            String appName = RemotingConfiguration.ApplicationName;
            if (appName != null && appName.Length > 0 && requestUri.Length > appName.Length)                                
                //  "/appname" should always be in front, otherwise we wouldn't
                //   be in this handler.                    
                requestUri = requestUri.Substring(appName.Length + 1);            
            
            return requestUri;
        }
        
        string GenerateFaultString(HttpContext context, Exception e) {
            //If the user has specified it's a development server (versus a production server) in ASP.NET config,
            //then we should just return e.ToString instead of extracting the list of messages.                        
            if (!CustomErrorsEnabled(context)) 
                return e.ToString();            
            else {                
                return CoreChannel.GetResourceString("Remoting_InternalError");                                                             
            }            
        }
        
        void WriteException(HttpContext context, Exception e) {
            InternalRemotingServices.RemotingTrace("HttpHandler: Exception thrown...\n");
            InternalRemotingServices.RemotingTrace(e.StackTrace);
            
            Stream outputStream = context.Response.OutputStream;
            context.Response.Clear();
            context.Response.ClearHeaders();
            context.Response.ContentType = ComposeContentType("text/plain", Encoding.UTF8);
            context.Response.TrySkipIisCustomErrors = true;
            context.Response.StatusCode = (int) HttpStatusCode.InternalServerError;
            context.Response.StatusDescription = CoreChannel.GetResourceString("Remoting_InternalError");                                                             
            StreamWriter writer = new StreamWriter(outputStream, new UTF8Encoding(false));
            writer.WriteLine(GenerateFaultString(context, e));
            writer.Flush();            
        }                
        
        internal static bool IsLocal(HttpContext context) {            
            string localAddress = context.Request.ServerVariables["LOCAL_ADDR"];
            string remoteAddress = context.Request.UserHostAddress;
            return (context.Request.Url.IsLoopback || (localAddress != null && remoteAddress != null && localAddress == remoteAddress));            
        }
        
        internal static bool CustomErrorsEnabled(HttpContext context) {
            try {            
                if (!context.IsCustomErrorEnabled)
                    return false;
                    
                return RemotingConfiguration.CustomErrorsEnabled(IsLocal(context));                
            }
            catch {
                return true;
            }                
        }

    } // HttpRemotingHandler

    public class HttpRemotingHandlerFactory : IHttpHandlerFactory
    {
        internal object _webServicesFactory = null;
        internal static Type s_webServicesFactoryType = null;
        // REMACT: internal static Type s_remActType = null;

        internal static Object s_configLock = new Object();

        internal static Hashtable s_registeredDynamicTypeTable = Hashtable.Synchronized(new Hashtable());
        

        void DumpRequest(HttpContext context)
        {
            HttpRequest request = context.Request;
            InternalRemotingServices.DebugOutChnl("Process Request called.");
            InternalRemotingServices.DebugOutChnl("Path = " + request.Path);
            InternalRemotingServices.DebugOutChnl("PhysicalPath = " + request.PhysicalPath);
            //InternalRemotingServices.DebugOutChnl("QueryString = " + request.Url.QueryString);
            InternalRemotingServices.DebugOutChnl("HttpMethod = " + request.HttpMethod);
            InternalRemotingServices.DebugOutChnl("ContentType = " + request.ContentType);
            InternalRemotingServices.DebugOutChnl("PathInfo = " + request.PathInfo);

            /*
            String[] keys = request.Headers.AllKeys;
            String[] values = request.Headers.All;

            for (int i=0; i<keys.Length; i++)
            {
                InternalRemotingServices.DebugOutChnl("Header :: " + keys[i] + "/" + values[i]);
            }
            */
        }

        private void ConfigureAppName(HttpRequest httpRequest)
        {
            if (RemotingConfiguration.ApplicationName == null)
            {
                lock (s_configLock)
                {
                    if (RemotingConfiguration.ApplicationName == null) 
                        RemotingConfiguration.ApplicationName = httpRequest.ApplicationPath;
                }
            }
        } // ConfigureAppName


        public IHttpHandler GetHandler(HttpContext context, string verb, string url, string filePath)
        {
            // REMACT: 
            // If this is a request to the root vdir, we will route it to the activation
            //   handler instead.
            //if (context.Request.ApplicationPath.Equals("/"))
            //{
            //    if (s_remActType == null)
            //        s_remActType = Type.GetType("System.Runtime.Remoting.Channels.Http.RemotingActivationHandler, System.Runtime.Remoting.Activation");
            //
            //    if (s_remActType != null)
            //        return (IHttpHandler)Activator.CreateInstance(s_remActType);                
            //}
        
            //if (CompModSwitches.Remote.TraceVerbose) DumpRequest(context);
            //System.Diagnostics.Debugger.Break();

            InternalRemotingServices.DebugOutChnl("HttpRemotingHandlderFactory::GetHanlder: IN");

            DumpRequest(context);  // 

            HttpRequest httpRequest = context.Request;
            ConfigureAppName(httpRequest);
            
            string queryString = httpRequest.QueryString[null];

            bool bVerbIsGET = (String.Compare(httpRequest.HttpMethod, "GET", StringComparison.OrdinalIgnoreCase) == 0);
            bool bFileExists = File.Exists(httpRequest.PhysicalPath);

            if (bVerbIsGET && bFileExists && queryString == null)
            {
                InternalRemotingServices.DebugOutChnl("HttpRemotingHandlderFactory::GetHanlder: non-post -- send to WebServices");
                return WebServicesFactory.GetHandler(context, verb, url, filePath);
            }
            else
            {
                InternalRemotingServices.DebugOutChnl("HttpRemotingHandlderFactory::GetHandler: post -- handling with Remoting");
                
                if (bFileExists)
                {
                    Type type = WebServiceParser.GetCompiledType(
                       url, context);

                    String machineAndAppName = Dns.GetHostName() + httpRequest.ApplicationPath;
        
                    // determine last part of url
                    String[] urlComponents = httpRequest.PhysicalPath.Split(new char[]{'\\'});
                    String uri = urlComponents[urlComponents.Length - 1] ;

                    // register the type if it has changed or hasn't been registered yet.
                    Type lastType = (Type)s_registeredDynamicTypeTable[uri];
                    if (lastType != type)
                    {
                        RegistrationHelper.RegisterType(machineAndAppName, type, uri);
                        s_registeredDynamicTypeTable[uri] = type;
                    }

                    return new HttpRemotingHandler();
                }
                else
                {
                  return new HttpRemotingHandler();
                }
            }
        }

        private IHttpHandlerFactory WebServicesFactory
        {
            get
            {
                if (_webServicesFactory == null)
                {
                    lock(this)
                    {
                        if (_webServicesFactory == null)
                        {
                            _webServicesFactory = Activator.CreateInstance(WebServicesFactoryType);
                        }
                    }
                }
                return (IHttpHandlerFactory)_webServicesFactory;
            }
        }

        private static Type WebServicesFactoryType
        {
            get
            {
                if (s_webServicesFactoryType == null)
                {
                    Assembly a = Assembly.Load("System.Web.Services, Version="+ThisAssembly.Version+", Culture=neutral, PublicKeyToken= "+AssemblyRef.MicrosoftPublicKey);
                    if (a == null)
                    {
                        throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_AssemblyLoadFailed"), "System.Web.Services"));
                    }
        
                    s_webServicesFactoryType = a.GetType("System.Web.Services.Protocols.WebServiceHandlerFactory");

                }
                return s_webServicesFactoryType;
            }
        }


        public void ReleaseHandler(IHttpHandler handler)
        {
            if (_webServicesFactory != null)
            {
                ((IHttpHandlerFactory)_webServicesFactory).ReleaseHandler(handler);
                _webServicesFactory = null;
            }
        }
    } //class HttpRemotingHandlerFactory    





    internal static class RegistrationHelper
    {    
        public static void RegisterType(String machineAndAppName, Type type, String uri)
        {
            RemotingConfiguration.RegisterWellKnownServiceType(type, uri, WellKnownObjectMode.SingleCall);

            Type[] allTypes = type.Assembly.GetTypes();
            foreach (Type asmType in allTypes)
            {
                RegisterSingleType(machineAndAppName, asmType);
            }            
        } // RegisterType

        private static void RegisterSingleType(String machineAndAppName, Type type)
        {
            String xmlName = type.Name;            
            String xmlNamespace = "http://" + machineAndAppName + "/" + type.FullName;
            SoapServices.RegisterInteropXmlElement(xmlName, xmlNamespace, type);
            SoapServices.RegisterInteropXmlType(xmlName, xmlNamespace, type);
            
            if (typeof(MarshalByRefObject).IsAssignableFrom(type))
            {
                // register soap action for all methods if this is a MarshalByRefObject type
                MethodInfo[] methods = type.GetMethods();
                foreach (MethodInfo mi in methods)
                {
                    SoapServices.RegisterSoapActionForMethodBase(mi, xmlNamespace + "#" + mi.Name);
                }
            }
            
        } // RegisterSingleType
        
    } // class RegistrationHelper
    




    // channel sink for interfacing with a sink chain
    internal class HttpHandlerTransportSink : IServerChannelSink    
    {
        private const int _defaultChunkSize = 2048;
    
        // sink state
        public IServerChannelSink _nextSink;


        public HttpHandlerTransportSink(IServerChannelSink nextSink)
        {
            _nextSink = nextSink;
        } // HttpHandlerTransportSink
        

        public void HandleRequest(HttpContext context)
        {
            HttpRequest httpRequest = context.Request;
            HttpResponse httpResponse = context.Response;

            // get headers
            BaseTransportHeaders requestHeaders = new BaseTransportHeaders();

            requestHeaders["__RequestVerb"] = httpRequest.HttpMethod;
            requestHeaders["__CustomErrorsEnabled"] = HttpRemotingHandler.CustomErrorsEnabled(context);
            requestHeaders.RequestUri = (string)context.Items["__requestUri"];

            NameValueCollection headers = httpRequest.Headers;          
            String[] allKeys = headers.AllKeys;

            for (int httpKeyCount=0; httpKeyCount< allKeys.Length; httpKeyCount++)
            {
                String headerName = allKeys[httpKeyCount];
                String headerValue = headers[headerName];
                requestHeaders[headerName] = headerValue;
            }

            // add ip address to headers list
            requestHeaders.IPAddress = IPAddress.Parse(httpRequest.UserHostAddress);

            // get request stream
            Stream requestStream = httpRequest.InputStream;

            // process message
            ServerChannelSinkStack sinkStack = new ServerChannelSinkStack();
            sinkStack.Push(this, null);
            
            IMessage responseMessage;
            ITransportHeaders responseHeaders;
            Stream responseStream;

            ServerProcessing processing = 
                _nextSink.ProcessMessage(sinkStack, null, requestHeaders, requestStream, 
                                         out responseMessage,
                                         out responseHeaders, out responseStream);
                
            // handle response
            switch (processing)
            {                    

            case ServerProcessing.Complete:
            {
                // Send the response. Call completed synchronously.             
                SendResponse(httpResponse, 200, responseHeaders, responseStream);                
                break;
            } // case ServerProcessing.Complete
            
            case ServerProcessing.OneWay:
            {
                // Just send back a 202 Accepted
                SendResponse(httpResponse, 202, responseHeaders, responseStream);                
                break;
            } // case ServerProcessing.OneWay

            case ServerProcessing.Async:
            {
                // Async dispatching was cut from V.1.
                //sinkStack.StoreAndDispatch(this, streamManager);
                break;
            }// case ServerProcessing.Async

            } // switch (processing)

            
        } // HandleRequest


        private void SendResponse(HttpResponse httpResponse, int statusCode,
                                  ITransportHeaders responseHeaders, Stream httpContentStream)
        {
            // store headers
            if (responseHeaders != null)
            {
                // set server string
                String serverHeader = (String)responseHeaders["Server"];
                if (serverHeader != null)
                    serverHeader = HttpServerTransportSink.ServerHeader + ", " + serverHeader;
                else
                    serverHeader = HttpServerTransportSink.ServerHeader;
                responseHeaders["Server"] = serverHeader;

                // set status code
                Object userStatusCode = responseHeaders["__HttpStatusCode"]; // someone might have stored an int

                if (userStatusCode != null)
                    statusCode = Convert.ToInt32(userStatusCode, CultureInfo.InvariantCulture);           

                // see if stream has a content length
                if (httpContentStream != null)
                {
                    int length = -1;
                    try
                    {
                        if (httpContentStream != null)
                            length = (int)httpContentStream.Length;
                    } 
                    catch {}

                    if (length != -1)
                        responseHeaders["Content-Length"] = length;
                }
                else
                    responseHeaders["Content-Length"] = 0;

                // add headers to the response
                foreach (DictionaryEntry entry in responseHeaders)
                {
                    String key = (String)entry.Key;                
                    if (!key.StartsWith("__", StringComparison.Ordinal))
                        httpResponse.AppendHeader(key, entry.Value.ToString());
                }
            }

            httpResponse.TrySkipIisCustomErrors = true;
            httpResponse.StatusCode = statusCode;

            // send stream
            Stream httpResponseStream = httpResponse.OutputStream;            

            if(httpContentStream != null)
            {
                StreamHelper.CopyStream(httpContentStream, httpResponseStream);
                httpContentStream.Close();
            }
            
        } // SendResponse                 


        //
        // IServerChannelSink implementation
        //

        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders,
            out Stream responseStream)
        {
            throw new NotSupportedException();
        }
           

        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)                 
        {
            // 


            throw new NotSupportedException();   
        } // AsyncProcessResponse


        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            // we always want a stream to read from
            return null;
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            get { return _nextSink; }
        } // Next


        public IDictionary Properties
        {
            get { return null; }
        } // Properties
        
        //
        // end of IServerChannelSink implementation
        //
        
    } // class HttpHandlerTransportSink
        
}//nameSpace

