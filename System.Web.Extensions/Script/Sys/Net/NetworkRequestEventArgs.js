#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="NetworkRequestEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Net.NetworkRequestEventArgs = function(webRequest) {
    /// <summary>This class is raised by the WebRequestManager when a WebRequest is about to be executed.</summary>
    /// <param name="webRequest" type="Sys.Net.WebRequest">The identifier for the event.</param>
    Sys.Net.NetworkRequestEventArgs.initializeBase(this);
    this._webRequest = webRequest;
}

Sys.Net.NetworkRequestEventArgs.prototype = {
    get_webRequest: function() {
        /// <summary>Returns the webRequest.</summary>
        /// <value type="Sys.Net.WebRequest">The request about to be executed.</value>
        return this._webRequest;
    }
}

Sys.Net.NetworkRequestEventArgs.registerClass('Sys.Net.NetworkRequestEventArgs', Sys.CancelEventArgs);
