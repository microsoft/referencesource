//------------------------------------------------------------------------------
// <copyright file="TrustManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
using System;
using System.Security;
using System.Security.Policy;
using System.Windows.Forms;
using System.Globalization;
using System.Text;
using System.IO;
using System.Collections;
using System.Reflection;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Security.Permissions;
using System.Xml;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Deployment.Internal;
using System.Deployment.Internal.Isolation;
using System.Deployment.Internal.Isolation.Manifest;
using System.Deployment.Internal.CodeSigning;
using Microsoft.Win32;
using System.Runtime.InteropServices;

namespace System.Security.Policy
{
    /// <include file='doc\TrustManager.uex' path='docs/doc[@for="TrustManager"]/*' />
    internal class TrustManager : IApplicationTrustManager
    {
        public const string PromptingLevelKeyName = @"Software\Microsoft\.NETFramework\Security\TrustManager\PromptingLevel";

        /// <include file='doc\TrustManager.uex' path='docs/doc[@for="TrustManager.TrustManager"]/*' />
        public TrustManager()
        {
        }
 
        // IApplicationTrustManager Implementation

        /// <include file='doc\TrustManager.uex' path='docs/doc[@for="TrustManager.IApplicationTrustManager.DetermineApplicationTrust"]/*' />
        [
            HostProtection(UI=true)
        ]
        public ApplicationTrust DetermineApplicationTrust(ActivationContext activationContext, TrustManagerContext trustManagerContext)
        {            
            if (activationContext == null)
            {
                throw new ArgumentNullException("activationContext");
            }

            ApplicationSecurityInfo info = new ApplicationSecurityInfo(activationContext);
            ApplicationTrustExtraInfo appTrustExtraInfo = new ApplicationTrustExtraInfo();
            // ISSUE - fix this....
            HostContextInternal hostContextInternal = new HostContextInternal(trustManagerContext);

            ICMS cms = (ICMS)InternalActivationContextHelper.GetDeploymentComponentManifest(activationContext);            

            ParsedData parsedData = new ParsedData();
            if (ParseManifest(cms, parsedData))
            {
                appTrustExtraInfo.RequestsShellIntegration = parsedData.RequestsShellIntegration; 
            }

            string deploymentUrl = GetDeploymentUrl(info);
            string zoneName = GetZoneNameFromDeploymentUrl(deploymentUrl);
            MemoryStream ms;
            PromptsAllowed promptsAllowed;

            if (!ExtractManifestContent(cms, out ms))
            {
                // Block prompt
                return BlockingPrompt(activationContext, parsedData, deploymentUrl, info, appTrustExtraInfo, zoneName, AppRequestsBeyondDefaultTrust(info) /*permissionElevationRequired*/);
            }
            
            bool distrustedPublisher, trustedPublisher, noCertificate;
            AnalyzeCertificate(parsedData, ms, out distrustedPublisher, out trustedPublisher, out noCertificate);

            /// Check whether application manifest allows to use deployment manifest certificate.
            /// If not then we have to use application manifest certificate instead.
            ICMS applicationCms = (ICMS)InternalActivationContextHelper.GetApplicationComponentManifest(activationContext);
            ParsedData applicationParsedData = new ParsedData();
            if (ParseManifest(applicationCms, applicationParsedData))
            {
                if (applicationParsedData.UseManifestForTrust)
                {
                    MemoryStream applicationMs;
                    if (ExtractManifestContent(applicationCms, out applicationMs))
                    {
                        /// Use the old parsedData.
                        bool applicationDistrustedPublisher, applicationTrustedPublisher, applicationNoCertificate;
                        AnalyzeCertificate(parsedData, applicationMs, out applicationDistrustedPublisher, out applicationTrustedPublisher, out applicationNoCertificate);
                        distrustedPublisher = applicationDistrustedPublisher;
                        trustedPublisher = applicationTrustedPublisher;
                        noCertificate = applicationNoCertificate;
                        parsedData.AppName = applicationParsedData.AppName;
                        parsedData.AppPublisher = applicationParsedData.AppPublisher;
                        parsedData.SupportUrl = applicationParsedData.SupportUrl;
                    }
                }
            }

            if (distrustedPublisher)
            {             
                promptsAllowed = GetPromptsAllowed(hostContextInternal, zoneName, parsedData);
                if (promptsAllowed == PromptsAllowed.None)
                {
                    // No prompt allowed, return Do Not Trust.
                    return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, false /*trust*/, false /*persist*/);
                }
                return BlockingPrompt(activationContext, parsedData, deploymentUrl, info, appTrustExtraInfo, zoneName, AppRequestsBeyondDefaultTrust(info) /*permissionElevationRequired*/);
            }            

            if (noCertificate)
            {            
                parsedData.AuthenticodedPublisher = null;
                parsedData.Certificate = null;
            }

