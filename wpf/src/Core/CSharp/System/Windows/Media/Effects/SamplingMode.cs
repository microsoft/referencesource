//------------------------------------------------------------------------------
//  Microsoft Windows Presentation Foundation
//  Copyright (c) Microsoft Corporation, 2008
//
//  File:       SamplingMode.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media.Effects
{
    public enum SamplingMode
    {
        // These values need to match how they're interpeted on the native
        // side.
        
        /// <summary>
        /// Always use nearest neighbor sampling.
        /// </summary>
        NearestNeighbor = 0x0,

        /// <summary>
        /// Always use bilinear sampling.
        /// </summary>
        Bilinear        = 0x1,

        /// <summary>
        /// System automatically chooses most appropriate sampling mode.
        /// </summary>
        Auto            = 0x2,
    }
}
