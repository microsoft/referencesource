using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Windows;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{

   ///
   public class ListBoxItemWrapperAutomationPeer: FrameworkElementAutomationPeer 
    {
        ///
        public ListBoxItemWrapperAutomationPeer(ListBoxItem owner): base(owner)
        {}

        ///
        override protected string GetClassNameCore()
        {
            return "ListBoxItem";
        }

        ///
        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.ListItem;
        }

    }
}



