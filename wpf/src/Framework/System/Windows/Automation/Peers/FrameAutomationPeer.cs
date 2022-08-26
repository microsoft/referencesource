using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;
using System.ComponentModel;

namespace System.Windows.Automation.Peers
{
    /// 
    public class FrameAutomationPeer : FrameworkElementAutomationPeer
    {
        ///
        public FrameAutomationPeer(Frame owner) : base(owner)
        { }

        ///
        override protected string GetClassNameCore()
        {
            return "Frame";
        }

        ///
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Pane;
        }
    }
}


