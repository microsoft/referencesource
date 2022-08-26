using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Windows;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Interop;
using System.Windows.Media;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{

    /// 
    public class RepeatButtonAutomationPeer : ButtonBaseAutomationPeer, IInvokeProvider
    {
        ///
        public RepeatButtonAutomationPeer(RepeatButton owner): base(owner)
        {}
    
        ///
        override protected string GetClassNameCore()
        {
            return "RepeatButton";
        }

        ///
        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Button;
        }

        /// 
        override public object GetPattern(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.Invoke)
            {
                return this;
            }
            else
            {
                return base.GetPattern(patternInterface);
            }
        }

        void IInvokeProvider.Invoke()
        {
            if(!IsEnabled())
                throw new ElementNotEnabledException();

            RepeatButton owner = (RepeatButton)Owner;
            owner.AutomationButtonBaseClick();
        }
    }
}

