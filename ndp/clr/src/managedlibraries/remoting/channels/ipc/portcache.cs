// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       PortCache.cs
//  Author:   Microsoft@Microsoft.Com
//  Summary:    Implements a cache for port handles
//
//==========================================================================

using System;
using System.Collections;
using System.Threading;
using System.Security.Principal;

namespace System.Runtime.Remoting.Channels.Ipc
{
    internal class PortConnection
    {
        private IpcPort _port;
        private DateTime _socketLastUsed;

        internal PortConnection(IpcPort port)
        {
            _port = port;
            _socketLastUsed = DateTime.Now;
        }

        internal IpcPort Port { get { return _port; } }
        internal DateTime LastUsed { get { return _socketLastUsed; } }
    }
    
    internal class ConnectionCache
    {
        // collection of RemoteConnection's.
        private static Hashtable _connections = new Hashtable();

        // socket timeout data
        private static RegisteredWaitHandle _registeredWaitHandle;
        private static WaitOrTimerCallback _socketTimeoutDelegate;
        private static AutoResetEvent _socketTimeoutWaitHandle;
        private static TimeSpan _socketTimeoutPollTime = TimeSpan.FromSeconds(10);
        private static TimeSpan _portLifetime = TimeSpan.FromSeconds(10);

        static ConnectionCache()
        {
            InitializeConnectionTimeoutHandler();
        }
            
        private static void InitializeConnectionTimeoutHandler()
        {
            _socketTimeoutDelegate = new WaitOrTimerCallback(TimeoutConnections);
            _socketTimeoutWaitHandle = new AutoResetEvent(false);
            _registeredWaitHandle = 
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "IpcConnectionTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once
        } // InitializeSocketTimeoutHandler

        private static void TimeoutConnections(Object state, Boolean wasSignalled)
        {
            DateTime currentTime = DateTime.UtcNow;

            lock (_connections)
            {
                foreach (DictionaryEntry entry in _connections)
                {
                    PortConnection connection = (PortConnection)entry.Value; 
                    if (DateTime.Now - connection.LastUsed > _portLifetime)
                        connection.Port.Dispose();
                }
            }
            
            _registeredWaitHandle.Unregister(null);
            _registeredWaitHandle =
                ThreadPool.UnsafeRegisterWaitForSingleObject(
                    _socketTimeoutWaitHandle, 
                    _socketTimeoutDelegate, 
                    "IpcConnectionTimeout", 
                    _socketTimeoutPollTime, 
                    true); // execute only once      
        } // TimeoutConnections
        
        // The key is expected to of the form portName
        public IpcPort GetConnection(String portName, bool secure, TokenImpersonationLevel level, int timeout)
        {
            PortConnection connection = null;
            lock (_connections)
            {
                bool cacheable = true;
                if (secure)
                {
                  try
                  {
                    WindowsIdentity currentId = WindowsIdentity.GetCurrent(true/*ifImpersonating*/);
                    if (currentId != null)
                    {
                      cacheable = false;
                      currentId.Dispose();
                    }
                  }
                  catch(Exception)
                  {
                      cacheable = false;
                  }
                }

                if (cacheable)
                {
                    connection = (PortConnection)_connections[portName];
                }
                if (connection == null || connection.Port.IsDisposed)
                {
                    connection = new PortConnection(IpcPort.Connect(portName, secure, level, timeout));
                    connection.Port.Cacheable = cacheable;
                }
                else
                {
                    // Remove the connection from the cache
                    _connections.Remove(portName); 
                }
            }
            return connection.Port;
        } // GetSocket

        public void ReleaseConnection(IpcPort port)
        {
            string portName = port.Name;
            PortConnection connection = (PortConnection)_connections[portName];
            if (port.Cacheable && (connection == null || connection.Port.IsDisposed))
            {
                lock(_connections)
                {
                    _connections[portName] = new PortConnection(port);
                }
            }
            else
            {
                // there should have been a connection, so let's just close
                //   this socket.
                port.Dispose();
            }
        } // ReleasePort

        
    } // ConnectionCache

}
