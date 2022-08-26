//------------------------------------------------------------------------------
//  Copyright (c) Microsoft Corporation, 2008
//
//  File: SineEase.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media.Animation
{
    /// <summary>
    ///     This class implements an easing function that gives a Sine curve toward the destination.
    /// </summary>
    public class SineEase : EasingFunctionBase
    {
        protected override double EaseInCore(double normalizedTime)
        {
            return 1.0 - Math.Sin(Math.PI * 0.5  * (1 - normalizedTime));
        }

        protected override Freezable CreateInstanceCore()
        {
            return new SineEase();
        }
    }
}
