//-----------------------------------------------------------------------
// <copyright file="StylusTip.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------

using System;

namespace System.Windows.Ink
{
    /// <summary>
    /// StylusTip
    /// </summary>
    public enum StylusTip
    {
        /// <summary>
        /// Rectangle
        /// </summary>
        Rectangle = 0,

        /// <summary>
        /// Ellipse
        /// </summary>
        Ellipse
    }

    /// <summary>
    /// Internal helper to avoid costly call to Enum.IsDefined
    /// </summary>
    internal static class StylusTipHelper
    {
        internal static bool IsDefined(StylusTip stylusTip)
        {
            if (stylusTip < StylusTip.Rectangle || stylusTip > StylusTip.Ellipse)
            {
                return false;
            }
            return true;
        }
    }
}
