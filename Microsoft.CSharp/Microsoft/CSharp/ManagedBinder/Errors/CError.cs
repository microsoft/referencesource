// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Globalization;

namespace Microsoft.CSharp.RuntimeBinder.Errors
{
    ////////////////////////////////////////////////////////////////////////////////
    // CError
    //
    // This object is the implementation of ICSError for all compiler errors,
    // including lexer, parser, and compiler errors.

    internal class CError
    {
        private string m_text;

        private static string ComputeString(ErrorCode code, string[] args)
        {
            return String.Format(CultureInfo.InvariantCulture, ErrorFacts.GetMessage(code), args);
        }

        public void Initialize(ErrorCode code, string[] args)
        {
            m_text = ComputeString(code, args);
        }

        public string Text { get { return m_text; } }
    }
}
