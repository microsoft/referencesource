#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="InitializeRequestEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.WebForms.InitializeRequestEventArgs = function(request, postBackElement, updatePanelsToUpdate) {
    /// <summary>The arguments for the PageRequestManager's initializeRequest event.
    /// The initializeRequest event is raised when a request is being prepared and can be cancelled.</summary>
    /// <param name="request" type="Sys.Net.WebRequest">The web request to be packaged in this EventArgs.</param>
    /// <param name="postBackElement" domElement="true" mayBeNull="true">The postback element that initiated the async postback.</param>
    /// <param name="updatePanelsToUpdate" type="Array" elementType="String" mayBeNull="true" optional="true">
    /// A list of UniqueIDs for UpdatePanel controls that are requested to update their rendering by the client.
    /// Server-side processing may update additional UpdatePanels.</param>
    Sys.WebForms.InitializeRequestEventArgs.initializeBase(this);
    this._request = request;
    this._postBackElement = postBackElement;
    this._updatePanelsToUpdate = updatePanelsToUpdate;
}

Sys.WebForms.InitializeRequestEventArgs.prototype = {
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
        /// rendering by the client. Server-side processing may update additional UpdatePanels. The list can be
        /// modified by an event handler to add or remove UpdatePanel controls dynamically.</summary>
        /// <value type="Array" elementType="String"></value>
        return this._updatePanelsToUpdate ? Array.clone(this._updatePanelsToUpdate) : [];
    },
    set_updatePanelsToUpdate: function(value) {
        this._updated = true;
        this._updatePanelsToUpdate = value;
    }
}

Sys.WebForms.InitializeRequestEventArgs.registerClass('Sys.WebForms.InitializeRequestEventArgs', Sys.CancelEventArgs);
