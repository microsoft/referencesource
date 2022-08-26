using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Windows;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{

    /// 
    public class InkCanvasAutomationPeer : FrameworkElementAutomationPeer
    {
        ///
        public InkCanvasAutomationPeer(InkCanvas owner)
            : base(owner)
        {}
    
        ///
        protected override string GetClassNameCore()
        {
            return "InkCanvas";
        }

        ///
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Custom;
        }
    }
}

