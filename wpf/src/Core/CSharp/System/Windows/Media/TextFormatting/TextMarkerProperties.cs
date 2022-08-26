//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2004
//
//  File:      TextMarkerProperties.cs
//
//  Contents:  Definition of text marker properties
//
//  Spec:      http://team/sites/Avalon/Specs/Text%20Formatting%20API.doc
//
//  Created:   1-2-2004 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Collections;
using System.Windows;


namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// </summary>
    public abstract class TextMarkerProperties
    {
        /// <summary>
        /// Distance from line start to the end of the marker symbol
        /// </summary>
        public abstract double Offset
        { get; }


        /// <summary>
        /// Source of text runs used for text marker
        /// </summary>
        public abstract TextSource TextSource
        { get; }
    }
}

