//------------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2008
//
//  File: QuadraticEase.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media.Animation
{
    /// <summary>
    ///     This class implements an easing function that gives a quadratic curve toward the destination
    /// </summary>
    public class QuadraticEase : EasingFunctionBase
    {
        protected override double EaseInCore(double normalizedTime)
        {
            return normalizedTime * normalizedTime;
        }

        protected override Freezable CreateInstanceCore()
        {
            return new QuadraticEase();
        }
    }
}
