using MS.Internal.AppModel;

namespace System.Windows.Automation.Peers
{

    /// 
    internal class RootBrowserWindowAutomationPeer : WindowAutomationPeer
    {
        ///
        public RootBrowserWindowAutomationPeer(RootBrowserWindow owner): base(owner)
        {}
    
        ///
        override protected string GetClassNameCore()
        {
            return "RootBrowserWindow";
        }

    }
}



