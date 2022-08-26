//---------------------------------------------------------------------
// <copyright file="DataServiceRequestException.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Exception class for batch requests and CUD operations.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    /// The exception that is thrown when executing a single query request.
    /// </summary>
#if !ASTORIA_LIGHT
    [Serializable]
#endif
    [System.Diagnostics.DebuggerDisplay("{Message}")]
    public sealed class DataServiceRequestException : InvalidOperationException
    {
        /// <summary>Actual response object.</summary>
#if !ASTORIA_LIGHT
        [NonSerialized]
#endif
        private readonly DataServiceResponse response;

        #region Constructors.

        /// <summary>
        /// Creates a new instance of DataServiceRequestException.
        /// </summary>
        public DataServiceRequestException()
            : base(Strings.DataServiceException_GeneralError)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceRequestException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        public DataServiceRequestException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceRequestException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        public DataServiceRequestException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Creates a new instance of DataServiceRequestException.
        /// </summary>
        /// <param name="message">error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        /// <param name="response">response object for this exception.</param>
        public DataServiceRequestException(string message, Exception innerException, DataServiceResponse response)
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
        /// the new DataServiceException.</param>
        /// <param name="context">
        /// A StreamingContext containing the source of the serialized stream 
        /// associated with the new DataServiceException.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1047", Justification = "Follows serialization info pattern.")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1032", Justification = "Follows serialization info pattern.")]
        protected DataServiceRequestException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
            : base(info, context)
        {
        }
#pragma warning restore 0628
#endif

        #endregion Constructors.

        #region Public properties.

        /// <summary>Error code to be used in payloads.</summary>
        public DataServiceResponse Response
        {
            get { return this.response; }
        }

        #endregion Public properties.
    }
}
