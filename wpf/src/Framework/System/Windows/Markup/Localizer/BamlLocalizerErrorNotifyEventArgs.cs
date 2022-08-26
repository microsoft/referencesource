//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:      BamlLocalizerErrorNotifyEventArgs.cs
//
//  Created:   2/27/2006 Garyyang
//------------------------------------------------------------------------
using System;

namespace System.Windows.Markup.Localizer
{
    /// <summary>
    /// The EventArgs for the BamlLocalizer.ErrorNotify event. 
    /// </summary>
    public class BamlLocalizerErrorNotifyEventArgs : EventArgs
    {
        BamlLocalizableResourceKey _key;    // The key of the localizable resources related to the error 
        BamlLocalizerError       _error;    // The error code. 
        
        internal BamlLocalizerErrorNotifyEventArgs(BamlLocalizableResourceKey key, BamlLocalizerError error)
        {
            _key = key; 

            // 
            _error = error;
        }

        /// <summary>
        /// The key of the BamlLocalizableResource related to the error
        /// </summary>
        public BamlLocalizableResourceKey Key { get { return _key; } }

        /// <summary>
        /// The error encountered by BamlLocalizer
        /// </summary>        
        public BamlLocalizerError Error { get { return _error; } }        
    }
}
