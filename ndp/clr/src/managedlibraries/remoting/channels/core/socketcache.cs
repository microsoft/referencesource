// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SocketCache.cs
//
//  Summary:    Cache for client sockets.
//
//==========================================================================


using System;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Threading;


namespace System.Runtime.Remoting.Channels
{

    // Delegate to method that will fabricate the appropriate socket handler
    internal delegate SocketHandler SocketHandlerFactory(Socket socket, 
                                                         SocketCache socketCache,
                                                         String machineAndPort);


    // Used to cache client connections to a single port on a server
    internal class RemoteConnection
    {
        private static char[] colonSep = new char[]{':'};
    
        private CachedSocketList _cachedSocketList;        

        // reference back to the socket cache
        private SocketCache _socketCache;        

        // remote endpoint data
        private String     _machineAndPort;
        private IPAddress[] _addressList;
        private int _port;
        private EndPoint _lkgIPEndPoint;
        private bool connectIPv6 = false;
        private Uri _uri;


        internal RemoteConnection(SocketCache socketCache, String machineAndPort)
        {
            _socketCache = socketCache;

            _cachedSocketList = new CachedSocketList(socketCache.SocketTimeout, 
                                                                        socketCache.CachePolicy);

            // parse "machinename:port", add a dummy scheme to get uri parsing
            _uri = new Uri("dummy://" + machineAndPort);

            _port = _uri.Port;
            _machineAndPort = machineAndPort;

        } // RemoteConnection


        internal SocketHandler GetSocket()
        {
            // try the cached socket list
            SocketHandler socketHandler = _cachedSocketList.GetSocket();
            if (socketHandler != null)
                return socketHandler;

            // Otherwise, we'll just create a new one.
            return CreateNewSocket();
        } // GetSocket

        internal void ReleaseSocket(SocketHandler socket)
        {
            socket.ReleaseControl();
            _cachedSocketList.ReturnSocket(socket);
        } // ReleaseSocket


        private bool HasIPv6Address(IPAddress[] addressList)
        {
            foreach (IPAddress address in addressList)
            {
                if (address.AddressFamily == AddressFamily.InterNetworkV6)
                    return true;
            }
            return false;
        }

        private void DisableNagleDelays(Socket socket)
        {
            // disable nagle delays                                           
            socket.SetSocketOption(SocketOptionLevel.Tcp, 
                                   SocketOptionName.NoDelay,
                                   1);
        }
        
        private SocketHandler CreateNewSocket()
        {
            //
            // We should not cache the DNS result
            //
            _addressList = Dns.GetHostAddresses(_uri.DnsSafeHost);
            connectIPv6 = Socket.OSSupportsIPv6 && HasIPv6Address(_addressList);

            // If there is only one entry in the list, just use that 
            // we will fail if the connect on it fails
            if (_addressList.Length == 1)
                    return CreateNewSocket(new IPEndPoint(_addressList[0], _port));

            // If LKG is set try using that
            if (_lkgIPEndPoint != null)
            {
                try{
                    return CreateNewSocket(_lkgIPEndPoint);
                }
                catch (Exception) {
                    // Since this fail null out LKG
                    _lkgIPEndPoint = null;    
                }
            }

            // If IPv6 is enabled try connecting to IP addresses
            if (connectIPv6)
            {
                try{
                    return CreateNewSocket(AddressFamily.InterNetworkV6);
                }catch (Exception) {}
            }

            // If everything fails try ipv4 addresses
            return CreateNewSocket(AddressFamily.InterNetwork);
                
        }
        
        private SocketHandler CreateNewSocket(EndPoint ipEndPoint)
        {        
            Socket socket = new Socket(ipEndPoint.AddressFamily,
                                       SocketType.Stream,
                                       ProtocolType.Tcp);

            DisableNagleDelays(socket);
            InternalRemotingServices.RemotingTrace("RemoteConnection::CreateNewSocket: connecting new socket :: " + ipEndPoint);

            socket.Connect(ipEndPoint);
            _lkgIPEndPoint = socket.RemoteEndPoint;
            return _socketCache.CreateSocketHandler(socket, _machineAndPort);
        } // CreateNewSocket

