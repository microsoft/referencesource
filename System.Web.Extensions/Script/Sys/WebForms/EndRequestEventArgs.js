#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="EndRequestEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.WebForms.EndRequestEventArgs = function(error, dataItems, response) {
    /// <summary>The arguments for the PageRequestManager's endRequest event.
    /// The endRequest event is raised when a response has finished processing.</summary>
    /// <param name="error" type="Error" mayBeNull="true"></param>
    /// <param name="dataItems" type="Object" mayBeNull="true"></param>
    /// <param name="response" type="Sys.Net.WebRequestExecutor"></param>

    Sys.WebForms.EndRequestEventArgs.initializeBase(this);
    this._errorHandled = false;
    this._error = error;
    // Need to use "new Object()" instead of "{}", since the latter breaks code coverage.
    this._dataItems = dataItems || new Object();
    this._response = response;
}

Sys.WebForms.EndRequestEventArgs.prototype = {
    get_dataItems : function() {
        /// <summary>Returns the data items registered by server controls.</summary>
        /// <value type="Object"/>
        return this._dataItems;
    },

    get_error : function() {
        /// <summary>Returns the error object.</summary>
        /// <value type="Error"/>
        return this._error;
    },

    get_errorHandled : function() {
        /// <summary>Returns whether or not the error was handled.</summary>
        /// <value type="Boolean"/>
        return this._errorHandled;
    },
    set_errorHandled : function(value) {
        this._errorHandled = value;
    },

    get_response : function() {
        /// <summary>Returns the Sys.Net.WebRequestExecutor containg the response information.</summary>
        /// <value type="Sys.Net.WebRequestExecutor"/>
        return this._response;
    }
}

Sys.WebForms.EndRequestEventArgs.registerClass('Sys.WebForms.EndRequestEventArgs', Sys.EventArgs);
