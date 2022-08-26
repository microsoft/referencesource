using System;
using System.Windows.Input;
using MS.Internal.KnownBoxes;

namespace System.Windows
{
    /////////////////////////////////////////////////////////////////////////

    internal class MouseCaptureWithinProperty : ReverseInheritProperty
    {
        /////////////////////////////////////////////////////////////////////

        internal MouseCaptureWithinProperty() : base(
            UIElement.IsMouseCaptureWithinPropertyKey,
            CoreFlags.IsMouseCaptureWithinCache,
            CoreFlags.IsMouseCaptureWithinChanged)
        {
        }

        /////////////////////////////////////////////////////////////////////

        internal override void FireNotifications(UIElement uie, ContentElement ce, UIElement3D uie3D, bool oldValue)
        {
            DependencyPropertyChangedEventArgs args = 
                    new DependencyPropertyChangedEventArgs(
                        UIElement.IsMouseCaptureWithinProperty, 
                        BooleanBoxes.Box(oldValue), 
                        BooleanBoxes.Box(!oldValue));
            
            if (uie != null)
            {
                uie.RaiseIsMouseCaptureWithinChanged(args);
            }
            else if (ce != null)
            {
                ce.RaiseIsMouseCaptureWithinChanged(args);
            }
            else if (uie3D != null)
            {
                uie3D.RaiseIsMouseCaptureWithinChanged(args);
            }
        }
    }
}

