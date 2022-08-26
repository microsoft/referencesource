using System;
using System.Collections;
using System.Text;
using System.Runtime.Remoting.Channels.Ipc;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Security;
using System.Security.Permissions;

namespace System.AddIn.Hosting 
{
    //This class exists only to do two security asserts, allowing us to use remoting with partial-trust app domains in external processes.
    internal class AddInIpcChannel : IpcChannel
    {
        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IpcChannel..ctor(System.Collections.IDictionary,System.Runtime.Remoting.Channels.IClientChannelSinkProvider,System.Runtime.Remoting.Channels.IServerChannelSinkProvider)" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        public AddInIpcChannel(IDictionary props, IClientChannelSinkProvider client, IServerChannelSinkProvider server)
            : base(props, new AddInBinaryClientFormaterSinkProvider(client), new AddInBinaryServerFormaterSinkProvider(server))
        {
        }
    }

    internal class AddInBinaryServerFormaterSinkProvider : IServerChannelSinkProvider
    {
        internal IServerChannelSinkProvider _sinkProvider;

        public AddInBinaryServerFormaterSinkProvider(IServerChannelSinkProvider sink)
        {
            _sinkProvider = sink;
        }

        #region IServerChannelSinkProvider Members

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IServerChannelSinkProvider.CreateSink(System.Runtime.Remoting.Channels.IChannelReceiver):System.Runtime.Remoting.Channels.IServerChannelSink" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public IServerChannelSink CreateSink(IChannelReceiver channel)
        {
            return new AddInBinaryServerSink(_sinkProvider.CreateSink(channel));
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IServerChannelSinkProvider.GetChannelData(System.Runtime.Remoting.Channels.IChannelDataStore):System.Void" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public void GetChannelData(IChannelDataStore channelData)
        {
            _sinkProvider.GetChannelData(channelData);
        }

        public IServerChannelSinkProvider Next
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IServerChannelSinkProvider.get_Next():System.Runtime.Remoting.Channels.IServerChannelSinkProvider" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get
            {
                return _sinkProvider.Next;
            }
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IServerChannelSinkProvider.set_Next(System.Runtime.Remoting.Channels.IServerChannelSinkProvider):System.Void" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            set
            {
                _sinkProvider.Next = value;
            }
        }

        #endregion
    }

    internal class AddInBinaryServerSink : IServerChannelSink
    {
        #region IServerChannelSink Members

        private IServerChannelSink _sink;

        public AddInBinaryServerSink(IServerChannelSink sink)
        {
            _sink = sink;
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IServerChannelSink.AsyncProcessResponse(System.Runtime.Remoting.Channels.IServerResponseChannelSinkStack,System.Object,System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders,System.IO.Stream):System.Void" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, object state, IMessage msg, ITransportHeaders headers, System.IO.Stream stream)
        {
             _sink.AsyncProcessResponse(sinkStack, state, msg, headers, stream);
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IServerChannelSink.GetResponseStream(System.Runtime.Remoting.Channels.IServerResponseChannelSinkStack,System.Object,System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders):System.IO.Stream" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public System.IO.Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, object state, IMessage msg, ITransportHeaders headers)
        {
            return _sink.GetResponseStream(sinkStack, state, msg, headers);
        }

        public IServerChannelSink NextChannelSink
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IServerChannelSink.get_NextChannelSink():System.Runtime.Remoting.Channels.IServerChannelSink" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get { return _sink.NextChannelSink; }
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IServerChannelSink.ProcessMessage(System.Runtime.Remoting.Channels.IServerChannelSinkStack,System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders,System.IO.Stream,System.Runtime.Remoting.Messaging.IMessage&amp;,System.Runtime.Remoting.Channels.ITransportHeaders&amp;,System.IO.Stream&amp;):System.Runtime.Remoting.Channels.ServerProcessing" />
        ///<Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack, IMessage requestMsg, ITransportHeaders requestHeaders, System.IO.Stream requestStream, out IMessage responseMsg, out ITransportHeaders responseHeaders, out System.IO.Stream responseStream)
        {
            new System.Security.Permissions.SecurityPermission(SecurityPermissionFlag.SerializationFormatter | SecurityPermissionFlag.UnmanagedCode).Assert();
            return _sink.ProcessMessage(sinkStack, requestMsg, requestHeaders, requestStream, out responseMsg, out responseHeaders, out responseStream);
        }

        #endregion

        #region IChannelSinkBase Members

        public IDictionary Properties
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IChannelSinkBase.get_Properties():System.Collections.IDictionary" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get { return _sink.Properties; }
        }

        #endregion
    }



