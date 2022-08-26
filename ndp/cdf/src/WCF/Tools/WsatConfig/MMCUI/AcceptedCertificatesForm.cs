//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Data;
    using System.Drawing;
    using System.Text;
    using System.Windows.Forms;
    using System.Globalization;
    using System.Security.Cryptography.X509Certificates;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;

    partial class AcceptedCertificatesForm : Form
    {
        X509Certificate2Collection store;
        string[] allowedCertificates;

        internal AcceptedCertificatesForm(X509Certificate2Collection store, string[] allowedCertificates)
        {
            if (store == null)
            {
                throw new ArgumentNullException("store");
            }
            if (allowedCertificates == null)
            {
                throw new ArgumentNullException("allowedCertificates");
            }

            InitializeComponent();
            this.store = store;
            this.allowedCertificates = allowedCertificates;

            IntitializeListItemsCheckState();
        }

        void IntitializeListItemsCheckState()
        {
            for (int i = 0; i < store.Count; i++)
            {
                StringBuilder purposesString = new StringBuilder();

                const string EnhancedKeyUsageOID = "2.5.29.37";
                X509Certificate2 cert = store[i];
                X509EnhancedKeyUsageExtension ext;
                ext = (X509EnhancedKeyUsageExtension)store[i].Extensions[EnhancedKeyUsageOID];
                if (ext != null)
                {
                    OidCollection oids = ext.EnhancedKeyUsages;
                    for (int j = 0; j < oids.Count; j++)
                    {
                        purposesString.Append(oids[j].FriendlyName);
                        if (j < oids.Count - 1)
                        {
                            purposesString.Append(", ");
                        }
                    }
                }

                string[] listViewItemData = new string[] {
                    String.Empty,
                    store[i].GetNameInfo(X509NameType.SimpleName, false),
                    store[i].GetNameInfo(X509NameType.SimpleName, true),
                    purposesString.ToString(),
                    String.IsNullOrEmpty(store[i].FriendlyName) ? SR.GetString(SR.FriendlyNameNone) : store[i].FriendlyName,
                    store[i].NotAfter.ToShortDateString() };

                ListViewItem listViewItem = new ListViewItem(listViewItemData, -1);
                listViewItem.StateImageIndex = 0;
                this.listAllowedCertificates.Items.Add(listViewItem);

                for (int j = 0; j < allowedCertificates.Length; j++)
                {
                    if (Utilities.SafeCompare(allowedCertificates[j], store[i].Thumbprint))
                    {
                        this.listAllowedCertificates.Items[i].Checked = true;
                        break;
                    }
                }
            }
        }

        void viewCertificate_Click(object sender, EventArgs e)
        {
            if (listAllowedCertificates.SelectedIndices.Count > 0)
            {
                X509Certificate2UI.DisplayCertificate(
                    this.store[listAllowedCertificates.SelectedIndices[0]],
                    this.Handle);
            }
        }

        void okButton_Click(object sender, EventArgs e)
        {
            this.allowedCertificates = new string[this.listAllowedCertificates.CheckedIndices.Count];
            for (int i = 0; i < allowedCertificates.Length; i++)
            {
                this.allowedCertificates[i] = store[listAllowedCertificates.CheckedIndices[i]].Thumbprint;
            }
        }

        void listAllowedCertificates_Resize(object sender, EventArgs e)
        {
            for (int i = 1; i < this.listAllowedCertificates.Columns.Count; i++)
            {
                this.listAllowedCertificates.Columns[i].Width = (this.listAllowedCertificates.Width - this.listAllowedCertificates.Columns[0].Width) / (this.listAllowedCertificates.Columns.Count - 1);
            }
        }

        internal string[] AllowedCertificates
        {
            get { return this.allowedCertificates; }
        }
    }
}
