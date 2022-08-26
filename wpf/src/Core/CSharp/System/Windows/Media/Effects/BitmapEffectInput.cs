//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2005
//
//  File:       BitmapEffectInput.cs
//------------------------------------------------------------------------------

using MS.Internal;
using System;
using System.IO;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Media;
using System.Windows.Markup;
using System.Windows.Media.Animation;
using System.Windows.Media.Composition;
using System.Windows.Media.Imaging;
using System.Collections.Generic;

namespace System.Windows.Media.Effects
{
    /// <summary>
    /// BitmapEffect class
    /// </summary>
    public sealed partial class BitmapEffectInput
    {
        private static BitmapSource s_defaultInputSource;
        /// <summary>
        /// Constructor
        /// </summary>
        public BitmapEffectInput()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="input">input image</param>
        public BitmapEffectInput(BitmapSource input)
        {
            Input = input;
        }

        /// <summary>
        /// ShouldSerializeInput - this is called by the serializer to determine whether or not to
        /// serialize the Input property.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public bool ShouldSerializeInput()
        {            
            return (Input != BitmapEffectInput.ContextInputSource);
        }

        /// <summary>
        /// Returns a sentinel value representing the source that is derived from context
        /// </summary>
        public static BitmapSource ContextInputSource
        {
            get
            {
                if (s_defaultInputSource == null)
                {
                    BitmapSource source = new UnmanagedBitmapWrapper(true);
                    source.Freeze();
                    s_defaultInputSource = source;
                }

                return s_defaultInputSource;
            }
        }

    }
}

