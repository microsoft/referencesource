//---------------------------------------------------------------------
// <copyright file="DataServiceBehavior.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Holds configuration of service behavior.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Data.Services.Common;

    /// <summary>Use this class to add settings that define service behavior.</summary>
    public sealed class DataServiceBehavior
    {
        /// <summary>
        /// Initializes a new <see cref="DataServiceBehavior"/>.
        /// </summary>
        internal DataServiceBehavior()
        {
            this.InvokeInterceptorsOnLinkDelete = true;
            this.AcceptCountRequests = true;
            this.AcceptProjectionRequests = true;
            this.MaxProtocolVersion = DataServiceProtocolVersion.V1;
        }

        /// <summary>
        /// Whether to invoke change interceptor on link delete.
        /// </summary>
        public bool InvokeInterceptorsOnLinkDelete
        {
            get;
            set;
        }

        /// <summary>
        /// Whether $count and $inlinecount requests should be accepted
        /// </summary>
        public bool AcceptCountRequests
        {
            get;
            set;
        }

        /// <summary>
        /// Whether projection requests should be accepted
        /// </summary>
        public bool AcceptProjectionRequests
        {
            get;
            set;
        }

        /// <summary>
        /// Max version of the response sent by server
        /// </summary>
        public DataServiceProtocolVersion MaxProtocolVersion
        {
            get;
            set;
        }

        /// <summary>
        /// Allow replace functions in the request url.
        /// </summary>
        public bool AcceptReplaceFunctionInQuery
        {
            get;
            set;
        }
    }
}
