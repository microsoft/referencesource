using System;

namespace System.Windows
{
    /// <summary>
    ///     Represents the method that will handle the event raised when a
    ///     DependencyProperty is changed on a DependencyObject.
    /// </summary>
    public delegate void DependencyPropertyChangedEventHandler(object sender, DependencyPropertyChangedEventArgs e);
}

