//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    partial class AcceptedCertificatesForm
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AcceptedCertificatesForm));
            this.okButton = new System.Windows.Forms.Button();
            this.cancelButton = new System.Windows.Forms.Button();
            this.headerLabel1 = new System.Windows.Forms.Label();
            this.viewButton = new System.Windows.Forms.Button();
            this.listAllowedCertificates = new System.Windows.Forms.ListView();
            this.Allow = new System.Windows.Forms.ColumnHeader();
            this.IssuedTo = new System.Windows.Forms.ColumnHeader();
            this.IssuedBy = new System.Windows.Forms.ColumnHeader();
            this.IntendedPurpose = new System.Windows.Forms.ColumnHeader();
            this.FriendlyName = new System.Windows.Forms.ColumnHeader();
            this.ExpirationDate = new System.Windows.Forms.ColumnHeader();
            this.headerLabel2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // okButton
            // 
            resources.ApplyResources(this.okButton, "okButton");
            this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.okButton.Name = "okButton";
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            resources.ApplyResources(this.cancelButton, "cancelButton");
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Name = "cancelButton";
            // 
            // headerLabel1
            // 
            resources.ApplyResources(this.headerLabel1, "headerLabel1");
            this.headerLabel1.Name = "headerLabel1";
            // 
            // viewButton
            // 
            resources.ApplyResources(this.viewButton, "viewButton");
            this.viewButton.Name = "viewButton";
            this.viewButton.Click += new System.EventHandler(this.viewCertificate_Click);
            // 
            // listAllowedCertificates
            // 
            resources.ApplyResources(this.listAllowedCertificates, "listAllowedCertificates");
            this.listAllowedCertificates.CheckBoxes = true;
            this.listAllowedCertificates.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Allow,
            this.IssuedTo,
            this.IssuedBy,
            this.IntendedPurpose,
            this.FriendlyName,
            this.ExpirationDate});
            this.listAllowedCertificates.FullRowSelect = true;
            this.listAllowedCertificates.MultiSelect = false;
            this.listAllowedCertificates.Name = "listAllowedCertificates";
            this.listAllowedCertificates.View = System.Windows.Forms.View.Details;
            this.listAllowedCertificates.Resize += new System.EventHandler(this.listAllowedCertificates_Resize);
            // 
            // Allow
            // 
            resources.ApplyResources(this.Allow, "Allow");
            // 
            // IssuedTo
            // 
            resources.ApplyResources(this.IssuedTo, "IssuedTo");
            // 
            // IssuedBy
            // 
            resources.ApplyResources(this.IssuedBy, "IssuedBy");
            // 
            // IntendedPurpose
            // 
            resources.ApplyResources(this.IntendedPurpose, "IntendedPurpose");
            // 
            // FriendlyName
            // 
            resources.ApplyResources(this.FriendlyName, "FriendlyName");
            // 
            // ExpirationDate
            // 
            resources.ApplyResources(this.ExpirationDate, "ExpirationDate");
            // 
            // headerLabel2
            // 
            resources.ApplyResources(this.headerLabel2, "headerLabel2");
            this.headerLabel2.Name = "headerLabel2";
            // 
            // AcceptedCertificatesForm
            // 
            this.AcceptButton = this.okButton;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelButton;
            this.Controls.Add(this.headerLabel2);
            this.Controls.Add(this.headerLabel1);
            this.Controls.Add(this.viewButton);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.okButton);
            this.Controls.Add(this.listAllowedCertificates);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AcceptedCertificatesForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.Button cancelButton;
        private System.Windows.Forms.Label headerLabel1;
        private System.Windows.Forms.Button viewButton;
        private System.Windows.Forms.ListView listAllowedCertificates;
        private System.Windows.Forms.ColumnHeader Allow;
        private System.Windows.Forms.ColumnHeader IssuedTo;
        private System.Windows.Forms.ColumnHeader IssuedBy;
        private System.Windows.Forms.ColumnHeader IntendedPurpose;
        private System.Windows.Forms.ColumnHeader FriendlyName;
        private System.Windows.Forms.ColumnHeader ExpirationDate;
        private System.Windows.Forms.Label headerLabel2;
    }
}
