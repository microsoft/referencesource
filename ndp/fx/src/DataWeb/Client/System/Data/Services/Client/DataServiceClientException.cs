//---------------------------------------------------------------------
// <copyright file="DataServiceClientException.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Exception class for server errors.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Security.Permissions;

    /// <summary>
    /// The exception that is thrown when the server returns an error.
    /// </summary>
#if !ASTORIA_LIGHT
    [Serializable]
#endif
    [System.Diagnostics.DebuggerDisplay("{Message}")]
    public sealed class DataServiceClientException : InvalidOperationException
    {
        /// <summary>status code as returned by the server.</summary>
        private readonly int statusCode;

        #region Constructors.

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        public DataServiceClientException()
            : this(Strings.DataServiceException_GeneralError)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        public DataServiceClientException(string message)
            : this(message, null)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        public DataServiceClientException(string message, Exception innerException)
            : this(message, innerException, 500)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="statusCode">status code as returned by the server.</param>
        public DataServiceClientException(string message, int statusCode)
            : this(message, null, statusCode)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        /// <param name="statusCode">status code as returned by the server.</param>
        public DataServiceClientException(string message, Exception innerException, int statusCode)
            : base(message, innerException)
        {
            this.statusCode = statusCode;
        }

#if !ASTORIA_LIGHT
#pragma warning disable 0628
        /// <summary>
        /// Initializes a new instance of the DataServiceQueryException class from the 
        /// specified SerializationInfo and StreamingContext instances.
        /// </summary>
        /// <param name="serializationInfo">
        /// A SerializationInfo containing the information required to serialize 
        /// the new DataServiceQueryException.</param>
        /// <param name="context">
        /// A StreamingContext containing the source of the serialized stream 
        /// associated with the new DataServiceQueryException.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1047", Justification = "Follows serialization info pattern.")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1032", Justification = "Follows serialization info pattern.")]
        protected DataServiceClientException(System.Runtime.Serialization.SerializationInfo serializationInfo, System.Runtime.Serialization.StreamingContext context)
            : base(serializationInfo, context)
        {
            if (serializationInfo != null)
            {
                this.statusCode = serializationInfo.GetInt32("statusCode");
            }
        }
#pragma warning restore 0628
#endif

        #endregion Constructors.

        #region Public properties.

        /// <summary>Error code to be used in payloads.</summary>
        public int StatusCode
        {
            get { return this.statusCode; }
        }

        #endregion Public properties.

        #region Methods.

#if !ASTORIA_LIGHT
        /// <summary>
        /// Sets the SerializationInfo with information about the exception.
        /// </summary>
        /// <param name="info">The SerializationInfo that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The StreamingContext that contains contextual information about the source or destination.</param>
        /* 
         * put in an explicit LinkDemand for FullTrust because the base GetObjectData is public and SecurityCritical which translates to a LinkDemand for FullTrust
         * We add it explicitly here so that we don't get an FXCOP CA2123
         */
        [PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
        [System.Security.SecurityCritical]
        public override void GetObjectData(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
        {
            if (info != null)
            {
                info.AddValue("statusCode", this.statusCode);
            }

            base.GetObjectData(info, context);
        }
#endif
        #endregion Methods.
    }
}
