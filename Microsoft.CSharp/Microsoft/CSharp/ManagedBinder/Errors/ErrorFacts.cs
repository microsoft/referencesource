// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Errors
{
    static class ErrorFacts
    {
        public static string GetMessage(ErrorCode code)
        {
            string codeStr = code.ToString();

            Debug.Assert(codeStr != null);

            if (codeStr == null)
            {
                return null;
            }

            Debug.Assert(codeStr.Length > 4);
            Debug.Assert(codeStr.StartsWith("ERR_", StringComparison.Ordinal));

            if (codeStr.Length <= 4)
            {
                return null;
            }

            // This strips off the "ERR_" and gets the resource with the rest of the name

            return SR.GetString(codeStr.Substring(4));
        }

        public static string GetMessage(MessageID id)
        {
            return SR.GetString(id.ToString());
        }
    }
}
