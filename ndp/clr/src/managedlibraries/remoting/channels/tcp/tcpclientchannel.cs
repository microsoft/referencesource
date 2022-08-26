// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//===========================================================================
//  File:       TcpClientChannel.cs
//
//  Summary:    Implements a channel that transmits method calls over TCP.
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
using System.Threading;
using System.Text;
using System.Globalization;
#if !FEATURE_PAL
using System.Security.Authentication;
#endif
using System.Security.Principal;
using System.Security.Permissions;




namespace System.Runtime.Remoting.Channels.Tcp
{

    public class TcpClientChannel : IChannelSender, ISecurableChannel
    {
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "tcp"; // channel name
        private bool _secure = false;
        private IDictionary _prop = null;

        private IClientChannelSinkProvider _sinkProvider = null; // sink chain provider


        public TcpClientChannel()
        {
            SetupChannel();        
        } // TcpClientChannel


        public TcpClientChannel(String name, IClientChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _sinkProvider = sinkProvider;

            SetupChannel();
        }


        // constructor used by config file
        public TcpClientChannel(IDictionary properties, IClientChannelSinkProvider sinkProvider)
        {
            if (properties != null)
            {
                _prop = properties;
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value, CultureInfo.InvariantCulture); break;
                    case "secure": _secure = Convert.ToBoolean(entry.Value, CultureInfo.InvariantCulture); break;
                    default:
                         break;
                    }
                }
            }

            _sinkProvider = sinkProvider;
            SetupChannel();
        } // TcpClientChannel


        private void SetupChannel()
        {
            if (_sinkProvider != null)
            {
                CoreChannel.AppendProviderToClientProviderChain(
                    _sinkProvider, new TcpClientTransportSinkProvider(_prop));
            }
            else
                _sinkProvider = CreateDefaultClientProviderChain();
        } // SetupChannel

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
        // IChannelSender implementation
        //

        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure, Infrastructure=true)]
        public virtual IMessageSink CreateMessageSink(String url, Object remoteChannelData, out String objectURI)
        {
            // Set the out parameters
            objectURI = null;
            String channelURI = null;


            if (url != null) // Is this a well known object?
            {
                // Parse returns null if this is not one of our url's
                channelURI = Parse(url, out objectURI);
            }
            else // determine if we want to connect based on the channel data
            {
                if (remoteChannelData != null)
                {
                    if (remoteChannelData is IChannelDataStore)
                    {
                        IChannelDataStore cds = (IChannelDataStore)remoteChannelData;

                        // see if this is an tcp uri
                        String simpleChannelUri = Parse(cds.ChannelUris[0], out objectURI);
                        if (simpleChannelUri != null)
                            channelURI = cds.ChannelUris[0];
                    }
                }
            }

            if (null != channelURI)
            {
                if (url == null)
                    url = channelURI;

                IClientChannelSink sink = _sinkProvider.CreateSink(this, url, remoteChannelData);

                // return sink after making sure that it implements IMessageSink
                IMessageSink msgSink = sink as IMessageSink;
                if ((sink != null) && (msgSink == null))
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString(
                            "Remoting_Channels_ChannelSinkNotMsgSink"));
                }

                return msgSink;
            }

            return null;
        } // CreateMessageSink


        //
        // end of IChannelSender implementation
        //

        private IClientChannelSinkProvider CreateDefaultClientProviderChain()
        {
            IClientChannelSinkProvider chain = new BinaryClientFormatterSinkProvider();
            IClientChannelSinkProvider sink = chain;

            sink.Next = new TcpClientTransportSinkProvider(_prop);

            return chain;
        } // CreateDefaultClientProviderChain

    } // class TcpClientChannel




    internal class TcpClientTransportSinkProvider : IClientChannelSinkProvider
    {
        IDictionary _prop = null;
        internal TcpClientTransportSinkProvider(IDictionary properties)
        {
            _prop = properties;
        }

        public IClientChannelSink CreateSink(IChannelSender channel, String url,
                                             Object remoteChannelData)
        {
            // url is set to the channel uri in CreateMessageSink
            TcpClientTransportSink sink = new TcpClientTransportSink(url, (TcpClientChannel) channel);
            if (_prop != null)
            {
                // Tansfer all properties from the channel to the sink
                foreach(Object key in _prop.Keys)
                {
                    sink[key] = _prop[key];
                }
            }
            return sink;
        }

        public IClientChannelSinkProvider Next
        {
            get { return null; }
            set { throw new NotSupportedException(); }
        }
    } // class TcpClientTransportSinkProvider



    internal class TcpClientTransportSink : BaseChannelSinkWithProperties, IClientChannelSink
    {
        // socket cache
        internal SocketCache ClientSocketCache;
        private bool authSet = false;

        private SocketHandler CreateSocketHandler(
            Socket socket, SocketCache socketCache, String machinePortAndSid)
        {
            Stream netStream = new SocketStream(socket);

            // Check if authentication is requested
            if(_channel.IsSecured)
            {
#if !FEATURE_PAL
                netStream = CreateAuthenticatedStream(netStream, machinePortAndSid);
#else
                throw new NotSupportedException();
#endif
            }

            return new TcpClientSocketHandler(socket, machinePortAndSid, netStream, this);
        } // CreateSocketHandler

#if !FEATURE_PAL
        private Stream CreateAuthenticatedStream(Stream netStream, String machinePortAndSid)
        {
            //Check for explicitly set userName, and authenticate using it
            NetworkCredential credentials = null;
            NegotiateStream negoClient = null;

            if (_securityUserName != null)
            {
                credentials = new NetworkCredential(_securityUserName, _securityPassword,  _securityDomain);
            }
            //else use default Credentials
            else
            {
                credentials = (NetworkCredential)CredentialCache.DefaultCredentials;
            }

            try {
                negoClient = new NegotiateStream(netStream);
                negoClient.AuthenticateAsClient(credentials, _spn, _protectionLevel, _tokenImpersonationLevel);
            }
            catch(IOException e){
                throw new RemotingException(
                            String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_AuthenticationFailed")), e);

            }
            return negoClient;

        }
#endif // !FEATURE_PAL

        // returns the connectiongroupname else the Sid for current credentials
        private String GetSid()
        {
            if (_connectionGroupName != null)
            {
                    return _connectionGroupName;
            }
#if !FEATURE_PAL            
            return CoreChannel.GetCurrentSidString();
#else
            return null;
#endif
        }

        // transport sink state
        private String m_machineName;
        private int    m_port;
        private TcpClientChannel _channel = null;

        private String _machineAndPort;
        private const String UserNameKey = "username";
        private const String PasswordKey = "password";
        private const String DomainKey = "domain";
        private const String ProtectionLevelKey = "protectionlevel";
        private const String ConnectionGroupNameKey = "connectiongroupname";
#if !FEATURE_PAL
        private const String TokenImpersonationLevelKey = "tokenimpersonationlevel";
#endif // !FEATURE_PAL
        private const String SocketCacheTimeoutKey = "socketcachetimeout";
        private const String ReceiveTimeoutKey = "timeout";
        private const String SocketCachePolicyKey = "socketcachepolicy";
        private const String SPNKey = "serviceprincipalname";
        private const String RetryCountKey = "retrycount";

        // property values
        private String _securityUserName = null;
        private String _securityPassword = null;
        private String _securityDomain = null;
        private String _connectionGroupName = null;
        private TimeSpan _socketCacheTimeout = TimeSpan.FromSeconds(10); // default timeout is 5 seconds
        private int _receiveTimeout = 0; // default timeout is infinite
        private SocketCachePolicy _socketCachePolicy = SocketCachePolicy.Default; // default is v1.0 behaviour
        private String _spn = string.Empty;
        private int _retryCount = 1; // defualt retry is 1 to keep it equivalent with v1.1
#if !FEATURE_PAL
        private TokenImpersonationLevel _tokenImpersonationLevel = TokenImpersonationLevel.Identification; // default is no authentication
#endif // !FEATURE_PAL
        private ProtectionLevel _protectionLevel = ProtectionLevel.EncryptAndSign;
        private static ICollection s_keySet = null;

        internal TcpClientTransportSink(String channelURI, TcpClientChannel channel)
        {
            String objectURI;
            _channel = channel;
            String simpleChannelUri = TcpChannelHelper.ParseURL(channelURI, out objectURI);
            ClientSocketCache = new SocketCache(new SocketHandlerFactory(this.CreateSocketHandler), _socketCachePolicy,
                                                     _socketCacheTimeout);
            // extract machine name and port
            Uri uri = new Uri(simpleChannelUri);

            if (uri.IsDefaultPort)
            {
                // If there is no colon, then there is no port number.
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString(
                            "Remoting_Tcp_UrlMustHavePort"),
                        channelURI));
            }
            m_machineName = uri.Host;
            IPAddress ipAddr = null;
            IPAddress.TryParse(m_machineName, out ipAddr);
            if ((ipAddr != null) && (ipAddr.IsIPv6LinkLocal || ipAddr.IsIPv6SiteLocal))
            {
                m_machineName = "[" + uri.DnsSafeHost + "]";
            }


            m_port = uri.Port;

            _machineAndPort = m_machineName + ":" + m_port;
        } // TcpClientTransportSink

        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {
            InternalRemotingServices.RemotingTrace("TcpClientTransportSink::ProcessMessage");

            // the call to SendRequest can block a func eval, so we want to notify the debugger that we're
            // about to call a blocking operation. 
            System.Diagnostics.Debugger.NotifyOfCrossThreadDependency();            

            TcpClientSocketHandler clientSocket =
                SendRequestWithRetry(msg, requestHeaders, requestStream);

            // receive response
            responseHeaders = clientSocket.ReadHeaders();
            responseStream = clientSocket.GetResponseStream();

            // The client socket will be returned to the cache
            //   when the response stream is closed.

        } // ProcessMessage


        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            InternalRemotingServices.RemotingTrace("TcpClientTransportSink::AsyncProcessRequest");

            TcpClientSocketHandler clientSocket =
                SendRequestWithRetry(msg, headers, stream);

            if (clientSocket.OneWayRequest)
            {
                clientSocket.ReturnToCache();
            }
            else
            {
                // do an async read on the reply
                clientSocket.DataArrivedCallback = new WaitCallback(this.ReceiveCallback);
                clientSocket.DataArrivedCallbackState = sinkStack;
                clientSocket.BeginReadMessage();
            }
        } // AsyncProcessRequest


        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, Object state,
                                         ITransportHeaders headers, Stream stream)
        {
            // We don't have to implement this since we are always last in the chain.
            throw new NotSupportedException();
        } // AsyncProcessRequest



        public Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            // Currently, we require a memory stream be handed to us since we need
            //   the length before sending.
            return null;
        } // GetRequestStream


        public IClientChannelSink NextChannelSink
        {
            get { return null; }
        } // Next


        private TcpClientSocketHandler SendRequestWithRetry(IMessage msg,
                                                            ITransportHeaders requestHeaders,
                                                            Stream requestStream)
        {
            // If the stream is seekable, we can retry once on a failure to write.
            long initialPosition = 0;
            bool sendException = true;
            bool bCanSeek = requestStream.CanSeek;
            if (bCanSeek)
                initialPosition = requestStream.Position;

            TcpClientSocketHandler clientSocket = null;
            // Add the sid string only if the channel is secure.
            String machinePortAndSid = _machineAndPort + (_channel.IsSecured ? "/" + GetSid() : null);
            
            // The authentication config entries are only valid if secure is true
            if (authSet && !_channel.IsSecured)
                throw new RemotingException(CoreChannel.GetResourceString(
                                                "Remoting_Tcp_AuthenticationConfigClient"));

            // If explicitUserName is set but connectionGroupName isnt we will need to authenticate on each call
            bool openNewAlways = (_channel.IsSecured)
                                    && (_securityUserName != null) && (_connectionGroupName == null);

            try
            {
                clientSocket = (TcpClientSocketHandler)ClientSocketCache.GetSocket(machinePortAndSid, openNewAlways);
                clientSocket.SendRequest(msg, requestHeaders, requestStream);
            }
            catch (SocketException)
            {
                // Retry sending if socketexception occured, stream is seekable, retrycount times
                for(int count = 0; (count < _retryCount) && (bCanSeek && sendException); count++)
                {
                    // retry sending if possible
                    try
                    {
                        // reset position...
                        requestStream.Position = initialPosition;

                        // ...and try again.
                        clientSocket = (TcpClientSocketHandler)
                            ClientSocketCache.GetSocket(machinePortAndSid, openNewAlways);

                        clientSocket.SendRequest(msg, requestHeaders, requestStream);
                        sendException = false;
                    }
                    catch(SocketException)
                    {
                    }
                }
                if (sendException){
                    throw;
                }
            }

            requestStream.Close();

            return clientSocket;
        } // SendRequestWithRetry


        private void ReceiveCallback(Object state)
        {
            TcpClientSocketHandler clientSocket = null;
            IClientChannelSinkStack sinkStack = null;

            try
            {
                clientSocket = (TcpClientSocketHandler)state;
                sinkStack = (IClientChannelSinkStack)clientSocket.DataArrivedCallbackState;

                ITransportHeaders responseHeaders = clientSocket.ReadHeaders();
                Stream responseStream = clientSocket.GetResponseStream();

                // call down the sink chain
                sinkStack.AsyncProcessResponse(responseHeaders, responseStream);
            }
            catch (Exception e)
            {
                try
                {
                    if (sinkStack != null)
                        sinkStack.DispatchException(e);
                }
                catch
                {
                    // Fatal Error.. ignore
                }
            }

            // The client socket will be returned to the cache
            //   when the response stream is closed.

        } // ReceiveCallback



        //
        // Properties
        //
        public override Object this[Object key]
        {
            get
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return null;

                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                case UserNameKey: return _securityUserName;
                case PasswordKey: return null; // Intentionally refuse to return password.
                case DomainKey: return _securityDomain;
                case SocketCacheTimeoutKey: return _socketCacheTimeout;
                case ReceiveTimeoutKey: return _receiveTimeout;
                case SocketCachePolicyKey: return _socketCachePolicy.ToString();
                case RetryCountKey: return _retryCount;
                case ConnectionGroupNameKey: return _connectionGroupName;
#if !FEATURE_PAL
                case TokenImpersonationLevelKey: 
                    if (authSet)
                        return _tokenImpersonationLevel.ToString();
                    break;
#endif // !FEATURE_PAL
                case ProtectionLevelKey: 
                    if (authSet)
                        return _protectionLevel.ToString();
                    break;
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulture))

                return null;
            }

            set
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return;

                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                case UserNameKey: _securityUserName = (String)value; break;
                case PasswordKey: _securityPassword = (String)value; break;
                case DomainKey: _securityDomain = (String)value; break;
                case SocketCacheTimeoutKey: 
                                                int timeout = Convert.ToInt32(value, CultureInfo.InvariantCulture);
                                                if (timeout < 0)
                                                    throw new RemotingException(
                                                            CoreChannel.GetResourceString(
                                                            "Remoting_Tcp_SocketTimeoutNegative"));
                                                _socketCacheTimeout = TimeSpan.FromSeconds(timeout);
                                                ClientSocketCache.SocketTimeout = _socketCacheTimeout;
                                                break;
                case ReceiveTimeoutKey: 
                                                _receiveTimeout = Convert.ToInt32(value, CultureInfo.InvariantCulture);
                                                ClientSocketCache.ReceiveTimeout = _receiveTimeout;
                                                break;
                case SocketCachePolicyKey:  _socketCachePolicy = (SocketCachePolicy)(value is SocketCachePolicy ?  value :
                                                                                            Enum.Parse(typeof(SocketCachePolicy),
                                                                                                       (String)value, true));
                                                ClientSocketCache.CachePolicy = _socketCachePolicy;
                                                break;
                case RetryCountKey: _retryCount = Convert.ToInt32(value, CultureInfo.InvariantCulture); break;
                case ConnectionGroupNameKey: _connectionGroupName = (String)value; break;
