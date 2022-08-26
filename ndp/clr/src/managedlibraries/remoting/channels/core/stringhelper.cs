// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    StringHelper.cs
//
// Summary: Helper methods for strings.
//
//===========================================================


using System;
using System.IO;
using System.Runtime.Remoting;


namespace System.Runtime.Remoting.Channels
{

    internal static class StringHelper
    {   
        internal static bool StartsWithDoubleUnderscore(String str)
        {
            if (str.Length < 2)
                return false;

            return (str[0] == '_') && (str[1] == '_');
        } // StartsWithDoubleUnderscore

        internal static bool StartsWithAsciiIgnoreCasePrefixLower(String str, String asciiPrefix)
        {
            // The caller should know that the arguments aren't null.

            int prefixLen = asciiPrefix.Length;
            if (str.Length < prefixLen)
                return false;
                
            for (int i = 0; i < prefixLen; i++)
            {
                // The prefix is assumed to be in lowercase
                if (ToLowerAscii(str[i]) != asciiPrefix[i])
                {
                    return false;
                }
            }

            return true;
        } // StartsWithAsciiIgnoreCase

        private static char ToLowerAscii(char ch)
        {
            if ((ch >= 'A') && (ch <= 'Z'))
            {
                return (char)(ch + ('a' - 'A'));
            }

            return ch;
        } // ToLowerAscii
        
        
    } // StringHelper

} // namespace System.Runtime.Remoting.Channels
