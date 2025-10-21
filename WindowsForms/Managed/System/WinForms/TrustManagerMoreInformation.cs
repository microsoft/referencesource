//------------------------------------------------------------------------------
// <copyright file="TrustManagerMoreInformation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using Microsoft.Win32;

namespace System.Security.Policy
{
    internal class TrustManagerMoreInformation : System.Windows.Forms.Form
    {
        private System.ComponentModel.IContainer components = null;

        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel;
        private System.Windows.Forms.Label lblPublisher;
        private System.Windows.Forms.Label lblPublisherContent;
        private System.Windows.Forms.Label lblMachineAccess;
        private System.Windows.Forms.Label lblMachineAccessContent;
        private System.Windows.Forms.Label lblInstallation;
        private System.Windows.Forms.Label lblInstallationContent;
        private System.Windows.Forms.Label lblLocation;
        private System.Windows.Forms.Label lblLocationContent;
        private System.Windows.Forms.PictureBox pictureBoxPublisher;
        private System.Windows.Forms.PictureBox pictureBoxMachineAccess;
        private System.Windows.Forms.PictureBox pictureBoxLocation;
        private System.Windows.Forms.PictureBox pictureBoxInstallation;
        private System.Windows.Forms.Button btnClose;

        internal TrustManagerMoreInformation(TrustManagerPromptOptions options, String publisherName)
        {
            InitializeComponent();
            this.Font = SystemFonts.MessageBoxFont;
            lblMachineAccess.Font = lblPublisher.Font = lblInstallation.Font = lblLocation.Font = new Font(lblMachineAccess.Font, FontStyle.Bold);
            FillContent(options, publisherName);

            if (AccessibilityImprovements.Level2)
            {
                // The form
                this.AccessibleName = this.Text;

                // The outer panel
                this.tableLayoutPanel.AccessibleName = string.Empty;

                // Publisher
                this.pictureBoxPublisher.AccessibleName = this.pictureBoxPublisher.AccessibleDescription;
                this.pictureBoxPublisher.AccessibleRole = AccessibleRole.Graphic;
                this.lblPublisher.AccessibleName = this.lblPublisher.Text;
                this.lblPublisherContent.AccessibleName = this.lblPublisherContent.Text;

                // Machine Access
                this.pictureBoxMachineAccess.AccessibleName = this.pictureBoxMachineAccess.AccessibleDescription;
                this.pictureBoxMachineAccess.AccessibleRole = AccessibleRole.Graphic;
                this.lblMachineAccess.AccessibleName = this.lblMachineAccess.Text;
                this.lblMachineAccessContent.AccessibleName = this.lblMachineAccessContent.Text;

                // Installation
                this.pictureBoxInstallation.AccessibleName = this.pictureBoxInstallation.AccessibleDescription;
                this.pictureBoxInstallation.AccessibleRole = AccessibleRole.Graphic;
                this.lblInstallation.AccessibleName = this.lblInstallation.Text;
                this.lblInstallationContent.AccessibleName = this.lblInstallationContent.Text;

                // Location
                this.pictureBoxLocation.AccessibleName = this.pictureBoxLocation.AccessibleDescription;
                this.pictureBoxLocation.AccessibleRole = AccessibleRole.Graphic;
                this.lblLocation.AccessibleName = this.lblLocation.Text;
                this.lblLocationContent.AccessibleName = this.lblLocationContent.Text;

                // Close button
                this.btnClose.AccessibleName = this.btnClose.Text;

                // Re-order controls to fix Narrator's Scan Mode navigation
                this.tableLayoutPanel.Controls.SetChildIndex(this.pictureBoxPublisher, 0);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblPublisher, 1);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblPublisherContent, 2);
                this.tableLayoutPanel.Controls.SetChildIndex(this.pictureBoxMachineAccess, 3);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblMachineAccess, 4);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblMachineAccessContent, 5);
                this.tableLayoutPanel.Controls.SetChildIndex(this.pictureBoxInstallation, 6);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblInstallation, 7);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblInstallationContent, 8);
                this.tableLayoutPanel.Controls.SetChildIndex(this.pictureBoxLocation, 9);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblLocation, 10);
                this.tableLayoutPanel.Controls.SetChildIndex(this.lblLocationContent, 11);
                this.tableLayoutPanel.Controls.SetChildIndex(this.btnClose, 12);
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }

        private void FillContent(TrustManagerPromptOptions options, String publisherName)
        {
            LoadWarningBitmap((publisherName == null) ? TrustManagerWarningLevel.Red : TrustManagerWarningLevel.Green, this.pictureBoxPublisher);
            LoadWarningBitmap(((options & (TrustManagerPromptOptions.RequiresPermissions | TrustManagerPromptOptions.WillHaveFullTrust)) != 0) ? TrustManagerWarningLevel.Red : TrustManagerWarningLevel.Green, this.pictureBoxMachineAccess);
            LoadWarningBitmap(((options & TrustManagerPromptOptions.AddsShortcut) != 0) ? TrustManagerWarningLevel.Yellow : TrustManagerWarningLevel.Green, this.pictureBoxInstallation);

            TrustManagerWarningLevel locationWarningLevel;
            if ((options & (TrustManagerPromptOptions.LocalNetworkSource | 
                            TrustManagerPromptOptions.LocalComputerSource | 
                            TrustManagerPromptOptions.TrustedSitesSource)) != 0)
            {
                locationWarningLevel = TrustManagerWarningLevel.Green;
            }
            else if ((options & TrustManagerPromptOptions.UntrustedSitesSource) != 0)
            {
                locationWarningLevel = TrustManagerWarningLevel.Red;
            }
            else
            {
                Debug.Assert((options & TrustManagerPromptOptions.InternetSource) != 0);
                locationWarningLevel = TrustManagerWarningLevel.Yellow;
            }
            LoadWarningBitmap(locationWarningLevel, this.pictureBoxLocation);

            if (publisherName == null)
            {
                this.lblPublisherContent.Text = SR.GetString(SR.TrustManagerMoreInfo_UnknownPublisher);
            }
            else
            {
                this.lblPublisherContent.Text = SR.GetString(SR.TrustManagerMoreInfo_KnownPublisher, publisherName);
            }

            if ((options & (TrustManagerPromptOptions.RequiresPermissions | TrustManagerPromptOptions.WillHaveFullTrust)) != 0)
            {
                this.lblMachineAccessContent.Text = SR.GetString(SR.TrustManagerMoreInfo_UnsafeAccess);
            }
            else
            {
                this.lblMachineAccessContent.Text = SR.GetString(SR.TrustManagerMoreInfo_SafeAccess);
            }

            if ((options & TrustManagerPromptOptions.AddsShortcut) != 0)
            {
                this.Text = SR.GetString(SR.TrustManagerMoreInfo_InstallTitle);
                this.lblInstallationContent.Text = SR.GetString(SR.TrustManagerMoreInfo_WithShortcut);
            }
            else
            {
                this.Text = SR.GetString(SR.TrustManagerMoreInfo_RunTitle);
                this.lblInstallationContent.Text = SR.GetString(SR.TrustManagerMoreInfo_WithoutShortcut);
            }

            string source;
            if ((options & TrustManagerPromptOptions.LocalNetworkSource) != 0)
            {
                source = SR.GetString(SR.TrustManagerMoreInfo_LocalNetworkSource);
            }
            else if ((options & TrustManagerPromptOptions.LocalComputerSource) != 0)
            {
                source = SR.GetString(SR.TrustManagerMoreInfo_LocalComputerSource);
            }
            else if ((options & TrustManagerPromptOptions.InternetSource) != 0)
            {
                source = SR.GetString(SR.TrustManagerMoreInfo_InternetSource);
            }
            else if ((options & TrustManagerPromptOptions.TrustedSitesSource) != 0)
            {
                source = SR.GetString(SR.TrustManagerMoreInfo_TrustedSitesSource);
            }
            else
            {
                Debug.Assert((options & TrustManagerPromptOptions.UntrustedSitesSource) != 0);
                source = SR.GetString(SR.TrustManagerMoreInfo_UntrustedSitesSource);
            }
            this.lblLocationContent.Text = SR.GetString(SR.TrustManagerMoreInfo_Location, source);
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrustManagerMoreInformation));
            this.tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.pictureBoxPublisher = new System.Windows.Forms.PictureBox();
            this.pictureBoxMachineAccess = new System.Windows.Forms.PictureBox();
            this.pictureBoxInstallation = new System.Windows.Forms.PictureBox();
            this.pictureBoxLocation = new System.Windows.Forms.PictureBox();
            this.lblPublisher = new System.Windows.Forms.Label();
            this.lblPublisherContent = new System.Windows.Forms.Label();
            this.lblMachineAccess = new System.Windows.Forms.Label();
            this.lblMachineAccessContent = new System.Windows.Forms.Label();
            this.lblInstallation = new System.Windows.Forms.Label();
            this.lblInstallationContent = new System.Windows.Forms.Label();
            this.lblLocation = new System.Windows.Forms.Label();
            this.lblLocationContent = new System.Windows.Forms.Label();
            this.btnClose = new System.Windows.Forms.Button();
            this.tableLayoutPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPublisher)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxMachineAccess)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxInstallation)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxLocation)).BeginInit();
            this.SuspendLayout();
            // 
            // tableLayoutPanel
            // 
            resources.ApplyResources(this.tableLayoutPanel, "tableLayoutPanel");
            this.tableLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 389F));
            this.tableLayoutPanel.Controls.Add(this.pictureBoxPublisher, 0, 0);
            this.tableLayoutPanel.Controls.Add(this.pictureBoxMachineAccess, 0, 2);
            this.tableLayoutPanel.Controls.Add(this.pictureBoxInstallation, 0, 4);
            this.tableLayoutPanel.Controls.Add(this.pictureBoxLocation, 0, 6);
            this.tableLayoutPanel.Controls.Add(this.lblPublisher, 1, 0);
            this.tableLayoutPanel.Controls.Add(this.lblPublisherContent, 1, 1);
            this.tableLayoutPanel.Controls.Add(this.lblMachineAccess, 1, 2);
            this.tableLayoutPanel.Controls.Add(this.lblMachineAccessContent, 1, 3);
            this.tableLayoutPanel.Controls.Add(this.lblInstallation, 1, 4);
            this.tableLayoutPanel.Controls.Add(this.lblInstallationContent, 1, 5);
            this.tableLayoutPanel.Controls.Add(this.lblLocation, 1, 6);
            this.tableLayoutPanel.Controls.Add(this.lblLocationContent, 1, 7);
            this.tableLayoutPanel.Controls.Add(this.btnClose, 1, 8);
            this.tableLayoutPanel.Margin = new System.Windows.Forms.Padding(12);
            this.tableLayoutPanel.Name = "tableLayoutPanel";
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // pictureBoxPublisher
            // 
            resources.ApplyResources(this.pictureBoxPublisher, "pictureBoxPublisher");
            this.pictureBoxPublisher.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.pictureBoxPublisher.Name = "pictureBoxPublisher";
            this.tableLayoutPanel.SetRowSpan(this.pictureBoxPublisher, 2);
            this.pictureBoxPublisher.TabStop = false;
            // 
            // pictureBoxMachineAccess
            // 
            resources.ApplyResources(this.pictureBoxMachineAccess, "pictureBoxMachineAccess");
            this.pictureBoxMachineAccess.Margin = new System.Windows.Forms.Padding(0, 10, 3, 0);
            this.pictureBoxMachineAccess.Name = "pictureBoxMachineAccess";
            this.tableLayoutPanel.SetRowSpan(this.pictureBoxMachineAccess, 2);
            this.pictureBoxMachineAccess.TabStop = false;
            // 
            // pictureBoxInstallation
            // 
            resources.ApplyResources(this.pictureBoxInstallation, "pictureBoxInstallation");
            this.pictureBoxInstallation.Margin = new System.Windows.Forms.Padding(0, 10, 3, 0);
            this.pictureBoxInstallation.Name = "pictureBoxInstallation";
            this.tableLayoutPanel.SetRowSpan(this.pictureBoxInstallation, 2);
            this.pictureBoxInstallation.TabStop = false;
            // 
            // pictureBoxLocation
            // 
            resources.ApplyResources(this.pictureBoxLocation, "pictureBoxLocation");
            this.pictureBoxLocation.Margin = new System.Windows.Forms.Padding(0, 10, 3, 0);
            this.pictureBoxLocation.Name = "pictureBoxLocation";
            this.tableLayoutPanel.SetRowSpan(this.pictureBoxLocation, 2);
            this.pictureBoxLocation.TabStop = false;
            // 
            // lblPublisher
            // 
            resources.ApplyResources(this.lblPublisher, "lblPublisher");
            this.lblPublisher.Margin = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.lblPublisher.Name = "lblPublisher";
            // 
            // lblPublisherContent
            // 
            resources.ApplyResources(this.lblPublisherContent, "lblPublisherContent");
            this.lblPublisherContent.Margin = new System.Windows.Forms.Padding(3, 0, 0, 10);
            this.lblPublisherContent.Name = "lblPublisherContent";
            // 
            // lblMachineAccess
            // 
            resources.ApplyResources(this.lblMachineAccess, "lblMachineAccess");
            this.lblMachineAccess.Margin = new System.Windows.Forms.Padding(3, 10, 0, 0);
            this.lblMachineAccess.Name = "lblMachineAccess";
            // 
            // lblMachineAccessContent
            // 
            resources.ApplyResources(this.lblMachineAccessContent, "lblMachineAccessContent");
            this.lblMachineAccessContent.Margin = new System.Windows.Forms.Padding(3, 0, 0, 10);
            this.lblMachineAccessContent.Name = "lblMachineAccessContent";
            // 
            // lblInstallation
            // 
            resources.ApplyResources(this.lblInstallation, "lblInstallation");
            this.lblInstallation.Margin = new System.Windows.Forms.Padding(3, 10, 0, 0);
            this.lblInstallation.Name = "lblInstallation";
            // 
            // lblInstallationContent
            // 
            resources.ApplyResources(this.lblInstallationContent, "lblInstallationContent");
            this.lblInstallationContent.Margin = new System.Windows.Forms.Padding(3, 0, 0, 10);
            this.lblInstallationContent.Name = "lblInstallationContent";
            // 
            // lblLocation
            // 
            resources.ApplyResources(this.lblLocation, "lblLocation");
            this.lblLocation.Margin = new System.Windows.Forms.Padding(3, 10, 0, 0);
            this.lblLocation.Name = "lblLocation";
            // 
            // lblLocationContent
            // 
            resources.ApplyResources(this.lblLocationContent, "lblLocationContent");
            this.lblLocationContent.Margin = new System.Windows.Forms.Padding(3, 0, 0, 10);
            this.lblLocationContent.Name = "lblLocationContent";
            // 
            // btnClose
            // 
            resources.ApplyResources(this.btnClose, "btnClose");
            this.btnClose.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Margin = new System.Windows.Forms.Padding(0, 10, 0, 0);
            this.btnClose.MinimumSize = new System.Drawing.Size(75, 23);
            this.btnClose.Name = "btnClose";
            this.btnClose.Padding = new System.Windows.Forms.Padding(10, 0, 10, 0);
            this.tableLayoutPanel.SetColumnSpan(this.btnClose, 2);
            // 
            // TrustManagerMoreInformation
            // 
            this.AcceptButton = this.btnClose;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.CancelButton = this.btnClose;
            this.Controls.Add(this.tableLayoutPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TrustManagerMoreInformation";
            this.tableLayoutPanel.ResumeLayout(false);
            this.tableLayoutPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPublisher)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxMachineAccess)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxInstallation)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxLocation)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        [
            SuppressMessage("Microsoft.Reliability", "CA2002:DoNotLockOnObjectsWithWeakIdentity")
        ]
        private static void LoadWarningBitmap(TrustManagerWarningLevel warningLevel, System.Windows.Forms.PictureBox pictureBox)
        {
            Bitmap bitmap;
            switch (warningLevel)
            {
                case TrustManagerWarningLevel.Green:
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        bitmap = QueryDPiMatchedSmallBitmap("TrustManagerOK.ico");
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerOKSm.bmp");
                    }
                    pictureBox.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_LowRisk), pictureBox.AccessibleDescription);
                    break;
                case TrustManagerWarningLevel.Yellow:
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        bitmap = QueryDPiMatchedSmallBitmap("TrustManagerWarning.ico");
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerWarningSm.bmp");
                    }
                    pictureBox.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_MediumRisk), pictureBox.AccessibleDescription);
                    break;
                default:
                    Debug.Assert(warningLevel == TrustManagerWarningLevel.Red);
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        bitmap = QueryDPiMatchedSmallBitmap("TrustManagerHighRisk.ico");
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerHighRiskSm.bmp");
                    }
                    pictureBox.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_HighRisk), pictureBox.AccessibleDescription);
                    break;
            }
            if (bitmap != null)
            {
                bitmap.MakeTransparent();
                pictureBox.Image = bitmap;
            }
        }

        internal static Bitmap QueryDPiMatchedSmallBitmap(string iconName)
        {
            var icon = new Icon(typeof(System.Windows.Forms.Form), iconName);

            // Querying 96 dpi icon. And smaller bit map is half that size.
            icon = new Icon(icon, icon.Width / 2, icon.Height / 2);
            
            if(icon != null)
            {
                return icon.ToBitmap();
            }

            return null;
        
        }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);
        }

        protected override void OnHandleDestroyed(EventArgs e)
        {
            SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);
            base.OnHandleDestroyed(e);
        }

        private void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e)
        {
            if (e.Category == UserPreferenceCategory.Window)
            {
                this.Font = SystemFonts.MessageBoxFont;
                this.lblLocation.Font =
                this.lblInstallation.Font =
                this.lblMachineAccess.Font =
                this.lblPublisher.Font = new Font(this.Font, FontStyle.Bold);
            }
            Invalidate(); // Workaround a bug where the form's background does not repaint properly
        }
    }
}
