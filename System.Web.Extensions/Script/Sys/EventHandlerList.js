#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="EventHandlerList.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.EventHandlerList = function() {
    /// <summary>The EventHandlerList class contains a dictionary of multicast events.</summary>
    this._list = {};
}

Sys.EventHandlerList.prototype = {
    _addHandler: function(id, handler) {
        Array.add(this._getEvent(id, true), handler);
    },
    addHandler: function(id, handler) {
        /// <summary>The addHandler method adds a handler to the event identified by id.</summary>
        /// <param name="id" type="String">The identifier for the event.</param>
        /// <param name="handler" type="Function">The handler to add to the event.</param>
        this._addHandler(id, handler);
    },
    _removeHandler: function(id, handler) {
        var evt = this._getEvent(id);
        if (!evt) return;
        Array.remove(evt, handler);
    },
    removeHandler: function(id, handler) {
        /// <summary>The removeHandler method removes a handler to the event identified by id.</summary>
        /// <param name="id" type="String">The identifier for the event.</param>
        /// <param name="handler" type="Function">The handler to remove from the event.</param>
        this._removeHandler(id, handler);
    },
    getHandler: function(id) {
        /// <summary>
        ///     The getHandler method returns a single function that will call all
        ///     handlers sequentially for the specified event.
        /// </summary>
        /// <param name="id" type="String">The identifier for the event.</param>
        /// <returns type="Function">A function that will call each handler sequentially.</returns>
        var evt = this._getEvent(id);
        if (!evt || (evt.length === 0)) return null;
        evt = Array.clone(evt);
        return function(source, args) {
            for (var i = 0, l = evt.length; i < l; i++) {
                evt[i](source, args);
            }
        };
    },
    _getEvent: function(id, create) {
        if (!this._list[id]) {
            if (!create) return null;
            this._list[id] = [];
        }
        return this._list[id];
    }
}
Sys.EventHandlerList.registerClass('Sys.EventHandlerList');
