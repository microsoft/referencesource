//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    partial class WsatControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WsatControl));
            this.textBoxNetworkDtcAccess = new System.Windows.Forms.TextBox();
            this.groupBoxTracing = new System.Windows.Forms.GroupBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.buttonTracingOptions = new System.Windows.Forms.Button();
            this.groupBoxTimeouts = new System.Windows.Forms.GroupBox();
            this.textBoxDefaultTimeout = new System.Windows.Forms.TextBox();
            this.textBoxMaxTimeout = new System.Windows.Forms.TextBox();
            this.labelSeconds2 = new System.Windows.Forms.Label();
            this.labelSeconds1 = new System.Windows.Forms.Label();
            this.labelMaxTimeout = new System.Windows.Forms.Label();
            this.labelDefaultTimeout = new System.Windows.Forms.Label();
            this.groupBoxNetwork = new System.Windows.Forms.GroupBox();
            this.textBoxHttpsPort = new System.Windows.Forms.TextBox();
            this.labelHttpsPort = new System.Windows.Forms.Label();
            this.labelEndpointCert = new System.Windows.Forms.Label();
            this.textBoxEndpointCert = new System.Windows.Forms.TextBox();
            this.buttonSelectEndpointCert = new System.Windows.Forms.Button();
            this.buttonSelectAuthorizedAccounts = new System.Windows.Forms.Button();
            this.buttonSelectAuthorizedCerts = new System.Windows.Forms.Button();
            this.labelAuthorizedCerts = new System.Windows.Forms.Label();
            this.labelAuthorizedAccounts = new System.Windows.Forms.Label();
            this.checkBoxEnableNetworkSupport = new System.Windows.Forms.CheckBox();
            this.groupBoxTracing.SuspendLayout();
            this.groupBoxTimeouts.SuspendLayout();
            this.groupBoxNetwork.SuspendLayout();
            this.SuspendLayout();
            // 
            // textBoxNetworkDtcAccess
            // 
            this.textBoxNetworkDtcAccess.BorderStyle = System.Windows.Forms.BorderStyle.None;
            resources.ApplyResources(this.textBoxNetworkDtcAccess, "textBoxNetworkDtcAccess");
            this.textBoxNetworkDtcAccess.Name = "textBoxNetworkDtcAccess";
            this.textBoxNetworkDtcAccess.ReadOnly = true;
            this.textBoxNetworkDtcAccess.TabStop = false;
            // 
            // groupBoxTracing
            // 
            this.groupBoxTracing.Controls.Add(this.textBox1);
            this.groupBoxTracing.Controls.Add(this.buttonTracingOptions);
            resources.ApplyResources(this.groupBoxTracing, "groupBoxTracing");
            this.groupBoxTracing.Name = "groupBoxTracing";
            this.groupBoxTracing.TabStop = false;
            // 
            // textBox1
            // 
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            resources.ApplyResources(this.textBox1, "textBox1");
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.TabStop = false;
            // 
            // buttonTracingOptions
            // 
            resources.ApplyResources(this.buttonTracingOptions, "buttonTracingOptions");
            this.buttonTracingOptions.Name = "buttonTracingOptions";
            this.buttonTracingOptions.UseVisualStyleBackColor = true;
            this.buttonTracingOptions.Click += new System.EventHandler(this.buttonTracingOptions_Click);
            // 
            // groupBoxTimeouts
            // 
            this.groupBoxTimeouts.Controls.Add(this.labelDefaultTimeout);
            this.groupBoxTimeouts.Controls.Add(this.textBoxDefaultTimeout);
            this.groupBoxTimeouts.Controls.Add(this.labelMaxTimeout);
            this.groupBoxTimeouts.Controls.Add(this.textBoxMaxTimeout);
            this.groupBoxTimeouts.Controls.Add(this.labelSeconds2);
            this.groupBoxTimeouts.Controls.Add(this.labelSeconds1);
            resources.ApplyResources(this.groupBoxTimeouts, "groupBoxTimeouts");
            this.groupBoxTimeouts.Name = "groupBoxTimeouts";
            this.groupBoxTimeouts.TabStop = false;
            // 
            // textBoxDefaultTimeout
            // 
            resources.ApplyResources(this.textBoxDefaultTimeout, "textBoxDefaultTimeout");
            this.textBoxDefaultTimeout.Name = "textBoxDefaultTimeout";
            this.textBoxDefaultTimeout.TextChanged += new System.EventHandler(this.textBoxDefaultTimeout_TextChanged);
            // 
            // textBoxMaxTimeout
            // 
            resources.ApplyResources(this.textBoxMaxTimeout, "textBoxMaxTimeout");
            this.textBoxMaxTimeout.Name = "textBoxMaxTimeout";
            this.textBoxMaxTimeout.TextChanged += new System.EventHandler(this.textBoxMaxTimeout_TextChanged);
            // 
            // labelSeconds2
            // 
            resources.ApplyResources(this.labelSeconds2, "labelSeconds2");
            this.labelSeconds2.Name = "labelSeconds2";
            // 
            // labelSeconds1
            // 
            resources.ApplyResources(this.labelSeconds1, "labelSeconds1");
            this.labelSeconds1.Name = "labelSeconds1";
            // 
            // labelMaxTimeout
            // 
            resources.ApplyResources(this.labelMaxTimeout, "labelMaxTimeout");
            this.labelMaxTimeout.Name = "labelMaxTimeout";
            // 
            // labelDefaultTimeout
            // 
            resources.ApplyResources(this.labelDefaultTimeout, "labelDefaultTimeout");
            this.labelDefaultTimeout.Name = "labelDefaultTimeout";
            // 
            // groupBoxNetwork
            // 
            this.groupBoxNetwork.Controls.Add(this.labelHttpsPort);
            this.groupBoxNetwork.Controls.Add(this.textBoxHttpsPort);
            this.groupBoxNetwork.Controls.Add(this.labelEndpointCert);
            this.groupBoxNetwork.Controls.Add(this.textBoxEndpointCert);
            this.groupBoxNetwork.Controls.Add(this.buttonSelectEndpointCert);
            this.groupBoxNetwork.Controls.Add(this.buttonSelectAuthorizedAccounts);
            this.groupBoxNetwork.Controls.Add(this.buttonSelectAuthorizedCerts);
            this.groupBoxNetwork.Controls.Add(this.labelAuthorizedCerts);
            this.groupBoxNetwork.Controls.Add(this.labelAuthorizedAccounts);
            resources.ApplyResources(this.groupBoxNetwork, "groupBoxNetwork");
            this.groupBoxNetwork.Name = "groupBoxNetwork";
            this.groupBoxNetwork.TabStop = false;
            // 
            // textBoxHttpsPort
            // 
            resources.ApplyResources(this.textBoxHttpsPort, "textBoxHttpsPort");
            this.textBoxHttpsPort.Name = "textBoxHttpsPort";
            this.textBoxHttpsPort.TextChanged += new System.EventHandler(this.textBoxHttpsPort_TextChanged);
            // 
            // labelHttpsPort
            // 
            resources.ApplyResources(this.labelHttpsPort, "labelHttpsPort");
            this.labelHttpsPort.Name = "labelHttpsPort";
            // 
            // labelEndpointCert
            // 
            resources.ApplyResources(this.labelEndpointCert, "labelEndpointCert");
            this.labelEndpointCert.Name = "labelEndpointCert";
            // 
            // textBoxEndpointCert
            // 
            this.textBoxEndpointCert.BackColor = System.Drawing.SystemColors.Control;
            this.textBoxEndpointCert.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            resources.ApplyResources(this.textBoxEndpointCert, "textBoxEndpointCert");
            this.textBoxEndpointCert.Name = "textBoxEndpointCert";
            this.textBoxEndpointCert.ReadOnly = true;
            // 
            // buttonSelectEndpointCert
            // 
            resources.ApplyResources(this.buttonSelectEndpointCert, "buttonSelectEndpointCert");
            this.buttonSelectEndpointCert.Name = "buttonSelectEndpointCert";
            this.buttonSelectEndpointCert.UseVisualStyleBackColor = true;
            this.buttonSelectEndpointCert.Click += new System.EventHandler(this.buttonSelectEndpointCert_Click);
            // 
            // buttonSelectAuthorizedAccounts
            // 
            resources.ApplyResources(this.buttonSelectAuthorizedAccounts, "buttonSelectAuthorizedAccounts");
            this.buttonSelectAuthorizedAccounts.Name = "buttonSelectAuthorizedAccounts";
            this.buttonSelectAuthorizedAccounts.UseVisualStyleBackColor = true;
            this.buttonSelectAuthorizedAccounts.Click += new System.EventHandler(this.buttonSelectAuthorizedAccounts_Click);
            // 
            // buttonSelectAuthorizedCerts
            // 
            resources.ApplyResources(this.buttonSelectAuthorizedCerts, "buttonSelectAuthorizedCerts");
            this.buttonSelectAuthorizedCerts.Name = "buttonSelectAuthorizedCerts";
            this.buttonSelectAuthorizedCerts.UseVisualStyleBackColor = true;
            this.buttonSelectAuthorizedCerts.Click += new System.EventHandler(this.buttonSelectAuthorizedCerts_Click);
            // 
            // labelAuthorizedCerts
            // 
            resources.ApplyResources(this.labelAuthorizedCerts, "labelAuthorizedCerts");
            this.labelAuthorizedCerts.Name = "labelAuthorizedCerts";
            // 
            // labelAuthorizedAccounts
            // 
            resources.ApplyResources(this.labelAuthorizedAccounts, "labelAuthorizedAccounts");
            this.labelAuthorizedAccounts.Name = "labelAuthorizedAccounts";
            // 
            // checkBoxEnableNetworkSupport
            // 
            resources.ApplyResources(this.checkBoxEnableNetworkSupport, "checkBoxEnableNetworkSupport");
            this.checkBoxEnableNetworkSupport.Name = "checkBoxEnableNetworkSupport";
            this.checkBoxEnableNetworkSupport.UseVisualStyleBackColor = true;
            this.checkBoxEnableNetworkSupport.CheckedChanged += new System.EventHandler(this.checkBoxEnableNetworkSupport_CheckedChanged);
            // 
            // WsatControl
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.textBoxNetworkDtcAccess);
            this.Controls.Add(this.checkBoxEnableNetworkSupport);
            this.Controls.Add(this.groupBoxNetwork);
            this.Controls.Add(this.groupBoxTimeouts);
            this.Controls.Add(this.groupBoxTracing);
            this.Name = "WsatControl";
            this.groupBoxTracing.ResumeLayout(false);
            this.groupBoxTracing.PerformLayout();
            this.groupBoxTimeouts.ResumeLayout(false);
            this.groupBoxTimeouts.PerformLayout();
            this.groupBoxNetwork.ResumeLayout(false);
            this.groupBoxNetwork.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox textBoxNetworkDtcAccess;
        private System.Windows.Forms.GroupBox groupBoxNetwork;
        private System.Windows.Forms.Label labelHttpsPort;
        private System.Windows.Forms.GroupBox groupBoxTimeouts;
        private System.Windows.Forms.Label labelEndpointCert;
        private System.Windows.Forms.Label labelAuthorizedCerts;
        private System.Windows.Forms.Label labelAuthorizedAccounts;
        private System.Windows.Forms.TextBox textBoxHttpsPort;
        private System.Windows.Forms.Button buttonSelectEndpointCert;
        private System.Windows.Forms.TextBox textBoxEndpointCert;
        private System.Windows.Forms.Label labelMaxTimeout;
        private System.Windows.Forms.Label labelDefaultTimeout;
        private System.Windows.Forms.CheckBox checkBoxEnableNetworkSupport;
        private System.Windows.Forms.Button buttonSelectAuthorizedCerts;
        private System.Windows.Forms.TextBox textBoxDefaultTimeout;
        private System.Windows.Forms.Label labelSeconds2;
        private System.Windows.Forms.Label labelSeconds1;
        private System.Windows.Forms.TextBox textBoxMaxTimeout;
        private System.Windows.Forms.Button buttonTracingOptions;
        private System.Windows.Forms.GroupBox groupBoxTracing;
        private System.Windows.Forms.Button buttonSelectAuthorizedAccounts;
        private System.Windows.Forms.TextBox textBox1;
    }
}
