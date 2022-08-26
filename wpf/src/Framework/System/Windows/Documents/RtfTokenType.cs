//---------------------------------------------------------------------------
// 
// File: RtfTokenType.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: Rtf token type that is the enumeration of Rtf token type.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// An enumeration of the Rtf token type
    /// </summary>
    internal enum RtfTokenType
    {
        TokenInvalid,
        TokenEOF,
        TokenText,
        TokenTextSymbol,
        TokenPictureData,
        TokenNewline,
        TokenNullChar,
        TokenControl,
        TokenDestination,
        TokenHex,
        TokenGroupStart,
        TokenGroupEnd
    };
}
