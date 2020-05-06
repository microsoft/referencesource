#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="BeginRequestEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.WebForms.BeginRequestEventArgs = function(request, postBackElement, updatePanelsToUpdate) {
    /// <summary>The arguments for the PageRequestManager's beginRequest event.
    /// The beginRequest event is raised when a request is about to be made.</summary>
    /// <param name="request" type="Sys.Net.WebRequest">The web request for the EventArgs.</param>
    /// <param name="postBackElement" domElement="true" mayBeNull="true">The postback element that initiated the async postback.</param>
    /// <param name="updatePanelsToUpdate" type="Array" elementType="String" mayBeNull="true" optional="true">
    /// A list of UniqueIDs for UpdatePanel controls that are requested to update their rendering by the client.
    /// Server-side processing may update additional UpdatePanels.</param>
    Sys.WebForms.BeginRequestEventArgs.initializeBase(this);
    this._request = request;
    this._postBackElement = postBackElement;
    this._updatePanelsToUpdate = updatePanelsToUpdate;
}

Sys.WebForms.BeginRequestEventArgs.prototype = {
    get_postBackElement : function() {
        /// <summary>Returns the postback element that initiated the async postback.</summary>
        /// <value domElement="true" mayBeNull="true"/>
        return this._postBackElement;
    },
    get_request : function() {
        /// <summary>Returns the request object.</summary>
        /// <value type="Sys.Net.WebRequest"/>
        return this._request;
    },
    get_updatePanelsToUpdate: function() {
        /// <summary>Returns the list of UniqueIDs for UpdatePanels controls that are requested to update their
        /// rendering by the client. Server-side processing may update additional UpdatePanels.</summary>
        /// <value type="Array" elementType="String"></value>
        return this._updatePanelsToUpdate ? Array.clone(this._updatePanelsToUpdate) : [];
    }
}

Sys.WebForms.BeginRequestEventArgs.registerClass('Sys.WebForms.BeginRequestEventArgs', Sys.EventArgs);
