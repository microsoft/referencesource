//------------------------------------------------------------------------------
//  Microsoft Windows Client Platform
//  Copyright (c) Microsoft Corporation, 2005
//
//  File: SlipBehavior.cs
//------------------------------------------------------------------------------

using System.Windows.Media.Animation;

namespace System.Windows.Media.Animation
{
    /// <summary>
    /// The SlipBehavior enumeration is used to indicate how a TimelineGroup will behave
    /// when one of its children slips.
    /// </summary>
    public enum SlipBehavior
    {
        /// <summary>
        /// Indicates that a TimelineGroup will not slip with the chidren, but will
        /// expand to fit all slipping children.
        /// NOTE: This is only effective when the TimelineGroup's duration is not explicitly
        /// specified.
        /// </summary>
        Grow,

        /// <summary>
        /// Indicates that a TimelineGroup will slip along with its first child that
        /// has CanSlip set to true.
        /// </summary>
        Slip,
    }
}

namespace MS.Internal
{
    internal static partial class TimeEnumHelper
    {
        private const int c_maxSlipBehavior = (int)SlipBehavior.Slip;

        static internal bool IsValidSlipBehavior(SlipBehavior value)
        {
            return (0 <= value && (int)value <= c_maxSlipBehavior);
        }
    }
}
