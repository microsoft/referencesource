// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

using ErrorCode = Interop.NCrypt.ErrorCode;

namespace System.Security.Cryptography
{
    internal static partial class ECCng
    {
        internal static ECParameters ExportExplicitParameters(
            CngKey key,
            bool includePrivateParameters)
        {
            ECParameters ecparams = new ECParameters();
            ExportExplicitParameters(key, includePrivateParameters, ref ecparams);
            return ecparams;
        }

        internal static void ExportExplicitParameters(
            CngKey key,
            bool includePrivateParameters,
            ref ECParameters ecparams)
        {
            byte[] blob = ExportFullKeyBlob(key, includePrivateParameters);
            ECCng.ExportPrimeCurveParameters(ref ecparams, blob, includePrivateParameters);
        }

        internal static ECParameters ExportParameters(
            CngKey key,
            bool includePrivateParameters)
        {
            ECParameters ecparams = new ECParameters();
            ExportParameters(key, includePrivateParameters, ref ecparams);
            return ecparams;
        }

        internal static void ExportParameters(
            CngKey key,
            bool includePrivateParameters,
            ref ECParameters ecparams)
        {
            string curveName = key.GetCurveName();
            if (string.IsNullOrEmpty(curveName))
            {
                byte[] fullKeyBlob = ECCng.ExportFullKeyBlob(key, includePrivateParameters);
                ECCng.ExportPrimeCurveParameters(ref ecparams, fullKeyBlob, includePrivateParameters);
            }
            else
            {
                byte[] keyBlob = ECCng.ExportKeyBlob(key, includePrivateParameters);
                ECCng.ExportNamedCurveParameters(ref ecparams, keyBlob, includePrivateParameters);
                ecparams.Curve = ECCurve.CreateFromFriendlyName(curveName);
            }
        }

        internal static byte[] ExportKeyBlob(
            CngKey key,
            bool includePrivateParameters,
            out CngKeyBlobFormat format,
            out string curveName)
        {
            curveName = key.GetCurveName();
            bool forceGenericBlob = false;

            if (string.IsNullOrEmpty(curveName))
            {
                // Normalize curveName to null.
                curveName = null;

                forceGenericBlob = true;
                format = includePrivateParameters ?
                    CngKeyBlobFormat.EccFullPrivateBlob :
                    CngKeyBlobFormat.EccFullPublicBlob;
            }
            else
            {
                format = includePrivateParameters ?
                    CngKeyBlobFormat.EccPrivateBlob :
                    CngKeyBlobFormat.EccPublicBlob;
            }

            byte[] blob = key.Export(format);

            // Importing a known NIST curve as explicit parameters NCryptExportKey may
            // cause it to export with the dwMagic of the known curve and a generic blob body.
            // This combination can't be re-imported. So correct the dwMagic value to allow it
            // to import.
            if (forceGenericBlob)
            {
                FixupGenericBlob(blob);
            }

            return blob;
        }

        internal static CngKey ImportECDsaParameters(ref ECParameters ecparams)
        {
            CngKeyBlobFormat format;
            string curveName;
            byte[] blob = ECDsaParametersToBlob(ref ecparams, out format, out curveName);

            return ImportKeyBlob(blob, curveName, format, ecparams.Curve.CurveType);
        }

        internal static CngKey ImportEcdhParameters(ref ECParameters ecparams)
        {
            CngKeyBlobFormat format;
            string curveName;
            byte[] blob = EcdhParametersToBlob(ref ecparams, out format, out curveName);

            return ImportKeyBlob(blob, curveName, format, ecparams.Curve.CurveType);
        }

        internal static byte[] ECDsaParametersToBlob(
            ref ECParameters parameters,
            out CngKeyBlobFormat format,
            out string curveName)
        {
            return ParametersToBlob(
                ref parameters,
                s_ecdsaNamedMagicResolver,
                s_ecdsaExplicitMagicResolver,
                out format,
                out curveName);
        }

        internal static byte[] EcdhParametersToBlob(
            ref ECParameters parameters,
            out CngKeyBlobFormat format,
            out string curveName)
        {
            return ParametersToBlob(
                ref parameters,
                s_ecdhNamedMagicResolver,
                s_ecdhExplicitMagicResolver,
                out format,
                out curveName);
        }

        [SecuritySafeCritical]
        internal static SafeNCryptKeyHandle ImportKeyBlob(
            string blobType,
            byte[] keyBlob,
            string curveName,
            SafeNCryptProviderHandle provider)
        {
            SafeNCryptKeyHandle keyHandle;

            var desc = new Interop.BCrypt.BCryptBufferDesc();
            var buff = new Interop.BCrypt.BCryptBuffer();

            IntPtr descPtr = IntPtr.Zero;
            IntPtr buffPtr = IntPtr.Zero;
            IntPtr curveNamePtr = IntPtr.Zero;

            try
            {
                curveNamePtr = Marshal.StringToHGlobalUni(curveName);
                descPtr = Marshal.AllocHGlobal(Marshal.SizeOf(desc));
                buffPtr = Marshal.AllocHGlobal(Marshal.SizeOf(buff));
                buff.cbBuffer = (curveName.Length + 1) * 2; // Add 1 for null terminator
                buff.BufferType = Interop.BCrypt.NCryptBufferDescriptors.NCRYPTBUFFER_ECC_CURVE_NAME;
                buff.pvBuffer = curveNamePtr;
                Marshal.StructureToPtr(buff, buffPtr, false);

                desc.cBuffers = 1;
                desc.pBuffers = buffPtr;
                desc.ulVersion = Interop.BCrypt.BCRYPTBUFFER_VERSION;
                Marshal.StructureToPtr(desc, descPtr, false);

                keyHandle = NCryptNative.ImportKey(
                    provider,
                    keyBlob,
                    blobType,
                    descPtr);
            }
            catch (CryptographicException e)
            {
                if (e.HResult == (int)ErrorCode.NTE_INVALID_PARAMETER)
                {
                    throw new PlatformNotSupportedException(
                        SR.GetString(SR.Cryptography_CurveNotSupported, curveName),
                        e);
                }

                throw;
            }
            finally
            {
                Marshal.FreeHGlobal(descPtr);
                Marshal.FreeHGlobal(buffPtr);
                Marshal.FreeHGlobal(curveNamePtr);
            }

            return keyHandle;
        }
    }
}
