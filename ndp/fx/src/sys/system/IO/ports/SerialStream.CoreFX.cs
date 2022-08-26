// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Diagnostics;

namespace System.IO.Ports
{
    internal sealed partial class SerialStream : Stream
    {
        internal sealed partial class EventLoopRunner
        {
            /// <summary>
            /// Call WaitForCommEvent (which is a thread function for a background thread)
            /// within an exception handler, so that unhandled exceptions in WaitForCommEvent
            /// don't cause process termination
            /// Ultimately it would be good to migrate to a Task here rather than simple background Thread
            /// This would let any exceptions be marshalled back to the owner of the SerialPort
            /// Some discussion in GitHub issue #17666
            /// </summary>
            internal void SafelyWaitForCommEvent()
            {
                try
                {
                    WaitForCommEvent();
                }
                catch (ObjectDisposedException)
                {
                    // These can happen in some messy tear-down situations (e.g. unexpected USB unplug)
                    // See GitHub issue #17661
                }
                catch (Exception ex)
                {
                    // We don't know of any reason why this should happen, but we still
                    // don't want process termination
                    Debug.Fail("Unhandled exception thrown from WaitForCommEvent", ex.ToString());
                }
            }
        }
    }
}
