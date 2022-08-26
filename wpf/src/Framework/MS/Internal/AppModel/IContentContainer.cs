//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2001
//
//  File:            IContentContainer.cs
//
//    Description:    BindProduct. Abstract representation of the product of a bind
//
//    History:        04-16-01 - marka - created
//------------------------------------------------------------------------------

using System;
using System.IO;
using MS.Internal.Utility;

namespace MS.Internal.AppModel
{
    // Interface that's required to be implemented by all classes that
    // can get get called back when a BindProduct become available.
    internal interface IContentContainer
    {
        /// Function that gets called back when the BindProduct becomes
        /// available.
        /// "contentType" - Mime type of bindproduct
        /// "content" - BindProduct that's just been created.
        void OnContentReady(ContentType contentType, Object content, Uri uri, Object navState);

        // Function that gets called each time number of bytes equal to
        // bytesInterval is read
        void OnNavigationProgress(Uri uri, long bytesRead, long maxBytes);

        // Function that gets called when Close() is called on the stream.
        void OnStreamClosed(Uri uri);
    }

    // This is the NavigationStatus for the IContentContainers
    internal enum NavigationStatus
    {
        // Currently idle
        Idle,
        // Currently navigating the navigable item
        Navigating,
        // An error occurred during Navigation
        NavigationFailed,
        // Root Element hooked up to source window or frame        
        Navigated,
        // Navigation was stopped
        Stopped
    }
}
