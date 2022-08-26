//---------------------------------------------------------------------
// <copyright file="StringUtil.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------



namespace System.Data.Common.Utils {

    // This class provides some useful string utilities, e.g., converting a
    // list to string.
    internal static class StringUtil {

        /// <summary>
        ///   This private static method checks a string to make sure that it is not empty.
        ///   Comparing with String.Empty is not sufficient since a string with nothing
        ///   but white space isn't considered "empty" by that rationale.
        /// </summary>
        internal static bool IsNullOrEmptyOrWhiteSpace(string value)
        {
            return IsNullOrEmptyOrWhiteSpace(value, 0);
        }
        
        internal static bool IsNullOrEmptyOrWhiteSpace(string value, int offset)
        {
            // don't use Trim(), which will copy the string, which may be large, just to test for emptyness
            //return String.IsNullOrEmpty(value) || String.IsNullOrEmpty(value.Trim());
            if (null != value)
            {
                for(int i = offset; i < value.Length; ++i)
                {
                    if (!Char.IsWhiteSpace(value[i]))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        
        // separate implementation from IsNullOrEmptyOrWhiteSpace(string, int) because that one will
        // pick up the jit optimization to avoid boundary checks and the this won't is unknown (most likely not)
        internal static bool IsNullOrEmptyOrWhiteSpace(string value, int offset, int length)
        {
            // don't use Trim(), which will copy the string, which may be large, just to test for emptyness
            //return String.IsNullOrEmpty(value) || String.IsNullOrEmpty(value.Trim());
            if (null != value)
            {
                length = Math.Min(value.Length, length);
                for(int i = offset; i < length; ++i)
                {
                    if (!Char.IsWhiteSpace(value[i]))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    }
}
