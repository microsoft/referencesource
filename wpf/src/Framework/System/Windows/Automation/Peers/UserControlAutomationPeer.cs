using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;
using System.ComponentModel;

namespace System.Windows.Automation.Peers
{
    /// 
    public class UserControlAutomationPeer : FrameworkElementAutomationPeer
    {
        ///
        public UserControlAutomationPeer(UserControl owner) : base(owner)
        { }

        ///
        override protected string GetClassNameCore()
        {
            return Owner.GetType().Name;
        }

        ///
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Custom;
        }
    }
}


