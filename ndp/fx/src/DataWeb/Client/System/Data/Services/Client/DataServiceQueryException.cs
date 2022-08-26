//---------------------------------------------------------------------
// <copyright file="DataServiceQueryException.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Exception class for query requests.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    /// The exception that is thrown when an error occurs while processing a batch request or
    /// during SaveChanges.
    /// </summary>
#if !ASTORIA_LIGHT
    [Serializable]
#endif
    [System.Diagnostics.DebuggerDisplay("{Message}")]
    public sealed class DataServiceQueryException : InvalidOperationException
    {
        #region Private fields.

        /// <summary>Actual response object.</summary>
#if !ASTORIA_LIGHT
        [NonSerialized]
#endif
        private readonly QueryOperationResponse response;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        public DataServiceQueryException()
            : base(Strings.DataServiceException_GeneralError)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        public DataServiceQueryException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        public DataServiceQueryException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceQueryException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        /// <param name="response">response object for this exception.</param>
        public DataServiceQueryException(string message, Exception innerException, QueryOperationResponse response)
            : base(message, innerException)
        {
            this.response = response;
        }

#if !ASTORIA_LIGHT
#pragma warning disable 0628
        /// <summary>
        /// Initializes a new instance of the DataServiceQueryException class from the 
        /// specified SerializationInfo and StreamingContext instances.
        /// </summary>
        /// <param name="info">
        /// A SerializationInfo containing the information required to serialize 
        /// the new DataServiceQueryException.</param>
        /// <param name="context">
        /// A StreamingContext containing the source of the serialized stream 
        /// associated with the new DataServiceQueryException.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1047", Justification = "Follows serialization info pattern.")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1032", Justification = "Follows serialization info pattern.")]
        protected DataServiceQueryException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
            : base(info, context)
        {
        }
#pragma warning restore 0628
#endif

        #endregion Constructors.

        #region Public properties.

        /// <summary>Error code to be used in payloads.</summary>
        public QueryOperationResponse Response
        {
            get { return this.response; }
        }

        #endregion Public properties.
    }
}
