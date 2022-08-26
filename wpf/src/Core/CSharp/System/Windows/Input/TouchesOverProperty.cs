using System;
using System.Windows;
using System.Windows.Input;
using MS.Internal.KnownBoxes;

namespace System.Windows.Input
{
    internal class TouchesOverProperty : ReverseInheritProperty
    {
        internal TouchesOverProperty() : 
            base(UIElement.AreAnyTouchesOverPropertyKey,
            CoreFlags.TouchesOverCache,
            CoreFlags.TouchesOverChanged,
            CoreFlags.TouchLeaveCache,
            CoreFlags.TouchEnterCache)
        {
        }

        internal override void FireNotifications(UIElement uie, ContentElement ce, UIElement3D uie3D, bool oldValue)
        {
        }
    }
}

