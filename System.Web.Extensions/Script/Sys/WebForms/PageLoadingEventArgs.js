#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="PageLoadingEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.WebForms.PageLoadingEventArgs = function(panelsUpdating, panelsDeleting, dataItems) {
    /// <summary>The arguments for the PageRequestManager's pageLoading event.
    /// The pageLoading event is raised before the DOM has been updated.</summary>
    /// <param name="panelsUpdating" type="Array">An array of UpdatePanels that are going to be updated.</param>
    /// <param name="panelsDeleting" type="Array">An array of UpdatePanels that are going to be deleted.</param>
    /// <param name="dataItems" type="Object" mayBeNull="true"></param>
    Sys.WebForms.PageLoadingEventArgs.initializeBase(this);

    this._panelsUpdating = panelsUpdating;
    this._panelsDeleting = panelsDeleting;
    // Need to use "new Object()" instead of "{}", since the latter breaks code coverage.
    this._dataItems = dataItems || new Object();
}

Sys.WebForms.PageLoadingEventArgs.prototype = {
    get_dataItems : function() {
        /// <summary>Returns the data items registered by server controls.</summary>
        /// <value type="Object"/>
        return this._dataItems;
    },

    get_panelsDeleting: function() {
        /// <summary>Returns the array of UpdatePanels that are going to be deleted.</summary>
        /// <value type="Array"/>
        return this._panelsDeleting;
    },

    get_panelsUpdating: function() {
        /// <summary>Returns the array of UpdatePanels that are going to be updated.</summary>
        /// <value type="Array"/>
        return this._panelsUpdating;
    }
}

Sys.WebForms.PageLoadingEventArgs.registerClass('Sys.WebForms.PageLoadingEventArgs', Sys.EventArgs);
