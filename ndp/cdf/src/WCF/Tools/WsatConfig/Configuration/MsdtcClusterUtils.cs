//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections.Generic;
    using System.Text;

    using Microsoft.Win32;
    
    static class MsdtcClusterUtils
    {
        static string clusterName = null;
        const string MsdtcResourceTypeName = "Distributed Transaction Coordinator";

        [SecurityCritical]
        internal static bool IsClusterServer(string machineName)
        {
#pragma warning suppress 56523
            SafeHCluster hCluster = SafeNativeMethods.OpenCluster(Utilities.IsLocalMachineName(machineName) ? null : machineName);
            bool result = false;
            using (hCluster)
            {
                result = !hCluster.IsInvalid;
            }
            return result;
        }

#if WSAT_UI
        [SecurityCritical]
        internal static bool ResolveVirtualServerName(string machineName, string dtcResourceName, out string virtualServer)
        {
            virtualServer = string.Empty;
#pragma warning suppress 56523
            SafeHCluster hCluster = SafeNativeMethods.OpenCluster(Utilities.IsLocalMachineName(machineName) ? null : machineName);
            if (hCluster.IsInvalid)
            {
                return false;
            }

            using (hCluster)
            {
#pragma warning suppress 56523
                SafeHClusEnum hEnum = SafeNativeMethods.ClusterOpenEnum(hCluster, ClusterEnum.Resource);
                if (hEnum.IsInvalid)
                {
                    return false;
                }

                using (hEnum)
                {
                    uint index = 0;
                    bool moreToEnumerate;
                    SafeHResource hResource;
                    do
                    {
                        string theResourceName = string.Empty;
                        moreToEnumerate = GetResourceFromEnumeration(hCluster, hEnum, index, out hResource, out theResourceName);
                        if (moreToEnumerate)
                        {
                            if (IsTransactionManager(hResource))
                            {
                                if (string.CompareOrdinal(theResourceName, dtcResourceName) == 0)
                                {
                                    virtualServer = GetResourceNetworkName(hResource);
                                    return true;
                                }
                            }
                            hResource.Dispose();
                            index++;
                        }
                    } while (moreToEnumerate);
                }
            }
            return false;
        }
#endif

        [SecurityCritical]
        internal static SafeHResource GetTransactionManagerClusterResource(string virtualServerName, out string[] nodes)
        {
            nodes = null;

#pragma warning suppress 56523
            SafeHCluster hCluster = SafeNativeMethods.OpenCluster(null);
            if (hCluster.IsInvalid)
            {
                return null;
            }

            using (hCluster)
            {
                //W2k3 cluster
                if (Utilities.OSMajor <= 5)
                {
                    if (Utilities.IsLocalMachineName(virtualServerName))
                    {
                        virtualServerName = GetClusterName(hCluster);
                    }
                }
#pragma warning suppress 56523
                SafeHClusEnum hEnum = SafeNativeMethods.ClusterOpenEnum(hCluster, ClusterEnum.Resource);
                if (hEnum.IsInvalid)
                {
                    return null;
                }

                using (hEnum)
                {
                    uint index = 0;
                    bool moreToEnumerate;
                    SafeHResource hResource;
                    do
                    {
                        string theResourceName = string.Empty; 
                        moreToEnumerate = GetResourceFromEnumeration(hCluster, hEnum, index, out hResource, out theResourceName);
                        if (moreToEnumerate)
                        {                            
                            if (IsTransactionManager(hResource))
                            {
                                string networkName = GetResourceNetworkName(hResource);
                                Utilities.Log("Resource network name: " + networkName);
                                if (Utilities.SafeCompare(networkName, virtualServerName))
                                {
                                    nodes = GetClusterNodes(hCluster);
                                    return hResource;
                                }
                            }
                            hResource.Dispose();
                            index++;
                        }
                    } while (moreToEnumerate);
                }
            }

            return null;
        }

        static string GetClusterName(SafeHCluster hCluster)
        {
            if (clusterName == null)
            {
                uint cch = 255u;
                StringBuilder sb = new StringBuilder((int)cch);
                int ret = SafeNativeMethods.GetClusterInformation(
                    hCluster,
                    sb,
                    ref cch,
                    IntPtr.Zero);
                if (ret != SafeNativeMethods.ERROR_SUCCESS || ret == SafeNativeMethods.ERROR_MORE_DATA)
                {
                    if (ret == SafeNativeMethods.ERROR_MORE_DATA)
                    {
                        sb = new StringBuilder((int)cch);
                        ret = SafeNativeMethods.GetClusterInformation(
                                                        hCluster,
                                                        sb,
                                                        ref cch,
                                                        IntPtr.Zero);
                        if (ret != SafeNativeMethods.ERROR_SUCCESS)
                        {
                            return null;
                        }
                    }
                }
                clusterName = sb.ToString();
            }
            return clusterName;
        }

        static bool IsTransactionManager(SafeHResource hResource)
        {
            Utilities.Log("Entered IsTransactionManager - ");

            string resourceType = GetResourceType(hResource);
            Utilities.Log("Resource type name: " + resourceType);

            return string.CompareOrdinal(resourceType, MsdtcResourceTypeName) == 0;
        }

        static string GetResourceType(SafeHResource hResource)
        {
            return IssueClusterResourceControlString(hResource, ClusterResourceControlCode.GetResourceType);
        }

        static string GetResourceNetworkName(SafeHResource hResource)
        {
            // On Vista and above, this API can be called with null and 0 to discover the true length
            // of the return buffer. However, on Win2003 that causes the API to fail with
            // ERROR_DEPENDENCY_NOT_FOUND. To play it safe, we simply pre-allocate a buffer up front
            // that should be enough to catch the two possible cases:
            // - 15-character NetBios names
            // - 63-character DNS labels
            
            uint cch = 64;
            StringBuilder sb = new StringBuilder((int)cch);
#pragma warning suppress 56523
            if (SafeNativeMethods.GetClusterResourceNetworkName(hResource, sb, ref cch))
            {
                return sb.ToString();
            }

            return null;
        }

        // return true if more items to enumerate, otherwise false
        [SecurityCritical]
        static bool GetResourceFromEnumeration(SafeHCluster hCluster,
                                               SafeHClusEnum hEnum,
                                               uint index, out SafeHResource hResource, out string theResourceName)
        {
            uint type, cch = 0;
            hResource = null;
            theResourceName = string.Empty;

            uint ret = SafeNativeMethods.ClusterEnum(hEnum, index, out type, null, ref cch);
            if (ret == SafeNativeMethods.ERROR_NO_MORE_ITEMS)
            {
                return false;
            }
            else if (ret == SafeNativeMethods.ERROR_SUCCESS || ret == SafeNativeMethods.ERROR_MORE_DATA)
            {
                StringBuilder sb = new StringBuilder((int)(++cch));
                ret = SafeNativeMethods.ClusterEnum(hEnum, index, out type, sb, ref cch);

                if (ret == SafeNativeMethods.ERROR_SUCCESS)
                {                    
                    string resourceName = sb.ToString();

#pragma warning suppress 56523
                    hResource = SafeNativeMethods.OpenClusterResource(hCluster, resourceName);
                    if (hResource.IsInvalid)
                    {
                        hResource = null;
                    }
                    else
                    {
                        theResourceName = resourceName;
                    }
                }
            }

            return true;
        }
        
        static string[] GetClusterNodes(SafeHCluster hCluster)
        {
#pragma warning suppress 56523
            SafeHClusEnum hEnum = SafeNativeMethods.ClusterOpenEnum(hCluster, ClusterEnum.Node);
            if (hEnum.IsInvalid)
            {
                return null;
            }

            List<string> nodeList = new List<string>(2); // 2 nodes are a typical cluster configuration            

            using (hEnum)
            {
                uint ret, index = 0;
                do
                {
                    uint type, cch = 0;
                    ret = SafeNativeMethods.ClusterEnum(hEnum, index, out type, null, ref cch);
                    if (ret == SafeNativeMethods.ERROR_NO_MORE_ITEMS)
                    {
                        break;
                    }
                    else if (ret == SafeNativeMethods.ERROR_SUCCESS || ret == SafeNativeMethods.ERROR_MORE_DATA)
                    {
                        StringBuilder sb = new StringBuilder((int)(++cch));
                        ret = SafeNativeMethods.ClusterEnum(hEnum, index, out type, sb, ref cch);
                        if (ret == SafeNativeMethods.ERROR_SUCCESS)
                        {
                            Utilities.Log("Found a node: [" + sb.ToString() + "]");
                            nodeList.Add(sb.ToString());
                        }
                    }
                    index++;
                } while (ret == SafeNativeMethods.ERROR_SUCCESS);
            }

            return nodeList.ToArray();
        }

        static byte[] IssueClusterResourceControl(SafeHResource hResource,
                                                  ClusterResourceControlCode code)
        {
            uint cb = 0;
            uint ret = SafeNativeMethods.ClusterResourceControl(hResource,
                                                                IntPtr.Zero,
                                                                code,
                                                                IntPtr.Zero,
                                                                0,
                                                                null,
                                                                0,
                                                                ref cb);

            if (ret == SafeNativeMethods.ERROR_SUCCESS || ret == SafeNativeMethods.ERROR_MORE_DATA)
            {
                byte[] buffer = new byte[cb];
                ret = SafeNativeMethods.ClusterResourceControl(hResource,
                                                               IntPtr.Zero,
                                                               code,
                                                               IntPtr.Zero,
                                                               0,
                                                               buffer,
                                                               cb,
                                                               ref cb);

                if (ret == SafeNativeMethods.ERROR_SUCCESS)
                {
                    return buffer;
                }
            }

            return null;
        }

        static string IssueClusterResourceControlString(SafeHResource hResource,
                                                        ClusterResourceControlCode code)
        {
            byte[] buffer = IssueClusterResourceControl(hResource, code);
            if (buffer == null)
            {
                return null;
            }

            try
            {
                return Encoding.Unicode.GetString(buffer, 0, buffer.Length - 2);
            }
            catch (ArgumentException)
            {
                return null;
            }
        }
    }
}
