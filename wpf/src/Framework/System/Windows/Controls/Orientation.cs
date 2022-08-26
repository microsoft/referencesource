using System;
using System.Windows;

namespace System.Windows.Controls
{
    /// <summary>
    /// Orientation indicates a direction of a control/layout that can exist in a horizontal or vertical state.
    /// Examples of these elements include: <see cref="Slider" /> and <see cref="Primitives.ScrollBar" />.
    /// </summary>
    [Localizability(LocalizationCategory.None, Readability = Readability.Unreadable)]
    public enum Orientation
    {
        /// <summary>
        /// Control/Layout should be horizontally oriented.
        /// </summary>
        Horizontal,
        /// <summary>
        /// Control/Layout should be vertically oriented.
        /// </summary>
        Vertical,
    }
}