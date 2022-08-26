//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;

    static internal partial class WsatKeys
    {
        internal const string RootRegKey = @"Software\Microsoft\";
        internal const string WsatRegKey = RootRegKey + @"WSAT\3.0\";
        internal const string WsatClusterRegKey = @"WSATPrivate\3.0";
        internal const string MsdtcClusterRegKey_OS6 = @"MSDTCPrivate\MSDTC";
        internal const string MsdtcClusterDataPointerRegKey_OS5 = "DataPointer";

        internal const string MsdtcRegKey = RootRegKey + @"MSDTC\";
        internal const string MsdtcSecurityKey = @"Security\";
        internal const string TransactionBridgeRegKey = "TransactionBridge";
        internal const string RegistryEntryX509CertificateIdentity = "X509CertificateIdentity";
        internal const string RegistryEntryHttpsPort = "HttpsPort";  // yes, not HttpsPort
        internal const string RegistryEntryTraceLevel = "DiagnosticTracing";
        internal const string RegistryEntryPropagateActivity = "DiagnosticTracingPropagateActivity";
        internal const string RegistryEntryActivityTracing = "DiagnosticTracingActivityTracing";
        internal const string RegistryEntryTracingPii = "DiagnosticTracingTracePII";
        internal const string RegistryEntryDefTimeout = "DefaultTimeout";
        internal const string RegistryEntryMaxTimeout = "MaxTimeout";
        internal const string RegistryEntryX509GlobalAcl = "X509GlobalAcl";
        internal const string RegistryEntryKerberosGlobalAcl = "KerberosGlobalAcl";
        
        internal const string DtcSecurityRegKey = RootRegKey + @"Security\";

        internal const string WSATAssemblyPathEntry = @"Path";

        internal const string WsatPropertySheetGUID = @"32739F38-BDB5-4807-BD0A-3CF038A8A804";
        internal const string WsatSnapinAboutGUID = @"32739F38-BDB6-4807-BD0A-3CF038A8A804";
        internal const string ComponentServicesComputerNodeGUID = @"0442836D-C770-11d1-87F4-00C04FC2C17B";
        internal const string ComponentServicesDtcNodeGUID = @"49e8d37b-e86e-464d-9790-f0947e9d514f";

        // register the new snapin extension
        internal const string WSATRegisterSnapinKey = @"Software\Microsoft\MMC\Snapins\";
        internal const string WSATRegisterSnapinWsatPropertySheetKey = WSATRegisterSnapinKey + "{" + WsatPropertySheetGUID + @"}\";
        internal const string WSATRegisterSnapinWsatPropertySheetAboutEntry = @"About";
        internal const string WSATRegisterSnapinWsatPropertySheetAboutValue = @"{" + WsatSnapinAboutGUID + @"}";
        internal const string WSATRegisterSnapinWsatPropertySheetNameEntry = @"NameString";
        internal const string WSATRegisterSnapinWsatPropertySheetNameValue = @"Wsat Configuration Utility"; // no need for localization

#if WSAT_UI
        // connect the snapin extension to the proper node
        internal static string WSATRegisterNodeTypesCompServCompNodeKey
        {
            get 
            {
                string mmcNodeGuid = Utilities.OSMajor > 5 ? ComponentServicesDtcNodeGUID : ComponentServicesComputerNodeGUID;

                return @"Software\Microsoft\MMC\NodeTypes\{" + mmcNodeGuid + @"}\Extensions\PropertySheet\"; 
            }
        }
#endif
        internal const string WSATRegisterNodeTypesWsatPropertySheetGUIDEntry = @"{" + WsatPropertySheetGUID + @"}";
        internal const string WSATRegisterNodeTypesWsatPropertySheetGUIDValue = @"Wsat Configuration Utility"; // no need for localization

        // WinFX installation (Paths stay the same across minor revisions since we are redbits)
        internal const string WinFXKey = RootRegKey + @"NET Framework Setup\NDP\";
        internal const string WcfSetupKey = WinFXKey + @"v3.0\Setup\Windows Communication Foundation\";
        internal const string WcfSetupKey40 = WinFXKey + @"v4.0\Setup\Windows Communication Foundation\";
        internal const string RuntimeInstallPath = "RuntimeInstallPath";

        // Wsat trace session
        internal const string MaxTraceSizeKey = "MaxTraceSize";
        internal const string TraceFileDiectoryKey = "TraceFileDirectory";
        internal const string MaxTraceBuffersKey = "MaxTraceBuffers";
    }
}
