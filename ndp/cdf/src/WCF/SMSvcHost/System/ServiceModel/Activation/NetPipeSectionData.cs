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

    class NetPipeSectionData
    {
        int maxPendingConnections;
        int maxPendingAccepts;
        TimeSpan receiveTimeout;
        List<SecurityIdentifier> allowAccounts;

        public NetPipeSectionData()
        {
            NetPipeSection section = (NetPipeSection)ConfigurationManager.GetSection(ConfigurationStrings.NetPipeSectionPath);
            if (section == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException());
            }

            this.maxPendingConnections = section.MaxPendingConnections;
            this.maxPendingAccepts = section.MaxPendingAccepts;
            this.receiveTimeout = section.ReceiveTimeout;
            this.allowAccounts = new List<SecurityIdentifier>();
            foreach (SecurityIdentifierElement element in section.AllowAccounts)
            {
                this.allowAccounts.Add(element.SecurityIdentifier);
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

        public List<SecurityIdentifier> AllowAccounts
        {
            get
            {
                return this.allowAccounts;
            }
        }
    }
}
