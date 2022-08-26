// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Diagnostics;
using System.Security.Cryptography;

namespace Internal.Cryptography
{
    internal static class CryptoThrowHelper
    {
        public static CryptographicException ToCryptographicException(this int hr)
        {
            throw new CryptographicException(hr);
        }
    }
}
