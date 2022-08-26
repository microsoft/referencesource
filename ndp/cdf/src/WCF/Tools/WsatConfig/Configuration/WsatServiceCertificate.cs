//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;

    class WsatServiceCertificate
    {
        X509Certificate2 cert;
        uint port;

        string certificateStore = "MY";

        internal WsatServiceCertificate(X509Certificate2 cert, uint port)
        {
            this.cert = cert;
            this.port = port;
        }

        internal void BindSSLCertificate()
        {
            if (Utilities.IsHttpApiLibAvailable)
            {
                BindSSL();
            }
        }

        internal void UnbindSSLCertificate()
        {
            if (Utilities.IsHttpApiLibAvailable)
            {
                this.UnbindSSL();
            }
        }

        void BindSSL()
        {
            int retVal = SafeNativeMethods.NoError;            
            WinsockSockAddr sockAddr = null;

            try
            {
                retVal = SafeNativeMethods.HttpInitialize(HttpWrapper.HttpApiVersion1, SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                if (SafeNativeMethods.NoError == retVal)
                {
                    IntPtr pOverlapped = IntPtr.Zero;
                    sockAddr = new WinsockSockAddr(new IPAddress(0), (short)this.port);

                    HttpServiceConfigSslSet sslConf = new HttpServiceConfigSslSet();
                    sslConf.KeyDesc.pIpPort = sockAddr.PinnedSockAddr;
                    sslConf.ParamDesc.DefaultCertCheckMode = 0;
                    sslConf.ParamDesc.DefaultFlags = SafeNativeMethods.HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT;
                    sslConf.ParamDesc.DefaultRevocationFreshnessTime = 0;
                    sslConf.ParamDesc.pSslCertStoreName = certificateStore;

                    byte[] sslHash = this.cert.GetCertHash();
                    sslConf.ParamDesc.pSslHash = new SafeLocalAllocation(sslHash.Length);
                    sslConf.ParamDesc.pSslHash.Copy(sslHash, 0, sslHash.Length);
                    sslConf.ParamDesc.SslHashLength = sslHash.Length;

                    int configInformationLength = Marshal.SizeOf(sslConf);

                    retVal = SafeNativeMethods.HttpSetServiceConfiguration_Ssl(IntPtr.Zero,
                        HttpServiceConfigId.HttpServiceConfigSSLCertInfo,
                        ref sslConf,
                        configInformationLength, pOverlapped);
                    if (SafeNativeMethods.ErrorAlreadyExists == retVal)
                    {
                        retVal = SafeNativeMethods.HttpDeleteServiceConfiguration_Ssl(IntPtr.Zero,
                            HttpServiceConfigId.HttpServiceConfigSSLCertInfo,
                            ref sslConf,
                            configInformationLength,
                            IntPtr.Zero);

                        if (SafeNativeMethods.NoError == retVal)
                        {
                            retVal = SafeNativeMethods.HttpSetServiceConfiguration_Ssl(IntPtr.Zero,
                                HttpServiceConfigId.HttpServiceConfigSSLCertInfo,
                                ref sslConf,
                                configInformationLength, pOverlapped);
                        }
                    }
                    
                    GC.KeepAlive(sockAddr);

                    sslConf.ParamDesc.pSslHash.Close();
                }
            }
            finally
            {
                if (sockAddr != null)
                {
                    sockAddr.Dispose();
                }
                SafeNativeMethods.HttpTerminate(SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if (SafeNativeMethods.NoError != retVal)
            {
                if (SafeNativeMethods.ErrorAlreadyExists == retVal)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.HTTPS_PORT_SSL_CERT_BINDING_ALREADYEXISTS,
                                                    SR.GetString(SR.ErrorHttpsPortSSLBindingAlreadyExists));
                }
                else
                {
                    throw new WsatAdminException(WsatAdminErrorCode.HTTPS_PORT_SSL_CERT_BINDING,
                                                    SR.GetString(SR.ErrorHttpsPortSSLBinding, retVal));
                }
            }
        }

        void UnbindSSL()
        {
            int retVal = SafeNativeMethods.NoError;
            WinsockSockAddr sockAddr = null;

            try
            {
                retVal = SafeNativeMethods.HttpInitialize(HttpWrapper.HttpApiVersion1, SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                if (SafeNativeMethods.NoError == retVal)
                {
                    IntPtr pOverlapped = IntPtr.Zero;

                    sockAddr = new WinsockSockAddr(new IPAddress(0), (short)this.port);
                    HttpServiceConfigSslSet sslConf = new HttpServiceConfigSslSet();                    
                    sslConf.KeyDesc.pIpPort = sockAddr.PinnedSockAddr;
                    sslConf.ParamDesc.DefaultCertCheckMode = 0;
                    sslConf.ParamDesc.DefaultFlags = SafeNativeMethods.HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT;
                    sslConf.ParamDesc.DefaultRevocationFreshnessTime = 0;
                    sslConf.ParamDesc.pSslCertStoreName = certificateStore;

                    byte[] sslHash = this.cert.GetCertHash();
                    sslConf.ParamDesc.pSslHash = new SafeLocalAllocation(sslHash.Length);
                    sslConf.ParamDesc.pSslHash.Copy(sslHash, 0, sslHash.Length);
                    sslConf.ParamDesc.SslHashLength = sslHash.Length;

                    int configInformationLength = System.Runtime.InteropServices.Marshal.SizeOf(sslConf);
                    retVal = SafeNativeMethods.HttpDeleteServiceConfiguration_Ssl(IntPtr.Zero,
                        HttpServiceConfigId.HttpServiceConfigSSLCertInfo,
                        ref sslConf,
                        configInformationLength, pOverlapped);
                    sslConf.ParamDesc.pSslHash.Close();
                    
                    GC.KeepAlive(sockAddr);                                       
                }
            }
            finally
            {
                if (sockAddr != null)
                {
                    sockAddr.Dispose();
                }
                SafeNativeMethods.HttpTerminate(SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if (retVal != SafeNativeMethods.NoError && retVal != SafeNativeMethods.FileNotFound && retVal != SafeNativeMethods.ErrorInvalidParameter)
            {                
                throw new WsatAdminException(WsatAdminErrorCode.HTTPS_PORT_SSL_CERT_UNBINDING,
                                                    SR.GetString(SR.ErrorHttpsPortSSLUnbinding, retVal));
            } 
        }
    }
}
