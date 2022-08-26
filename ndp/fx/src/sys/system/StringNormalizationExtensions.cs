// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System.Text;
using System.Diagnostics.Contracts;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace System
{
    [EditorBrowsable(EditorBrowsableState.Never)]    
    public static class StringNormalizationExtensions
    {
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool IsNormalized(this string value)
        {
            return value.IsNormalized(NormalizationForm.FormC);
        }

        [System.Security.SecurityCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool IsNormalized(this string value, NormalizationForm normalizationForm)
        {
            return value.IsNormalized(normalizationForm);
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static String Normalize(this string value)
        {
            // Default to Form C
            return value.Normalize(NormalizationForm.FormC);
        }

        [System.Security.SecurityCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static String Normalize(this string value, NormalizationForm normalizationForm)
        {
            return value.Normalize(normalizationForm);
        }
    }
}