#if !FEATURE_PAL
                case TokenImpersonationLevelKey: _tokenImpersonationLevel = (TokenImpersonationLevel)(value is TokenImpersonationLevel ? value :
                                                                                            Enum.Parse(typeof(TokenImpersonationLevel),
                                                                                               (String)value, true));
                                          authSet = true;                                                  
                                          break;
#endif // !FEATURE_PAL
                case ProtectionLevelKey: _protectionLevel = (ProtectionLevel)(value is ProtectionLevel ? value :
                                                                                         Enum.Parse(typeof(ProtectionLevel),
                                                                                         (String)value, true));
                                           authSet = true;                                              
                                          break;
                case SPNKey: _spn =  (String)value; authSet = true; break;
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulturey))
            }
        } // this[]


        public override ICollection Keys
        {
            get
            {
                if (s_keySet == null)
                {
                    // Don't need to synchronize. Doesn't matter if the list gets
                    // generated twice.
                    ArrayList keys = new ArrayList(6);
                    keys.Add(UserNameKey);
                    keys.Add(PasswordKey);
                    keys.Add(DomainKey);
                    keys.Add(SocketCacheTimeoutKey);
                    keys.Add(SocketCachePolicyKey);
                    keys.Add(RetryCountKey);
#if !FEATURE_PAL
                    keys.Add(TokenImpersonationLevelKey);
#endif // !FEATURE_PAL
                    keys.Add(ProtectionLevelKey);
                    keys.Add(ConnectionGroupNameKey);
                    keys.Add(ReceiveTimeoutKey);

                    s_keySet = keys;
                }

                return s_keySet;
            }
        } // Keys

        //
        // end of Properties
        //

    } // class TcpClientTransportSink


} // namespace namespace System.Runtime.Remoting.Channels.Tcp
