using System;
using System.Windows.Input;
using MS.Internal.KnownBoxes;
	
namespace System.Windows
{
    /////////////////////////////////////////////////////////////////////////

    internal class FocusWithinProperty : ReverseInheritProperty
    {
        /////////////////////////////////////////////////////////////////////

        internal FocusWithinProperty() : base(
            UIElement.IsKeyboardFocusWithinPropertyKey,
            CoreFlags.IsKeyboardFocusWithinCache,
            CoreFlags.IsKeyboardFocusWithinChanged)
        {
        }

        /////////////////////////////////////////////////////////////////////

        internal override void FireNotifications(UIElement uie, ContentElement ce, UIElement3D uie3D, bool oldValue)
        {
            DependencyPropertyChangedEventArgs args = 
                    new DependencyPropertyChangedEventArgs(
                        UIElement.IsKeyboardFocusWithinProperty, 
                        BooleanBoxes.Box(oldValue), 
                        BooleanBoxes.Box(!oldValue));
            
            if (uie != null)
            {
                uie.RaiseIsKeyboardFocusWithinChanged(args);
            }
            else if (ce != null)
            {
                ce.RaiseIsKeyboardFocusWithinChanged(args);
            }
            else if (uie3D != null)
            {
                uie3D.RaiseIsKeyboardFocusWithinChanged(args);
            }
        }
    }
}

