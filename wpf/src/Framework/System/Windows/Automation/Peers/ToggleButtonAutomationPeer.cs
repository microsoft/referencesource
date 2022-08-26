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
    public class ToggleButtonAutomationPeer : ButtonBaseAutomationPeer, IToggleProvider
    {
        ///
        public ToggleButtonAutomationPeer(ToggleButton owner): base(owner)
        {}
    
        ///
        override protected string GetClassNameCore()
        {
            return "Button";
        }

        ///
        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Button;
        }

        /// 
        override public object GetPattern(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.Toggle)
            {
                return this;
            }
            else
            {
                return base.GetPattern(patternInterface);
            }
        }
        
        void IToggleProvider.Toggle()
        {
            if(!IsEnabled())
                throw new ElementNotEnabledException();

            ToggleButton owner = (ToggleButton)Owner;
            owner.OnToggle();
        }

        ToggleState IToggleProvider.ToggleState
        {
            get 
            { 
                ToggleButton owner = (ToggleButton)Owner;
                return ConvertToToggleState(owner.IsChecked); 
            }
        }

        // 
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        internal virtual void RaiseToggleStatePropertyChangedEvent(bool? oldValue, bool? newValue)
        {
            if (oldValue != newValue)
            {
                RaisePropertyChangedEvent(TogglePatternIdentifiers.ToggleStateProperty, ConvertToToggleState(oldValue), ConvertToToggleState(newValue));
            }
        }

        private static ToggleState ConvertToToggleState(bool? value)
        {
            switch (value)
            {
                case (true):    return ToggleState.On;
                case (false):   return ToggleState.Off;
                default:        return ToggleState.Indeterminate;
            }
        }
    }
}

