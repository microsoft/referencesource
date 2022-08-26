// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//===========================================================================
//  File:       TcpServerChannel.cs
//
//  Summary:    Implements a channel that receives method calls over TCP.
//
//==========================================================================

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using System.Threading;
#if !FEATURE_PAL
using System.Security.Authentication;
#endif

using System.Runtime.InteropServices;
using System.Globalization;
using System.Security.Permissions;


namespace System.Runtime.Remoting.Channels.Tcp
{

    public class TcpServerChannel : IChannelReceiver, ISecurableChannel
    {
        private int               _channelPriority = 1;  // priority of channel (default=1)
        private String            _channelName = "tcp";  // channel name
        private String            _machineName = null;   // machine name
        private int               _port = -1;            // port to listen on
        private ChannelDataStore  _channelData = null;   // channel data

        private String _forcedMachineName = null; // an explicitly configured machine name
        private bool _bUseIpAddress = true; // by default, we'll use the ip address.
        //Assumption is Socket.OSSupportsIPv4 will be false only on OS >= Vista with IPv4 turned off.
        private IPAddress _bindToAddr = (Socket.OSSupportsIPv4) ? IPAddress.Any : IPAddress.IPv6Any; // address to bind to.
        private bool _bSuppressChannelData = false;  // should we hand out null for our channel data
        private bool _impersonate = false; // default is no impersonation
        private ProtectionLevel _protectionLevel = ProtectionLevel.EncryptAndSign;       // default is encrypt and sign is secure is true
        private bool _secure = false; // by default authentication is off
        private AsyncCallback _acceptSocketCallback;
        private IAuthorizeRemotingConnection _authorizeRemotingConnection;
        private bool authSet = false;

        private IServerChannelSinkProvider _sinkProvider = null;
        private TcpServerTransportSink    _transportSink = null;


        private ExclusiveTcpListener  _tcpListener;
        private bool                  _bExclusiveAddressUse = true;
        private bool                  _bListening = false;

        public TcpServerChannel(int port)
        {
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel

        public TcpServerChannel(String name, int port)
        {
            _channelName =name;
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel

        public TcpServerChannel(String name, int port, IServerChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _port = port;
            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel


        public TcpServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider) : this (properties, sinkProvider, null)
        {
        }
        
        public TcpServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider, IAuthorizeRemotingConnection authorizeCallback)
        {
            _authorizeRemotingConnection = authorizeCallback;
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;
                    case "bindTo": _bindToAddr = IPAddress.Parse((String)entry.Value); break;
                    case "port": _port = Convert.ToInt32(entry.Value, CultureInfo.InvariantCulture); break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value, CultureInfo.InvariantCulture); break;
                    case "secure": _secure = Convert.ToBoolean(entry.Value); break;
                    case "impersonate": _impersonate = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); authSet = true; break;
                    case "protectionLevel": _protectionLevel = (ProtectionLevel)(entry.Value is ProtectionLevel ? entry.Value :
                                                                                                  Enum.Parse(typeof(ProtectionLevel),
                                                                                                  (String)entry.Value, true));
                                             authSet = true;                                           
                                             break;

                    case "machineName": _forcedMachineName = (String)entry.Value; break;

                    case "rejectRemoteRequests":
                    {
                        bool bReject = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture);
                        if (bReject)
                        {
                            if (Socket.OSSupportsIPv4)
                            {
                                _bindToAddr = IPAddress.Loopback;
                            }
                            else
                            {
                                _bindToAddr = IPAddress.IPv6Loopback;
                            }
                        }
                        break;
                    }

