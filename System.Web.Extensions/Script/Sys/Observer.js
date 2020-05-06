Sys.Observer = function() {
    #if DEBUG
    throw Error.invalidOperation();
    #endif
}
Sys.Observer.registerClass("Sys.Observer");

Sys.Observer.makeObservable = function(target) {
    /// <summary>Makes an object directly observable by adding observable methods to it.</summary>
    /// <param name="target" mayBeNull="false">The object, array, or DOM element to make observable.</param>
    /// <returns>The observable object.</returns>
    var isArray = target instanceof Array,
        o = Sys.Observer;
    ##DEBUG Sys.Observer._ensureObservable(target);
    if (target.setValue === o._observeMethods.setValue) return target;
    o._addMethods(target, o._observeMethods);
    if (isArray) {
        o._addMethods(target, o._arrayMethods);
    }
    return target;
}
#if DEBUG
Sys.Observer._ensureObservable = function(target) {
    var type = typeof target;
    if ((type === "string") || (type === "number") || (type === "boolean") || (type === "date")) {
        throw Error.invalidOperation(String.format(Sys.Res.notObservable, type));
    }
}
#endif
Sys.Observer._addMethods = function(target, methods) {
    for (var m in methods) {
        #if DEBUG
        if (target[m] && (target[m] !== methods[m])) {
            throw Error.invalidOperation(String.format(Sys.Res.observableConflict, m));
        }
        #endif
        target[m] = methods[m];
    }
}
// Make use of private version for significant perf improvement in Binding.
// See references of these functions in binding.js for more details.
Sys.Observer._addEventHandler = function(target, eventName, handler) {
    Sys.Observer._getContext(target, true).events._addHandler(eventName, handler);
}
Sys.Observer.addEventHandler = function(target, eventName, handler) {
    /// <summary>Adds an observable event handler to the target.</summary>
    /// <param name="target"></param>
    /// <param name="eventName" type="String"></param>
    /// <param name="handler" type="Function"></param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._addEventHandler(target, eventName, handler);
}
// Make use of private version for significant perf improvement in Binding.
// See references of these functions in binding.js for more details.
Sys.Observer._removeEventHandler = function(target, eventName, handler) {
    Sys.Observer._getContext(target, true).events._removeHandler(eventName, handler);
}
Sys.Observer.removeEventHandler = function(target, eventName, handler) {
    /// <summary>Adds an observable event handler to the target.</summary>
    /// <param name="target"></param>
    /// <param name="eventName" type="String"></param>
    /// <param name="handler" type="Function"></param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._removeEventHandler(target, eventName, handler);
}
Sys.Observer.raiseEvent = function(target, eventName, eventArgs) {
    /// <summary>Raises an observable event on the target.</summary>
    /// <param name="target"></param>
    /// <param name="eventName" type="String"></param>
    /// <param name="eventArgs" type="Sys.EventArgs"></param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    var ctx = Sys.Observer._getContext(target);
    if (!ctx) return;
    var handler = ctx.events.getHandler(eventName);
    if (handler) {
        handler(target, eventArgs);
    }
}
Sys.Observer.addPropertyChanged = function(target, handler) {
    /// <summary>Adds a propertyChanged event handler to the target.</summary>
    /// <param name="target" mayBeNull="false">The object to observe.</param>
    /// <param name="handler" type="Function">The event handler.</param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._addEventHandler(target, "propertyChanged", handler);
}
Sys.Observer.removePropertyChanged = function(target, handler) {
    /// <summary>Removes a propertyChanged event handler from the target.</summary>
    /// <param name="target" mayBeNull="false">The object to observe.</param>
    /// <param name="handler" type="Function">The event handler.</param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._removeEventHandler(target, "propertyChanged", handler);
}
Sys.Observer.beginUpdate = function(target) {
    /// <summary></summary>
    /// <param name="target" mayBeNull="false"></param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._getContext(target, true).updating = true;
}
Sys.Observer.endUpdate = function(target) {
    /// <summary></summary>
    /// <param name="target" mayBeNull="false"></param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    var ctx = Sys.Observer._getContext(target);
    if (!ctx || !ctx.updating) return;
    ctx.updating = false;
    var dirty = ctx.dirty;
    ctx.dirty = false;
    if (dirty) {
        if (target instanceof Array) {
            var changes = ctx.changes;
            ctx.changes = null;
            Sys.Observer.raiseCollectionChanged(target, changes);
        }
        Sys.Observer.raisePropertyChanged(target, "");
    }
}
Sys.Observer.isUpdating = function(target) {
    /// <summary></summary>
    /// <param name="target" mayBeNull="false"></param>
    /// <returns type="Boolean"></returns>
    ##DEBUG Sys.Observer._ensureObservable(target);
    var ctx = Sys.Observer._getContext(target);
    return ctx ? ctx.updating : false;
}
// Make use of private version for significant perf improvement in Binding.
// See references of these functions in binding.js for more details.
Sys.Observer._setValue = function(target, propertyName, value) {
    var getter, setter, mainTarget = target, path = propertyName.split('.');
    for (var i = 0, l = (path.length - 1); i < l ; i++) {
        var name = path[i];
        getter = target["get_" + name]; 
        if (typeof (getter) === "function") {
            target = getter.call(target);
        }
        else {
            target = target[name];
        }
        var type = typeof (target);
        if ((target === null) || (type === "undefined")) {
            throw Error.invalidOperation(String.format(Sys.Res.nullReferenceInPath, propertyName));
        }
    }    
    var currentValue, lastPath = path[l];
    getter = target["get_" + lastPath];
    setter = target["set_" + lastPath];
    if (typeof(getter) === 'function') {
        currentValue = getter.call(target);
    }
    else {
        currentValue = target[lastPath];
    }
    if (typeof(setter) === 'function') {
        setter.call(target, value);
    }
    else {
        target[lastPath] = value;
    }
    if (currentValue !== value) {
        var ctx = Sys.Observer._getContext(mainTarget);
        if (ctx && ctx.updating) {
            ctx.dirty = true;
            return;
        };
        Sys.Observer.raisePropertyChanged(mainTarget, path[0]);
    }
}
Sys.Observer.setValue = function(target, propertyName, value) {
    /// <summary>Sets a property or field on the target in an observable manner.</summary>
    /// <param name="target" mayBeNull="false">The object to set a property on.</param>
    /// <param name="propertyName" type="String">The name of the property to field to set.</param>
    /// <param name="value" mayBeNull="true">The value to set.</param>
    ##DEBUG Sys.Observer._ensureObservable(target);
    Sys.Observer._setValue(target, propertyName, value);
}
Sys.Observer.raisePropertyChanged = function(target, propertyName) {
    /// <summary>Raises a change notification event.</summary>
    /// <validationOptions enabled="false"/>
    /// <param name="target" mayBeNull="false">The object to raise the event on.</param>
    /// <param name="propertyName" type="String">The name of the property that changed.</param>
    Sys.Observer.raiseEvent(target, "propertyChanged", new Sys.PropertyChangedEventArgs(propertyName));
}

