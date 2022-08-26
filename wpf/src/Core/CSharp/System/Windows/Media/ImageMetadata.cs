//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, All Rights Reserved
//
//  File: ImageMetadata.cs
//
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Reflection;
using MS.Internal;
using MS.Win32.PresentationCore;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.IO;
using System.Security;
using System.Security.Permissions;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using System.Text;
using MS.Internal.PresentationCore;                        // SecurityHelper

namespace System.Windows.Media
{
    #region ImageMetadata

    /// <summary>
    /// Metadata for ImageSource.
    /// </summary>
    abstract public partial class ImageMetadata : Freezable
    {
        #region Constructors

        /// <summary>
        ///
        /// </summary>
        internal ImageMetadata()
        {
        }

        #endregion

        #region Freezable

        /// <summary>
        ///     Shadows inherited Copy() with a strongly typed
        ///     version for convenience.
        /// </summary>
        public new ImageMetadata Clone()
        {
            return (ImageMetadata)base.Clone();
        }

        #endregion
    }

    #endregion
}