                    case "suppressChannelData": _bSuppressChannelData = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "useIpAddress": _bUseIpAddress = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "exclusiveAddressUse": _bExclusiveAddressUse = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    case "authorizationModule": 
                          _authorizeRemotingConnection = (IAuthorizeRemotingConnection) Activator.CreateInstance(Type.GetType((String)entry.Value, true));
                          break;
                    default:
                         break;
                    }
                }

            }

            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel


        //
        // ISecurableChannel implementation
        //
        public bool IsSecured
        {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            get { return _secure; }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
            set { _secure = value; }
        }

        private void SetupMachineName()
        {
            if (_forcedMachineName != null)
            {
                // an explicitly configured machine name was used
                _machineName = CoreChannel.DecodeMachineName(_forcedMachineName);
            }
            else
            {
                if (!_bUseIpAddress)
                    _machineName = CoreChannel.GetMachineName();
                else
                {
                    if (_bindToAddr == IPAddress.Any || _bindToAddr == IPAddress.IPv6Any)
                    {
                        _machineName = CoreChannel.GetMachineIp();
                    }
                    else
                    {
                        _machineName = _bindToAddr.ToString();
                    }

                    // Add [] around the ipadress for IPv6
                    if (_bindToAddr.AddressFamily == AddressFamily.InterNetworkV6)
                        _machineName = "[" + _machineName + "]";

                }
            }
        } // SetupMachineName



        private void SetupChannel()
        {
            // set channel data
            // (These get changed inside of StartListening(), in the case where the listen
            //   port is 0, because we can't determine the port number until after the
            //   TcpListener starts.)

            if (authSet && !_secure)
                throw new RemotingException(CoreChannel.GetResourceString(
                                                "Remoting_Tcp_AuthenticationConfigServer"));

            _channelData = new ChannelDataStore(null);
            if (_port > 0)
            {
                _channelData.ChannelUris = new String[1];
                _channelData.ChannelUris[0] = GetChannelUri();
            }

            // set default provider (soap formatter) if no provider has been set
            if (_sinkProvider == null)
                _sinkProvider = CreateDefaultServerProviderChain();

            CoreChannel.CollectChannelDataFromServerSinkProviders(_channelData, _sinkProvider);

            // construct sink chain
            IServerChannelSink sink = ChannelServices.CreateServerChannelSinkChain(_sinkProvider, this);
            _transportSink = new TcpServerTransportSink(sink, _impersonate);
            // Initialize the accept socket callback
            _acceptSocketCallback = new AsyncCallback(AcceptSocketCallbackHelper);    
            if (_port >= 0)
            {
                // Open a TCP port and create a thread to start listening
                _tcpListener = new ExclusiveTcpListener(_bindToAddr, _port);
                // Wait for thread to spin up
                StartListening(null);
            }
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
            return TcpChannelHelper.ParseURL(url, out objectURI);
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
            return "tcp://" + _machineName + ":" + _port;
        } // GetChannelUri


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public virtual String[] GetUrlsForUri(String objectUri)
        {
            String[] retVal = new String[1];

            if (!objectUri.StartsWith("/", StringComparison.Ordinal))
                objectUri = "/" + objectUri;
            retVal[0] = GetChannelUri() + objectUri;

            return retVal;
        } // GetURLsforURI


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void StartListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HTTPChannel.StartListening");

            if (_port >= 0)
            {
                _tcpListener.Start(_bExclusiveAddressUse);
                _bListening = true;
                // get new port assignment if a port of 0 was used to auto-select a port
                if (_port == 0)
                {
                    _port = ((IPEndPoint)_tcpListener.LocalEndpoint).Port;
                    if (_channelData != null)
                    {
                        _channelData.ChannelUris = new String[1];
                        _channelData.ChannelUris[0] = GetChannelUri();
                    }
                }
                _tcpListener.BeginAcceptSocket(_acceptSocketCallback, null);
            }
        } // StartListening


        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public void StopListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HTTPChannel.StopListening");

            if (_port > 0)
            {
                _bListening = false;

                // Ask the TCP listener to stop listening on the port
                if(null != _tcpListener)
                {
                    _tcpListener.Stop();
                }
            }
        } // StopListening

        
        //
        // end of IChannelReceiver implementation
        //

        // Helper to invoke the method asynchronously if 
        // BeginAccept returns synchronously. In most cases
        // we will simply invoke AcceptSocketCallback
        void AcceptSocketCallbackHelper(IAsyncResult ar)
        {
            if (ar.CompletedSynchronously)
               ThreadPool.QueueUserWorkItem(new WaitCallback(this.AcceptSocketCallbackAsync), ar);
            else
                AcceptSocketCallback(ar);
        }

        // This method is used by the helper to convert a sync BeginAccept to async
        void AcceptSocketCallbackAsync(Object state)
        {
            AcceptSocketCallback((IAsyncResult) state);
        }

        // AcceptSocket method which will invoke the
        // authorization callbacks
        void AcceptSocketCallback(IAsyncResult ar)
        {
            Socket socket = null;
            InternalRemotingServices.RemotingTrace("TCPChannel::Listen - tcpListen.Pending() == true");
            TcpServerSocketHandler streamManager = null;
            bool closeImmediately = true;
            try
            {
                //
                // Wait for an incoming socket
                // if the listener is still active
                if (_tcpListener.IsListening)
                    _tcpListener.BeginAcceptSocket(_acceptSocketCallback, null);

                socket = _tcpListener.EndAcceptSocket(ar);

                if (socket == null)
                {
                    throw new RemotingException(
                        String.Format(
                            CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Socket_Accept"),
                            Marshal.GetLastWin32Error().ToString(CultureInfo.CurrentCulture)));
                }

                if (_authorizeRemotingConnection != null)
                {
                        bool authorized = _authorizeRemotingConnection.IsConnectingEndPointAuthorized(socket.RemoteEndPoint);
                        if (!authorized)
                            throw new RemotingException(CoreChannel.GetResourceString(
                                                                "Remoting_Tcp_ServerAuthorizationEndpointFailed"));
                }                        

                // disable nagle delay
                socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);
                // Set keepalive flag, so that inactive sockets can be cleaned up
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.KeepAlive, 1);

                // set linger option
                LingerOption lingerOption = new LingerOption(true, 3);
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, lingerOption);

                Stream netStream = new SocketStream(socket);
                streamManager = new TcpServerSocketHandler(socket, CoreChannel.RequestQueue, netStream);

