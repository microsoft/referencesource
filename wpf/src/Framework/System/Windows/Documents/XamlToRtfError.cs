//---------------------------------------------------------------------------
// 
// File: XamlToRtfError.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: XamlToRtf error enumeration that indicates the error from
//              converting XamlToRtf or RtfToXaml cases.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// XamlToRtf or RtfToXaml content converting error enumeration.
    /// </summary>
    internal enum XamlToRtfError
    {
        None,
        InvalidFormat,
        InvalidParameter,
        InsufficientMemory,
        OutOfRange,
        Unknown
    }
}
