//---------------------------------------------------------------------
// <copyright file="HandleExceptionArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to provide data to the exception handling
//      process.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;

    /// <summary>Use this class to customize how exceptions are handled.</summary>
    public class HandleExceptionArgs
    {
        #region Private fields.

        /// <summary>Whether the response has already been written out.</summary>
        private readonly bool responseWritten;

        /// <summary>The MIME type used to write the response.</summary>
        private readonly string responseContentType;

        /// <summary>The <see cref="Exception"/> being handled.</summary>
        private Exception exception;

        /// <summary>Whether a verbose response is appropriate.</summary>
        private bool useVerboseErrors;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initalizes a new <see cref="HandleExceptionArgs"/> instance.</summary>
        /// <param name="exception">The <see cref="Exception"/> being handled.</param>
        /// <param name="responseWritten">Whether the response has already been written out.</param>
        /// <param name="contentType">The MIME type used to write the response.</param>
        /// <param name="verboseResponse">Whether a verbose response is appropriate.</param>
        internal HandleExceptionArgs(Exception exception, bool responseWritten, string contentType, bool verboseResponse)
        {
            this.exception = WebUtil.CheckArgumentNull(exception, "exception");
            this.responseWritten = responseWritten;
            this.responseContentType = contentType;
            this.useVerboseErrors = verboseResponse;
        }

        #endregion Constructors.

        #region Public properties.

        /// <summary>Gets or sets the <see cref="Exception"/> being handled.</summary>
        /// <remarks>This property may be null.</remarks>
        public Exception Exception
        {
            get { return this.exception; }
            set { this.exception = value; }
        }

        /// <summary>Gets the content type for response.</summary>
        public string ResponseContentType
        {
            get
            {
                return this.responseContentType;
            }
        }

        /// <summary>Gets the HTTP status code for the response.</summary>
        public int ResponseStatusCode
        {
            get
            {
                if (this.exception is DataServiceException)
                {
                    return ((DataServiceException)this.exception).StatusCode;
                }
                else
                {
                    return 500; // Internal Server Error.
                }
            }
        }

        /// <summary>Gets a value indicating whether the response has already been written out.</summary>
        public bool ResponseWritten
        {
            get { return this.responseWritten; }
        }

        /// <summary>Gets or sets whether a verbose response is appropriate.</summary>
        public bool UseVerboseErrors
        {
            get { return this.useVerboseErrors; }
            set { this.useVerboseErrors = value; }
        }

        #endregion Public properties.

        #region Internal properties.

        /// <summary>The value for the 'Allow' response header.</summary>
        internal string ResponseAllowHeader
        {
            get
            {
                if (this.exception is DataServiceException)
                {
                    return ((DataServiceException)this.exception).ResponseAllowHeader;
                }
                else
                {
                    return null;
                }
            }
        }

        #endregion Internal properties.
    }
}
