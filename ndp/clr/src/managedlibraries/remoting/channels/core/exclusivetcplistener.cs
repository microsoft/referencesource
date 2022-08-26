//------------------------------------------------------------------------------
// <copyright file="ExclusiveTcpListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Net;
using System.Net.Sockets;

namespace System.Runtime.Remoting.Channels
{
    
    // This class provides a TcpListener that is capable of setting the ExclusiveAddressUse flag
    // on a socket, which will prevent another app from hijacking our port. This flag is not supported
    // on Win9x, so we just omit the call to SetSocketOption on non-NT platforms.
    internal class ExclusiveTcpListener : TcpListener
    {
        internal ExclusiveTcpListener(IPAddress localaddr, int port) : base(localaddr, port) {}

        // Start will attempt to start listening.  If exclusiveAddressUse is true, then
        // we will attempt to use the ExclusiveAddressUse flag, but if bind fails (which will
        // happen for a regular user on win2k and xp), we try again without the flag.
        internal void Start(bool exclusiveAddressUse)
        {
            // we only attempt to set the socket option if
            //   1. the exclusiveAddressUse param is true
            //   2. the platform is NT - this option is unavailable on other platforms
            //   3. Server is not null - if it IS null, base.Start will throw a nice error for us
            //   4. the listener is not already listening - it's too late in that case (base.Start will return immediately)
            bool attemptSetSocketOption = exclusiveAddressUse &&
#if !FEATURE_PAL            
                                          Environment.OSVersion.Platform == PlatformID.Win32NT &&
#endif // !FEATURE_PAL                                          
                                          base.Server != null &&
                                          !base.Active;

            if (attemptSetSocketOption)
            {
                // Attempt to set the option.  We won't actually find out if this fails until
                // we try to bind (which happens in base.Start()).
                base.Server.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ExclusiveAddressUse, 1);
            }
                
            try
            {
                base.Start();
            }
            catch (SocketException)
            {
                if (attemptSetSocketOption)
                {
                    // Turn off the option and try again - maybe this process doesn't have
                    // permission to use the option.
                    Server.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ExclusiveAddressUse, 0);
                    base.Start();
                }
                else
                {
                    // It wasn't because we set the ExclusiveAddressUse option - let the
                    // exception bubble up
                    throw;
                }
            }
        }

        internal bool IsListening { get { return Active; } }

    }

} // namespace System.Runtime.Remoting.Channels
