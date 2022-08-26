// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
 
using System;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Globalization;

namespace System.Globalization
{
    public static class GlobalizationExtensions
    {
        private const CompareOptions ValidCompareMaskOffFlags =
            ~(CompareOptions.IgnoreCase | CompareOptions.IgnoreSymbols | CompareOptions.IgnoreNonSpace |
              CompareOptions.IgnoreWidth | CompareOptions.IgnoreKanaType | CompareOptions.StringSort);

        public static StringComparer GetStringComparer(this CompareInfo compareInfo, CompareOptions options)
        {
            if (compareInfo == null)
            {
                throw new ArgumentNullException(nameof(compareInfo));
            }
 
            if (options == CompareOptions.Ordinal)
            {
                return StringComparer.Ordinal;
            }
 
            if (options == CompareOptions.OrdinalIgnoreCase)
            {
                return StringComparer.OrdinalIgnoreCase;
            }
 
            if ((options & ValidCompareMaskOffFlags) != 0)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidFlag"), nameof(options));
            }
 
            return new CultureAwareComparer(compareInfo, options);
        }
    }
}
