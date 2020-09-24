#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Timer.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.UI.PermaLink = function(element) {
    Sys.UI.PermaLink.initializeBase(this,[element]);
}
Sys.UI.PermaLink.prototype = {
    dispose: function() {
        if (this._navigateHandler) {
            Sys.Application.remove_navigate(this._navigateHandler);
            delete this._navigateHandler;
        }
        Sys.UI.PermaLink.callBaseMethod(this,"dispose");
    },
    initialize: function() {
        Sys.UI.PermaLink.callBaseMethod(this,"initialize");
        this._navigateHandler = Function.createDelegate(this, this._onNavigate);
        Sys.Application.add_navigate(this._navigateHandler);
        this._onNavigate(this, Sys.EventArgs.Empty);
    },
    _onNavigate: function(sender, args) {
        this.get_element().href = '#' + Sys.Application.get_stateString();
    }
}
Sys.UI.PermaLink.registerClass("Sys.UI.PermaLink", Sys.UI.Control);