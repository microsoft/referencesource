using System;
using System.Windows;
using System.Windows.Controls;

using MS.Internal;
using MS.Win32;

namespace System.Windows.Automation.Peers
{
    // work around DDVSO 410007
    // this class should be public
    internal class ItemsControlWrapperAutomationPeer : ItemsControlAutomationPeer
    {
        public ItemsControlWrapperAutomationPeer(ItemsControl owner)
            : base(owner)
        { }

        override protected ItemAutomationPeer CreateItemAutomationPeer(object item)
        {
            return new ItemsControlItemAutomationPeer(item, this);
        }

        override protected string GetClassNameCore()
        {
            return "ItemsControl";
        }

        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.List;
        }
    }
}