        private SocketHandler CreateNewSocket(AddressFamily family)
        {        
            Socket socket = new Socket(family,
                                       SocketType.Stream,
                                       ProtocolType.Tcp);

            DisableNagleDelays(socket);

            socket.Connect(_addressList, _port);
            _lkgIPEndPoint = socket.RemoteEndPoint;
            return _socketCache.CreateSocketHandler(socket, _machineAndPort);
        } // CreateNewSocket


        internal void TimeoutSockets(DateTime currentTime)
        {            
            _cachedSocketList.TimeoutSockets(currentTime, _socketCache.SocketTimeout);                 
        } // TimeoutSockets


            
    } // class RemoteConnection



    internal class CachedSocket
    {
        private SocketHandler _socket;
        private DateTime _socketLastUsed;

        private CachedSocket _next;

        internal CachedSocket(SocketHandler socket, CachedSocket next)
        {
            _socket = socket;
            _socketLastUsed = DateTime.UtcNow;
            _next = next;
        } // CachedSocket        

        internal SocketHandler Handler { get { return _socket; } }
        internal DateTime LastUsed { get { return _socketLastUsed; } }

        internal CachedSocket Next 
        {
            get { return _next; }
            set { _next = value; }
        } 
        
    } // class CachedSocket


    internal class CachedSocketList
    {
        private int _socketCount;
        private TimeSpan _socketLifetime;
        private SocketCachePolicy _socketCachePolicy;
        private CachedSocket _socketList; // linked list

        internal CachedSocketList(TimeSpan socketLifetime, SocketCachePolicy socketCachePolicy)
        {
            _socketCount = 0;
            _socketLifetime = socketLifetime;
            _socketCachePolicy = socketCachePolicy;
            _socketList = null;
        } // CachedSocketList
        

        internal SocketHandler GetSocket()
        {
            if (_socketCount == 0)
                return null;
        
            lock (this)
            {    
                if (_socketList != null)
                {
                    SocketHandler socket = _socketList.Handler;
                    _socketList = _socketList.Next;

                    bool bRes = socket.RaceForControl();

                    // We should always have control since there shouldn't
                    //   be contention here.
                    InternalRemotingServices.RemotingAssert(bRes, "someone else has the socket?");

                    _socketCount--;
                    return socket;
                }                                      
            }

            return null;
        } // GetSocket


        internal void ReturnSocket(SocketHandler socket)
        {                        
            TimeSpan socketActiveTime = DateTime.UtcNow - socket.CreationTime;
            bool closeSocket = false;
            lock (this)
            {
                // Check if socket active time has exceeded the lifetime
                // If it has just close the Socket
                if (_socketCachePolicy != SocketCachePolicy.AbsoluteTimeout ||
                    socketActiveTime < _socketLifetime)
                {
                    // Ignore duplicate entries for the same socket handler
                    for (CachedSocket curr = _socketList; curr != null; curr = curr.Next){
                        if (socket == curr.Handler)
                            return;
                    }
                    
                    _socketList = new CachedSocket(socket, _socketList);
                    _socketCount++;                      
                }
                else
                {
                    closeSocket = true;
                }
            }
            if(closeSocket)
            {
                socket.Close();
            }
        } // ReturnSocket


        internal void TimeoutSockets(DateTime currentTime, TimeSpan socketLifetime)
        {            
           lock (this)
           {        
                CachedSocket prev = null;
                CachedSocket curr = _socketList;

                while (curr != null)
                {
                    // see if it's lifetime has expired
                    if ((_socketCachePolicy == SocketCachePolicy.AbsoluteTimeout && // This is for absolute
                            (currentTime - curr.Handler.CreationTime) > socketLifetime) ||
                        (currentTime - curr.LastUsed) > socketLifetime) // This is relative behaviour    
                    {                        
                        curr.Handler.Close();

                        // remove current cached socket from list
                        if (prev == null)
                        {
                            // it's the first item, so update _socketList
                            _socketList = curr.Next;
                            curr = _socketList;
                        }
                        else
                        {
                            // remove current item from the list
                            curr = curr.Next;
                            prev.Next = curr;
                        }

                        // decrement socket count
                        _socketCount--;
                    }       
                    else
                    {
                        prev = curr;
                        curr = curr.Next;
                    }
                }          
            }
        } // TimeoutSockets

        
    } // class CachedSocketList
    




