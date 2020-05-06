#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Component.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Component = function() {
    /// <summary>Base class for Control, Behavior and any object that wants its lifetime to be managed.</summary>
    if (Sys.Application) Sys.Application.registerDisposableObject(this);
}
Sys.Component.prototype = {
    _id: null,
    ##DEBUG _idSet: false,
    _initialized: false,
    _updating: false,
    get_events: function() {
        /// <value type="Sys.EventHandlerList">
        ///   The collection of event handlers for this behavior.
        ///   This property should only be used by derived behaviors
        ///   and should not be publicly called by other code.
        /// </value>
        if (!this._events) {
            this._events = new Sys.EventHandlerList();
        }
        return this._events;
    },
    get_id: function() {
        /// <value type="String"/>
        return this._id;
    },
    set_id: function(value) {
        #if DEBUG
        if (this._idSet) throw Error.invalidOperation(Sys.Res.componentCantSetIdTwice);
        this._idSet = true;
        var oldId = this.get_id();
        if (oldId && Sys.Application.findComponent(oldId)) throw Error.invalidOperation(Sys.Res.componentCantSetIdAfterAddedToApp);
        #endif
        this._id = value;
    },
    get_isInitialized: function() {
        /// <value type="Boolean"/>
        return this._initialized;
    },
    get_isUpdating: function() {
        /// <value type="Boolean"/>
        return this._updating;
    },
    add_disposing: function(handler) {
        this.get_events().addHandler("disposing", handler);
    },
    remove_disposing: function(handler) {
        this.get_events().removeHandler("disposing", handler);
    },
    add_propertyChanged: function(handler) {
        this.get_events().addHandler("propertyChanged", handler);
    },
    remove_propertyChanged: function(handler) {
        this.get_events().removeHandler("propertyChanged", handler);
    },
    beginUpdate: function() {
        this._updating = true;
    },
    dispose: function() {
        if (this._events) {
            var handler = this._events.getHandler("disposing");
            if (handler) {
                handler(this, Sys.EventArgs.Empty);
            }
        }
        delete this._events;
        Sys.Application.unregisterDisposableObject(this);
        Sys.Application.removeComponent(this);
    },
    endUpdate: function() {
        this._updating = false;
        if (!this._initialized) this.initialize();
        this.updated();
    },
    initialize: function() {
        this._initialized = true;
    },
    raisePropertyChanged: function(propertyName) {
        /// <summary>Raises a change notification event.</summary>
        /// <param name="propertyName" type="String">The name of the property that changed.</param>
        if (!this._events) return;
        var handler = this._events.getHandler("propertyChanged");
        if (handler) {
            handler(this, new Sys.PropertyChangedEventArgs(propertyName));
        }
    },
    updated: function() {
    }
}
Sys.Component.registerClass('Sys.Component', null, Sys.IDisposable, Sys.INotifyPropertyChange, Sys.INotifyDisposing);

function Sys$Component$_setProperties(target, properties) {
    /// <summary>Recursively sets properties on an object.</summary>
    /// <param name="target">The object on which to set the property values.</param>
    /// <param name="properties">A JSON object containing the property values.</param>
    var current;
    var targetType = Object.getType(target);
    var isObject = (targetType === Object) || (targetType === Sys.UI.DomElement);
    var isComponent = Sys.Component.isInstanceOfType(target) && !target.get_isUpdating();
    if (isComponent) target.beginUpdate();
    for (var name in properties) {
        var val = properties[name];
        var getter = isObject ? null : target["get_" + name];
        if (isObject || typeof(getter) !== 'function') {
            // No getter, looking for an existing field.
            var targetVal = target[name];
            ##DEBUG if (!isObject && typeof(targetVal) === 'undefined') throw Error.invalidOperation(String.format(Sys.Res.propertyUndefined, name));
            if (!val || (typeof(val) !== 'object') || (isObject && !targetVal)) {
                target[name] = val;
            }
            else {
                Sys$Component$_setProperties(targetVal, val);
            }
        }
        else {
            var setter = target["set_" + name];
            if (typeof(setter) === 'function') {
                // The setter exists, using it in all cases.
                setter.apply(target, [val]);
            }
            else if (val instanceof Array) {
                // There is a getter but no setter and the value to set is an array. Adding to the existing array.
                current = getter.apply(target);
                ##DEBUG if (!(current instanceof Array)) throw new Error.invalidOperation(String.format(Sys.Res.propertyNotAnArray, name));
                for (var i = 0, j = current.length, l= val.length; i < l; i++, j++) {
                    current[j] = val[i];
                }
            }
            else if ((typeof(val) === 'object') && (Object.getType(val) === Object)) {
                // There is a getter but no setter and the value to set is a plain object. Adding to the existing object.
                current = getter.apply(target);
                ##DEBUG if ((typeof(current) === 'undefined') || (current === null)) throw new Error.invalidOperation(String.format(Sys.Res.propertyNullOrUndefined, name));
                Sys$Component$_setProperties(current, val);
            }
            #if DEBUG
            else {
                // No setter, and the value is not an array or object, throwing.
                throw new Error.invalidOperation(String.format(Sys.Res.propertyNotWritable, name));
            }
            #endif
        }
    }
    if (isComponent) target.endUpdate();
}

