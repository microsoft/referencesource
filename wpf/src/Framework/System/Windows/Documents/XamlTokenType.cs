//---------------------------------------------------------------------------
// 
// File: XamlTokenType.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: Define the Xaml token type to parse Xaml on XamlToRtf converter.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// XamlTokenType
    /// </summary>
    internal enum XamlTokenType
    {
        XTokInvalid,
        XTokEOF,
        XTokCharacters,
        XTokEntity,
        XTokStartElement,
        XTokEndElement,
        XTokCData,
        XTokPI,
        XTokComment,
        XTokWS
    };
}
