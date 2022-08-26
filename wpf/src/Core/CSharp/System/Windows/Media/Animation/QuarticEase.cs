//------------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2008
//
//  File: QuarticEase.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media.Animation
{
    /// <summary>
    ///     This class implements an easing function that gives a quartic curve toward the destination
    /// </summary>
    public class QuarticEase : EasingFunctionBase
    {
        protected override double EaseInCore(double normalizedTime)
        {
            return normalizedTime * normalizedTime * normalizedTime * normalizedTime;
        }

        protected override Freezable CreateInstanceCore()
        {
            return new QuarticEase();
        }
    }
}
