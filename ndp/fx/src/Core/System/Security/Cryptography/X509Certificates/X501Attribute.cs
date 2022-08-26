// Copyright (c) Microsoft Corporation.  All rights reserved.

namespace System.Security.Cryptography.X509Certificates
{
    internal class X501Attribute : AsnEncodedData
    {
        internal X501Attribute(string oid, byte[] rawData)
            : base(oid, rawData)
        {
        }
    }
}
