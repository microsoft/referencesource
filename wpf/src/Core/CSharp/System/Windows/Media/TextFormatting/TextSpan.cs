//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation
//
//  File:      TextSpan.cs
//
//  Contents:  A simple pairing of an object of type T and a run length
//
//  Spec:      http://team/sites/Avalon/Specs/Text%20Formatting%20API.doc
//
//  Created:   2-5-2004 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Collections.Generic;
using System.Diagnostics;


namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// A simple pairing of an object of type T and a run length
    /// </summary>
    public class TextSpan<T>
    {
        private int     _length;
        private T       _value;


        /// <summary>
        /// Construct an object/length pairing
        /// </summary>
        /// <param name="length">run length</param>
        /// <param name="value">value</param>
        public TextSpan(
            int     length,
            T       value
            )
        {
            _length = length;
            _value = value;
        }


        /// <summary>
        /// Number of characters in span
        /// </summary>
        public int Length
        {
            get { return _length; }
        }


        /// <summary>
        /// Value    associated with span
        /// </summary>
        public T Value
        {
            get { return _value; }
        }
    }
}

