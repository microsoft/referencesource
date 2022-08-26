//---------------------------------------------------------------------
// <copyright file="ChangesetResponse.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Response from SaveChanges.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections.Generic;
    using System.Diagnostics;

    /// <summary>
    /// Response from SaveChanges.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1010", Justification = "required for this feature")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public sealed class ChangeOperationResponse : OperationResponse
    {
        /// <summary>descriptor containing the response object.</summary>
        private Descriptor descriptor;

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="headers">HTTP headers</param>
        /// <param name="descriptor">response object containing information about resources that got changed.</param>
        internal ChangeOperationResponse(Dictionary<string, string> headers, Descriptor descriptor)
            : base(headers)
        {
            Debug.Assert(descriptor != null, "descriptor != null");
            this.descriptor = descriptor;
        }

        /// <summary>Descriptor containing the response object.</summary>
        public Descriptor Descriptor
        {
            get { return this.descriptor; }
        }
    }
}
