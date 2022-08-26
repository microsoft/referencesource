//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{    
    using System;
    using System.Globalization;    
    using System.Runtime.InteropServices;    
    
    class WsatServiceAddress
    {
        uint port;                
        string wsatServiceAddress;

        const string wsatServiceAddressFormat = "https://+:{0}/WsatService/";
        const string wsatSecurityDescriptor = "D:(A;;GA;;;NS)";

        internal WsatServiceAddress(uint port)
        {
            this.port = port;
            wsatServiceAddress = String.Format(CultureInfo.InvariantCulture, wsatServiceAddressFormat, this.port);
        }

        internal void ReserveWsatServiceAddress()
        {
            if (Utilities.IsHttpApiLibAvailable)
            {
                ReserveURL(wsatServiceAddress, wsatSecurityDescriptor);
            }
        }

        internal void FreeWsatServiceAddress()
        {
            if (Utilities.IsHttpApiLibAvailable)
            {
                FreeURL(wsatServiceAddress, wsatSecurityDescriptor);
            }
        }

        static void ReserveURL(string networkURL, string securityDescriptor)
        {
            int retVal = SafeNativeMethods.NoError;

            try
            {
                retVal = SafeNativeMethods.HttpInitialize(HttpWrapper.HttpApiVersion1, SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);

                if (SafeNativeMethods.NoError == retVal)
                {
                    HttpServiceConfigUrlAclKey keyDesc = new HttpServiceConfigUrlAclKey(networkURL);
                    HttpServiceConfigUrlAclParam paramDesc = new HttpServiceConfigUrlAclParam(securityDescriptor);

                    HttpServiceConfigUrlAclSet configInformation = new HttpServiceConfigUrlAclSet();
                    configInformation.KeyDesc = keyDesc;
                    configInformation.ParamDesc = paramDesc;

                    int configInformationLength = Marshal.SizeOf(configInformation);

                    retVal = SafeNativeMethods.HttpSetServiceConfiguration_UrlAcl(IntPtr.Zero,
                        HttpServiceConfigId.HttpServiceConfigUrlAclInfo,
                        ref configInformation,
                        configInformationLength,
                        IntPtr.Zero);

                    if (SafeNativeMethods.ErrorAlreadyExists == retVal)
                    {
                        retVal = SafeNativeMethods.HttpDeleteServiceConfiguration_UrlAcl(IntPtr.Zero,
                            HttpServiceConfigId.HttpServiceConfigUrlAclInfo,
                            ref configInformation,
                            configInformationLength,
                            IntPtr.Zero);

                        if (SafeNativeMethods.NoError == retVal)
                        {
                            retVal = SafeNativeMethods.HttpSetServiceConfiguration_UrlAcl(IntPtr.Zero,
                                HttpServiceConfigId.HttpServiceConfigUrlAclInfo,
                                ref configInformation,
                                configInformationLength,
                                IntPtr.Zero);
                        }
                    }
                }
            }
            finally
            {
                SafeNativeMethods.HttpTerminate(SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if (SafeNativeMethods.NoError != retVal)
            {
                if (SafeNativeMethods.ErrorAlreadyExists == retVal)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.REGISTER_HTTPS_PORT_ALREADYEXISTS,
                                                    SR.GetString(SR.ErrorRegisterHttpsPortAlreadyExists));
                        
                }
                else
                {
                    throw new WsatAdminException(WsatAdminErrorCode.REGISTER_HTTPS_PORT,
                                                    SR.GetString(SR.ErrorRegisterHttpsPort, retVal));
                }
            }
        }

        static void FreeURL(string networkURL, string securityDescriptor)
        {
            int retVal = SafeNativeMethods.NoError;
            try
            {
                retVal = SafeNativeMethods.HttpInitialize(HttpWrapper.HttpApiVersion1, SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                if (SafeNativeMethods.NoError == retVal)
                {
                    HttpServiceConfigUrlAclKey urlAclKey = new HttpServiceConfigUrlAclKey(networkURL);
                    HttpServiceConfigUrlAclParam urlAclParam = new HttpServiceConfigUrlAclParam(securityDescriptor);

                    HttpServiceConfigUrlAclSet configInformation = new HttpServiceConfigUrlAclSet();
                    configInformation.KeyDesc = urlAclKey;
                    configInformation.ParamDesc = urlAclParam;

                    int configInformationSize = Marshal.SizeOf(configInformation);

                    retVal = SafeNativeMethods.HttpDeleteServiceConfiguration_UrlAcl(IntPtr.Zero,
                        HttpServiceConfigId.HttpServiceConfigUrlAclInfo,
                        ref configInformation,
                        configInformationSize,
                        IntPtr.Zero);                    
                }
            }
            finally
            {
                SafeNativeMethods.HttpTerminate(SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if (retVal != SafeNativeMethods.NoError && retVal != SafeNativeMethods.FileNotFound && retVal != SafeNativeMethods.ErrorInvalidParameter)
            {
                throw new WsatAdminException(WsatAdminErrorCode.UNREGISTER_HTTPS_PORT,
                                                SR.GetString(SR.ErrorUnregisterHttpsPort, retVal));
            }
        }
    }
}
