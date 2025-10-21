//------------------------------------------------------------------------------
// <copyright file="TrustManagerPromptUI.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Security.Cryptography.X509Certificates;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace System.Security.Policy
{
    [Flags]
    internal enum TrustManagerPromptOptions
    {
        None = 0x0000,
        StopApp = 0x0001,
        RequiresPermissions = 0x0002,
        WillHaveFullTrust = 0x0004,
        AddsShortcut = 0x0008,
        LocalNetworkSource = 0x0010,
        LocalComputerSource = 0x0020,
        InternetSource = 0x0040,
        TrustedSitesSource = 0x0080,
        UntrustedSitesSource = 0x0100
    }

    internal enum TrustManagerWarningLevel
    {
        Green = 1,
        Yellow = 2,
        Red = 3
    }


    internal class TrustManagerPromptUI : System.Windows.Forms.Form
    {
        private System.ComponentModel.IContainer components = null;

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnInstall;
        private System.Windows.Forms.Label lblFrom;
        private System.Windows.Forms.Label lblName;
        private System.Windows.Forms.Label lblPublisher;
        private System.Windows.Forms.Label lblQuestion;
        private System.Windows.Forms.LinkLabel linkLblFromUrl;
        private System.Windows.Forms.LinkLabel linkLblMoreInformation;
        private System.Windows.Forms.LinkLabel linkLblName;
        private System.Windows.Forms.LinkLabel linkLblPublisher;
        private System.Windows.Forms.PictureBox pictureBoxQuestion;
        private System.Windows.Forms.PictureBox pictureBoxWarning;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelButtons;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelInfo;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelOuter;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelQuestion;
        private System.Windows.Forms.ToolTip toolTipFromUrl;

        private string m_appName;
        private string m_defaultBrowserExePath;
        private string m_supportUrl;
        private string m_deploymentUrl;
        private string m_publisherName;
        private X509Certificate2 m_certificate;
        private TrustManagerPromptOptions m_options;
        private Label lineLabel;
        private TableLayoutPanel warningTextTableLayoutPanel;
        private bool controlToolTip;

        internal TrustManagerPromptUI(string appName,
                                      string defaultBrowserExePath,
                                      string supportUrl,
                                      string deploymentUrl,
                                      string publisherName,
                                      X509Certificate2 certificate,
                                      TrustManagerPromptOptions options)
        {
            this.m_appName = appName;
            this.m_defaultBrowserExePath = defaultBrowserExePath;
            this.m_supportUrl = supportUrl;
            this.m_deploymentUrl = deploymentUrl;
            this.m_publisherName = publisherName;
            this.m_certificate = certificate;
            this.m_options = options;

            InitializeComponent();
            LoadResources();

            if (AccessibilityImprovements.Level2)
            {
                // The outer pane
                this.tableLayoutPanelOuter.AccessibleName = string.Empty;

                // The "Do you want to install this application?" question pane and inner controls
                this.tableLayoutPanelQuestion.AccessibleName = string.Empty;
                this.lblQuestion.AccessibleName = this.lblQuestion.Text;
                this.pictureBoxQuestion.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_GlobeIcon);
                this.pictureBoxQuestion.AccessibleRole = AccessibleRole.Graphic;

                // The application information pane and inner controls
                this.tableLayoutPanelInfo.AccessibleName = string.Empty;
                this.lblName.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_Name);
                this.linkLblName.AccessibleName = this.linkLblName.Text;
                this.lblFrom.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_From);
                this.linkLblFromUrl.AccessibleName = this.linkLblFromUrl.Text;
                this.lblPublisher.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_Publisher);
                this.linkLblPublisher.AccessibleName = this.linkLblPublisher.Text;

                // The buttons pane and inner controls
                this.tableLayoutPanelButtons.AccessibleName = string.Empty;
                this.btnInstall.AccessibleName = StripOutAccelerator(this.btnInstall.Text);
                this.btnCancel.AccessibleName = StripOutAccelerator(this.btnCancel.Text);

                // The security summary pane and inner controls
                this.warningTextTableLayoutPanel.AccessibleName = string.Empty;
                this.pictureBoxWarning.AccessibleName = this.pictureBoxWarning.AccessibleDescription;
                this.pictureBoxWarning.AccessibleRole = AccessibleRole.Graphic;
                this.linkLblMoreInformation.AccessibleName = this.linkLblMoreInformation.Text;

                // The line separator
                this.lineLabel.AccessibleName = string.Empty;

                // Assigning accessible role to be in consistant with other dialogs.
                this.lineLabel.AccessibleRole = AccessibleRole.Separator;

                // Re-order panes to fix Narrator's Scan Mode navigation
                this.tableLayoutPanelOuter.Controls.SetChildIndex(this.tableLayoutPanelQuestion, 0);
                this.tableLayoutPanelOuter.Controls.SetChildIndex(this.tableLayoutPanelInfo, 1);
                this.tableLayoutPanelOuter.Controls.SetChildIndex(this.tableLayoutPanelButtons, 2);
                this.tableLayoutPanelOuter.Controls.SetChildIndex(this.warningTextTableLayoutPanel, 3);
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
                controlToolTip = false;
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrustManagerPromptUI));
            this.tableLayoutPanelOuter = new System.Windows.Forms.TableLayoutPanel();
            this.warningTextTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.pictureBoxWarning = new System.Windows.Forms.PictureBox();
            this.linkLblMoreInformation = new System.Windows.Forms.LinkLabel();
            this.tableLayoutPanelQuestion = new System.Windows.Forms.TableLayoutPanel();
            this.lblQuestion = new System.Windows.Forms.Label();
            this.pictureBoxQuestion = new System.Windows.Forms.PictureBox();
            this.tableLayoutPanelButtons = new System.Windows.Forms.TableLayoutPanel();
            this.btnInstall = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.tableLayoutPanelInfo = new System.Windows.Forms.TableLayoutPanel();
            this.lblName = new System.Windows.Forms.Label();            
            this.lblFrom = new System.Windows.Forms.Label();
            this.lblPublisher = new System.Windows.Forms.Label();
            this.linkLblName = new System.Windows.Forms.LinkLabel();
            this.linkLblFromUrl = new System.Windows.Forms.LinkLabel();
            this.linkLblPublisher = new System.Windows.Forms.LinkLabel();
            this.lineLabel = new System.Windows.Forms.Label();
            this.toolTipFromUrl = new System.Windows.Forms.ToolTip(this.components);
            this.tableLayoutPanelOuter.SuspendLayout();
            this.warningTextTableLayoutPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxWarning)).BeginInit();
            this.tableLayoutPanelQuestion.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxQuestion)).BeginInit();
            this.tableLayoutPanelButtons.SuspendLayout();
            this.tableLayoutPanelInfo.SuspendLayout();
            this.SuspendLayout();
            // 
            // tableLayoutPanelOuter
            // 
            resources.ApplyResources(this.tableLayoutPanelOuter, "tableLayoutPanelOuter");
            this.tableLayoutPanelOuter.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanelOuter.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 510F));
            this.tableLayoutPanelOuter.Controls.Add(this.warningTextTableLayoutPanel, 0, 4);
            this.tableLayoutPanelOuter.Controls.Add(this.tableLayoutPanelQuestion, 0, 0);
            this.tableLayoutPanelOuter.Controls.Add(this.tableLayoutPanelButtons, 0, 2);
            this.tableLayoutPanelOuter.Controls.Add(this.tableLayoutPanelInfo, 0, 1);
            this.tableLayoutPanelOuter.Controls.Add(this.lineLabel, 0, 3);
            this.tableLayoutPanelOuter.Margin = new System.Windows.Forms.Padding(0, 0, 0, 12);
            this.tableLayoutPanelOuter.Name = "tableLayoutPanelOuter";
            this.tableLayoutPanelOuter.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelOuter.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelOuter.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelOuter.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelOuter.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // warningTextTableLayoutPanel
            // 
            resources.ApplyResources(this.warningTextTableLayoutPanel, "warningTextTableLayoutPanel");
            this.warningTextTableLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.warningTextTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.warningTextTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.warningTextTableLayoutPanel.Controls.Add(this.pictureBoxWarning, 0, 0);
            this.warningTextTableLayoutPanel.Controls.Add(this.linkLblMoreInformation, 1, 0);
            this.warningTextTableLayoutPanel.Margin = new System.Windows.Forms.Padding(12, 6, 0, 0);
            this.warningTextTableLayoutPanel.Name = "warningTextTableLayoutPanel";
            this.warningTextTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // pictureBoxWarning
            // 
            resources.ApplyResources(this.pictureBoxWarning, "pictureBoxWarning");
            this.pictureBoxWarning.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.pictureBoxWarning.Name = "pictureBoxWarning";
            this.pictureBoxWarning.TabStop = false;
            // 
            // linkLblMoreInformation
            // 
            resources.ApplyResources(this.linkLblMoreInformation, "linkLblMoreInformation");
            this.linkLblMoreInformation.Margin = new System.Windows.Forms.Padding(3, 0, 3, 0);
            this.linkLblMoreInformation.Name = "linkLblMoreInformation";
            this.linkLblMoreInformation.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.TrustManagerPromptUI_ShowMoreInformation);
            // 
            // tableLayoutPanelQuestion
            // 
            resources.ApplyResources(this.tableLayoutPanelQuestion, "tableLayoutPanelQuestion");
            this.tableLayoutPanelQuestion.BackColor = System.Drawing.SystemColors.Window;
            this.tableLayoutPanelQuestion.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanelQuestion.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 58F));
            this.tableLayoutPanelQuestion.Controls.Add(this.lblQuestion, 0, 0);
            this.tableLayoutPanelQuestion.Controls.Add(this.pictureBoxQuestion, 1, 0);
            this.tableLayoutPanelQuestion.Margin = new System.Windows.Forms.Padding(0);
            this.tableLayoutPanelQuestion.Name = "tableLayoutPanelQuestion";
            this.tableLayoutPanelQuestion.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // lblQuestion
            // 
            resources.ApplyResources(this.lblQuestion, "lblQuestion");
            this.lblQuestion.Margin = new System.Windows.Forms.Padding(12, 12, 12, 0);
            this.lblQuestion.Name = "lblQuestion";
            // 
            // pictureBoxQuestion
            // 
            resources.ApplyResources(this.pictureBoxQuestion, "pictureBoxQuestion");
            this.pictureBoxQuestion.Margin = new System.Windows.Forms.Padding(0);
            this.pictureBoxQuestion.Name = "pictureBoxQuestion";
            this.pictureBoxQuestion.TabStop = false;
            // 
            // tableLayoutPanelButtons
            // 
            resources.ApplyResources(this.tableLayoutPanelButtons, "tableLayoutPanelButtons");
            this.tableLayoutPanelButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanelButtons.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanelButtons.Controls.Add(this.btnInstall, 0, 0);
            this.tableLayoutPanelButtons.Controls.Add(this.btnCancel, 1, 0);
            this.tableLayoutPanelButtons.Margin = new System.Windows.Forms.Padding(0, 6, 12, 12);
            this.tableLayoutPanelButtons.Name = "tableLayoutPanelButtons";
            this.tableLayoutPanelButtons.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // btnInstall
            // 
            resources.ApplyResources(this.btnInstall, "btnInstall");
            this.btnInstall.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btnInstall.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.btnInstall.MinimumSize = new System.Drawing.Size(75, 23);
            this.btnInstall.Name = "btnInstall";
            this.btnInstall.Padding = new System.Windows.Forms.Padding(10, 0, 10, 0);
            // 
            // btnCancel
            // 
            resources.ApplyResources(this.btnCancel, "btnCancel");
            this.btnCancel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Margin = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.btnCancel.MinimumSize = new System.Drawing.Size(75, 23);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Padding = new System.Windows.Forms.Padding(10, 0, 10, 0);
            // 
            // tableLayoutPanelInfo
            // 
            resources.ApplyResources(this.tableLayoutPanelInfo, "tableLayoutPanelInfo");
            this.tableLayoutPanelInfo.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanelInfo.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanelInfo.Controls.Add(this.lblName, 0, 0);
            this.tableLayoutPanelInfo.Controls.Add(this.linkLblName, 0, 1);
            this.tableLayoutPanelInfo.Controls.Add(this.lblFrom, 0, 2);
            this.tableLayoutPanelInfo.Controls.Add(this.linkLblFromUrl, 0, 3);
            this.tableLayoutPanelInfo.Controls.Add(this.lblPublisher, 0, 4);
            this.tableLayoutPanelInfo.Controls.Add(this.linkLblPublisher, 0, 5);
            this.tableLayoutPanelInfo.Margin = new System.Windows.Forms.Padding(30, 22, 12, 3);
            this.tableLayoutPanelInfo.Name = "tableLayoutPanelInfo";
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelInfo.RowStyles.Add(new System.Windows.Forms.RowStyle());
            // 
            // lblName
            // 
            resources.ApplyResources(this.lblName, "lblName");
            this.lblName.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.lblName.Name = "lblName";
            // 
            // lblFrom
            // 
            resources.ApplyResources(this.lblFrom, "lblFrom");
            this.lblFrom.Margin = new System.Windows.Forms.Padding(0, 8, 3, 0);
            this.lblFrom.Name = "lblFrom";
            // 
            // lblPublisher
            // 
            resources.ApplyResources(this.lblPublisher, "lblPublisher");
            this.lblPublisher.Margin = new System.Windows.Forms.Padding(0, 8, 3, 0);
            this.lblPublisher.Name = "lblPublisher";
            // 
            // linkLblName
            // 
            resources.ApplyResources(this.linkLblName, "linkLblName");
            this.linkLblName.AutoEllipsis = true;
            this.linkLblName.Margin = new System.Windows.Forms.Padding(3, 0, 3, 8);
            this.linkLblName.Name = "linkLblName";
            this.linkLblName.TabStop = true;
            this.linkLblName.UseMnemonic = false;
            this.linkLblName.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.TrustManagerPromptUI_ShowSupportPage);
            // 
            // linkLblFromUrl
            // 
            resources.ApplyResources(this.linkLblFromUrl, "linkLblFromUrl");
            this.linkLblFromUrl.AutoEllipsis = true;
            this.linkLblFromUrl.Margin = new System.Windows.Forms.Padding(3, 0, 3, 8);
            this.linkLblFromUrl.Name = "linkLblFromUrl";
            this.linkLblFromUrl.TabStop = true;
            this.linkLblFromUrl.UseMnemonic = false;
            this.linkLblFromUrl.MouseEnter += new System.EventHandler(this.linkLblFromUrl_MouseEnter);
            this.linkLblFromUrl.MouseLeave += new System.EventHandler(this.linkLblFromUrl_MouseLeave);
            // 
            // linkLblPublisher
            // 
            resources.ApplyResources(this.linkLblPublisher, "linkLblPublisher");
            this.linkLblPublisher.AutoEllipsis = true;
            this.linkLblPublisher.Margin = new System.Windows.Forms.Padding(3, 0, 3, 0);
            this.linkLblPublisher.Name = "linkLblPublisher";
            this.linkLblPublisher.TabStop = true;
            this.linkLblPublisher.UseMnemonic = false;
            this.linkLblPublisher.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.TrustManagerPromptUI_ShowPublisherCertificate);
            // 
            // lineLabel
            // 
            resources.ApplyResources(this.lineLabel, "lineLabel");
            this.lineLabel.BackColor = System.Drawing.SystemColors.ControlDark;
            this.lineLabel.Margin = new System.Windows.Forms.Padding(0);
            this.lineLabel.Name = "lineLabel";
            // 
            // TrustManagerPromptUI
            //
            this.AcceptButton = this.btnCancel;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.CancelButton = this.btnCancel;
            this.Controls.Add(this.tableLayoutPanelOuter);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TrustManagerPromptUI";

            // Bug# 398538 - Explicitly setting RighToLeft property for RightToLeft cultures at runtime for TrustManager dialog.
            if (SR.GetString(SR.RTL) != "RTL_False")
            {
                this.RightToLeft = RightToLeft.Yes;
                this.RightToLeftLayout = true;
            }

            this.VisibleChanged += new System.EventHandler(this.TrustManagerPromptUI_VisibleChanged);
            this.Load += new System.EventHandler(this.TrustManagerPromptUI_Load);
            this.tableLayoutPanelOuter.ResumeLayout(false);
            this.tableLayoutPanelOuter.PerformLayout();
            this.warningTextTableLayoutPanel.ResumeLayout(false);
            this.warningTextTableLayoutPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxWarning)).EndInit();
            this.tableLayoutPanelQuestion.ResumeLayout(false);
            this.tableLayoutPanelQuestion.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxQuestion)).EndInit();
            this.tableLayoutPanelButtons.ResumeLayout(false);
            this.tableLayoutPanelButtons.PerformLayout();
            this.tableLayoutPanelInfo.ResumeLayout(false);
            this.tableLayoutPanelInfo.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();
        }


        [
            SuppressMessage("Microsoft.Reliability", "CA2002:DoNotLockOnObjectsWithWeakIdentity")
        ]
        private void LoadGlobeBitmap()
        {
            Bitmap bitmap;
            lock (typeof(System.Windows.Forms.Form))
            {
                if (!LocalAppContextSwitches.UseLegacyImages)
                {
                    var globeIcon = new Icon(typeof(System.Windows.Forms.Form), "TrustManagerGlobe.ico");
                    bitmap = globeIcon.ToBitmap();                    
                }
                else
                {
                    Bitmap globeBmp = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerGlobe.bmp");
                    this.ScaleBitmapLogicalToDevice(ref globeBmp);
                    bitmap = globeBmp;
                }
            }
            if (bitmap != null)
            {
                bitmap.MakeTransparent();
                this.pictureBoxQuestion.Image = bitmap;
            }
        }

        [
            SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters") // OK to use "  "
        ]
        private void LoadResources()
        {
            SuspendAllLayout(this);
            
            LoadGlobeBitmap();

            UpdateFonts();

            // Pick the buttons
            if ((this.m_options & TrustManagerPromptOptions.StopApp) != 0)
            {
                this.btnInstall.Visible = false;
                this.btnCancel.Text = SR.GetString(SR.TrustManagerPromptUI_Close);
                this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.OK;
            }
            else
            {
                if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.btnCancel.Text = SR.GetString(SR.TrustManagerPromptUI_DoNotInstall);
                }
                else
                {
                    this.btnCancel.Text = SR.GetString(SR.TrustManagerPromptUI_DoNotRun);
                }
                this.btnInstall.DialogResult = System.Windows.Forms.DialogResult.OK;
                this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            }

            // Set the links
            this.linkLblName.Links.Clear();
            this.linkLblPublisher.Links.Clear();
            this.linkLblFromUrl.Links.Clear();
            this.linkLblMoreInformation.Links.Clear();

            this.linkLblName.Text = this.m_appName;
            if (this.m_defaultBrowserExePath != null && 
                this.m_certificate != null && 
                this.m_supportUrl != null && 
                this.m_supportUrl.Length > 0)
            {
                this.linkLblName.Links.Add(0, this.m_appName.Length, this.m_supportUrl);
            }
            if (this.linkLblName.Links.Count == 0)
            {
                // Since there is no link to activate, remove the potential accelerator
                this.lblName.Text = StripOutAccelerator(this.lblName.Text);
            }

            this.linkLblFromUrl.Text = this.m_deploymentUrl;

            if (this.m_publisherName == null)
            {
                this.linkLblPublisher.Text = SR.GetString(SR.TrustManagerPromptUI_UnknownPublisher);
                if (this.m_certificate != null)
                {
                    this.linkLblPublisher.Links.Add(0, this.linkLblPublisher.Text.Length);
                }
            }
            else
            {
                this.linkLblPublisher.Text = this.m_publisherName;
                Debug.Assert(this.m_certificate != null);
                if (this.m_publisherName.Length > 0)
                {
                    this.linkLblPublisher.Links.Add(0, this.m_publisherName.Length);
                }
            }
            if (this.linkLblPublisher.Links.Count == 0)
            {
                // Since there is no link to activate, remove the potential accelerator
                this.lblPublisher.Text = StripOutAccelerator(this.lblPublisher.Text);
            }

            // Choose a title
            if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
            {
                this.Text = SR.GetString(SR.TrustManagerPromptUI_InstallTitle);
            }
            else
            {
                this.Text = SR.GetString(SR.TrustManagerPromptUI_RunTitle);
                this.btnInstall.Text = SR.GetString(SR.TrustManagerPromptUI_Run);
            }

            // Choose a question
            if ((this.m_options & TrustManagerPromptOptions.StopApp) != 0)
            {
                this.lblQuestion.Text = SR.GetString(SR.TrustManagerPromptUI_BlockedApp);
            }
            else if (this.m_publisherName == null)
            {
                if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.lblQuestion.Text = SR.GetString(SR.TrustManagerPromptUI_NoPublisherInstallQuestion);
                }
                else
                {
                    this.lblQuestion.Text = SR.GetString(SR.TrustManagerPromptUI_NoPublisherRunQuestion);
                }
            }
            else if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
            {
                this.lblQuestion.Text = SR.GetString(SR.TrustManagerPromptUI_InstallQuestion);
            }
            else
            {
                this.lblQuestion.Text = SR.GetString(SR.TrustManagerPromptUI_RunQuestion);
            }

            // Choose warning text
            if ((this.m_options & TrustManagerPromptOptions.StopApp) != 0)
            {
                // App is blocked - do not display a More Information link
                if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_InstalledAppBlockedWarning);
                }
                else
                {
                    this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_RunAppBlockedWarning);
                }
                this.linkLblMoreInformation.TabStop = false;
                this.linkLblMoreInformation.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_WarningAccessibleDescription);
                this.linkLblMoreInformation.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_WarningAccessibleName);
            }
            else
            {
                string strMoreInfo = SR.GetString(SR.TrustManagerPromptUI_MoreInformation);

                if ((this.m_options & TrustManagerPromptOptions.LocalComputerSource) != 0)
                {
                    // App comes from local machine
                    if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                    {
                        this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_InstallFromLocalMachineWarning, strMoreInfo);
                    }
                    else
                    {
                        this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_RunFromLocalMachineWarning, strMoreInfo);
                    }
                }
                else if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_InstallWarning, strMoreInfo);
                }
                else
                {
                    this.linkLblMoreInformation.Text = SR.GetString(SR.TrustManagerPromptUI_RunWarning, strMoreInfo);
                }
                this.linkLblMoreInformation.TabStop = true;
                this.linkLblMoreInformation.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_MoreInformationAccessibleDescription);
                this.linkLblMoreInformation.AccessibleName = SR.GetString(SR.TrustManagerPromptUI_MoreInformationAccessibleName);
                this.linkLblMoreInformation.Links.Add(new System.Windows.Forms.LinkLabel.Link(this.linkLblMoreInformation.Text.Length - strMoreInfo.Length, strMoreInfo.Length));
            }

            // Choose the warning icon and its accessible description
            if ((this.m_options & TrustManagerPromptOptions.StopApp) != 0 || this.m_publisherName == null)
            {
                if ((this.m_options & TrustManagerPromptOptions.RequiresPermissions) == 0 &&
                    (this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                     LoadWarningBitmap(TrustManagerWarningLevel.Yellow);
                }
                else
                {
                    LoadWarningBitmap(TrustManagerWarningLevel.Red);
                }
            }
            else if ((this.m_options & TrustManagerPromptOptions.RequiresPermissions) == 0)
            {
                Debug.Assert(m_certificate != null);
                LoadWarningBitmap(TrustManagerWarningLevel.Green);
            }
            else
            {
                LoadWarningBitmap(TrustManagerWarningLevel.Yellow);
            }

            // Set the context sensitive accessible descriptions
            if ((this.m_options & TrustManagerPromptOptions.StopApp) != 0)
            {
                if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_InstallBlocked);
                }
                else
                {
                    this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_RunBlocked);
                }
            }
            else if ((this.m_options & TrustManagerPromptOptions.RequiresPermissions) != 0)
            {
                if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
                {
                    this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_InstallWithElevatedPermissions);
                }
                else
                {
                    this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_RunWithElevatedPermissions);
                }
            }
            else if ((this.m_options & TrustManagerPromptOptions.AddsShortcut) != 0)
            {
                this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_InstallConfirmation);
            }
            else
            {
                this.AccessibleDescription = SR.GetString(SR.TrustManagerPromptUI_AccessibleDescription_RunConfirmation);
            }

            ResumeAllLayout(this, true);
        }

        [
            SuppressMessage("Microsoft.Reliability", "CA2002:DoNotLockOnObjectsWithWeakIdentity")
        ]
        private void LoadWarningBitmap(TrustManagerWarningLevel warningLevel)
        {
            Bitmap bitmap;
            switch (warningLevel)
            {
                case TrustManagerWarningLevel.Green:
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        var icon = new Icon(typeof(System.Windows.Forms.Form), "TrustManagerOK.ico");
                        bitmap = icon.ToBitmap();
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerOK.bmp");
                    }
                    this.pictureBoxWarning.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_LowRisk), this.pictureBoxWarning.AccessibleDescription);
                    break;
                case TrustManagerWarningLevel.Yellow:
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        var icon = new Icon(typeof(System.Windows.Forms.Form), "TrustManagerWarning.ico");
                        bitmap = icon.ToBitmap();
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerWarning.bmp");                       
                    }
                    this.pictureBoxWarning.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_MediumRisk), this.pictureBoxWarning.AccessibleDescription);
                    break;
                default:
                    Debug.Assert(warningLevel == TrustManagerWarningLevel.Red);
                    if (!LocalAppContextSwitches.UseLegacyImages)
                    {
                        var icon = new Icon(typeof(System.Windows.Forms.Form), "TrustManagerHighRisk.ico");
                        bitmap = icon.ToBitmap();
                    }
                    else
                    {
                        bitmap = new Bitmap(typeof(System.Windows.Forms.Form), "TrustManagerHighRisk.bmp");
                    }
                    this.pictureBoxWarning.AccessibleDescription = string.Format(CultureInfo.CurrentCulture, SR.GetString(SR.TrustManager_WarningIconAccessibleDescription_HighRisk), this.pictureBoxWarning.AccessibleDescription);
                    break;
            }
            if (bitmap != null)
            {
                bitmap.MakeTransparent();
                this.pictureBoxWarning.Image = bitmap;
            }
        }

        private static string StripOutAccelerator(string text)
        {
            // &Name:     ==>   Name:
            // Name(&N):  ==>   Name:
            int ampIndex = text.IndexOf('&');
            if (ampIndex != -1)
            {
                if (ampIndex > 0 && text[ampIndex - 1] == '(' &&
                    text.Length > ampIndex + 2 && text[ampIndex + 2] == ')')
                {
                    return text.Remove(ampIndex - 1, 4);
                }
                else
                {
                    return text.Replace("&", "");
                }
            }
            return text;
        }

        private void TrustManagerPromptUI_Load(object sender, EventArgs e)
        {
            this.ActiveControl = this.btnCancel;
        }

        private void linkLblFromUrl_MouseEnter(object sender, EventArgs e)
        {
            if (!controlToolTip && toolTipFromUrl != null)
            {
                IntSecurity.AllWindows.Assert();
                try 
                {
                    controlToolTip = true;
                    toolTipFromUrl.Show(linkLblFromUrl.Text, linkLblFromUrl);
                    UnsafeNativeMethods.SendMessage(new HandleRef(toolTipFromUrl, toolTipFromUrl.Handle), NativeMethods.TTM_SETMAXTIPWIDTH, 0, 600);
                }
                finally 
                {
                    System.Security.CodeAccessPermission.RevertAssert();
                    controlToolTip = false;
                }
            }
        }

        private void linkLblFromUrl_MouseLeave(object sender, EventArgs e)
        {
            if (!controlToolTip && toolTipFromUrl != null && toolTipFromUrl.GetHandleCreated())
            {
                toolTipFromUrl.RemoveAll();
                IntSecurity.AllWindows.Assert();
                try 
                {
                    toolTipFromUrl.Hide(this);
                }
                finally 
                {
                    System.Security.CodeAccessPermission.RevertAssert();
                }
            }
        }
                
        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes") // Intentionally eating exceptions
        ]
        private void TrustManagerPromptUI_ShowMoreInformation(System.Object sender, System.Windows.Forms.LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                using (TrustManagerMoreInformation moreInformation = new TrustManagerMoreInformation(m_options, m_publisherName))
                {
                    moreInformation.ShowDialog(this);
                }
            }
            catch (Exception ex)
            {
                Debug.Fail("Error occurred while showing More Information: " + ex.Message);
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes") // Intentionally eating exceptions
        ]
        private void TrustManagerPromptUI_ShowPublisherCertificate(System.Object sender, System.Windows.Forms.LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                Debug.Assert(m_certificate != null);
                X509Certificate2UI.DisplayCertificate(m_certificate, this.Handle);
            }
            catch (Exception ex)
            {
                Debug.Fail("Couldn't display certificate: " + ex.Message);
            }
        }

        [
            SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes") // Intentionally eating exceptions
        ]
        private void TrustManagerPromptUI_ShowSupportPage(System.Object sender, System.Windows.Forms.LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                Debug.Assert(this.m_defaultBrowserExePath != null);
                Debug.Assert(this.m_certificate != null);
                System.Diagnostics.Process.Start(this.m_defaultBrowserExePath, e.Link.LinkData.ToString());
            }
            catch (Exception ex)
            {
                Debug.Fail("Couldn't launch support Url: " + ex.Message);
            }
        }

        private void TrustManagerPromptUI_VisibleChanged(System.Object sender, System.EventArgs e)
        {
            if (this.Visible && Form.ActiveForm != this)
            {
                // Make this form the active form of the application
                this.Activate();
                // Give focus to the Cancel/OK button
                this.ActiveControl = this.btnCancel;
            }
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
                UpdateFonts();
            }
            Invalidate(); // Workaround a bug where the form's background does not repaint properly
        }

        private void UpdateFonts()
        {
            // Choose the fonts.
            this.Font = SystemFonts.MessageBoxFont;
            this.lblQuestion.Font =
            this.lblPublisher.Font =
            this.lblFrom.Font = 
            this.lblName.Font = new Font(this.Font, FontStyle.Bold);

            // Make sure the link labels don't wrap.
            this.linkLblPublisher.MaximumSize =
            this.linkLblFromUrl.MaximumSize =
            this.linkLblName.MaximumSize = new Size(0, this.Font.Height + 2);
        }
    }
}
