#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Control.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.Control = function(element) {
    /// <param name="element" domElement="true">The DOM element the behavior is associated with.</param>
    ##DEBUG if (element.control !== null && typeof(element.control) !== 'undefined') throw Error.invalidOperation(Sys.Res.controlAlreadyDefined);
    Sys.UI.Control.initializeBase(this);

    this._element = element;
    element.control = this;
    // Add support for WAI-ARIA role property.
    var role = this.get_role();
    if (role) {
        element.setAttribute("role", role);
    }
}
Sys.UI.Control.prototype = {
    _parent: null,
    _visibilityMode: Sys.UI.VisibilityMode.hide,

    get_element: function() {
        /// <value domElement="true">The DOM element this behavior is associated with</value>
        return this._element;
    },
    get_id: function() {
        /// <value type="String"/>
        if (!this._element) return '';
        return this._element.id;
    },
    set_id: function(value) {
        throw Error.invalidOperation(Sys.Res.cantSetId);
    },
    get_parent: function() {
        /// <summary>
        ///   Returns the parent control for this control.
        ///   If it has never been set, it looks up the DOM to find the first parent element
        ///   that has a control associated with it.
        /// </summary>
        /// <value type="Sys.UI.Control"/>
        if (this._parent) return this._parent;
        if (!this._element) return null;
        
        var parentElement = this._element.parentNode;
        while (parentElement) {
            if (parentElement.control) {
                return parentElement.control;
            }
            parentElement = parentElement.parentNode;
        }
        return null;
    },
    set_parent: function(value) {
        #if DEBUG
        if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        var parents = [this];
        var current = value;
        while (current) {
            if (Array.contains(parents, current)) throw Error.invalidOperation(Sys.Res.circularParentChain);
            parents[parents.length] = current;
            current = current.get_parent();
        }
        #endif
        this._parent = value;
    },
    get_role: function() {
        /// <value type="String"></value>
        return null;
    },
    get_visibilityMode: function() {
        /// <value type="Sys.UI.VisibilityMode"/>
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        return Sys.UI.DomElement.getVisibilityMode(this._element);
    },
    set_visibilityMode: function(value) {
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        Sys.UI.DomElement.setVisibilityMode(this._element, value);
    },
    get_visible: function() {
        /// <value type="Boolean"/>
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        return Sys.UI.DomElement.getVisible(this._element);
    },
    set_visible: function(value) {
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        Sys.UI.DomElement.setVisible(this._element, value)
    },
    addCssClass: function(className) {
        /// <summary>Adds a CSS class to the control if it doesn't already have it.</summary>
        /// <param name="className" type="String">The name of the CSS class to add.</param>
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        Sys.UI.DomElement.addCssClass(this._element, className);
    },
    dispose: function() {
        Sys.UI.Control.callBaseMethod(this, 'dispose');
        if (this._element) {
            this._element.control = null;
            delete this._element;
        }
        if (this._parent) delete this._parent;
    },
    onBubbleEvent: function(source, args) {
        /// <param name="source">The object that triggered the event.</param>
        /// <param name="args" type="Sys.EventArgs">The event arguments.</param>
        /// <returns type="Boolean">
        ///  False, because the event was not handled and should bubble up further.
        ///  Derived classes should override that and return true whenever they handle the event to
        ///  prevent it from bubbling up.
        /// </returns>
        return false;
    },
    raiseBubbleEvent: function(source, args) {
        /// <param name="source">The object that triggered the event.</param>
        /// <param name="args" type="Sys.EventArgs">The event arguments.</param>
        this._raiseBubbleEvent(source, args);
    },
    _raiseBubbleEvent: function(source, args) {
        var currentTarget = this.get_parent();
        while (currentTarget) {
            if (currentTarget.onBubbleEvent(source, args)) {
                return;
            }
            currentTarget = currentTarget.get_parent();
        }
    },
    removeCssClass: function(className) {
        /// <summary>Removes a CSS class from the control.</summary>
        /// <param name="className" type="String">The name of the CSS class to remove.</param>
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        Sys.UI.DomElement.removeCssClass(this._element, className);
    },
    toggleCssClass: function(className) {
        /// <summary>Toggles a CSS class on and off on the control.</summary>
        /// <param name="className" type="String">The name of the CSS class to toggle.</param>
        ##DEBUG if (!this._element) throw Error.invalidOperation(Sys.Res.cantBeCalledAfterDispose);
        Sys.UI.DomElement.toggleCssClass(this._element, className);
    }
}
Sys.UI.Control.registerClass('Sys.UI.Control', Sys.Component);
