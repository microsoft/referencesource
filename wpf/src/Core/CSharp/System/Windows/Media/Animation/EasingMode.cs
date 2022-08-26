//------------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2008
//
//  File: EasingMode.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media.Animation
{
    /// <summary>
    ///     This enum defines the modes in which classes deriving from EasingFunctioBase
    ///     can will perform their easing.
    /// </summary>
    public enum EasingMode
    {
        EaseIn,    // the easing is performed at the start of the animation
        EaseOut,   // the easing is performed at the end of the animation
        EaseInOut, // the easing is performed both at the start and the end of the animation
    }
}
