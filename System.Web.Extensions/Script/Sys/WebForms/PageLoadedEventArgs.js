#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="PageLoadedEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.WebForms.PageLoadedEventArgs = function(panelsUpdated, panelsCreated, dataItems) {
    /// <summary>The arguments for the PageRequestManager's pageLoaded event.
    /// The pageLoaded event is raised after the DOM has been updated.</summary>
    /// <param name="panelsUpdated" type="Array">An array of UpdatePanels that were updated.</param>
    /// <param name="panelsCreated" type="Array">An array of UpdatePanels that were created.</param>
    /// <param name="dataItems" type="Object" mayBeNull="true"></param>
    Sys.WebForms.PageLoadedEventArgs.initializeBase(this);

    this._panelsUpdated = panelsUpdated;
    this._panelsCreated = panelsCreated;
    // Need to use "new Object()" instead of "{}", since the latter breaks code coverage.
    this._dataItems = dataItems || new Object();
}

Sys.WebForms.PageLoadedEventArgs.prototype = {
    get_dataItems : function() {
        /// <summary>Returns the data items registered by server controls.</summary>
        /// <value type="Object"/>
        return this._dataItems;
    },

    get_panelsCreated: function() {
        /// <summary>Returns the array of UpdatePanels that were created.</summary>
        /// <value type="Array"/>
        return this._panelsCreated;
    },

    get_panelsUpdated: function() {
        /// <summary>Returns the array of UpdatePanels that were updated.</summary>
        /// <value type="Array"/>
        return this._panelsUpdated;
    }
}

Sys.WebForms.PageLoadedEventArgs.registerClass('Sys.WebForms.PageLoadedEventArgs', Sys.EventArgs);
