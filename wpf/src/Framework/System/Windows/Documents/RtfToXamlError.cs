//---------------------------------------------------------------------------
// 
// File: RtfToXamlError.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: Define the RtfToXaml conversion errors.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// RtfToXaml content converting error enumeration.
    /// </summary>
    internal enum RtfToXamlError
    {
        None,
        InvalidFormat,
        InvalidParameter,
        InsufficientMemory,
        OutOfRange
    }
}
