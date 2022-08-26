//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2008
//
//  File:       BitmapCacheMode.cs
//------------------------------------------------------------------------------

namespace System.Windows.Media
{
    public partial class BitmapCache : CacheMode
    {
        public BitmapCache()
        {

        }

        public BitmapCache(double renderAtScale)
        {
            RenderAtScale = renderAtScale;
        }
    }
}
