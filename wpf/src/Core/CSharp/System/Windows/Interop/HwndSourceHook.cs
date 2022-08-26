
using System;
namespace System.Windows.Interop
{
    /// <summary>
    ///     The callback signature for HwndSource hooks.
    /// </summary>
    public delegate IntPtr HwndSourceHook(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled);
}