    internal class AddInBinaryClientFormaterSinkProvider : IClientChannelSinkProvider
    {
        IClientChannelSinkProvider _provider;

        public AddInBinaryClientFormaterSinkProvider(IClientChannelSinkProvider provider)
        {
            _provider = provider;
        }

        #region IClientChannelSinkProvider Members

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IClientChannelSinkProvider.CreateSink(System.Runtime.Remoting.Channels.IChannelSender,System.String,System.Object):System.Runtime.Remoting.Channels.IClientChannelSink" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public IClientChannelSink CreateSink(IChannelSender channel, string url, object remoteChannelData)
        {
            return new AddInBinaryClientFormaterSink(_provider.CreateSink(channel, url, remoteChannelData));
        }

        public IClientChannelSinkProvider Next
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IClientChannelSinkProvider.get_Next():System.Runtime.Remoting.Channels.IClientChannelSinkProvider" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get
            {
                return _provider.Next;
            }
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IClientChannelSinkProvider.set_Next(System.Runtime.Remoting.Channels.IClientChannelSinkProvider):System.Void" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            set
            {
                _provider.Next = value;
            }
        }

        #endregion
    }

    internal class AddInBinaryClientFormaterSink : IClientChannelSink, IMessageSink
    {
        IClientChannelSink _sink;
        IMessageSink _mSink;

        public AddInBinaryClientFormaterSink(IClientChannelSink sink)
        {
            _sink = sink;
            _mSink = (IMessageSink)sink;
        }

        #region IClientChannelSink Members

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IClientChannelSink.AsyncProcessRequest(System.Runtime.Remoting.Channels.IClientChannelSinkStack,System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders,System.IO.Stream):System.Void" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg, ITransportHeaders headers, System.IO.Stream stream)
        {
             _sink.AsyncProcessRequest(sinkStack, msg, headers, stream);
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IClientChannelSink.AsyncProcessResponse(System.Runtime.Remoting.Channels.IClientResponseChannelSinkStack,System.Object,System.Runtime.Remoting.Channels.ITransportHeaders,System.IO.Stream):System.Void" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, object state, ITransportHeaders headers, System.IO.Stream stream)
        {
             _sink.AsyncProcessResponse(sinkStack, state, headers, stream);
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IClientChannelSink.GetRequestStream(System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders):System.IO.Stream" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public System.IO.Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            return _sink.GetRequestStream(msg, headers);
        }

        public IClientChannelSink NextChannelSink
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IClientChannelSink.get_NextChannelSink():System.Runtime.Remoting.Channels.IClientChannelSink" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get { return _sink.NextChannelSink; }
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IClientChannelSink.ProcessMessage(System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Channels.ITransportHeaders,System.IO.Stream,System.Runtime.Remoting.Channels.ITransportHeaders&amp;,System.IO.Stream&amp;):System.Void" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public void ProcessMessage(IMessage msg, ITransportHeaders requestHeaders, System.IO.Stream requestStream, out ITransportHeaders responseHeaders, out System.IO.Stream responseStream)
        {
            _sink.ProcessMessage(msg, requestHeaders, requestStream, out responseHeaders,out responseStream);
        }

        #endregion

        #region IChannelSinkBase Members

        public IDictionary Properties
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IChannelSinkBase.get_Properties():System.Collections.IDictionary" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get { return _sink.Properties; }
        }

        #endregion

        #region IMessageSink Members

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IMessageSink.AsyncProcessMessage(System.Runtime.Remoting.Messaging.IMessage,System.Runtime.Remoting.Messaging.IMessageSink):System.Runtime.Remoting.Messaging.IMessageCtrl" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            return _mSink.AsyncProcessMessage(msg, replySink);
        }

        public IMessageSink NextSink
        {
            ///<SecurityKernel Critical="True" Ring="0">
            ///<SatisfiesLinkDemand Name="IMessageSink.get_NextSink():System.Runtime.Remoting.Messaging.IMessageSink" />
            ///</SecurityKernel>
            [System.Security.SecurityCritical]
            get { return _mSink.NextSink; }
        }

        ///<SecurityKernel Critical="True" Ring="0">
        ///<SatisfiesLinkDemand Name="IMessageSink.SyncProcessMessage(System.Runtime.Remoting.Messaging.IMessage):System.Runtime.Remoting.Messaging.IMessage" />
        ///<Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        ///</SecurityKernel>
        [System.Security.SecurityCritical]
        public IMessage SyncProcessMessage(IMessage msg)
        {
            new System.Security.Permissions.SecurityPermission(SecurityPermissionFlag.SerializationFormatter | SecurityPermissionFlag.UnmanagedCode).Assert();
            return _mSink.SyncProcessMessage(msg);
        }

        #endregion
    }
}