function Sys$Component$_setReferences(component, references) {
    for (var name in references) {
        var setter = component["set_" + name];
        var reference = $find(references[name]);
        #if DEBUG
        if (typeof(setter) !== 'function') throw new Error.invalidOperation(String.format(Sys.Res.propertyNotWritable, name));
        if (!reference) throw Error.invalidOperation(String.format(Sys.Res.referenceNotFound, references[name]));
        #endif
        setter.apply(component, [reference]);
    }
}

var $create = Sys.Component.create = function(type, properties, events, references, element) {
    /// <summary>
    ///   Instantiates a component of the specified type, attaches it to the specified element if it's
    ///   a Control or Behavior, sets the properties as described by the specified JSON object,
    ///   then calls initialize.
    /// </summary>
    /// <param name="type" type="Type">The type of the component to create.</param>
    /// <param name="properties" optional="true" mayBeNull="true">
    ///   A JSON object that describes the properties and their values.
    /// </param>
    /// <param name="events" optional="true" mayBeNull="true">
    ///   A JSON object that describes the events and their handlers.
    /// </param>
    /// <param name="references" optional="true" mayBeNull="true">
    ///   A JSON object that describes the properties that are references to other components.
    ///   The contents of this object consists of name/id pairs.
    ///   If in a two-pass creation, the setting of these properties will be delayed until the second pass.
    /// </param>
    /// <param name="element" domElement="true" optional="true" mayBeNull="true">
    ///   The DOM element the component must be attached to.
    /// </param>
    /// <returns type="Sys.UI.Component">The component instance.</returns>
    #if DEBUG
    if (!type.inheritsFrom(Sys.Component)) {
        throw Error.argument('type', String.format(Sys.Res.createNotComponent, type.getName()));
    }
    if (type.inheritsFrom(Sys.UI.Behavior) || type.inheritsFrom(Sys.UI.Control)) {
        if (!element) throw Error.argument('element', Sys.Res.createNoDom);
    }
    else if (element) throw Error.argument('element', Sys.Res.createComponentOnDom);
    #endif
    var component = (element ? new type(element): new type());
    var app = Sys.Application;
    var creatingComponents = app.get_isCreatingComponents();

    component.beginUpdate();
    if (properties) {
        Sys$Component$_setProperties(component, properties);
    }
    if (events) {
        for (var name in events) {
            #if DEBUG
            if (!(component["add_" + name] instanceof Function)) throw new Error.invalidOperation(String.format(Sys.Res.undefinedEvent, name));
            if (!(events[name] instanceof Function)) throw new Error.invalidOperation(Sys.Res.eventHandlerNotFunction);
            #endif
            component["add_" + name](events[name]);
        }
    }

    if (component.get_id()) {
        app.addComponent(component);
    }
    if (creatingComponents) {
        // DevDiv 81690: Do not add to createdComponent list unless we are in 2 pass mode,
        // which is during the first GET and on partial updates. 
        app._createdComponents[app._createdComponents.length] = component;
        if (references) {
            app._addComponentToSecondPass(component, references);
        }
        else {
            component.endUpdate();
        }
    }
    else {
        if (references) {
            Sys$Component$_setReferences(component, references);
        }
        component.endUpdate();
    }

    return component;
}
