//---------------------------------------------------------------------------
//
// <copyright file="wgx_sdk_version.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// This file was generated, please do not edit it directly.
//
// Please see http://wiki/default.aspx/Microsoft.Projects.Avalon/MilCodeGen.html for more information.
//
//---------------------------------------------------------------------------

// This code is generated from mcg\generators\ProtocolFingerprint.cs

namespace MS.Internal.Composition
{
    /// <summary>
    /// This is the automatically generated part of the versioning class that
    /// contains definitions of methods returning MIL and DWM SDK versions.
    /// </summary>
    internal static class Version
    {
        /// <summary>
        /// Returns the MIL SDK version this binary has been compiled against
        /// </summary>
        internal static uint MilSdkVersion
        {
            get
            {
                unchecked
                {
                    return (uint)0x200184C0;
                }
            }
        }

        /// <summary>
        /// Returns the DWM SDK version this binary has been compiled against
        /// </summary>
        internal static uint DwmSdkVersion
        {
            get
            {
                unchecked
                {
                    return (uint)0xBDDCB2B;
                }
            }
        }
    }
}

