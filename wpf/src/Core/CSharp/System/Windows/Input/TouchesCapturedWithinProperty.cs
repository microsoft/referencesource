using System;
using System.Windows;
using System.Windows.Input;
using MS.Internal.KnownBoxes;

namespace System.Windows.Input
{
    internal class TouchesCapturedWithinProperty : ReverseInheritProperty
    {
        internal TouchesCapturedWithinProperty() : 
            base(UIElement.AreAnyTouchesCapturedWithinPropertyKey,
            CoreFlags.TouchesCapturedWithinCache,
            CoreFlags.TouchesCapturedWithinChanged)
        {
        }

        internal override void FireNotifications(UIElement uie, ContentElement ce, UIElement3D uie3D, bool oldValue)
        {
        }
    }
}

