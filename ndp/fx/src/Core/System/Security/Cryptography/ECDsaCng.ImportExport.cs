// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.Diagnostics;
using System.IO;
using System.Security;
using System.Security.Permissions;
using System.Diagnostics.Contracts;
using Microsoft.Win32.SafeHandles;

namespace System.Security.Cryptography
{
    public sealed partial class ECDsaCng : ECDsa
    {
        /// <summary>
        ///  ImportParameters will replace the existing key that ECDsaCng is working with by creating a
        ///  new CngKey. If the parameters contains only Q, then only a public key will be imported.
        ///  If the parameters also contains D, then a full key pair will be imported. 
        ///  The parameters Curve value specifies the type of the curve to import.
        /// </summary>
        /// <exception cref="CryptographicException">
        ///  if <paramref name="parameters" /> does not contain valid values.
        /// </exception>
        /// <exception cref="NotSupportedException">
        ///  if <paramref name="parameters" /> references a curve that cannot be imported.
        /// </exception>
        /// <exception cref="PlatformNotSupportedException">
        ///  if <paramref name="parameters" /> references a curve that is not supported by this platform.
        /// </exception>
        public override void ImportParameters(ECParameters parameters)
        {
            Key = ECCng.ImportECDsaParameters(ref parameters);
        }

        /// <summary>
        ///  Exports the key and explicit curve parameters used by the ECC object into an <see cref="ECParameters"/> object.
        /// </summary>
        /// <exception cref="CryptographicException">
        ///  if there was an issue obtaining the curve values.
        /// </exception>
        /// <exception cref="PlatformNotSupportedException">
        ///  if explicit export is not supported by this platform. Windows 10 or higher is required.
        /// </exception>
        /// <returns>The key and explicit curve parameters used by the ECC object.</returns>
        public override ECParameters ExportExplicitParameters(bool includePrivateParameters)
        {
            return ECCng.ExportExplicitParameters(Key, includePrivateParameters);
        }

        /// <summary>
        ///  Exports the key used by the ECC object into an <see cref="ECParameters"/> object.
        ///  If the key was created as a named curve, the Curve property will contain named curve parameters
        ///  otherwise it will contain explicit parameters.
        /// </summary>
        /// <exception cref="CryptographicException">
        ///  if there was an issue obtaining the curve values.
        /// </exception>
        /// <returns>The key and named curve parameters used by the ECC object.</returns>
        public override ECParameters ExportParameters(bool includePrivateParameters)
        {
            return ECCng.ExportParameters(Key, includePrivateParameters);
        }
    }
}
