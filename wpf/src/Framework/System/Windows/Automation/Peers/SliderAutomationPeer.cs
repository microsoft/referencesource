using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{
    ///
    public class SliderAutomationPeer : RangeBaseAutomationPeer
    {
        ///
        public SliderAutomationPeer(Slider owner): base(owner)
        {
        }

        ///
        override protected string GetClassNameCore()
        {
            return "Slider";
        }

        ///
        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Slider;
        }

        ///
        protected override Point GetClickablePointCore()
        {
            return new Point(double.NaN, double.NaN);
        }
    }
}

