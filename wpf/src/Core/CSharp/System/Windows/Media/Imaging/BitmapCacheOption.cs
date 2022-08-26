//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2001
//
//  File: BitmapCreateOptions.cs
//
//------------------------------------------------------------------------------

using System;

namespace System.Windows.Media.Imaging
{
    /// <summary>
    /// BitmapCacheOptions are used to specify various performance related options
    /// that influence creating backing stores when loading bitmaps. These options
    /// currently include cache the entire image in memory, cache only requested data,
    /// and don’t cache at all.
    /// </summary>
    public enum BitmapCacheOption
    {
        /// <summary>
        /// By default cache the entire image in memory
        /// </summary>
        Default = 0,

        /// <summary>
        /// Create a backing store for requested data only. The first request will
        /// hit the file, but subsequent requests will be filled from the cache
        /// </summary>
        OnDemand = Default,

        /// <summary>
        /// Cache the entire image into memory at load time. Every request will be
        /// filled from the memory store, the file will not be accessed.
        /// </summary>
        OnLoad = 1,

        /// <summary>
        /// Do not create any cache store. Every request may potentially hit the
        /// file.
        /// </summary>
        None = 2
    }
}
