#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="CancelEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.CancelEventArgs = function() {
    /// <summary>CancelEventArgs is the base class for classes containing event data, which can be used to cancel the event.</summary>
    Sys.CancelEventArgs.initializeBase(this);

    this._cancel = false;
}

Sys.CancelEventArgs.prototype = {
    get_cancel: function() {
        /// <summary>Returns whether the event should be cancelled.</summary>
        /// <value type="Boolean"/>
        return this._cancel;
    },
    set_cancel: function(value) {
        this._cancel = value;
    }
}

Sys.CancelEventArgs.registerClass('Sys.CancelEventArgs', Sys.EventArgs);