            if (!hostContextInternal.IgnorePersistedDecision)
            {
                // Check if there are previously trusted versions installed.
                ArrayList matchingTrusts;
                if (SearchPreviousTrustedVersion(activationContext, out matchingTrusts))
                {
                    Debug.Assert(matchingTrusts != null && matchingTrusts.Count > 0);

                    // Found a matching app, with normally a different version.
                    if (ExistingTrustApplicable(info, matchingTrusts))
                    {
                        // There is at least one old version that requires at the same or more permissions.
                        // ExistingTrustApplicable removed the non-applicable version from the matchingTrusts arrays.
                        Debug.Assert(matchingTrusts != null && matchingTrusts.Count > 0);

                        // Check if the new app requires shell integration while none of the old ones did
                        if (appTrustExtraInfo.RequestsShellIntegration &&
                            !SomePreviousTrustedVersionRequiresShellIntegration(matchingTrusts) &&
                            !trustedPublisher)
                        {
                            promptsAllowed = GetPromptsAllowed(hostContextInternal, zoneName, parsedData);
                            switch (promptsAllowed)
                            {
                                case PromptsAllowed.None:
                                    // No prompt allowed, return Do Not Trust.
                                    return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, false /*trust*/, false /*persist*/);
                                case PromptsAllowed.BlockingOnly:
                                    return BlockingPrompt(activationContext, parsedData, deploymentUrl, info, appTrustExtraInfo, zoneName, AppRequestsBeyondDefaultTrust(info) /*permissionElevationRequired*/);
                                case PromptsAllowed.All:
                                    // New app requires shell integration - bring up the Basic Install Prompt
                                    return BasicInstallPrompt(activationContext,
                                                              parsedData, 
                                                              deploymentUrl, 
                                                              hostContextInternal, 
                                                              info, 
                                                              appTrustExtraInfo,
                                                              zoneName, 
                                                              AppRequestsBeyondDefaultTrust(info) /*permissionElevationRequired*/);
                            }
                        }

                        // No prompt, return Trust & Persist.
                        return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, true /*trust*/, hostContextInternal.Persist /*persist*/);
                    }
                }
            }            

            bool permissionElevationRequired = AppRequestsBeyondDefaultTrust(info);
            if (!permissionElevationRequired || trustedPublisher)
            {             
                if (!trustedPublisher)
                {
                    Debug.Assert(!permissionElevationRequired);
                    promptsAllowed = GetPromptsAllowed(hostContextInternal, zoneName, parsedData);
                    switch (promptsAllowed)
                    {
                        case PromptsAllowed.BlockingOnly:
                            return BlockingPrompt(activationContext, parsedData, deploymentUrl, info, appTrustExtraInfo, zoneName, permissionElevationRequired);
                        case PromptsAllowed.None:
                            // XBaps should also prompt in InternetZone
                            // Originally xbaps were silently trusted, along with other ClickOnce apps
                        case PromptsAllowed.All:
                            // App shell integrates and is not from a trusted deployer, bring up the Basic Install Prompt.
                            return BasicInstallPrompt(activationContext,
                                                      parsedData, 
                                                      deploymentUrl, 
                                                      hostContextInternal, 
                                                      info,
                                                      appTrustExtraInfo,
                                                      zoneName,
                                                      false /*permissionElevationRequired*/);
                    }
                }
                else
                {
                    // App does not shell integrate and does not run in "Internet" zone, or is from a trusted deployer, return Trust
                    return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, true /*trust*/, hostContextInternal.Persist /*persist*/);
                }
            }            

            promptsAllowed = GetPromptsAllowed(hostContextInternal, zoneName, parsedData);
            switch (promptsAllowed)
            {
                case PromptsAllowed.None:
                    // No prompt allowed, return Do Not Trust.
                    return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, false /*trust*/, false /*persist*/);
                case PromptsAllowed.BlockingOnly:
                    return BlockingPrompt(activationContext, parsedData, deploymentUrl, info, appTrustExtraInfo, zoneName, true /*permissionElevationRequired*/);
                default: // PromptsAllowed.All:
                    // Bring up the HighRisk Install Prompt if the app shell integrates, or the HighRisk Run Prompt otherwise.
                    return HighRiskPrompt(activationContext, parsedData, deploymentUrl, hostContextInternal, info, appTrustExtraInfo, zoneName);
            }
        }

        // ISecurityEncodable implementation
        
        /// <include file='doc\TrustManager.uex' path='docs/doc[@for="TrustManager.ISecurityEncodable.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            SecurityElement elRoot = new SecurityElement("IApplicationTrustManager");
            elRoot.AddAttribute("class", SecurityElement.Escape(this.GetType().AssemblyQualifiedName));
            elRoot.AddAttribute("version", "1");
            return elRoot;
        }
    
        /// <include file='doc\TrustManager.uex' path='docs/doc[@for="TrustManager.ISecurityEncodable.FromXml"]/*' />
        public void FromXml(SecurityElement element)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            if (!String.Equals(element.Tag, "IApplicationTrustManager", StringComparison.Ordinal))
            {
                throw new ArgumentException(SR.GetString(SR.TrustManagerBadXml ,"IApplicationTrustManager"));
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static string DefaultBrowserExePath
        {
            get
            {
                try
                {
                    string strBrowserPath = null;
                    new RegistryPermission(PermissionState.Unrestricted).Assert();
                    try 
                    {
                        RegistryKey key = Registry.ClassesRoot.OpenSubKey("http\\shell\\open\\command");
                        if (key != null)
                        {
                            string strBrowserCommand = (string) key.GetValue(string.Empty);
                            key.Close();

                            if (strBrowserCommand != null)
                            {
                                strBrowserCommand = strBrowserCommand.Trim();
                                if (strBrowserCommand.Length != 0)
                                {
                                    if (strBrowserCommand[0] == '\"')
                                    {
                                        int closingQuoteIndex = strBrowserCommand.IndexOf('"', 1);
                                        if (closingQuoteIndex != -1)
                                        {
                                            strBrowserPath = strBrowserCommand.Substring(1, closingQuoteIndex - 1);
                                        }
                                    }
                                    else
                                    {
                                        int firstSpaceIndex = strBrowserCommand.IndexOf(' ');
                                        if (firstSpaceIndex != -1)
                                        {
                                            strBrowserPath = strBrowserCommand.Substring(0, firstSpaceIndex);
                                        }
                                        else
                                        {
                                            strBrowserPath = strBrowserCommand;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    finally
                    {
                        System.Security.CodeAccessPermission.RevertAssert();
                    }
                    return strBrowserPath;
                }
                catch (Exception ex)
                {
                    Debug.Fail("Exception occurred while locating default browser executable: " + ex.Message);
                    return null;
                }
            }
        }

        private static void ProcessSignerInfo(SignedCmiManifest2 signedManifest, out bool distrustedPublisher, out bool noCertificate)
        {
            Debug.Assert(signedManifest != null);
            Debug.Assert(signedManifest.AuthenticodeSignerInfo != null);

            const int TRUST_E_REVOKED = unchecked((int)0x80092010);
            distrustedPublisher = false;
            noCertificate = false;

            //
            // AuthenticodeSignerInfo can be null if the manifest is only strong name signed
            // but not authenticode signed. This can happen when the strong name
            // signature is invalid.
            //
            int error = signedManifest.AuthenticodeSignerInfo.ErrorCode;
            if (error == Win32.TRUST_E_EXPLICIT_DISTRUST || error == TRUST_E_REVOKED)
            {
                distrustedPublisher = true;
                return;
            }
            
            if (error == Win32.TRUST_E_SUBJECT_NOT_TRUSTED)
            {
                // Certificate is valid but publisher is not in trusted list
                return;
            }

            // No certificate is equivalent to strong name signing only - run app only with user consent
            noCertificate = true;
            return;
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static bool AnalyzeCertificate(ParsedData parsedData, MemoryStream ms, out bool distrustedPublisher, out bool trustedPublisher, out bool noCertificate)
        {
            Debug.Assert(parsedData != null);
            Debug.Assert(ms != null);

            distrustedPublisher = false;
            trustedPublisher = false;
            noCertificate = false;
            SignedCmiManifest2 signedManifest = null;
            XmlDocument deploymentManifestXmlDom = null;
            try
            {
                deploymentManifestXmlDom = new XmlDocument();
                deploymentManifestXmlDom.PreserveWhitespace = true;
                deploymentManifestXmlDom.Load(ms);

                signedManifest = new SignedCmiManifest2(deploymentManifestXmlDom, true);

                signedManifest.Verify(CmiManifestVerifyFlags.None);                
            }
            catch (Exception e)
            {
                if (!(e is CryptographicException))
                {
                    return false;
                }
                if (signedManifest.StrongNameSignerInfo != null && signedManifest.StrongNameSignerInfo.ErrorCode != Win32.TRUST_E_BAD_DIGEST) {
                    if (signedManifest.AuthenticodeSignerInfo == null) 
                    {
                        return false;
                    }
                    ProcessSignerInfo(signedManifest, out distrustedPublisher, out noCertificate);
                    return true;
                }
                // try SHA1 
                try
                {
                    signedManifest = new SignedCmiManifest2(deploymentManifestXmlDom, false);
                    signedManifest.Verify(CmiManifestVerifyFlags.None);
                }
                catch (Exception ex)
                {
                    if (!(ex is CryptographicException)) 
                    {
                        return false;
                    }

                    if (signedManifest.AuthenticodeSignerInfo != null) 
                    {
                        ProcessSignerInfo(signedManifest, out distrustedPublisher, out noCertificate);
                        return true;
                    }
                    return false;
                }
                
            }
            finally
            {                
                if (signedManifest != null && 
                    signedManifest.AuthenticodeSignerInfo != null && 
                    signedManifest.AuthenticodeSignerInfo.SignerChain != null)
                {             
                    parsedData.Certificate = signedManifest.AuthenticodeSignerInfo.SignerChain.ChainElements[0].Certificate;
                    parsedData.AuthenticodedPublisher = parsedData.Certificate.GetNameInfo(X509NameType.SimpleName, false);
                }
            }

            if (signedManifest == null || signedManifest.AuthenticodeSignerInfo == null)
            {
                noCertificate = true;
            }
            else
            {
                trustedPublisher = true;
            }

            return true;
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static bool AppRequestsBeyondDefaultTrust(ApplicationSecurityInfo info)
        {
            Debug.Assert(info != null);

            try
            {
                PermissionSet permSetDefaultZone = SecurityManager.GetStandardSandbox(info.ApplicationEvidence);
                PermissionSet permSetRequested = GetRequestedPermissionSet(info);

                if (permSetDefaultZone == null && permSetRequested != null)
                {
                    // No permissions are granted, and this launch requests some permissions
                    return true;
                }
                else if (permSetDefaultZone != null && permSetRequested == null)
                {
                    // Some permissions are granted, and this launch does not require any permissions
                    return false;
                }

                Debug.Assert(permSetDefaultZone != null);
                Debug.Assert(permSetRequested != null);

                return !permSetRequested.IsSubsetOf(permSetDefaultZone);
            }
            catch (Exception)
            {
                return true;
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static ApplicationTrust BasicInstallPrompt(ActivationContext activationContext,
                                                           ParsedData parsedData,
                                                           String deploymentUrl,
                                                           HostContextInternal hostContextInternal, 
                                                           ApplicationSecurityInfo info,
                                                           ApplicationTrustExtraInfo appTrustExtraInfo,
                                                           string zoneName,
                                                           bool permissionElevationRequired)
        {
            DialogResult ret;
            TrustManagerPromptOptions options = CompletePromptOptions(permissionElevationRequired ? TrustManagerPromptOptions.RequiresPermissions : TrustManagerPromptOptions.None, appTrustExtraInfo, zoneName, info);
            try
            {
                TrustManagerPromptUIThread basicInstallDialog = new TrustManagerPromptUIThread(string.IsNullOrEmpty(parsedData.AppName) ? info.ApplicationId.Name : parsedData.AppName,
                                                                                   DefaultBrowserExePath,
                                                                                   parsedData.SupportUrl,
                                                                                   GetHostFromDeploymentUrl(deploymentUrl),
                                                                                   parsedData.AuthenticodedPublisher /*publisherName*/,
                                                                                   parsedData.Certificate,
                                                                                   options);
                ret = basicInstallDialog.ShowDialog();
            }
            catch (Exception ex)
            {
                Debug.Fail("Error occurred while showing basic install dialog: " + ex.Message);
                ret = DialogResult.No;
            }

            return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, ret == DialogResult.OK /*trust*/, hostContextInternal.Persist && ret == DialogResult.OK /*persist*/);
        }

        private static TrustManagerPromptOptions CompletePromptOptions(TrustManagerPromptOptions options,
                                                                       ApplicationTrustExtraInfo appTrustExtraInfo,
                                                                       string zoneName, 
                                                                       ApplicationSecurityInfo info)
        {
            if (appTrustExtraInfo.RequestsShellIntegration)
            {
                options |= TrustManagerPromptOptions.AddsShortcut;
            }
            if (zoneName != null)
            {
                if (string.Compare(zoneName, "Internet", true, CultureInfo.InvariantCulture) == 0)
                {
                    options |= TrustManagerPromptOptions.InternetSource;
                }
                else if (string.Compare(zoneName, "TrustedSites", true, CultureInfo.InvariantCulture) == 0)
                {
                    options |= TrustManagerPromptOptions.TrustedSitesSource;
                }
                else if (string.Compare(zoneName, "UntrustedSites", true, CultureInfo.InvariantCulture) == 0)
                {
                    options |= TrustManagerPromptOptions.UntrustedSitesSource;
                }
                else if (string.Compare(zoneName, "LocalIntranet", true, CultureInfo.InvariantCulture) == 0)
                {
                    options |= TrustManagerPromptOptions.LocalNetworkSource;
                }
                else if (string.Compare(zoneName, "MyComputer", true, CultureInfo.InvariantCulture) == 0)
                {
                    options |= TrustManagerPromptOptions.LocalComputerSource;
                }
            }
            if (info != null)
            {
                PermissionSet pset = info.DefaultRequestSet;
                if (pset != null && pset.IsUnrestricted())
                {
                    options |= TrustManagerPromptOptions.WillHaveFullTrust;
                }
            }
            return options;
        }

        private static ApplicationTrust CreateApplicationTrust(ActivationContext activationContext,
                                                               ApplicationSecurityInfo info,
                                                               ApplicationTrustExtraInfo appTrustExtraInfo,
                                                               bool trust,
                                                               bool persist)
        {
            ApplicationTrust appTrust = new ApplicationTrust(activationContext.Identity);

            appTrust.ExtraInfo = appTrustExtraInfo;
            appTrust.IsApplicationTrustedToRun = trust;
            appTrust.DefaultGrantSet = new PolicyStatement(info.DefaultRequestSet, (PolicyStatementAttribute) 0);
            appTrust.Persist = persist;

            return appTrust;
        }

        private static bool ExistingTrustApplicable(ApplicationSecurityInfo info, 
                                                    ArrayList matchingTrusts)
        {
            Debug.Assert(info != null);
            Debug.Assert(matchingTrusts != null);

            int entry = 0;
            while (entry < matchingTrusts.Count)
            {
                ApplicationTrust matchingTrust = (ApplicationTrust) matchingTrusts[entry];
                if (!matchingTrust.IsApplicationTrustedToRun)
                {
                    // Microsoft: Can this ever happen?  I have serious doubts...
                    matchingTrusts.RemoveAt(entry);
                }

                PermissionSet permSetRequested = GetRequestedPermissionSet(info);
                PermissionSet permSetGranted = matchingTrust.DefaultGrantSet.PermissionSet; //PolicyStatement makes of copy of its permission set here.

                if (permSetGranted == null && permSetRequested != null)
                {
                    // No permissions were granted, and this launch requests some permissions
                    matchingTrusts.RemoveAt(entry);
                }
                else if (permSetGranted != null && permSetRequested == null)
                {
                    // Some permissions were granted, and this launch does not require any permissions
                    entry++;
                    continue;
                }

                Debug.Assert(permSetGranted != null);
                Debug.Assert(permSetRequested != null);

                if (permSetRequested.IsSubsetOf(permSetGranted))
                {
                    entry++; // Found a version that requires at least as much
                }
                else
                {
                    // This version does not require as much
                    matchingTrusts.RemoveAt(entry);
                }
            }
            return matchingTrusts.Count > 0;
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private unsafe static bool ExtractManifestContent(ICMS cms, out MemoryStream ms)
        {
            ms = new MemoryStream();
            try
            {
                System.Runtime.InteropServices.ComTypes.IStream pStream = cms as System.Runtime.InteropServices.ComTypes.IStream;
                if (pStream == null)
                {
                    return false;
                }

                byte[] pv = new byte[4096];
                int size = 4096;
                do {
                    pStream.Read(pv, size, new IntPtr(&size));
                    ms.Write(pv, 0, size);
                }
                while (size == 4096);
                ms.Position = 0;
                return true;
            }
            catch (Exception)
            {
                Debug.Fail("Exception occurred in ExtractDeploymentManifestContent.");
                return false;
            }
        }

        private static bool IsInternetZone(string zoneName)
        {
            if (string.Compare(zoneName, "Internet", true, CultureInfo.InvariantCulture) == 0)
            {
                return true;
            }

            return false;
        }
        
        private static PromptingLevel GetDefaultPromptingLevel(string zoneName)
        {
            PromptingLevel promptingLevel;
            switch (zoneName)
            {
                case "Internet":
                case "LocalIntranet":
                case "MyComputer":
                case "TrustedSites":
                    promptingLevel = PromptingLevel.Prompt;
                    break;
                case "UntrustedSites":
                    promptingLevel = PromptingLevel.Disabled;
                    break;
                default:
                    promptingLevel = PromptingLevel.Disabled;
                    break;
            }
            return promptingLevel;
        }

        private static string GetDeploymentUrl(ApplicationSecurityInfo info)
        {
            Debug.Assert(info != null);
            Evidence appEvidence = info.ApplicationEvidence;

            Url deploymentUrl = appEvidence.GetHostEvidence<Url>();
            if (deploymentUrl != null)
            {
                return deploymentUrl.Value;
            }

            // Couldn't find deployment Url
            return null;
        }

        private static PermissionSet GetRequestedPermissionSet(ApplicationSecurityInfo info)
        {
            Debug.Assert(info != null);

            PermissionSet pset = info.DefaultRequestSet;
            PermissionSet permSetRequested = null;
            if (pset != null)
            {
                permSetRequested = pset.Copy();
            }
            return permSetRequested;
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static string GetHostFromDeploymentUrl(string deploymentUrl)
        {
            if (deploymentUrl == null)
            { 
                return string.Empty;
            }
            string host = null;
            try
            {
                System.Uri uri = new System.Uri(deploymentUrl);
                if (uri.Scheme == System.Uri.UriSchemeHttp || uri.Scheme == System.Uri.UriSchemeHttps)
                {
                    host = uri.Host;                    
                }
                if (string.IsNullOrEmpty(host))
                {
                    host = uri.AbsolutePath;
                    int separatorIndex = -1;
                    if (string.IsNullOrEmpty(uri.Host) && host.StartsWith("/"))
                    {
                        host = host.TrimStart('/');
                        separatorIndex = host.IndexOf('/');
                    }
                    else if (uri.LocalPath.Length > 2 && (uri.LocalPath[1] == ':' || uri.LocalPath.StartsWith("\\\\")))
                    {
                        host = uri.LocalPath;
                        separatorIndex = host.LastIndexOf('\\');
                    }
                    if (separatorIndex != -1)
                    {
                        host = host.Remove(separatorIndex);
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.Fail("Exception occurred in GetHostFromDeploymentUrl: " + ex.Message);
                return string.Empty;
            }
            return host;
        }

        private static PromptsAllowed GetPromptsAllowed(HostContextInternal hostContextInternal, 
                                                        string zoneName, 
                                                        ParsedData parsedData)
        {
            Debug.Assert(hostContextInternal != null);
            Debug.Assert(zoneName != null);
            Debug.Assert(parsedData != null);

            if (hostContextInternal.NoPrompt)
            {
                return PromptsAllowed.None;
            }

            PromptingLevel promptingLevel = GetZonePromptingLevel(zoneName);
            if (promptingLevel == PromptingLevel.Disabled || 
                (promptingLevel == PromptingLevel.PromptOnlyForAuthenticode && parsedData.AuthenticodedPublisher == null))
            {
                return PromptsAllowed.BlockingOnly;
            }

            return PromptsAllowed.All;
        }

        private static string GetZoneNameFromDeploymentUrl(String deploymentUrl)
        {
            Zone zone = Zone.CreateFromUrl(deploymentUrl);
            if (zone == null || zone.SecurityZone == SecurityZone.NoZone)
            {
                return "UntrustedSites";
            }
            switch (zone.SecurityZone)
            {
                case SecurityZone.Internet:
                    return "Internet";
                case SecurityZone.Intranet:
                    return "LocalIntranet";
                case SecurityZone.MyComputer:
                    return "MyComputer";
                case SecurityZone.Trusted:
                    return "TrustedSites";
                case SecurityZone.Untrusted:
                    return "UntrustedSites";
                default:
                    Debug.Fail("Unexpected SecurityZone in GetZoneNameFromDeploymentUrl");
                    return "UntrustedSites";
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static PromptingLevel GetZonePromptingLevel(string zoneName)
        {
            try
            {
                string promptingLevelStr = null;
                new RegistryPermission(PermissionState.Unrestricted).Assert();
                try 
                {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(PromptingLevelKeyName))
                    {
                        if (key != null) 
                        {
                            promptingLevelStr = (string)key.GetValue(zoneName);
                        }
                    }
                }
                finally
                {
                    System.Security.CodeAccessPermission.RevertAssert();
                }

                if (string.IsNullOrEmpty(promptingLevelStr))
                {
                    return GetDefaultPromptingLevel(zoneName);
                }
                else if (string.Compare(promptingLevelStr, "Enabled", true, CultureInfo.InvariantCulture) == 0)
                {
                    return PromptingLevel.Prompt;
                }
                else if (string.Compare(promptingLevelStr, "Disabled", true, CultureInfo.InvariantCulture) == 0)
                {
                    return PromptingLevel.Disabled;
                }
                else if (string.Compare(promptingLevelStr, "AuthenticodeRequired", true, CultureInfo.InvariantCulture) == 0)
                {
                    return PromptingLevel.PromptOnlyForAuthenticode;
                }
                else
                {
                    return GetDefaultPromptingLevel(zoneName);
                }
            }
            catch (Exception ex)
            {
                Debug.Fail("Exception occurred in GetZonePromptingLevel: " + ex.Message);
                return GetDefaultPromptingLevel(zoneName);
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static ApplicationTrust HighRiskPrompt(ActivationContext activationContext,
                                                       ParsedData parsedData,
                                                       String deploymentUrl,
                                                       HostContextInternal hostContextInternal, 
                                                       ApplicationSecurityInfo info, 
                                                       ApplicationTrustExtraInfo appTrustExtraInfo,
                                                       string zoneName)
        {
            DialogResult ret;
            TrustManagerPromptOptions options = CompletePromptOptions(TrustManagerPromptOptions.RequiresPermissions, appTrustExtraInfo, zoneName, info);
            try
            {
                TrustManagerPromptUIThread highRiskDialog = new TrustManagerPromptUIThread(string.IsNullOrEmpty(parsedData.AppName) ? info.ApplicationId.Name : parsedData.AppName,
                                                                               DefaultBrowserExePath,
                                                                               parsedData.SupportUrl, 
                                                                               GetHostFromDeploymentUrl(deploymentUrl),
                                                                               parsedData.AuthenticodedPublisher /*publisherName*/, 
                                                                               parsedData.Certificate,
                                                                               options);
                ret = highRiskDialog.ShowDialog();
            }
            catch (Exception ex)
            {
                Debug.Fail("Error occurred while showing high risk dialog: " + ex.Message);
                ret = DialogResult.No;
            }

            return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, ret == DialogResult.OK /*trust*/, hostContextInternal.Persist && ret == DialogResult.OK /*persist*/);
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static ApplicationTrust BlockingPrompt(ActivationContext activationContext,
                                                       ParsedData parsedData,
                                                       String deploymentUrl,
                                                       ApplicationSecurityInfo info,
                                                       ApplicationTrustExtraInfo appTrustExtraInfo,
                                                       string zoneName,
                                                       bool permissionElevationRequired)
        {
            TrustManagerPromptOptions options = CompletePromptOptions(permissionElevationRequired ? (TrustManagerPromptOptions.StopApp | TrustManagerPromptOptions.RequiresPermissions) : TrustManagerPromptOptions.StopApp,
                                                                      appTrustExtraInfo, zoneName, info);
            try
            {
                TrustManagerPromptUIThread errorDialog = new TrustManagerPromptUIThread(string.IsNullOrEmpty(parsedData.AppName) ? info.ApplicationId.Name : parsedData.AppName,
                                                                            DefaultBrowserExePath,
                                                                            parsedData.SupportUrl,
                                                                            GetHostFromDeploymentUrl(deploymentUrl),
                                                                            parsedData.AuthenticodedPublisher /*publisherName*/,
                                                                            parsedData.Certificate,
                                                                            options);
                errorDialog.ShowDialog();
            }
            catch (Exception ex)
            {
                Debug.Fail("Error occurred while showing error dialog: " + ex.Message);
            }

            // Trust Manager should prompt, but is not allowed to. Return Don't Trust and Don't Persist.
            return CreateApplicationTrust(activationContext, info, appTrustExtraInfo, false /*trust*/, false /*persist*/);
        }
                

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")
        ]
        private static bool ParseManifest(ICMS cms, ParsedData parsedData)
        {
            // parsedData is safely initialized to requestsShellIntegration:false/supportUrl:null/authenticodedPublisher:null  //deploymentProviderCodebase:null
            try
            {
                if (cms != null)
                {
                    // Extract SupportUrl and Shell Visibility
                    if (cms.MetadataSectionEntry != null)
                    {
                        IMetadataSectionEntry metaDataSectionEntry = cms.MetadataSectionEntry as IMetadataSectionEntry;
                        if (metaDataSectionEntry != null)
                        {
                            IDescriptionMetadataEntry description = metaDataSectionEntry.DescriptionData;
                            if (description != null)
                            {
                                parsedData.SupportUrl = description.SupportUrl;
                                parsedData.AppName = description.Product;
                                parsedData.AppPublisher = description.Publisher;
                            }

                            IDeploymentMetadataEntry deployment = metaDataSectionEntry.DeploymentData;
                            if (deployment != null)
                            {
                                parsedData.RequestsShellIntegration = ((deployment.DeploymentFlags & 
                                    (uint)(CMS_ASSEMBLY_DEPLOYMENT_FLAG.CMS_ASSEMBLY_DEPLOYMENT_FLAG_INSTALL)) != 0);
                                // parsedData.DeploymentProviderCodebase = deployment.DeploymentProviderCodebase;
                            }

                            if ((metaDataSectionEntry.ManifestFlags & (uint)CMS_MANIFEST_FLAG.CMS_MANIFEST_FLAG_USEMANIFESTFORTRUST) != 0)
                            {                                
                                parsedData.UseManifestForTrust = true;
                            }
                            else
                            {                             
                                parsedData.UseManifestForTrust = false;
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Debug.Fail("Error parsing manifest: " + e.Message);
                return false;
            }
            return true;
        }

        private static bool SomePreviousTrustedVersionRequiresShellIntegration(ArrayList /*ApplicationTrust[]*/ matchingTrusts)
        {
            Debug.Assert(matchingTrusts != null);
            foreach (ApplicationTrust matchingTrust in matchingTrusts)
            {
                ApplicationTrustExtraInfo matchingAppTrustExtraInfo = matchingTrust.ExtraInfo as ApplicationTrustExtraInfo;
                if (matchingAppTrustExtraInfo != null && matchingAppTrustExtraInfo.RequestsShellIntegration)
                {
                    return true;
                }
                if (null == matchingAppTrustExtraInfo && matchingTrust.DefaultGrantSet.PermissionSet.IsUnrestricted())
                {   // Full trust trumps RequestsShellIntegration, so return true in this case.
                    return true;
                }
            }
            // No previous trusted version requires shell integration
            return false;
        }

        private static bool SearchPreviousTrustedVersion(ActivationContext activationContext,
                                                         out ArrayList matchingTrusts)
        {
            // No match found by default
            matchingTrusts = null;

            ApplicationTrustCollection appTrusts = ApplicationSecurityManager.UserApplicationTrusts;
            foreach (ApplicationTrust appTrust in appTrusts) {
                IDefinitionAppId appTrustAppId = IsolationInterop.AppIdAuthority.TextToDefinition(0, appTrust.ApplicationIdentity.FullName);
                IDefinitionAppId actCtxAppId = IsolationInterop.AppIdAuthority.TextToDefinition(0, activationContext.Identity.FullName);
                if (IsolationInterop.AppIdAuthority.AreDefinitionsEqual((uint) IAPPIDAUTHORITY_ARE_DEFINITIONS_EQUAL_FLAGS.IAPPIDAUTHORITY_ARE_DEFINITIONS_EQUAL_FLAG_IGNORE_VERSION, appTrustAppId, actCtxAppId))
                {
                    // Found an older matching app (or same version app which should never occur in theory) 
                    if (matchingTrusts == null)
                    {
                        matchingTrusts = new ArrayList();
                    }
                    matchingTrusts.Add(appTrust);
                }
            }

            // (matchingTrusts != null) <==> Found a prior version that was allowed to run.
            return (matchingTrusts != null);
        }

        private enum PromptingLevel
        {
            /// <devdoc>
            ///     <para>
            ///         In the cases the Trust Manager needs to prompt, it needs to show the blocking prompt
            ///     </para>
            /// </devdoc>
            Disabled = 0,
            /// <devdoc>
            ///     <para>
            ///         Prompting is allowed for strong named and authenticode signed application deployments
            ///     </para>
            /// </devdoc>
            Prompt = 1,
            /// <devdoc>
            ///     <para>
            ///         Prompting is allowed for authenticode signed application deployments only
            ///     </para>
            /// </devdoc>
            PromptOnlyForAuthenticode = 2
        }

        private enum PromptsAllowed
        {
            /// <devdoc>
            ///     <para>
            ///         The Trust Manager is allowed to show any prompt
            ///     </para>
            /// </devdoc>
            All = 0,
            /// <devdoc>
            ///     <para>
            ///         The Trust Manager is only allowed to show the blocking prompt
            ///     </para>
            /// </devdoc>
            BlockingOnly = 1,
            /// <devdoc>
            ///     <para>
            ///         The Trust Manager is not allowed to prompt at all
            ///     </para>
            /// </devdoc>
            None = 2
        }
    }

    [Serializable]
    internal class ApplicationTrustExtraInfo
    {
        private bool requestsShellIntegration = true;  // If manifest parsing fails, we assume shell integration is required.

        public ApplicationTrustExtraInfo()
        {
        }

        public bool RequestsShellIntegration
        {
            get
            {
                return this.requestsShellIntegration;
            }

            set
            {
                this.requestsShellIntegration = value;
            }
        }
    }

    internal class TrustManagerPromptUIThread {
        private string m_appName, m_defaultBrowserExePath, m_supportUrl, m_deploymentUrl, m_publisherName;
        private X509Certificate2 m_certificate;
        private TrustManagerPromptOptions m_options;
        private DialogResult m_ret = DialogResult.No;

        public TrustManagerPromptUIThread(string appName, string defaultBrowserExePath, string supportUrl, string deploymentUrl,
                                          string publisherName, X509Certificate2 certificate, TrustManagerPromptOptions options) {
            this.m_appName = appName;
            this.m_defaultBrowserExePath = defaultBrowserExePath;
            this.m_supportUrl = supportUrl;
            this.m_deploymentUrl = deploymentUrl;
            this.m_publisherName = publisherName;
            this.m_certificate = certificate;
            this.m_options = options;
        }

        public DialogResult ShowDialog() {
            System.Threading.Thread thread = new System.Threading.Thread(new System.Threading.ThreadStart(ShowDialogWork));
            thread.SetApartmentState(System.Threading.ApartmentState.STA);
            thread.Start();
            thread.Join();
            return m_ret;
        }

        [SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        private void ShowDialogWork()
        {
            try {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                using (TrustManagerPromptUI trustManagerDialog = new TrustManagerPromptUI(this.m_appName, this.m_defaultBrowserExePath,
                                                                                          this.m_supportUrl, this.m_deploymentUrl,
                                                                                          this.m_publisherName, this.m_certificate, this.m_options)) {
                    m_ret = trustManagerDialog.ShowDialog();
                }
            }
            catch {
            }
            finally {
                Application.ExitThread(); //explicitly call Dispose [DevDiv2 bug 184375, OleUnitinialize not being called]
            }
        }
    }

    internal class ParsedData
    {
        private bool requestsShellIntegration; /* == false by default. shellVisible attribute is optional */
        private string appName;
        private string appPublisher;
        private string supportUrl;
        private string authenticodedPublisher;
        private bool disallowTrustOverride;
        private X509Certificate2 certificate;
//        private string deploymentProviderCodebase;

        public ParsedData()
        {
        }

        public bool RequestsShellIntegration
        {
            get
            {
                return this.requestsShellIntegration;
            }
            set
            {
                this.requestsShellIntegration = value;
            }
        }

        public X509Certificate2 Certificate
        {
            get
            {
                return this.certificate;
            }
            set
            {
                this.certificate = value;
            }
        }

        public string AppName
        {
            get
            {
                return this.appName;
            }
            set
            {
                this.appName = value;
            }
        }

        public string AppPublisher
        {
            get
            {
                return this.appPublisher;
            }
            set
            {
                this.appPublisher = value;
            }
        }

        public string AuthenticodedPublisher
        {
            get
            {
                return this.authenticodedPublisher;
            }
            set
            {
                this.authenticodedPublisher = value;
            }
        }

        public bool UseManifestForTrust
        {
            get
            {
                return disallowTrustOverride;
            }

            set
            {
                disallowTrustOverride = value;
            }
        }

//        public string DeploymentProviderCodebase
//        {
//            get
//            {
//                return this.deploymentProviderCodebase;
//            }
//            set
//            {
//                this.deploymentProviderCodebase = value;
//            }
//        }

        public string SupportUrl
        {
            get
            {
                return this.supportUrl;
            }
            set
            {
                this.supportUrl = value;
            }
        }
    }

    internal class HostContextInternal
    {
        private bool ignorePersistedDecision;
        private bool noPrompt;
        private bool persist;
        private ApplicationIdentity previousAppId;

        public HostContextInternal(TrustManagerContext trustManagerContext)
        {
            if (trustManagerContext == null)
            {
                // Used in case DetermineApplicationTrust is not given a TrustManagerContext object.
                this.persist = true;
            }
            else
            {
                // ISSUE - fix this...
                // Note that exclusiveGrant is never set. It is read in CreateApplicationTrust however.
                this.ignorePersistedDecision = trustManagerContext.IgnorePersistedDecision;
                this.noPrompt = trustManagerContext.NoPrompt;
                this.persist = trustManagerContext.Persist;
                this.previousAppId = trustManagerContext.PreviousApplicationIdentity;
            }
        }

        public bool IgnorePersistedDecision
        { 
            get
            {
                return this.ignorePersistedDecision; 
            }
        }

        public bool NoPrompt
        { 
            get
            {
                return this.noPrompt; 
            }
        }

        public bool Persist
        { 
            get
            {
                return this.persist; 
            }
        }

        public ApplicationIdentity PreviousAppId
        {
            get
            {
                return this.previousAppId;
            }
        }
    }
}
