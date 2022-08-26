using System;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Navigation;
using System.Windows.Interop;
using System.Windows.Media;
using System.ComponentModel;

namespace System.Windows.Automation.Peers
{
    /// 
    public class NavigationWindowAutomationPeer : WindowAutomationPeer
    {
        ///
        public NavigationWindowAutomationPeer(NavigationWindow owner): base(owner)
        {}
    
        ///
        override protected string GetClassNameCore()
        {
            return "NavigationWindow";
        }

        // 
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        internal static void RaiseAsyncContentLoadedEvent(AutomationPeer peer, long bytesRead, long maxBytes)
        {
            double percentComplete = 0d;
            AsyncContentLoadedState asyncContentState = AsyncContentLoadedState.Beginning;

            if (bytesRead > 0)
            {
                if (bytesRead < maxBytes)
                {
                    percentComplete = maxBytes > 0 ? (bytesRead * 100d / maxBytes) : 0;
                    asyncContentState = AsyncContentLoadedState.Progress;
                }
                else
                {
                    percentComplete = 100d;
                    asyncContentState = AsyncContentLoadedState.Completed;
                }
            }

            peer.RaiseAsyncContentLoadedEvent(new AsyncContentLoadedEventArgs(asyncContentState, percentComplete));
        }

    }

}


