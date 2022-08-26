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
using System.Windows.Interop;
using System.Windows.Media;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{

    /// 
    public class ListBoxAutomationPeer: SelectorAutomationPeer 
    {
        ///
        public ListBoxAutomationPeer(ListBox owner): base(owner)
        {}

        ///
        override protected ItemAutomationPeer CreateItemAutomationPeer(object item)
        {
            return new ListBoxItemAutomationPeer(item, this);
        }

        ///
        override protected string GetClassNameCore()
        {
            return "ListBox";
        }
    }
}


