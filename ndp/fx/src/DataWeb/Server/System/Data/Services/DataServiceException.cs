//---------------------------------------------------------------------
// <copyright file="DataServiceException.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Base class for exceptions thrown by the web data services.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Security.Permissions;

    #endregion Namespaces.

    /// <summary>
    /// The exception that is thrown when an error occurs while processing
    /// a web data service request.
    /// </summary>
    /// <remarks>
    /// The DataServiceException is thrown to indicate an error during
    /// request processing, specifying the appropriate response for
    /// the request.
    /// 
    /// RFC2616 about the status code values:
    ///     1xx: Informational  - Request received, continuing process
    ///     "100"  ; Section 10.1.1: Continue
    ///     "101"  ; Section 10.1.2: Switching Protocols
    ///     
    ///     2xx: Success        - The action was successfully received, understood, and accepted
    ///     "200"  ; Section 10.2.1: OK
    ///     "201"  ; Section 10.2.2: Created
    ///     "202"  ; Section 10.2.3: Accepted
    ///     "203"  ; Section 10.2.4: Non-Authoritative Information
    ///     "204"  ; Section 10.2.5: No Content
    ///     "205"  ; Section 10.2.6: Reset Content
    ///     "206"  ; Section 10.2.7: Partial Content
    ///     
    ///     3xx: Redirection    - Further action must be taken in order to complete the request
    ///     "300"  ; Section 10.3.1: Multiple Choices
    ///     "301"  ; Section 10.3.2: Moved Permanently
    ///     "302"  ; Section 10.3.3: Found
    ///     "303"  ; Section 10.3.4: See Other
    ///     "304"  ; Section 10.3.5: Not Modified
    ///     "305"  ; Section 10.3.6: Use Proxy
    ///     "307"  ; Section 10.3.8: Temporary Redirect
    ///     
    ///     4xx: Client Error   - The request contains bad syntax or cannot be fulfilled
    ///     "400"  ; Section 10.4.1: Bad Request
    ///     "401"  ; Section 10.4.2: Unauthorized
    ///     "402"  ; Section 10.4.3: Payment Required
    ///     "403"  ; Section 10.4.4: Forbidden
    ///     "404"  ; Section 10.4.5: Not Found
    ///     "405"  ; Section 10.4.6: Method Not Allowed
    ///     "406"  ; Section 10.4.7: Not Acceptable
    ///     "407"  ; Section 10.4.8: Proxy Authentication Required
    ///     "408"  ; Section 10.4.9: Request Time-out
    ///     "409"  ; Section 10.4.10: Conflict
    ///     "410"  ; Section 10.4.11: Gone
    ///     "411"  ; Section 10.4.12: Length Required
    ///     "412"  ; Section 10.4.13: Precondition Failed
    ///     "413"  ; Section 10.4.14: Request Entity Too Large
    ///     "414"  ; Section 10.4.15: Request-URI Too Large
    ///     "415"  ; Section 10.4.16: Unsupported Media Type
    ///     "416"  ; Section 10.4.17: Requested range not satisfiable
    ///     "417"  ; Section 10.4.18: Expectation Failed
    ///     
    ///     5xx: Server Error   - The server failed to fulfill an apparently valid request
    ///     "500"  ; Section 10.5.1: Internal Server Error
    ///     "501"  ; Section 10.5.2: Not Implemented
    ///     "502"  ; Section 10.5.3: Bad Gateway
    ///     "503"  ; Section 10.5.4: Service Unavailable
    ///     "504"  ; Section 10.5.5: Gateway Time-out
    ///     "505"  ; Section 10.5.6: HTTP Version not supported
    /// </remarks>
    [Serializable]
    [DebuggerDisplay("{statusCode}: {Message}")]
    public sealed class DataServiceException : InvalidOperationException
    {
        #region Private fields.

        /// <summary>Language for the exception message.</summary>
        private readonly string messageLanguage;

        /// <summary>Error code to be used in payloads.</summary>
        private readonly string errorCode;

        /// <summary>HTTP response status code for this exception.</summary>
        private readonly int statusCode;

        /// <summary>'Allow' response for header.</summary>
        private string responseAllowHeader;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Initializes a new instance of the DataServiceException class.
        /// </summary>
        /// <remarks>
        /// The Message property is initialized to a system-supplied message 
        /// that describes the error. This message takes into account the 
        /// current system culture. The StatusCode property is set to 500
        /// (Internal Server Error).
        /// </remarks>
        public DataServiceException()
            : this(500, Strings.DataServiceException_GeneralError)
        {
        }

        /// <summary>
        /// Initializes a new instance of the DataServiceException class.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <remarks>
        /// The StatusCode property is set to 500 (Internal Server Error).
        /// </remarks>
        public DataServiceException(string message)
            : this(500, message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the DataServiceException class.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        /// <remarks>
        /// The StatusCode property is set to 500 (Internal Server Error).
        /// </remarks>
        public DataServiceException(string message, Exception innerException)
            : this(500, null, message, null, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the DataServiceException class.
        /// </summary>
        /// <param name="statusCode">HTTP response status code for this exception.</param>
        /// <param name="message">Plain text error message for this exception.</param>
        public DataServiceException(int statusCode, string message)
            : this(statusCode, null, message, null, null)
        {
        }

        /// <summary>
        /// Initializes a new instance of the DataServiceException class.
        /// </summary>
        /// <param name="statusCode">HTTP response status code for this exception.</param>
        /// <param name="errorCode">Error code to be used in payloads.</param>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <param name="messageXmlLang">Language of the <paramref name="message"/>.</param>
        /// <param name="innerException">Exception that caused this exception to be thrown.</param>
        public DataServiceException(int statusCode, string errorCode, string message, string messageXmlLang, Exception innerException)
            : base(message, innerException)
        {
            this.errorCode = errorCode ?? String.Empty;
            this.messageLanguage = messageXmlLang ?? CultureInfo.CurrentCulture.Name;
            this.statusCode = statusCode;
        }

#pragma warning disable 0628

        // Warning CS0628:
        // A sealed class cannot introduce a protected member because no other class will be able to inherit from the 
        // sealed class and use the protected member.
        //
        // This method is used by the runtime when deserializing an exception. It follows the standard pattern,
        // which will also be necessary when this class is subclassed by DataServiceException<T>.

        /// <summary>
        /// Initializes a new instance of the DataServiceException class from the 
        /// specified SerializationInfo and StreamingContext instances.
        /// </summary>
        /// <param name="serializationInfo">
        /// A SerializationInfo containing the information required to serialize 
        /// the new DataServiceException.
        /// </param>
        /// <param name="streamingContext">
        /// A StreamingContext containing the source of the serialized stream 
        /// associated with the new DataServiceException.
        /// </param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1047", Justification = "Follows serialization info pattern.")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1032", Justification = "Follows serialization info pattern.")]
        protected DataServiceException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext)
        {
            if (serializationInfo != null)
            {
                this.errorCode = serializationInfo.GetString("errorCode");
                this.messageLanguage = serializationInfo.GetString("messageXmlLang");
                this.responseAllowHeader = serializationInfo.GetString("responseAllowHeader");
                this.statusCode = serializationInfo.GetInt32("statusCode");
            }
        }

#pragma warning restore 0628

        #endregion Constructors.

        #region Public properties.

        /// <summary>Error code to be used in payloads.</summary>
        public string ErrorCode
        {
            get { return this.errorCode; }
        }

        /// <summary>Language for the exception Message.</summary>
        public string MessageLanguage
        {
            get { return this.messageLanguage; }
        }

        /// <summary>Response status code for this exception.</summary>
        public int StatusCode
        {
            get { return this.statusCode; }
        }

        #endregion Public properties.

        #region Internal properties.

        /// <summary>'Allow' response for header.</summary>
        internal string ResponseAllowHeader
        {
            get { return this.responseAllowHeader; }
        }

        #endregion Internal properties.

        #region Methods.

        /// <summary>
        /// Sets the SerializationInfo with information about the exception.
        /// </summary>
        /// <param name="info">The SerializationInfo that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The StreamingContext that contains contextual information about the source or destination.</param>
        /* 
         * put in an explicit LinkDemand for FullTrust because the base GetObjectData is public and SecurityCritical which translates to a LinkDemand for FullTrust
         * We add it explicitly here so that we don't get an FXCOP CA2123
         */
        [System.Security.SecurityCritical]
        [PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info != null)
            {
                info.AddValue("errorCode", this.errorCode);
                info.AddValue("messageXmlLang", this.messageLanguage);
                info.AddValue("responseAllowHeader", this.responseAllowHeader);
                info.AddValue("statusCode", this.statusCode);
            }

            base.GetObjectData(info, context);
        }

        /// <summary>Creates a new "Bad Request" exception for recursion limit exceeded.</summary>
        /// <param name="recursionLimit">Recursion limit that was reaced.</param>
        /// <returns>A new exception to indicate that the request is rejected.</returns>
        internal static DataServiceException CreateDeepRecursion(int recursionLimit)
        {
            return DataServiceException.CreateBadRequestError(Strings.BadRequest_DeepRecursion(recursionLimit));
        }

        /// <summary>Creates a new "Bad Request" exception for recursion limit exceeded.</summary>
        /// <returns>A new exception to indicate that the request is rejected.</returns>
        internal static DataServiceException CreateDeepRecursion_General()
        {
            return DataServiceException.CreateBadRequestError(Strings.BadRequest_DeepRecursion_General);
        }

        /// <summary>Creates a new "Forbidden" exception.</summary>
        /// <returns>A new exception to indicate that the request is forbidden.</returns>
        internal static DataServiceException CreateForbidden()
        {
            // 403: Forbidden
            return new DataServiceException(403, Strings.RequestUriProcessor_Forbidden);
        }

        /// <summary>Creates a new "Resource Not Found" exception.</summary>
        /// <param name="identifier">segment identifier information for which resource was not found.</param>
        /// <returns>A new exception to indicate the requested resource cannot be found.</returns>
        internal static DataServiceException CreateResourceNotFound(string identifier)
        {
            // 404: Not Found
            return new DataServiceException(404, Strings.RequestUriProcessor_ResourceNotFound(identifier));
        }

        /// <summary>Creates a new "Resource Not Found" exception.</summary>
        /// <param name="errorMessage">Plain text error message for this exception.</param>
        /// <returns>A new exception to indicate the requested resource cannot be found.</returns>
        internal static DataServiceException ResourceNotFoundError(string errorMessage)
        {
            // 404: Not Found
            return new DataServiceException(404, errorMessage);
        }

        /// <summary>Creates a new exception to indicate a syntax error.</summary>
        /// <returns>A new exception to indicate a syntax error.</returns>
        internal static DataServiceException CreateSyntaxError()
        {
            return CreateSyntaxError(Strings.RequestUriProcessor_SyntaxError);
        }

        /// <summary>Creates a new exception to indicate a syntax error.</summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <returns>A new exception to indicate a syntax error.</returns>
        internal static DataServiceException CreateSyntaxError(string message)
        {
            return DataServiceException.CreateBadRequestError(message);
        }

        /// <summary>
        /// Creates a new exception to indicate Precondition error.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <returns>A new exception to indicate a Precondition failed error.</returns>
        internal static DataServiceException CreatePreConditionFailedError(string message)
        {
            // 412 - Precondition failed
            return new DataServiceException(412, message);
        }

        /// <summary>
        /// Creates a new exception to indicate Precondition error.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <param name="innerException">Inner Exception.</param>
        /// <returns>A new exception to indicate a Precondition failed error.</returns>
        internal static DataServiceException CreatePreConditionFailedError(string message, Exception innerException)
        {
            // 412 - Precondition failed
            return new DataServiceException(412, null, message, null, innerException);
        }

        /// <summary>
        /// Creates a new exception to indicate BadRequest error.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <returns>A new exception to indicate a bad request error.</returns>
        internal static DataServiceException CreateBadRequestError(string message)
        {
            // 400 - Bad Request
            return new DataServiceException(400, message);
        }

        /// <summary>
        /// Creates a new exception to indicate BadRequest error.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <param name="innerException">Inner Exception.</param>
        /// <returns>A new exception to indicate a bad request error.</returns>
        internal static DataServiceException CreateBadRequestError(string message, Exception innerException)
        {
            // 400 - Bad Request
            return new DataServiceException(400, null, message, null, innerException);
        }

        /// <summary>Creates a new "Method Not Allowed" exception.</summary>
        /// <param name="message">Error message.</param>
        /// <param name="allow">String value for 'Allow' header in response.</param>
        /// <returns>A new exception to indicate the requested method is not allowed on the response.</returns>
        internal static DataServiceException CreateMethodNotAllowed(string message, string allow)
        {
            // 405 - Method Not Allowed
            DataServiceException result = new DataServiceException(405, message);
            result.responseAllowHeader = allow;
            return result;
        }

        /// <summary>
        /// Creates a new exception to indicate MethodNotImplemented error.
        /// </summary>
        /// <param name="message">Plain text error message for this exception.</param>
        /// <returns>A new exception to indicate a MethodNotImplemented error.</returns>
        internal static DataServiceException CreateMethodNotImplemented(string message)
        {
            // 501 - Method Not Implemented
            return new DataServiceException(501, message);
        }

        #endregion Methods.
    }
}
