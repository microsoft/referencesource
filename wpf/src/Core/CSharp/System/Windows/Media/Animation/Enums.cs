//------------------------------------------------------------------------------
//  Microsoft Windows Client Platform
//  Copyright (c) Microsoft Corporation, 2003
//                                             
//  File:       Enums.cs
//------------------------------------------------------------------------------

using System;
using MS.Internal;
using System.Runtime.InteropServices;

namespace System.Windows.Media.Animation
{
    // IMPORTANT: If you change public Enums, TimeEnumHelper.cs must be updated
    //  to reflect the maximum enumerated values for the public enums.

    /// <summary>
    /// Specifies the behavior of the <see cref="ClockController.Seek"/>
    /// method by defining the meaning of that method's offset parameter.
    /// </summary>
    public enum TimeSeekOrigin
    {
        /// <summary>
        /// The offset parameter specifies the new position of the timeline as a
        /// distance from time t=0
        /// </summary>
        BeginTime,

        /// <summary>
        /// The offset parameter specifies the new position of the timeline as a
        /// distance from the end of the simple duration. If the duration is not
        /// defined, this causes the method call to have no effect.
        /// </summary>
        Duration
    }

    /// <summary>
    /// Possible states the time manager may be in.
    /// </summary>
    // TODO leov: this structure is obsolete and should go away soon.
    internal enum TimeState
    {
        /// <summary>
        /// Time is stopped.
        /// </summary>
        /// <remarks>
        /// This is the default (constructor) value.
        /// </remarks>
        Stopped,
        /// <summary>
        /// Time is paused.
        /// </summary>
        Paused,
        /// <summary>
        /// Time is running.
        /// </summary>
        Running,
    }
}
