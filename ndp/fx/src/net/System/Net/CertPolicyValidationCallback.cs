//------------------------------------------------------------------------------
// <copyright file="CertPolicyValidationCallback.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net
{
    using System.Net.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;

    // This turned to be a legacy type name that is simply forwarded to System.Security.Authentication.SslProtocols defined values.
#if !FEATURE_PAL
    internal class CertPolicyValidationCallback
    {
        readonly ICertificatePolicy m_CertificatePolicy;
        readonly ExecutionContext m_Context;

        internal CertPolicyValidationCallback()
        {
            m_CertificatePolicy = new DefaultCertPolicy();
            m_Context = null;
        }

        internal CertPolicyValidationCallback(ICertificatePolicy certificatePolicy)
        {
            m_CertificatePolicy = certificatePolicy;
            m_Context = ExecutionContext.Capture();
        }

        internal ICertificatePolicy CertificatePolicy
        {
            get { return m_CertificatePolicy; }
        }

        internal bool UsesDefault
        {
            get { return m_Context == null; }
        }

        internal void Callback(object state)
        {
            CallbackContext context = (CallbackContext)state;
            context.result = context.policyWrapper.CheckErrors(context.hostName,
                                                               context.certificate,
                                                               context.chain,
                                                               context.sslPolicyErrors);
        }

        internal bool Invoke(string hostName,
                             ServicePoint servicePoint,
                             X509Certificate certificate,
                             WebRequest request,
                             X509Chain chain,
                             SslPolicyErrors sslPolicyErrors)
        {
            PolicyWrapper policyWrapper = new PolicyWrapper(m_CertificatePolicy,
                                                            servicePoint,
                                                            (WebRequest)request);

            if (m_Context == null)
            {
                return policyWrapper.CheckErrors(hostName,
                                                 certificate,
                                                 chain,
                                                 sslPolicyErrors);
            }
            else
            {
                ExecutionContext execContext = m_Context.CreateCopy();
                CallbackContext callbackContext = new CallbackContext(policyWrapper,
                                                                      hostName,
                                                                      certificate,
                                                                      chain,
                                                                      sslPolicyErrors);
                ExecutionContext.Run(execContext, Callback, callbackContext);
                return callbackContext.result;
            }
        }

        private class CallbackContext
        {
            internal CallbackContext(PolicyWrapper policyWrapper,
                                     string hostName,
                                     X509Certificate certificate,
                                     X509Chain chain,
                                     SslPolicyErrors sslPolicyErrors)
            {
                this.policyWrapper = policyWrapper;
                this.hostName = hostName;
                this.certificate = certificate;
                this.chain = chain;
                this.sslPolicyErrors = sslPolicyErrors;
            }

            internal readonly PolicyWrapper policyWrapper;
            internal readonly string hostName;
            internal readonly X509Certificate certificate;
            internal readonly X509Chain chain;
            internal readonly SslPolicyErrors sslPolicyErrors;

            internal bool result;
        }
    }

    internal class ServerCertValidationCallback
    {
        readonly RemoteCertificateValidationCallback m_ValidationCallback;
        readonly ExecutionContext m_Context;

        internal ServerCertValidationCallback(RemoteCertificateValidationCallback validationCallback)
        {
            m_ValidationCallback = validationCallback;
            m_Context = ExecutionContext.Capture();
        }

        internal RemoteCertificateValidationCallback ValidationCallback
        {
            get { return m_ValidationCallback; }
        }

        internal void Callback(object state)
        {
            CallbackContext context = (CallbackContext)state;
            context.result = m_ValidationCallback(context.request,
                                                  context.certificate,
                                                  context.chain,
                                                  context.sslPolicyErrors);
        }

        internal bool Invoke(object request,
                             X509Certificate certificate,
                             X509Chain chain,
                             SslPolicyErrors sslPolicyErrors)
        {
            if (m_Context == null)
            {
                return m_ValidationCallback(request, certificate, chain, sslPolicyErrors);
            }
            else
            {
                ExecutionContext execContext = m_Context.CreateCopy();
                CallbackContext callbackContext = new CallbackContext(request,
                                                                      certificate,
                                                                      chain,
                                                                      sslPolicyErrors);
                ExecutionContext.Run(execContext, Callback, callbackContext);
                return callbackContext.result;
            }
        }

        private class CallbackContext
        {
            internal readonly Object request;
            internal readonly X509Certificate certificate;
            internal readonly X509Chain chain;
            internal readonly SslPolicyErrors sslPolicyErrors;

            internal bool result;

            internal CallbackContext(Object request,
                                     X509Certificate certificate,
                                     X509Chain chain,
                                     SslPolicyErrors sslPolicyErrors)
            {
                this.request = request;
                this.certificate = certificate;
                this.chain = chain;
                this.sslPolicyErrors = sslPolicyErrors;
            }
        }
    }

#endif // !FEATURE_PAL
}
