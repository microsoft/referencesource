//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Drawing;
    using System.Data;
    using System.Text;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Windows.Forms;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Security.Principal;

    public partial class WsatControl : UserControl
    {
        bool changed = false;
        string machineName;
        string virtualServer;
        IntPtr propPage = IntPtr.Zero;
        IntPtr propSheetDialog = IntPtr.Zero;
        WsatPropertySheet propertySheetPage;
        WsatConfiguration previousConfig, config;
        MsdtcWrapper msdtc;
        bool oldNetworkSupportEnabledValue = false;

        [SecurityCritical]
        public WsatControl(IntPtr propPageParam, IntPtr propSheetDialogParam, WsatPropertySheet propertySheetPage)
        {
            if (propertySheetPage == null)
            {
                throw new ArgumentNullException("propSheetPage");
            }

            // The order of the following calls is important!
            InitializeComponent();
            this.propPage = propPageParam;
            this.propSheetDialog = propSheetDialogParam;
            this.propertySheetPage = propertySheetPage;
            this.machineName = this.propertySheetPage.MachineName;
            this.virtualServer = this.propertySheetPage.VirtualServer;

            InitializeConfig();

            if (config != null)
            {
                this.oldNetworkSupportEnabledValue = this.config.TransactionBridgeEnabled || this.config.TransactionBridge30Enabled;
                propertySheetPage.ShowPropertyPage += new ShowWindowEventHander(OnShowContainerPropertyPage);
                propertySheetPage.BeforeApplyChanges += new BeforeApplyChangesEventHandler(OnBeforeApplyChanges);
                propertySheetPage.ApplyChanges += new ApplyChangesEventHandler(OnApplyChanges);
            }
            Application.EnableVisualStyles();
        }

        [SecurityCritical]
        void InitializeConfig()
        {
            try
            {
                previousConfig = new WsatConfiguration(machineName, virtualServer, null, false);
                previousConfig.LoadFromRegistry();

                config = new WsatConfiguration(machineName, virtualServer, previousConfig, false);
                msdtc = config.GetMsdtcWrapper();
                ConfigurationToUI();
            }
            catch (WsatAdminException e)
            {
                HandleException(e);
            }
        }
        
        void CheckNetworkDtcAccessStatus()
        {
            Utilities.Log("CheckNetworkDtcAccessStatus");

            bool networkTransactionEnabled;
            try
            {
                networkTransactionEnabled = msdtc.GetNetworkTransactionAccess();
            }
            catch (WsatAdminException e)  
            {
                HandleException(e);
                networkTransactionEnabled = oldNetworkSupportEnabledValue;
            }

            if (!networkTransactionEnabled)
            {
                // If Msdtc network access is disabled, we'll disable the NetworkSupport checkbox anyway
                Utilities.Log("CheckNetworkDtcAccessStatus: msdtc.GetNetworkTransactionAccess = false");
                this.checkBoxEnableNetworkSupport.Checked = false;
                this.checkBoxEnableNetworkSupport.Enabled = false;
            }
            else
            {
                // Otherwise, we can't simply check it - we should use the original state saved before
                // navigating away from the tab
                Utilities.Log("CheckNetworkDtcAccessStatus: msdtc.GetNetworkTransactionAccess = true");
                this.checkBoxEnableNetworkSupport.Enabled = true;
                this.checkBoxEnableNetworkSupport.Checked = this.oldNetworkSupportEnabledValue;
            }
        }

        internal bool OnBeforeApplyChanges()
        {
            try
            {
                if (changed && Enabled)
                {
                    UIToConfiguration();
                    config.ValidateThrow();
                }

                return true;
            }
            catch (WsatAdminException e)
            {
                HandleException(e);
                return false;
            }
        }

        internal bool OnApplyChanges()
        {
            bool succeeded = true;

            try
            {
                if (changed && Enabled)
                {
                    DialogResult restart = MessageBox.Show(
                        SR.GetString(SR.PromptWhetherToRestartDTCMessage),
                        SR.GetString(SR.WSATUITitle),
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Question,
                        MessageBoxDefaultButton.Button1,
                        (MessageBoxOptions)0);

                    config.Save(restart == DialogResult.Yes);

                    if (restart == DialogResult.Yes)
                    {
                        MessageBox.Show(SR.GetString(SR.PromptMSDTCRestartedMessage),
                                        SR.GetString(SR.WSATUITitle),
                                        MessageBoxButtons.OK,
                                        MessageBoxIcon.Information,
                                        MessageBoxDefaultButton.Button1,
                                        (MessageBoxOptions)0);
                    }

                    succeeded = true;
                    changed = false;
                }
            }
            catch (WsatAdminException e)
            {
                succeeded = false;
                HandleException(e);
            }

            return succeeded;
        }

        static bool IsLocalAdmin()
        {
            WindowsPrincipal principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }

        internal void OnShowContainerPropertyPage(bool show)
        {
            if (show)
            {
                if (Utilities.IsLocalMachineName(machineName) && !IsLocalAdmin())
                {
                    this.Enabled = false; 
                }
                else
                {
                    CheckNetworkDtcAccessStatus();
                }
            }
            else
            {
                this.oldNetworkSupportEnabledValue = this.checkBoxEnableNetworkSupport.Checked;
            }
        }

        string GetDisplayStringForCert(X509Certificate2 cert)
        {
            if (cert == null)
            {
                return SR.GetString(SR.WSATControlEndpointCertNotSelected);
            }
            string ret = cert.SubjectName.Name;
            if (string.IsNullOrEmpty(ret))
            {
                ret = cert.Thumbprint;
            }
            return ret;
        }

        void ComponentChanged()
        {
            changed = true;
            SafeNativeMethods.SendMessage(propSheetDialog, PSM.CHANGED, propPage, IntPtr.Zero);
        }

        void ConfigurationToUI()
        {
            this.checkBoxEnableNetworkSupport.Checked = this.config.TransactionBridgeEnabled || this.config.TransactionBridge30Enabled;
            this.textBoxHttpsPort.Text = this.config.HttpsPort.ToString(CultureInfo.InvariantCulture);
            this.textBoxEndpointCert.Text = GetDisplayStringForCert(config.X509Certificate);
            this.textBoxDefaultTimeout.Text = this.config.DefaultTimeout.ToString(CultureInfo.InvariantCulture);
            this.textBoxMaxTimeout.Text = this.config.MaxTimeout.ToString(CultureInfo.InvariantCulture);
            checkBoxEnableNetworkSupport_CheckedChanged(null, null);
        }

        void UIToConfiguration()
        {
            ushort shortTemp = 0;
            
            // Network Support
            this.config.TransactionBridgeEnabled = this.checkBoxEnableNetworkSupport.Checked;

            // Max Timeout
            if (UInt16.TryParse(this.textBoxMaxTimeout.Text, out shortTemp))
            {
                this.config.MaxTimeout = shortTemp;
            }
            else
            {
                this.textBoxMaxTimeout.Focus();
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_MAXTIMEOUT_ARGUMENT,
                                                SR.GetString(SR.ErrorMaximumTimeoutRange));
            }

            // The following perperties only matter when network support is enabled
            if (this.config.TransactionBridgeEnabled)
            {
                // HTTPS port
                if (UInt16.TryParse(this.textBoxHttpsPort.Text, out shortTemp))
                {
                    this.config.HttpsPort = shortTemp;
                }
                else
                {
                    this.textBoxHttpsPort.Focus();
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_HTTPS_PORT,
                                                    SR.GetString(SR.ErrorHttpsPortRange));
                }

                // Default Timeout
                if (UInt16.TryParse(this.textBoxDefaultTimeout.Text, out shortTemp))
                {
                    this.config.DefaultTimeout = shortTemp;
                }
                else
                {
                    this.textBoxDefaultTimeout.Focus();
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_DEF_TIMEOUT,
                                                    SR.GetString(SR.ErrorDefaultTimeoutRange));
                }

                // The certs and Kerberos ACLs should have been validated and attached to configuration
                // when users perform the corresponding selection steps
            }
        }

        void checkBoxEnableNetworkSupport_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBoxEnableNetworkSupport.Enabled)
            {
                try
                {
                    QfeChecker.CheckQfe();
                }
                catch (WsatAdminException ex)
                {
                    HandleException(ex);
                }
            }
            groupBoxNetwork.Enabled = groupBoxTracing.Enabled = groupBoxTimeouts.Enabled = checkBoxEnableNetworkSupport.Checked;
            ComponentChanged();
        }

        void textBoxHttpsPort_TextChanged(object sender, EventArgs e)
        {
            ComponentChanged();
        }

        void textBoxDefaultTimeout_TextChanged(object sender, EventArgs e)
        {
            ComponentChanged();
        }

        void textBoxMaxTimeout_TextChanged(object sender, EventArgs e)
        {
            ComponentChanged();
        }

        void buttonSelectEndpointCert_Click(object sender, EventArgs e)
        {
            try
            {
                SafeCertificateStore storeHandle = CertificateManager.GetCertificateStorePointer(machineName);

                // do not display the Location column on the CryptUIDlgSelectCertificateFromStore
#pragma warning suppress 56523
                SafeCertificateContext certContext = SafeNativeMethods.CryptUIDlgSelectCertificateFromStore(
                        storeHandle,
                        propPage,
                        SR.GetString(SR.SSLBindingTitle),
                        SR.GetString(SR.SSLBindingMessage),
                        SafeNativeMethods.CRYPTUI_SELECT_LOCATION_COLUMN,
                        0, IntPtr.Zero);

                if (!certContext.IsInvalid)
                {
                    config.X509Certificate = certContext.GetNewX509Certificate();
                    textBoxEndpointCert.Text = GetDisplayStringForCert(config.X509Certificate);
                    ComponentChanged();
                }

                certContext.Close();
                storeHandle.Close();
            }
            catch (WsatAdminException ex)
            {
                HandleException(ex);
            }
        }

        void buttonSelectAuthorizedAccounts_Click(object sender, EventArgs e)
        {
            // popup a new ACL window, providing the WSATSecurityModel
            // and a handle for the parent window (this)
            if (ACLWrapper.EditACLSecurity(new WsatSecurityModel(Utilities.LocalHostName, config), this.Handle))
            {
                ComponentChanged();
            }
        }

        void buttonSelectAuthorizedCerts_Click(object sender, EventArgs e)
        {
            try
            {
                SafeCertificateStore storeHandle = CertificateManager.GetCertificateStorePointer(machineName);

                SafeCertificateContext prev = new SafeCertificateContext();
                SafeCertificateContext crt = new SafeCertificateContext();

                X509Certificate2Collection certificateCollection = new X509Certificate2Collection();
                do
                {
#pragma warning suppress 56523
                    crt = SafeNativeMethods.CertFindCertificateInStore(
                        storeHandle,
                        SafeNativeMethods.X509_ASN_ENCODING,
                        0,
                        SafeNativeMethods.CERT_FIND_ANY,
                        IntPtr.Zero,
                        prev);
                    prev = crt;
                    if (!crt.IsInvalid)
                    {
                        certificateCollection.Add(crt.GetNewX509Certificate());
                    }
                } while (!crt.IsInvalid);

                storeHandle.Close();
                prev.Close();
                crt.Close();

                AcceptedCertificatesForm dlg = new AcceptedCertificatesForm(certificateCollection, config.X509GlobalAcl);
                DialogResult dialogResult = dlg.ShowDialog(this);

                if (dialogResult == DialogResult.OK)
                {
                    this.config.X509GlobalAcl = dlg.AllowedCertificates;
                    if (this.config.X509GlobalAcl.Length > 0)
                    {
                        Utilities.Log("selected allowed client cert [0]: " + this.config.X509GlobalAcl[0]);
                    }
                    ComponentChanged();
                }
            }
            catch (WsatAdminException ex)
            {
                HandleException(ex);
            }
        }

        void buttonTracingOptions_Click(object sender, EventArgs e)
        {
            TraceOptionsForm form = new TraceOptionsForm(this.config);
            DialogResult result = form.ShowDialog();
            if (result == DialogResult.OK)
            {
                ComponentChanged();
            }
        }

        void HandleException(WsatAdminException ex)
        {
            if (ex.ErrorCode == WsatAdminErrorCode.REGISTRY_ACCESS
                || ex.ErrorCode == WsatAdminErrorCode.WRONG_WCF_INSTALLED 
                || ex.ErrorCode == WsatAdminErrorCode.CANNOT_ENABLE_NETWORK_SUPPORT_WHEN_QFE_IS_NOT_INSTALLED)
            {
                MessageBox.Show(ex.Message, SR.GetString(SR.WSATUITitle),
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Information,
                            MessageBoxDefaultButton.Button1,
                            (MessageBoxOptions)0);
                if (this.Enabled)
                {
                    this.Enabled = false;
                }
            }
            else
            {
                MessageBox.Show(ex.Message, SR.GetString(SR.ErrorMessageBoxTitle),
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error,
                                MessageBoxDefaultButton.Button1,
                                (MessageBoxOptions)0);
            }
        }
    }
}
