// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       IpcServerChannel.cs
//  Author:   Microsoft@Microsoft.Com
//  Summary:    Implements a channel that receives method calls over LPC.
//
//==========================================================================

using System;
using System.Collections;
using System.IO;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Security.Principal;
using System.Security.AccessControl;
using System.Threading;

using System.Runtime.InteropServices;
using System.Globalization;
using System.Security.Permissions;


namespace System.Runtime.Remoting.Channels.Ipc
{

    public class IpcServerChannel : IChannelReceiver, ISecurableChannel
    {
        private int               _channelPriority = 20;  // priority of channel (default=20)
        private String            _channelName = "ipc server";  // channel name
        private String            _portName = null;
        private ChannelDataStore  _channelData = null;   // channel data
        private IpcPort _port = null;

        private bool _bSuppressChannelData = false;  // should we hand out null for our channel data
        private bool _secure = false; // default is no authentication
        private bool _impersonate = false; // default is no impersonation
        private string _authorizedGroup = null; // default is authenticated users + deny Network sid
        private CommonSecurityDescriptor _securityDescriptor = null;
        private bool authSet = false;
        private bool _bExclusiveAddressUse = true;

        private IServerChannelSinkProvider _sinkProvider = null;
        private IpcServerTransportSink    _transportSink = null;

        
        private Thread                _listenerThread;
        private bool                  _bListening = false;
        private Exception             _startListeningException = null; // if an exception happens on the listener thread when attempting
                                                                       //   to start listening, that will get set here.
        private AutoResetEvent  _waitForStartListening = new AutoResetEvent(false);


        public IpcServerChannel(string portName)
        {
            if (portName == null)
                throw new RemotingException(CoreChannel.GetResourceString(
                                            "Remoting_Ipc_NoPortNameSpecified"));
            _portName = portName;
            SetupChannel();
        } // IpcServerChannel
    
        public IpcServerChannel(String name, string portName)
        {
            if (portName == null)
                throw new RemotingException(CoreChannel.GetResourceString(
                                            "Remoting_Ipc_NoPortNameSpecified"));
            _channelName = name;
            _portName = portName;
            SetupChannel();
        } // IpcServerChannel

        public IpcServerChannel(String name, string portName, IServerChannelSinkProvider sinkProvider)
        {
            if (portName == null)
                throw new RemotingException(CoreChannel.GetResourceString(
                                            "Remoting_Ipc_NoPortNameSpecified"));
            _channelName = name;
            _portName = portName;
            _sinkProvider = sinkProvider;
            SetupChannel();
        } // IpcServerChannel

        public IpcServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider)
                : this(properties, sinkProvider, null)
        {                   
        }