Sys.Observer.addCollectionChanged = function(target, handler) {
    /// <summary></summary>
    /// <param name="target" type="Array" elementMayBeNull="true"></param>
    /// <param name="handler" type="Function"></param>
    Sys.Observer._addEventHandler(target, "collectionChanged", handler);
}
Sys.Observer.removeCollectionChanged = function(target, handler) {
    /// <summary></summary>
    /// <param name="target" type="Array" elementMayBeNull="true"></param>
    /// <param name="handler" type="Function"></param>
    Sys.Observer._removeEventHandler(target, "collectionChanged", handler);
}
Sys.Observer._collectionChange = function(target, change) {
    var ctx = Sys.Observer._getContext(target);
    if (ctx && ctx.updating) {
        ctx.dirty = true;
        var changes = ctx.changes;
        if (!changes) {
            ctx.changes = changes = [change];
        }
        else {
            changes.push(change);
        }
    }
    else {
        Sys.Observer.raiseCollectionChanged(target, [change]);
        Sys.Observer.raisePropertyChanged(target, 'length');
    }
}
Sys.Observer.add = function(target, item) {
    /// <summary>Adds an item to the collection in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to add to.</param>
    /// <param name="item" mayBeNull="true">The item to add.</param>
    var change = new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.add, [item], target.length);
    Array.add(target, item);
    Sys.Observer._collectionChange(target, change);
}
Sys.Observer.addRange = function(target, items) {
    /// <summary>Adds items to the collection in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to add to.</param>
    /// <param name="items" type="Array" elementMayBeNull="true">The array of items to add.</param>
    var change = new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.add, items, target.length);
    Array.addRange(target, items);
    Sys.Observer._collectionChange(target, change);
}
Sys.Observer.clear = function(target) {
    /// <summary>Clears the array of its elements in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to clear.</param>
    var oldItems = Array.clone(target);
    Array.clear(target);
    Sys.Observer._collectionChange(target, new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.reset, null, -1, oldItems, 0));
}
Sys.Observer.insert = function(target, index, item) {
    /// <summary>Inserts an item at the specified index in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to insert into.</param>
    /// <param name="index" type="Number" integer="true">The index where the item will be inserted.</param>
    /// <param name="item" mayBeNull="true">The item to insert.</param>
    Array.insert(target, index, item);
    Sys.Observer._collectionChange(target, new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.add, [item], index));
}
Sys.Observer.remove = function(target, item) {
    /// <summary>Removes the first occurence of an item from the array in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to remove from.</param>
    /// <param name="item" mayBeNull="true">The item to remove.</param>
    /// <returns type="Boolean">True if the item was found.</returns>
    var index = Array.indexOf(target, item);
    if (index !== -1) {
        Array.remove(target, item);
        Sys.Observer._collectionChange(target, new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.remove, null, -1, [item], index));
        return true;
    }
    return false;
}
Sys.Observer.removeAt = function(target, index) {
    /// <summary>Removes the item at the specified index from the array in an observable manner.</summary>
    /// <param name="target" type="Array" elementMayBeNull="true">The array to remove from.</param>
    /// <param name="index" type="Number" integer="true">The index of the item to remove.</param>
    if ((index > -1) && (index < target.length)) {
        var item = target[index];
        Array.removeAt(target, index);
        Sys.Observer._collectionChange(target, new Sys.CollectionChange(Sys.NotifyCollectionChangedAction.remove, null, -1, [item], index));
    }
}
Sys.Observer.raiseCollectionChanged = function(target, changes) {
    /// <summary>Raises the collectionChanged event.</summary>
    /// <validationOptions enabled="false"/>
    /// <param name="target">The collection to raise the event on.</param>
    /// <param name="changes" type="Array" elementType="Sys.CollectionChange">A list of changes that were performed on the collection since the last event.</param>
    Sys.Observer.raiseEvent(target, "collectionChanged", new Sys.NotifyCollectionChangedEventArgs(changes));
}

