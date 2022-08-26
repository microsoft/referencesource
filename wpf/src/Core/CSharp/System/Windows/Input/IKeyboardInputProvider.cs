using System.Windows.Media;

using System;

namespace System.Windows.Input 
{
    /// <summary>
    ///     An interface for controlling the keyboard input provider.
    /// </summary>
    internal interface IKeyboardInputProvider : IInputProvider
    {
        /// <summary>
        ///     Requests that the input provider acquire the keyboard focus.
        /// </summary>
        bool AcquireFocus(bool checkOnly);
    }
}