        public IpcServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider, CommonSecurityDescriptor securityDescriptor)
        {                   
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;  
                    case "portName": _portName = (String)entry.Value; break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value, CultureInfo.InvariantCulture); break;
                    case "secure": _secure = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "impersonate": _impersonate = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); authSet = true; break;
                    case "suppressChannelData": _bSuppressChannelData = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "authorizedGroup": _authorizedGroup = (String)entry.Value; break;               
                    case "exclusiveAddressUse": _bExclusiveAddressUse = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;                   
                    default: 
                         break;
                    }
                }

            }
            if (_portName == null)
                throw new RemotingException(CoreChannel.GetResourceString(
                                            "Remoting_Ipc_NoPortNameSpecified"));
            _sinkProvider = sinkProvider;
            _securityDescriptor = securityDescriptor;
            SetupChannel();
        } // IpcServerChannel

        //
        // ISecurableChannel implementation
        //
        public bool IsSecured
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _secure; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set {
                _secure = value;
                if (_transportSink != null)
                    _transportSink.IsSecured = value;
            }
        }
        
        private void SetupChannel()
        {   
            // set channel data
            // (These get changed inside of StartListening(), in the case where the listen
            //   port is 0, because we can't determine the port number until after the
            //   IpcListener starts.)
            
            if (authSet && !_secure)
                throw new RemotingException(CoreChannel.GetResourceString(
                                                "Remoting_Ipc_AuthenticationConfig"));

            _channelData = new ChannelDataStore(null);
            _channelData.ChannelUris = new String[1];
                _channelData.ChannelUris[0] = GetChannelUri();

            // set default provider (soap formatter) if no provider has been set
            if (_sinkProvider == null)
                _sinkProvider = CreateDefaultServerProviderChain();

            CoreChannel.CollectChannelDataFromServerSinkProviders(_channelData, _sinkProvider);

            // construct sink chain
            IServerChannelSink sink = ChannelServices.CreateServerChannelSinkChain(_sinkProvider, this);
            _transportSink = new IpcServerTransportSink(sink, _secure, _impersonate);

            // Start a thread to listen for incoming requests
            ThreadStart t = new ThreadStart(this.Listen);
            _listenerThread = new Thread(t);
            _listenerThread.IsBackground = true;

            // Wait for thread to spin up
            StartListening(null);
        } // SetupChannel


        private IServerChannelSinkProvider CreateDefaultServerProviderChain()
        {
            IServerChannelSinkProvider chain = new BinaryServerFormatterSinkProvider();            
            IServerChannelSinkProvider sink = chain;
            
            sink.Next = new SoapServerFormatterSinkProvider();
            
            return chain;
        } // CreateDefaultServerProviderChain


        //
        // IChannel implementation
        //

        public int ChannelPriority
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _channelPriority; }
        }

        public String ChannelName
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _channelName; }
        }

        // returns channelURI and places object uri into out parameter
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public String Parse(String url, out String objectURI)
        {            
            return IpcChannelHelper.ParseURL(url, out objectURI);
        } // Parse

        //
        // end of IChannel implementation
        //


        //
        // IChannelReceiver implementation
        //

        public Object ChannelData
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get
            {
                if (_bSuppressChannelData || !_bListening)
                {
                    return null;
                }
                else
                {
                    return _channelData;
                }
            }
        } // ChannelData

        public String GetChannelUri()
        {
            return "ipc://" + _portName;
        } // GetChannelUri


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public virtual String[] GetUrlsForUri(String objectUri)
        {
            if (objectUri == null)
                throw new ArgumentNullException("objectUri");

            String[] retVal = new String[1];

            if (!objectUri.StartsWith("/", StringComparison.Ordinal))
                objectUri = "/" + objectUri;
            retVal[0] = GetChannelUri() + objectUri;

            return retVal;
        } // GetURLsforURI


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void StartListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("IpcChannel.StartListening");

            if (_listenerThread.IsAlive == false)
            {
                _listenerThread.Start();
                _waitForStartListening.WaitOne(); // listener thread will signal this after starting IpcListener

                if (_startListeningException != null)
                {
                    // An exception happened when we tried to start listening (such as "socket already in use)
                    Exception e = _startListeningException;
                    _startListeningException = null;
                    throw e;
                }


            }
        } // StartListening


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void StopListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("IpcChannel.StopListening");
            _bListening = false;
            _port.Dispose();

        } // StopListening

        //
        // end of IChannelReceiver implementation
        //

        // Thread for listening
        void Listen()
        {
            InternalRemotingServices.RemotingTrace( "Waiting to Accept a Connection on Port: " + _portName);

            //
            // Wait for an incoming client connection
            //
            IntPtr handle = IntPtr.Zero;

            bool connected = false;
            CommonSecurityDescriptor descriptor = _securityDescriptor;

            // Connect with exlusive flag the first time
            try{
                // If the descriptor is not explicitly set through code use the _authorizedGroup config
                if (descriptor == null && _authorizedGroup != null)
                {
                    NTAccount ntAccount = new NTAccount(_authorizedGroup);
                    descriptor = IpcPort.CreateSecurityDescriptor((SecurityIdentifier)ntAccount.Translate(typeof(SecurityIdentifier)));
                }

                _port = IpcPort.Create(_portName, descriptor, _bExclusiveAddressUse);
            }
            catch (Exception e) {
                _startListeningException = e;
            }
            finally {
                _bListening = (_startListeningException == null);
                _waitForStartListening.Set(); // allow main thread to continue now that we have tried to start the socket
            }

            if (_port != null){
                connected = _port.WaitForConnect(); 
            }

            while (_bListening)
            {
                InternalRemotingServices.RemotingTrace("IpcChannel::Listen");

                // For DevDiv#220882, we need to create new IpcPort before handling the current message
                // to avoid race condition in case the message is handled and finished before 
                // the new IpcPort is created.  The client will intermittently fail with Port not found.
                IpcPort port = IpcPort.Create(_portName, descriptor, false);

                if (connected)
                {
                    // Initialize the server handler and perform an async read
                    IpcServerHandler serverHandler = new IpcServerHandler(_port, CoreChannel.RequestQueue, new PipeStream(_port));
                    serverHandler.DataArrivedCallback = new WaitCallback(_transportSink.ServiceRequest);
                    serverHandler.BeginReadMessage();
                }
                _port = port;
                connected = _port.WaitForConnect(); 
            }
       }

    } // class IpcServerChannel



    internal class IpcServerTransportSink : IServerChannelSink
    {
        //private const int _defaultChunkSize = 4096;
        private const int s_MaxSize =  (2 << 24); // Max size of the payload

        // sink state
        private IServerChannelSink _nextSink;

        // AuthenticationMode
        bool _secure;
        bool _impersonate;
        
        public IpcServerTransportSink(IServerChannelSink nextSink, bool secure, bool impersonate)
        {
            _nextSink = nextSink;
            _secure = secure;
            _impersonate = impersonate;
         } // IpcServerTransportSink
        
    
        internal bool IsSecured
        {
            get { return _secure; }
            set { _secure = value; }
        }
        internal void ServiceRequest(Object state)
        {
            IpcServerHandler ipcServerHandler = (IpcServerHandler)state;

            // Read the headers from the stream, using the header size in the message
            ITransportHeaders headers = ipcServerHandler.ReadHeaders();

            // Get the request Stream
            Stream requestStream = ipcServerHandler.GetRequestStream();
            // customErrors should be disabled, since we are on the same machine
            headers["__CustomErrorsEnabled"] = false;

            // process request
            ServerChannelSinkStack sinkStack = new ServerChannelSinkStack();
            sinkStack.Push(this, ipcServerHandler);

            IMessage responseMessage = null;
            ITransportHeaders responseHeaders = null;
            Stream responseStream = null;
            WindowsIdentity identity = null;
            IPrincipal oldPrincipal = null;
            bool impersonated = false;
            bool principalChanged = false;
            ServerProcessing processing = ServerProcessing.Complete;
            
            try{
                if (_secure)
                {
                    IpcPort port = ipcServerHandler.Port;
                    port.ImpersonateClient();
                    oldPrincipal = Thread.CurrentPrincipal;
                    principalChanged = true;
                    impersonated = true;
                    identity = WindowsIdentity.GetCurrent();    
                    // If the authentication mode is to identify callers only revert the impersonation immediately
                    if (!_impersonate)
                    {
                        NativePipe.RevertToSelf();
                        Thread.CurrentPrincipal = new GenericPrincipal(identity, null);
                        impersonated = false;
                    }
                    else
                    {
                        if (identity.ImpersonationLevel == TokenImpersonationLevel.Impersonation || 
                            identity.ImpersonationLevel == TokenImpersonationLevel.Delegation)
                        {
                            // Set the current principal
                            Thread.CurrentPrincipal = new WindowsPrincipal(identity);
                        }
                        else
                            throw new RemotingException(CoreChannel.GetResourceString(
                                                            "Remoting_Ipc_TokenImpersonationFailure"));
                    }
                }
                
                processing = 
                        _nextSink.ProcessMessage(sinkStack, null, headers, requestStream, 
                                                 out responseMessage,
                                                 out responseHeaders, out responseStream);
            }
            catch( Exception e) {
                ipcServerHandler.CloseOnFatalError(e);
            }
            finally{
                // Revert the principal if we had changed the principal
                if (principalChanged)
                {
                    Thread.CurrentPrincipal = oldPrincipal;
                }
                // Revert the impersonation if we had impersonated
                if (impersonated)
                {
                    NativePipe.RevertToSelf();
                    impersonated = false;
                }
            }

            // handle response
            switch (processing)
            {                    

            case ServerProcessing.Complete:
            {
                // Send the response. Call completed synchronously.
                sinkStack.Pop(this);
                // Send the response back to the client
                ipcServerHandler.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.Complete
            
            case ServerProcessing.OneWay:
            {                       
                // No response needed, but the following method will make sure that
                //   we send at least a skeleton reply if the incoming request was
                //   not marked OneWayRequest (client/server metadata could be out of
                //   sync).
                ipcServerHandler.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.OneWay

            case ServerProcessing.Async:
            {
                sinkStack.StoreAndDispatch(this, ipcServerHandler);
                break;
            }// case ServerProcessing.Async

            } // switch (processing) 
                   
            // Start waiting for the next request
            if (processing != ServerProcessing.Async)
            {
                ipcServerHandler.BeginReadMessage();
            }
        } // ServiceRequest               


        //
        // IServerChannelSink implementation
        //

        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders,
            out Stream responseStream)
        {
            // NOTE: This doesn't have to be implemented because the server transport
            //   sink is always first.
            throw new NotSupportedException();
        }
           

        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)                 
        {
            IpcServerHandler ipcServerHandler = null;

            ipcServerHandler = (IpcServerHandler)state;

            // send the response
            ipcServerHandler.SendResponse(headers, stream);
            
            //if (streamManager.CanServiceAnotherRequest())
            //    streamManager.BeginReadMessage();
            //else
            //    streamManager.Close(); 
        } // AsyncProcessResponse


        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {            
            // We always want a stream to read from.
            return null;
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            get { return _nextSink; }
        }


        public IDictionary Properties
        {
            get { return null; }
        } // Properties
        
        //
        // end of IServerChannelSink implementation
        //

        
    } // class IpcServerTransportSink


} // namespace System.Runtime.Remoting.Channels.Ipc