    internal class SocketCache
    {
        // collection of RemoteConnection's.
        private static Hashtable _connections = new Hashtable();

        private SocketHandlerFactory _handlerFactory;

        // socket timeout data
        private static RegisteredWaitHandle _registeredWaitHandle;
        private static WaitOrTimerCallback _socketTimeoutDelegate;
        private static AutoResetEvent _socketTimeoutWaitHandle;
        private static TimeSpan _socketTimeoutPollTime = TimeSpan.FromSeconds(10);
        private SocketCachePolicy _socketCachePolicy;
        private TimeSpan _socketTimeout;
        private int _receiveTimeout = 0;


        static SocketCache()
        {
            InitializeSocketTimeoutHandler();
        }
            
        internal SocketCache(SocketHandlerFactory handlerFactory, SocketCachePolicy socketCachePolicy,
                                                                        TimeSpan socketTimeout)
        {        
            _handlerFactory = handlerFactory;
            _socketCachePolicy = socketCachePolicy;
            _socketTimeout = socketTimeout;
        } // SocketCache

        internal TimeSpan SocketTimeout { get { return _socketTimeout; } set { _socketTimeout = value;} }
        internal int ReceiveTimeout { get { return _receiveTimeout; } set { _receiveTimeout = value;} }
        internal SocketCachePolicy CachePolicy { get { return _socketCachePolicy; } set { _socketCachePolicy = value;}}


        private static void InitializeSocketTimeoutHandler()
        {
            _socketTimeoutDelegate = new WaitOrTimerCallback(TimeoutSockets);
            _socketTimeoutWaitHandle = new AutoResetEvent(false);
            _registeredWaitHandle = 
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "TcpChannelSocketTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once
        } // InitializeSocketTimeoutHandler

        private static void TimeoutSockets(Object state, Boolean wasSignalled)
        {
            DateTime currentTime = DateTime.UtcNow;

            lock (_connections)
            {
                foreach (DictionaryEntry entry in _connections)
                {
                    RemoteConnection connection = (RemoteConnection)entry.Value; 
                    connection.TimeoutSockets(currentTime);
                }
            }
            
            _registeredWaitHandle.Unregister(null);
            _registeredWaitHandle =
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "TcpChannelSocketTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once      
        } // TimeoutSockets
        


        internal SocketHandler CreateSocketHandler(Socket socket, String machineAndPort)
        {
            socket.ReceiveTimeout = _receiveTimeout;
            return _handlerFactory(socket, this, machineAndPort);
        }


        // The key is expected to of the form "machinename:port"
        public SocketHandler GetSocket(String machinePortAndSid, bool openNew)
        {
            RemoteConnection connection = (RemoteConnection)_connections[machinePortAndSid];
            if (openNew || connection == null)
            {
                connection = new RemoteConnection(this, machinePortAndSid);

                // doesn't matter if different RemoteConnection's get written at
                //   the same time (GC will come along and close them).
                lock (_connections)
                {
                    _connections[machinePortAndSid] = connection;
                }
            }

            return connection.GetSocket();
        } // GetSocket

        public void ReleaseSocket(String machinePortAndSid, SocketHandler socket)
        {
            RemoteConnection connection = (RemoteConnection)_connections[machinePortAndSid];
            if (connection != null)
            {
                connection.ReleaseSocket(socket);
            }
            else
            {
                // there should have been a connection, so let's just close
                //   this socket.
                socket.Close();
            }
        } // ReleaseSocket

        
    } // SocketCache




} // namespace System.Runtime.Remoting.Channels

