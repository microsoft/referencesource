using System;
namespace System.Windows.Input
{
    /// <summary>
    ///    The raw actions being reported from WM_*UISTATE* messages.
    /// </summary>
    internal enum RawUIStateActions
    {
        /// <summary>
        ///     The targets should be set.
        /// </summary>
        Set = 1,            // == UIS_SET

        /// <summary>
        ///     The targets should be cleared.
        /// </summary>
        Clear = 2,          // == UIS_CLEAR

        /// <summary>
        ///     The targets should be initialized.
        /// </summary>
        Initialize = 3,     // == UIS_INITIALIZE
    }
}