// note: triple-slash comments in these methods works with the preprocessor despite them being defined in this unique way
Sys.Observer._observeMethods = {
    add_propertyChanged: function(handler) {
        Sys.Observer._addEventHandler(this, "propertyChanged", handler);
    },
    remove_propertyChanged: function(handler) {
        Sys.Observer._removeEventHandler(this, "propertyChanged", handler);
    },
    addEventHandler: function(eventName, handler) {
        /// <summary>Adds an observable event handler.</summary>
        /// <param name="eventName" type="String"></param>
        /// <param name="handler" type="Function"></param>
        Sys.Observer._addEventHandler(this, eventName, handler);
    },
    removeEventHandler: function(eventName, handler) {
        /// <summary>Removes an observable event handler.</summary>
        /// <param name="eventName" type="String"></param>
        /// <param name="handler" type="Function"></param>
        Sys.Observer._removeEventHandler(this, eventName, handler);
    },
    get_isUpdating: function() {
        /// <summary></summary>
        /// <validationOptions enabled="false"/>
        /// <returns type="Boolean"></returns>
        return Sys.Observer.isUpdating(this);
    },
    beginUpdate: function() {
        /// <summary></summary>
        /// <validationOptions enabled="false"/>
        Sys.Observer.beginUpdate(this);
    },
    endUpdate: function() {
        /// <summary></summary>
        /// <validationOptions enabled="false"/>
        Sys.Observer.endUpdate(this);
    },
    setValue: function(name, value) {
        /// <summary>Sets a property or field on the target in an observable manner.</summary>
        /// <param name="name" type="String">The name of the property to field to set.</param>
        /// <param name="value" mayBeNull="true">The value to set.</param>
        Sys.Observer._setValue(this, name, value);
    },
    raiseEvent: function(eventName, eventArgs) {
        /// <summary>Raises an observable event.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="eventName" type="String"></param>
        /// <param name="eventArgs" type="Sys.EventArgs"></param>
        Sys.Observer.raiseEvent(this, eventName, eventArgs);
    },
    raisePropertyChanged: function(name) {
        /// <summary>Raises a change notification event.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="name" type="String">The name of the property that changed.</param>
        Sys.Observer.raiseEvent(this, "propertyChanged", new Sys.PropertyChangedEventArgs(name));
    }
}
Sys.Observer._arrayMethods = {
    add_collectionChanged: function(handler) {
        Sys.Observer._addEventHandler(this, "collectionChanged", handler);
    },
    remove_collectionChanged: function(handler) {
        Sys.Observer._removeEventHandler(this, "collectionChanged", handler);
    },
    add: function(item) {
        /// <summary>Adds an item to the collection in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="item" mayBeNull="true">The item to add.</param>
        Sys.Observer.add(this, item);
    },
    addRange: function(items) {
        /// <summary>Adds items to the collection in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="items" type="Array" elementMayBeNull="true">The array of items to add.</param>
        Sys.Observer.addRange(this, items);
    },
    clear: function() {
        /// <summary>Clears the array of its elements in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        Sys.Observer.clear(this);
    },
    insert: function(index, item) { 
        /// <summary>Inserts an item at the specified index in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="index" type="Number" integer="true">The index where the item will be inserted.</param>
        /// <param name="item" mayBeNull="true">The item to insert.</param>
        Sys.Observer.insert(this, index, item);
    },
    remove: function(item) {
        /// <summary>Removes the first occurence of an item from the array in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="item" mayBeNull="true">The item to remove.</param>
        /// <returns type="Boolean">True if the item was found.</returns>
        return Sys.Observer.remove(this, item);
    },
    removeAt: function(index) {
        /// <summary>Removes the item at the specified index from the array in an observable manner.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="index" type="Number" integer="true">The index of the item to remove.</param>
        Sys.Observer.removeAt(this, index);
    },
    raiseCollectionChanged: function(changes) {
        /// <summary>Raises the collectionChanged event.</summary>
        /// <validationOptions enabled="false"/>
        /// <param name="changes" type="Array" elementType="Sys.CollectionChange">A list of changes that were performed on the collection since the last event.</param>
        Sys.Observer.raiseEvent(this, "collectionChanged", new Sys.NotifyCollectionChangedEventArgs(changes));
    }
}
Sys.Observer._getContext = function(obj, create) {
    var ctx = obj._observerContext;
    if (ctx) return ctx();
    if (create) {
        return (obj._observerContext = Sys.Observer._createContext())();
    }
    return null;
}
Sys.Observer._createContext = function() {
    // instead of attaching an EventHandlerList, etc directly onto the observed object, we attach a function
    // which returns it as a closure. This prevents the need to attach a field to the object which is not a 
    // function, which could have a negative impact on serializers, whereas they typically always skip functions.
    // It's also better for sparse arrays, where a for-var-in loop need only ignore functions.
    // Return an object instead of the handler list directly so we have a place to put other fields without having
    // to add separate getX() methods for each.
    var ctx = {
        events: new Sys.EventHandlerList()
    };
    return function() {
        return ctx;
    }
}
