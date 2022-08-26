//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;

    using System.Management;
    using System.ServiceProcess;
    using System.IO;
    
    class MsdtcWrapper
    {
        // the wrapper cache. one entry for one machine name
        // the cache is ever-growing in size, but this does not matter given the characteristics
        // the program
        static Dictionary<string, MsdtcWrapper> wrappers = new Dictionary<string, MsdtcWrapper>();
        // the GUID of the IDtcNetworkAccessConfig interface
        static Guid IDtcNetworkAccessConfigInterfaceId = typeof(IDtcNetworkAccessConfig).GUID;

        IDtcNetworkAccessConfig proxy;
        ConfigurationProvider configProvider;

        ServiceController controller;
        string machineName;
        string virtualServerName;

        static string GetTransactionManagerHostName(string machineName, string virtualServerName)
        {
            if (MsdtcClusterUtils.IsClusterServer(machineName))
            {
                if (!string.IsNullOrEmpty(virtualServerName))
                {
                    return virtualServerName;
                }
                return Utilities.IsLocalMachineName(machineName) ? Utilities.LocalHostName : machineName;
            }

            return Utilities.IsLocalMachineName(machineName) ? string.Empty : machineName;
        }

        internal static MsdtcWrapper GetWrapper(string machineName, string virtualServerName, ConfigurationProvider configProvider)
        {
            MsdtcWrapper wrapper;

            string key = GetTransactionManagerHostName(machineName, virtualServerName);
            wrappers.TryGetValue(key, out wrapper);
            if (wrapper == null)
            {
                wrapper = new MsdtcWrapper(machineName, virtualServerName, configProvider);
                wrappers.Add(key, wrapper);
            }
            return wrapper;
        }

        MsdtcWrapper(string machineName, string virtualServerName, ConfigurationProvider configProvider)
        {
            if (configProvider == null)
            {
                throw new ArgumentNullException("configProvider");
            }

            this.machineName = machineName;
            this.virtualServerName = virtualServerName;
            this.configProvider = configProvider;

            this.proxy = null;

            int hr;

            string hostName = GetTransactionManagerHostName(machineName, virtualServerName);
            if (IsLongHornClusterLocalDtc(machineName, virtualServerName))
            {
                SafeNativeMethods.OLE_TM_CONFIG_PARAMS_V2 configParams = new SafeNativeMethods.OLE_TM_CONFIG_PARAMS_V2();
                configParams.dwVersion = SafeNativeMethods.OLE_TM_CONFIG_VERSION_2;
                configParams.pwszClusterResourceName = "LOCAL";

                hr = SafeNativeMethods.DtcGetTransactionManagerEx_WithConfigParams(
                    hostName,
                    string.Empty,
                    ref IDtcNetworkAccessConfigInterfaceId,
                    SafeNativeMethods.OLE_TM_FLAG_NONE,
                    ref configParams,
                    out proxy);
            }
            else
            {
                hr = SafeNativeMethods.DtcGetTransactionManagerEx(
                            hostName,
                            string.Empty,
                            ref IDtcNetworkAccessConfigInterfaceId,
                            SafeNativeMethods.OLE_TM_FLAG_NONE,
                            IntPtr.Zero,
                            out proxy);
            }

            if (proxy == null)
            {
                // Chances are we are on a XP machine; even on XP SP2 this interface is not available
                Utilities.Log("Unable to query IDtcNetworkAccessConfig" + hr);

                if (Utilities.IsLocalMachineName(machineName))
                {
                    controller = new System.ServiceProcess.ServiceController("MSDTC");
                }
                else
                {
                    // We do not use ServiceController to control remote services for secuirty concerns - use WMI remote process invocation to do that
                    controller = null;
                }
            }
        }

        static bool IsLongHornClusterLocalDtc(string machineName, string virtualServerName)
        {
            if (Utilities.GetOSMajor(machineName) > 5 && MsdtcClusterUtils.IsClusterServer(machineName))
            {
                if (string.IsNullOrEmpty(virtualServerName))
                {
                    return true;
                }
            }
            return false;
        }

        // Get whether the NetworkTransactions are enabled
        // it uses a fallback strategy: if the proxy could not be instantiated, it tries to read registry to find out
        internal bool GetNetworkTransactionAccess()
        {
            if (proxy != null)
            {
                try
                {
                    bool retVal = false;
                    proxy.GetNetworkTransactionAccess(ref retVal);
                    return retVal;
                }
                catch (COMException e)
                {
                    if (!IsLongHornClusterLocalDtc(machineName, virtualServerName))
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.CANNOT_GET_MSDTC_NETWORK_ACCESS_SETTING,
                             SR.GetString(SR.ErrorCannotGetMsdtcNetworkAccessSetting, e.Message), e);
                    }
                    // Otherwise, it's very likely that, due to WinOS bug 1455344, we are not able to call 
                    // proxy.GetNetworkTransactionAccess on a local DTC TM of a LHS cluster.
                    // The workaround is to determine network transaction access by msdtc security registry settings
                }
                catch (UnauthorizedAccessException e)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.MSDTC_NETWORK_ACCESS_DENIED,
                                                 SR.GetString(SR.ErrorMsdtcNetworkAccessDenied), e);
                }
                catch (FileNotFoundException e)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.CANNOT_GET_MSDTC_NETWORK_ACCESS_SETTING,
                                                SR.GetString(SR.ErrorCannotGetMsdtcNetworkAccessSettingWithDiagnostics), e);
                }
            }

            return DetermineNetworkTransactionAccessByRegistrySettings();
        }

        bool DetermineNetworkTransactionAccessByRegistrySettings()
        {
            ConfigurationProvider dtcSecurityConfigProvider = configProvider.OpenKey(WsatKeys.MsdtcSecurityKey);
            bool networkDtcAccess, networkDtcAccessInbound, networkDtcAccessOutbound, networkDtcAccessTransactions;
            networkDtcAccess = (dtcSecurityConfigProvider.ReadUInt32(NetworkDtcAccessRegValues.NetworkDtcAccess, 0) != 0);
            networkDtcAccessInbound = (dtcSecurityConfigProvider.ReadUInt32(NetworkDtcAccessRegValues.NetworkDtcAccessInbound, 0) != 0);
            networkDtcAccessOutbound = (dtcSecurityConfigProvider.ReadUInt32(NetworkDtcAccessRegValues.NetworkDtcAccessOutbound, 0) != 0);
            networkDtcAccessTransactions = (dtcSecurityConfigProvider.ReadUInt32(NetworkDtcAccessRegValues.NetworkDtcAccessTransactions, 0) != 0);
            return (networkDtcAccess && (networkDtcAccessInbound || networkDtcAccessOutbound) && networkDtcAccessTransactions);
        }

        static class NetworkDtcAccessRegValues
        {
            public const string NetworkDtcAccess = "NetworkDtcAccess";
            public const string NetworkDtcAccessInbound = "NetworkDtcAccessInbound";
            public const string NetworkDtcAccessOutbound = "NetworkDtcAccessOutbound";
            public const string NetworkDtcAccessTransactions = "NetworkDtcAccessTransactions";
        }

        // Restarts the MSDTC_NOTIF for the local/remote machine
        // it uses a fallback strategy: if the proxy could not be instantiated, 
        // it tries will restart MSDTC_NOTIF by using the ServiceController class
        // The proxy ONLY works on Windows VISTA
        internal void RestartDtcService()
        {
            if (proxy != null)
            {
                int hr = proxy.RestartDtcService();
                if (hr != 0)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTCWithErrorCode, hr));
                }
            }
            else
            {
                System.Diagnostics.Debug.Assert(controller != null,
                                                "Do not call RestartDtcService for remote machines, use SaveRemote instead!");
                if (controller.Status != System.ServiceProcess.ServiceControllerStatus.Stopped)
                {
                    if (controller.Status != System.ServiceProcess.ServiceControllerStatus.StopPending)
                    {
                        if (controller.CanStop)
                        {
                            controller.Stop();
                        }
                        else
                        {
                            throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTC));
                        }
                    }

                    controller.WaitForStatus(System.ServiceProcess.ServiceControllerStatus.Stopped, TimeSpan.FromSeconds(30));
                    if (controller.Status != System.ServiceProcess.ServiceControllerStatus.Stopped)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTC));
                    }
                }

                controller.Start();
                controller.WaitForStatus(System.ServiceProcess.ServiceControllerStatus.Running, TimeSpan.FromSeconds(30));
                if (controller.Status != System.ServiceProcess.ServiceControllerStatus.Running)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTC));
                }
            }
        }
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("9797C15D-A428-4291-87B6-0995031A678D")]
    interface IDtcNetworkAccessConfig
    {
        void GetAnyNetworkAccess(
            /* [out] */ ref bool anyNetworkAccess);

        void SetAnyNetworkAccess(
            /* [in] */ bool anyNetworkAccess);

        void GetNetworkAdministrationAccess(
            /* [out] */ ref bool networkAdministrationAccess);

        void SetNetworkAdministrationAccess(
            /* [in] */ bool networkAdministrationAccess);

        void GetNetworkTransactionAccess(
            /* [out] */ ref bool networkTransactionAccess);

        void SetNetworkTransactionAccess(
            /* [in] */ bool networkTransactionAccess);

        void GetNetworkClientAccess(
            /* [out] */ ref bool networkClientAccess);

        void SetNetworkClientAccess(
            /* [in] */ bool networkClientAccess);

        void GetNetworkTipAccess(
            /* [out] */ ref bool networkTipAccess);

        void SetNetworkTipAccess(
            /* [in] */ bool networkTipAccess);

        void GetXAAccess(
            /* [out] */ ref bool xaAccess);

        void SetXAAccess(
            /* [in] */ bool xaAccess);

        [PreserveSig()]
        int RestartDtcService();
    }
}
