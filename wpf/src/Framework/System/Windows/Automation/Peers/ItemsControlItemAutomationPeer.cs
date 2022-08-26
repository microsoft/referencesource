using System;
using System.Windows;
using System.Windows.Controls;

namespace System.Windows.Automation.Peers
{
    // work around DDVSO 410007
    // this class should be public
    internal class ItemsControlItemAutomationPeer : ItemAutomationPeer
    {
        public ItemsControlItemAutomationPeer(object item, ItemsControlWrapperAutomationPeer parent)
            : base(item, parent)
        { }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.DataItem;
        }

        protected override string GetClassNameCore()
        {
            return "ItemsControlItem";
        }
    }
}