#if !FEATURE_PAL
                WindowsIdentity identity = null;
#endif // !FEATURE_PAL
                // If authentication is requested wait for auth request.
                closeImmediately = false;
                if (_secure)
                {
#if !FEATURE_PAL
                    identity = Authenticate(ref netStream, streamManager);
                    // Create a new SocketHandler to wrap the new netStream
                    streamManager = new TcpServerSocketHandler(socket, CoreChannel.RequestQueue, netStream);
                    if (_authorizeRemotingConnection != null)
                    {
                        bool authorized = _authorizeRemotingConnection.IsConnectingIdentityAuthorized(identity);
                        if (!authorized)
                            throw new RemotingException(CoreChannel.GetResourceString(
                                                                "Remoting_Tcp_ServerAuthorizationIdentityFailed"));
                    }
#else
                    throw new NotSupportedException();
#endif // !FEATURE_PAL
                }

#if !FEATURE_PAL
                // Cache the identity for impersonation
                streamManager.ImpersonationIdentity = identity;
#endif // !FEATURE_PAL


                streamManager.DataArrivedCallback = new WaitCallback(_transportSink.ServiceRequest);
                streamManager.BeginReadMessage();
            }
            catch (Exception e)
            {
                // Close the socket pre-emptively. We also close the socket if
                // We need to catch all exceptions if we hit ObjectDisposedException
                try{
                    if (streamManager != null){
                        streamManager.SendErrorResponse(e, false);
                    }
                    if (socket != null){
                        if (closeImmediately)
                             socket.Close(0);
                        else
                            socket.Close();
                    }
                }catch(Exception){}
                if (!_bListening)
                {
                    // We called Stop() on the tcp listener, so gracefully exit.
                    //bOkToListen = false;
                }
                else
                {
                    // we want the exception to show up as unhandled since this
                    //   is an unexpected failure.
                    if (!(e is SocketException))
                    {
                        // <



                    }
                }
            }
        }
            
