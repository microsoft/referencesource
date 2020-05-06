#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="WebRequestExecutor.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Net.WebRequestExecutor = function() {
    /// <summary>Base class for WebRequestExecutors which handle the actual execution of a WebRequest</summary>
    this._webRequest = null;
    this._resultObject = null;
}

Sys.Net.WebRequestExecutor.prototype = {
    get_webRequest: function() {
        /// <summary>Gets the webRequest property.</summary>
        /// <value type="Sys.Net.WebRequest"/>
        return this._webRequest;
    },

    _set_webRequest: function(value) {
        #if DEBUG
        if (this.get_started()) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOnceStarted, 'set_webRequest'));
        }
        #endif

        this._webRequest = value;
    },

    // properties
    get_started: function() {
        /// <summary>Returns whether the executor has started.</summary>
        /// <value type="Boolean"/>
        throw Error.notImplemented();
    },

    get_responseAvailable: function() {
        /// <summary>Returns whether the executor has successfully completed.</summary>
        /// <value type="Boolean"/>
        throw Error.notImplemented();
    },

    get_timedOut: function() {
        /// <summary>Returns whether the executor has timed out.</summary>
        /// <value type="Boolean"/>
        throw Error.notImplemented();
    },
    get_aborted: function() {
        /// <summary>Returns whether the executor has aborted.</summary>
        /// <value type="Boolean"/>
        throw Error.notImplemented();
    },
    get_responseData: function() {
        /// <summary>Returns the response data.</summary>
        /// <value type="String"/>
        throw Error.notImplemented();
    },
    get_statusCode: function() {
        /// <summary>Returns the status code for the response.</summary>
        /// <value type="Number"/>
        throw Error.notImplemented();
    },
    get_statusText: function() {
        /// <summary>Returns the status text for the response.</summary>
        /// <value type="String"/>
        throw Error.notImplemented();
    },
    get_xml: function() {
        /// <summary>Returns the response in xml format.</summary>
        /// <value/>
        throw Error.notImplemented();
    },
    get_object: function() {
        /// <summary>Returns the JSON evaled object of the response.</summary>
        /// <value>The JSON eval'd response.</value>
        if (!this._resultObject) {
            this._resultObject = Sys.Serialization.JavaScriptSerializer.deserialize(this.get_responseData());
        }
        return this._resultObject;
    },

    // methods
    executeRequest: function() {
        /// <summary>Begins execution of the request.</summary>
        throw Error.notImplemented();
    },
    abort: function() {
        /// <summary>Aborts the request.</summary>
        throw Error.notImplemented();
    },
    getResponseHeader: function(header) {
        /// <summary>Returns a response header.</summary>
        /// <param name="header" type="String">The requested header.</param>
        throw Error.notImplemented();
    },
    getAllResponseHeaders: function() {
        /// <summary>Returns all the responses header.</summary>
        throw Error.notImplemented();
    }
}
Sys.Net.WebRequestExecutor.registerClass('Sys.Net.WebRequestExecutor');
