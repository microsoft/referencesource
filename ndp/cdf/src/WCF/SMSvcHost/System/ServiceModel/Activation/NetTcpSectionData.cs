//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.Collections.Generic;
    using System.Security.Principal;
    using System.Configuration;
    using System.Diagnostics;    
    using System.ServiceModel.Activation.Configuration;

    class NetTcpSectionData
    {
        int listenBacklog;
        int maxPendingConnections;
        int maxPendingAccepts;
        TimeSpan receiveTimeout;
        bool teredoEnabled;
        List<SecurityIdentifier> allowAccounts;

        public NetTcpSectionData()
        {
            NetTcpSection section = (NetTcpSection)ConfigurationManager.GetSection(ConfigurationStrings.NetTcpSectionPath);
            if (section == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException());
            }

            this.listenBacklog = section.ListenBacklog;
            this.maxPendingConnections = section.MaxPendingConnections;
            this.maxPendingAccepts = section.MaxPendingAccepts;
            this.receiveTimeout = section.ReceiveTimeout;
            this.teredoEnabled = section.TeredoEnabled;
            this.allowAccounts = new List<SecurityIdentifier>();
            foreach (SecurityIdentifierElement element in section.AllowAccounts)
            {
                this.allowAccounts.Add(element.SecurityIdentifier);
            }
        }

        public int ListenBacklog
        {
            get
            {
                return this.listenBacklog;
            }
        }

        public int MaxPendingConnections
        {
            get
            {
                return this.maxPendingConnections;
            }
        }

        public int MaxPendingAccepts
        {
            get
            {
                return this.maxPendingAccepts;
            }
        }
        public TimeSpan ReceiveTimeout
        {
            get
            {
                return this.receiveTimeout;
            }
        }

        public bool TeredoEnabled
        {
            get
            {
                return this.teredoEnabled;
            }
        }

        public List<SecurityIdentifier> AllowAccounts
        {
            get
            {
                return this.allowAccounts;
            }
        }
    }
}