#if !FEATURE_PAL

        private WindowsIdentity Authenticate(ref Stream netStream, TcpServerSocketHandler streamManager)
        {
            // Use the identity for impersonation etc.
            NegotiateStream negoServer = null;
            try
            {
                negoServer = new NegotiateStream(netStream);
                // Block for authentication request
                TokenImpersonationLevel impLevel = TokenImpersonationLevel.Identification;
                if (_impersonate)
                    impLevel = TokenImpersonationLevel.Impersonation;
                negoServer.AuthenticateAsServer((NetworkCredential)CredentialCache.DefaultCredentials, _protectionLevel, impLevel);
                netStream = negoServer;
                return (WindowsIdentity)negoServer.RemoteIdentity;
            }
            catch
            {
                streamManager.SendErrorResponse(
                    String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_ServerAuthenticationFailed")), false);
                if (negoServer != null)
                    negoServer.Close();
                throw;
            }
        }

#endif // !FEATURE_PAL

    } // class TcpServerChannel



    internal class TcpServerTransportSink : IServerChannelSink
    {
        //private const int _defaultChunkSize = 4096;
        private const int s_MaxSize =  (2 << 24); // Max size of the payload

        // sink state
        private IServerChannelSink _nextSink;

        // AuthenticationMode
        private bool _impersonate;

        internal TcpServerTransportSink(IServerChannelSink nextSink, bool impersonate)
        {
            _nextSink = nextSink;
            _impersonate = impersonate;
        } // TcpServerTransportSink


        
        internal void ServiceRequest(Object state)
        {
            TcpServerSocketHandler streamManager = (TcpServerSocketHandler)state;

            ITransportHeaders headers = streamManager.ReadHeaders();
            Stream requestStream = streamManager.GetRequestStream();
            headers["__CustomErrorsEnabled"] = streamManager.CustomErrorsEnabled();

            // process request
            ServerChannelSinkStack sinkStack = new ServerChannelSinkStack();
            sinkStack.Push(this, streamManager);

            IMessage responseMessage;
            ITransportHeaders responseHeaders;
            Stream responseStream;

#if !FEATURE_PAL
            // If Impersonation was requested impersonate the client identity
            WindowsIdentity identity = streamManager.ImpersonationIdentity;
            WindowsImpersonationContext context = null;
            IPrincipal oldPrincipal = null;
            bool principalChanged = false;
            if(identity != null)
            {
                oldPrincipal = Thread.CurrentPrincipal;
                principalChanged = true;
                if (_impersonate)
                {
                    Thread.CurrentPrincipal = new WindowsPrincipal(identity);
                    context = identity.Impersonate();
                }
                else
                {
                    Thread.CurrentPrincipal = new GenericPrincipal(identity, null);
                }
            }
#endif // !FEATURE_PAL
            ServerProcessing processing;
            // wrap Undo in an outer try block
            try{
                try{
                    processing =
                        _nextSink.ProcessMessage(sinkStack, null, headers, requestStream,
                                                 out responseMessage,
                                                 out responseHeaders, out responseStream);
                }
                finally{
    #if !FEATURE_PAL
                    // Revert the principal if we had changed the principal
                    if (principalChanged)
                    {
                        Thread.CurrentPrincipal = oldPrincipal;
                    }
                    // Revert the impersonation if we had impersonated
                    if (_impersonate)
                    {
                        context.Undo();
                    }
    #endif // !FEATURE_PAL
                }
            }
            catch { throw; }

            // handle response
            switch (processing)
            {

            case ServerProcessing.Complete:
            {
                // Send the response. Call completed synchronously.
                sinkStack.Pop(this);
                streamManager.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.Complete

            case ServerProcessing.OneWay:
            {
                // No response needed, but the following method will make sure that
                //   we send at least a skeleton reply if the incoming request was
                //   not marked OneWayRequest (client/server metadata could be out of
                //   sync).
                streamManager.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.OneWay

            case ServerProcessing.Async:
            {
                sinkStack.StoreAndDispatch(this, streamManager);
                break;
            }// case ServerProcessing.Async

            } // switch (processing)


            // async processing will take care if handling this later
            if (processing != ServerProcessing.Async)
            {
                if (streamManager.CanServiceAnotherRequest())
                    streamManager.BeginReadMessage();
                else
                    streamManager.Close();
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
            TcpServerSocketHandler streamManager = null;

            streamManager = (TcpServerSocketHandler)state;

            // send the response
            streamManager.SendResponse(headers, stream);

            if (streamManager.CanServiceAnotherRequest())
                streamManager.BeginReadMessage();
            else
                streamManager.Close();
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


    } // class TcpServerTransportSink


} // namespace System.Runtime.Remoting.Channels.Tcp
