// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       IpcClientChannel.cs
//  Author:   Microsoft@Microsoft.Com
//  Summary:    Implements a channel that transmits method calls over Ipc.
//
//==========================================================================

using System;
using System.Collections;
using System.IO;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading;
using System.Text;
using System.Globalization;
using System.Security.Permissions;


namespace System.Runtime.Remoting.Channels.Ipc
{
    internal delegate IClientChannelSinkStack AsyncMessageDelegate(IMessage msg,
                                                               ITransportHeaders requestHeaders, Stream requestStream,
                                                               out ITransportHeaders responseHeaders, out Stream responseStream,
                                                               IClientChannelSinkStack sinkStack);
    
    public class IpcClientChannel : IChannelSender, ISecurableChannel
    {
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "ipc client"; // channel name
        private bool _secure = false; // default is unsecure
        private IDictionary _prop = null;
        
        private IClientChannelSinkProvider _sinkProvider = null; // sink chain provider
        

        public IpcClientChannel()
        {
            SetupChannel();        
        } // IpcClientChannel


        public IpcClientChannel(String name, IClientChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _sinkProvider = sinkProvider;

            SetupChannel();
        }


        // constructor used by config file
        public IpcClientChannel(IDictionary properties, IClientChannelSinkProvider sinkProvider)
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
        } // IpcClientChannel


        private void SetupChannel()
        {
            if (_sinkProvider != null)
            {
                CoreChannel.AppendProviderToClientProviderChain(
                    _sinkProvider, new IpcClientTransportSinkProvider(_prop));                                                
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
            return IpcChannelHelper.ParseURL(url, out objectURI);
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

                        // see if this is an Ipc uri
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
            
            sink.Next = new IpcClientTransportSinkProvider( _prop);
            
            return chain;
        } // CreateDefaultClientProviderChain

    } // class IpcClientChannel




    internal class IpcClientTransportSinkProvider : IClientChannelSinkProvider
    {
        IDictionary _prop = null;
        internal IpcClientTransportSinkProvider(IDictionary properties )
        {
            _prop = properties;
        }    
   
        public IClientChannelSink CreateSink(IChannelSender channel, String url, 
                                             Object remoteChannelData)
        {
            // url is set to the channel uri in CreateMessageSink        
            IpcClientTransportSink sink = new IpcClientTransportSink(url, (IpcClientChannel) channel);
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
    } // class IpcClientTransportSinkProvider



    internal class IpcClientTransportSink : BaseChannelSinkWithProperties, IClientChannelSink
    {
        // Port cache
        private ConnectionCache portCache = new ConnectionCache();
        
        private IpcClientChannel _channel = null;

        // port name
        private String _portName;
        private const String TokenImpersonationLevelKey = "tokenimpersonationlevel";
        private bool authSet = false;
        private const String ConnectionTimeoutKey = "connectiontimeout";
        private TokenImpersonationLevel _tokenImpersonationLevel = TokenImpersonationLevel.Identification; 
        private int _timeout = 1000; // Default is one second
        private static ICollection s_keySet = null;

        internal IpcClientTransportSink(String channelURI, IpcClientChannel channel)
        {
            String objectURI;
            _channel = channel;
            // Parse the URI 
            String simpleChannelUri = IpcChannelHelper.ParseURL(channelURI, out objectURI);
            // extract machine name and port
            int start = simpleChannelUri.IndexOf("://");
            start += 3;

            _portName = simpleChannelUri.Substring(start);;
        } // IpcClientTransportSink

        internal ConnectionCache Cache { get { return portCache; } }

        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {
            InternalRemotingServices.RemotingTrace("IpcClientChannel::ProcessMessage");
            IpcPort port = null;

            // the call to SendRequest can block a func eval, so we want to notify the debugger that we're
            // about to call a blocking operation. 
            System.Diagnostics.Debugger.NotifyOfCrossThreadDependency();            

            // The authentication config entries are only valid if secure is true
            if (authSet && ! _channel.IsSecured)
                throw new RemotingException(CoreChannel.GetResourceString(
                                                "Remoting_Ipc_AuthenticationConfig"));

            port = portCache.GetConnection(_portName, _channel.IsSecured, _tokenImpersonationLevel, _timeout);

            IMethodCallMessage mcm = (IMethodCallMessage)msg;
            int requestLength = (int)requestStream.Length;

            Stream ipcStream = new PipeStream(port);
            IpcClientHandler handler = new IpcClientHandler(port, ipcStream, this);
            handler.SendRequest(msg, requestHeaders, requestStream);
            responseHeaders = handler.ReadHeaders();
            responseStream = handler.GetResponseStream();

            // The client port will be returned to the cache
            //   when the response stream is closed.

        } // ProcessMessage

        private IClientChannelSinkStack AsyncProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream, 
                                   IClientChannelSinkStack sinkStack)
        {
            ProcessMessage(msg, requestHeaders, requestStream, out responseHeaders, out responseStream);
            return sinkStack;
        }

        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream requestStream)
        {
            InternalRemotingServices.RemotingTrace("IpcClientTransportSink::AsyncProcessRequest");
            ITransportHeaders responseHeaders;
            Stream responseStream;
            // Invoke ProcessMessage asynchronously. This should be improved
            // by using Datagram message 
            AsyncCallback callback = new AsyncCallback(this.AsyncFinishedCallback);
            AsyncMessageDelegate async = new AsyncMessageDelegate(this.AsyncProcessMessage);
            async.BeginInvoke(msg, headers, requestStream , out responseHeaders, 
                                                         out responseStream, sinkStack, callback, null);
        } // AsyncProcessRequest

        private void AsyncFinishedCallback(IAsyncResult ar)
        {
            IClientChannelSinkStack sinkStack = null;
            try            
            {
                AsyncMessageDelegate d = (AsyncMessageDelegate)(((AsyncResult)ar).AsyncDelegate);
                ITransportHeaders responseHeaders;
                Stream responseStream;
                sinkStack = d.EndInvoke(out responseHeaders, out responseStream, ar);
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

        }

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
                    case TokenImpersonationLevelKey: return _tokenImpersonationLevel.ToString();
                    case ConnectionTimeoutKey: return _timeout;
                    default:
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
                    case TokenImpersonationLevelKey: _tokenImpersonationLevel = (TokenImpersonationLevel)(value is TokenImpersonationLevel ? value :
                                                                                            Enum.Parse(typeof(TokenImpersonationLevel),
                                                                                               (String)value, true)); 
                                                     authSet = true;
                          break;
                    case ConnectionTimeoutKey: _timeout = Convert.ToInt32(value, CultureInfo.InvariantCulture); 
                          break;
                    default:
                          break;
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
                }

                return s_keySet;
            }
        } // Keys

        //
        // end of Properties
        //

    } // class IpcClientTransportSink


} // namespace namespace System.Runtime.Remoting.Channels.Ipc
