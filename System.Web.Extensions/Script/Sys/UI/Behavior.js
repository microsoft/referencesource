#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Behavior.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.Behavior = function(element) {
    /// <param name="element" domElement="true">The DOM element the behavior is associated with.</param>
    Sys.UI.Behavior.initializeBase(this);

    this._element = element;

    var behaviors = element._behaviors;
    if (!behaviors) {
        element._behaviors = [this];
    }
    else {
        behaviors[behaviors.length] = this;
    }
}
Sys.UI.Behavior.prototype = {
    _name: null,
    get_element: function() {
        /// <value domElement="true">The DOM element this behavior is associated with</value>
        return this._element;
    },
    get_id: function() {
        /// <value type="String"/>
        var baseId = Sys.UI.Behavior.callBaseMethod(this, 'get_id');
        if (baseId) return baseId;
        if (!this._element || !this._element.id) return '';
        return this._element.id + '$' + this.get_name();
    },
    get_name: function() {
        /// <value type="String"/>
        if (this._name) return this._name;
        var name = Object.getTypeName(this);
        var i = name.lastIndexOf('.');
        if (i !== -1) name = name.substr(i + 1);
        if (!this.get_isInitialized()) this._name = name;
        return name;
    },
    set_name: function(value) {
        #if DEBUG
        if ((value === '') || (value.charAt(0) === ' ') || (value.charAt(value.length - 1) === ' '))
            throw Error.argument('value', Sys.Res.invalidId);
        if (typeof(this._element[value]) !== 'undefined')
            throw Error.invalidOperation(String.format(Sys.Res.behaviorDuplicateName, value));
        if (this.get_isInitialized()) throw Error.invalidOperation(Sys.Res.cantSetNameAfterInit);
        #endif
        this._name = value;
    },
    initialize: function() {
        Sys.UI.Behavior.callBaseMethod(this, 'initialize');
        var name = this.get_name();
        if (name) this._element[name] = this;
    },
    dispose: function() {
        Sys.UI.Behavior.callBaseMethod(this, 'dispose');
        var e = this._element;
        if (e) {
            var name = this.get_name();
            if (name) {
                e[name] = null;
            }
            var behaviors = e._behaviors;
            Array.remove(behaviors, this);
            if (behaviors.length === 0) {
                e._behaviors = null;
            }
            delete this._element;
        }
    }
}
Sys.UI.Behavior.registerClass('Sys.UI.Behavior', Sys.Component);

Sys.UI.Behavior.getBehaviorByName = function(element, name) {
    /// <summary>Gets a behavior with the specified name from the dom element.</summary>
    /// <param name="element" domElement="true">The DOM element to inspect.</param>
    /// <param name="name" type="String">The name of the behavior to look for.</param>
    /// <returns type="Sys.UI.Behavior" mayBeNull="true">
    ///   The behaviors or null if it was not found.
    /// </returns>
    var b = element[name];
    return (b && Sys.UI.Behavior.isInstanceOfType(b)) ? b : null;
}

Sys.UI.Behavior.getBehaviors = function(element) {
    /// <summary>Gets a collection containing the behaviors associated with an element.</summary>
    /// <param name="element" domElement="true">The DOM element.</param>
    /// <returns type="Array" elementType="Sys.UI.Behavior">
    ///   An array containing the behaviors associated with the DOM element.
    /// </returns>
    if (!element._behaviors) return [];
    return Array.clone(element._behaviors);
}

Sys.UI.Behavior.getBehaviorsByType = function(element, type) {
    /// <summary>Gets an array of behaviors with the specified type from the dom element.</summary>
    /// <param name="element" domElement="true">The DOM element to inspect.</param>
    /// <param name="type" type="Type">The type of behavior to look for.</param>
    /// <returns type="Array" elementType="Sys.UI.Behavior">
    ///   An array containing the behaviors of the specified type found on the element.
    ///   The array is empty if no behavior of this type was found.
    /// </returns>
    var behaviors = element._behaviors;
    var results = [];
    if (behaviors) {
        for (var i = 0, l = behaviors.length; i < l; i++) {
            if (type.isInstanceOfType(behaviors[i])) {
                results[results.length] = behaviors[i];
            }
        }
    }
    return results;
}
