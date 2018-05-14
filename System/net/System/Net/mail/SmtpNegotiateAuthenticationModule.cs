//-----------------------------------------------------------------------------
// <copyright file="SmtpNtlmAuthenticationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Net.Mail
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Net;
    using System.Security.Permissions;
    using System.ComponentModel;
    using System.Security.Authentication.ExtendedProtection;

    internal class SmtpNegotiateAuthenticationModule : ISmtpAuthenticationModule
    {
        Hashtable sessions = new Hashtable();

        internal SmtpNegotiateAuthenticationModule()
        {
        }

        #region ISmtpAuthenticationModule Members

        // Security this method will access NetworkCredential properties that demand UnmanagedCode and Environment Permission
        [EnvironmentPermission(SecurityAction.Assert, Unrestricted=true)]
        [SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Authorization Authenticate(string challenge, NetworkCredential credential, object sessionCookie, string spn, ChannelBinding channelBindingToken)
        {
            if(Logging.On)Logging.Enter(Logging.Web, this, "Authenticate", null);
            try {
                lock (this.sessions)
                {

                    NTAuthentication clientContext = this.sessions[sessionCookie] as NTAuthentication;
                    
                    if (clientContext == null)
                    {
                        if(credential == null){
                            return null;
                        }

                        this.sessions[sessionCookie] =
                            clientContext =
                            new NTAuthentication(false, "Negotiate", credential, spn,
                                                 ContextFlags.Connection | ContextFlags.InitIntegrity, channelBindingToken);
                    }
                    byte[] byteResp; 
                    string resp = null;
                    
                    if (!clientContext.IsCompleted) {

                        // If auth is not yet completed keep producing
                        // challenge responses with GetOutgoingBlob

                        SecurityStatus statusCode;
                        byte[] decodedChallenge = null;
                        if (challenge != null) {
                            decodedChallenge = 
                                Convert.FromBase64String(challenge);
                        }
                        byteResp = clientContext.GetOutgoingBlob(
                                                    decodedChallenge, 
                                                    false,
                                                    out statusCode);
                        if (byteResp != null) {
                            resp = Convert.ToBase64String(byteResp);
                        }                        
                    } else {
                    
                        // If auth completed and still have a challenge then
                        // server may be doing "correct" form of GSSAPI SASL.
                        // Validate incoming and produce outgoing SASL security 
                        // layer negotiate message.
                    
                        resp = GetSecurityLayerOutgoingBlob(
                                    challenge,
                                    clientContext);
                    }

                    return new Authorization(resp, clientContext.IsCompleted);
                }
            }
            finally {
                if(Logging.On)Logging.Exit(Logging.Web, this, "Authenticate", null);
            }
        }

        public string AuthenticationType
        {
            get
            {
                return "gssapi";
            }
        }

        public void CloseContext(object sessionCookie) {
            NTAuthentication clientContext = null;
            lock (sessions) {
                clientContext = sessions[sessionCookie] as NTAuthentication;
                if (clientContext != null) {
                    sessions.Remove(sessionCookie);
                }
            }
            if (clientContext != null) {
                clientContext.CloseContext();
            }
        }

        #endregion

        // Function for SASL security layer negotiation after
        // authorization completes.   
        //
        // Returns null for failure, Base64 encoded string on
        // success.
        private string GetSecurityLayerOutgoingBlob(
                            string challenge, 
                            NTAuthentication clientContext) {

            // must have a security layer challenge

            if (challenge == null) 
                return null;

            // "unwrap" challenge

            byte[] input = Convert.FromBase64String(challenge);

            int len;

            try {
                len = clientContext.VerifySignature(input, 0, input.Length);
            }
            catch (Win32Exception) {
                // any decrypt failure is an auth failure
                return null;
            }

            // Per RFC 2222 Section 7.2.2:
            //   the client should then expect the server to issue a 
            //   token in a subsequent challenge.  The client passes
            //   this token to GSS_Unwrap and interprets the first 
            //   octet of cleartext as a bit-mask specifying the 
            //   security layers supported by the server and the 
            //   second through fourth octets as the maximum size 
            //   output_message to send to the server.   
            // Section 7.2.3
            //   The security layer and their corresponding bit-masks
            //   are as follows:
            //     1 No security layer
            //     2 Integrity protection
            //       Sender calls GSS_Wrap with conf_flag set to FALSE
            //     4 Privacy protection
            //       Sender calls GSS_Wrap with conf_flag set to TRUE
            //
            // Our SmtpClient only supports "No security layer". So, make
            // sure that the server also supports this. We don't care if the
            // server also supports other features (Integrity or Privacy).
            // Therefore verify first bit in first byte is value 1.
            // We ignore the 2nd-4th bytes since token size is not
            // applicable when we select no security layer.
            if (len < 4 || (input[0] & 0x1) != 1) {
                return null;                
            }

            // Continuing with RFC 2222 section 7.2.2:
            //   The client then constructs data, with the first octet 
            //   containing the bit-mask specifying the selected security
            //   layer, the second through fourth octets containing in 
            //   network byte order the maximum size output_message the client
            //   is able to receive, and the remaining octets containing the
            //   authorization identity.  
            // 
            // Our response back to server is very simple. We select "No security layer".
            input[0] = 1;

            // let MakeSignature figure out length of output
            byte[] output = null; 
            try {
                len = clientContext.MakeSignature(input, 0, 4, ref output); 
            }
            catch (Win32Exception) {
                // any decrypt failure is an auth failure
                return null;
            }

            // return Base64 encoded string of signed payload
            return Convert.ToBase64String(output, 0, len); 
        }
    }
}
