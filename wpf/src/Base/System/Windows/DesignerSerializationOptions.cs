//---------------------------------------------------------------------------
//
// File: DesignerSerializationOptions.cs
//
// Description:
//   Specifies the serialization flags per property
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;

namespace System.Windows.Markup
{
    /// <summary>
    ///     Specifies the serialization flags per property
    /// </summary>
    [Flags]
    public enum DesignerSerializationOptions : int
    {
        /// <summary>
        ///     Serialize the property as an attibute
        /// </summary>
        SerializeAsAttribute = 0x001
    }
}

