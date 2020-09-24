#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="WebServiceError.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
// Class returned to client if server throws an exception during ProcessRequest
Sys.Net.WebServiceError = function(timedOut, message, stackTrace, exceptionType, errorObject) {
    /// <summary>Represents a webservice error</summary>
    /// <param name="timedOut" type="Boolean">The error status enum.</param>
    /// <param name="message" type="String" mayBeNull="true">The error message.</param>
    /// <param name="stackTrace" type="String" mayBeNull="true" optional="true">The stack trace of the error.</param>
    /// <param name="exceptionType" type="String" mayBeNull="true" optional="true">The server exception type.</param>
    /// <param name="errorObject" type="Object" mayBeNull="true" optional="true">The raw error information.</param>
    this._timedOut = timedOut;
    this._message = message;
    this._stackTrace = stackTrace;
    this._exceptionType = exceptionType;
    this._errorObject = errorObject;
    this._statusCode = -1;
}

Sys.Net.WebServiceError.prototype = {
    get_timedOut: function() {
        /// <summary>True if the webservice failed due to timeout.</summary>
        /// <value type="Boolean">Whether the service failed due to timeout.</value>
        return this._timedOut;
    },

    get_statusCode: function() {
        /// <summary>Http status code of the response if any, defaults to -1 otherwise</summary>
        /// <value type="Number">Int representing the status of the response.</value>
        return this._statusCode;
    },

    get_message: function() {
        /// <summary>Error message</summary>
        /// <value type="String">Error message</value>
        return this._message;
    },

    get_stackTrace: function() {
        /// <summary>Stack trace of the error</summary>
        /// <value type="String">Stack trace of the error.</value>
        return this._stackTrace || "";
    },

    get_exceptionType: function() {
        /// <summary>Exception type of the error</summary>
        /// <value type="String">Exception type of the error.</value>
        return this._exceptionType || "";
    },
    
    get_errorObject: function() {
        /// <value type="Object">The raw error object returned by the service.</value>
        return this._errorObject || null;
    }
}
Sys.Net.WebServiceError.registerClass('Sys.Net.WebServiceError');
