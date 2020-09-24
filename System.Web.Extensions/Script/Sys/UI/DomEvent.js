#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="DomEvent.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.DomEvent = function(eventObject) {
    /// <summary>A cross-browser object that represents event properties.</summary>
    /// <param name="eventObject">The browser-specific event object (window.event for IE).</param>
    /// <field name="altKey" type="Boolean"/>
    /// <field name="button" type="Sys.UI.MouseButton"/>
    /// <field name="charCode" type="Number" integer="true">The character code for the pressed key.</field>
    /// <field name="clientX" type="Number" integer="true"/>
    /// <field name="clientY" type="Number" integer="true"/>
    /// <field name="ctrlKey" type="Boolean"/>
    /// <field name="keyCode" type="Number" integer="true">The key code for the pressed key.</field>
    /// <field name="offsetX" type="Number" integer="true"/>
    /// <field name="offsetY" type="Number" integer="true"/>
    /// <field name="screenX" type="Number" integer="true"/>
    /// <field name="screenY" type="Number" integer="true"/>
    /// <field name="shiftKey" type="Boolean"/>
    /// <field name="target"/>
    /// <field name="type" type="String"/>
    var ev = eventObject;
    var etype = this.type = ev.type.toLowerCase();
    this.rawEvent = ev;
    this.altKey = ev.altKey;
    if (typeof(ev.button) !== 'undefined') {
        this.button = (typeof(ev.which) !== 'undefined') ? ev.button :
            (ev.button === 4) ? Sys.UI.MouseButton.middleButton :
            (ev.button === 2) ? Sys.UI.MouseButton.rightButton :
            Sys.UI.MouseButton.leftButton;
    }
    if (etype === 'keypress') {
        this.charCode = ev.charCode || ev.keyCode;
    }
    else if (ev.keyCode && (ev.keyCode === 46)) {
        this.keyCode = 127;
    }
    else {
        this.keyCode = ev.keyCode;
    }
    this.clientX = ev.clientX;
    this.clientY = ev.clientY;
    this.ctrlKey = ev.ctrlKey;
    this.target = ev.target ? ev.target : ev.srcElement;
    if (!etype.startsWith('key')) {
        if ((typeof(ev.offsetX) !== 'undefined') && (typeof(ev.offsetY) !== 'undefined')) {
            this.offsetX = ev.offsetX;
            this.offsetY = ev.offsetY;
        }
        else if (this.target && (this.target.nodeType !== 3) && (typeof(ev.clientX) === 'number')) {
            var loc = Sys.UI.DomElement.getLocation(this.target);
            var w = Sys.UI.DomElement._getWindow(this.target);
            this.offsetX = (w.pageXOffset || 0) + ev.clientX - loc.x;
            this.offsetY = (w.pageYOffset || 0) + ev.clientY - loc.y;
        }
    }
    this.screenX = ev.screenX;
    this.screenY = ev.screenY;
    this.shiftKey = ev.shiftKey;
}
Sys.UI.DomEvent.prototype = {
    preventDefault: function() {
        /// <summary>
        ///   Prevents the default event action from happening. For example, a textbox keydown event,
        ///   if suppressed, will prevent the character from being appended to the textbox.
        /// </summary>
        if (this.rawEvent.preventDefault) {
            this.rawEvent.preventDefault();
        }
        else if (window.event) {
            this.rawEvent.returnValue = false;
        }
    },
    stopPropagation: function() {
        /// <summary>Prevents the event from being propagated to parent elements.</summary>
        if (this.rawEvent.stopPropagation) {
            this.rawEvent.stopPropagation();
        }
        else if (window.event) {
            this.rawEvent.cancelBubble = true;
        }
    }
}
Sys.UI.DomEvent.registerClass('Sys.UI.DomEvent');

var $addHandler = Sys.UI.DomEvent.addHandler = function(element, eventName, handler, autoRemove) {
    /// <summary>A cross-browser way to add a DOM event handler to an element.</summary>
    /// <param name="element">The element or text node that exposes the event.</param>
    /// <param name="eventName" type="String">
    ///   The name of the event. Do not include the 'on' prefix, for example, 'click' instead of 'onclick'.
    /// </param>
    /// <param name="handler" type="Function">The event handler to add.</param>
    /// <param name="autoRemove" type="Boolean" optional="true">
    /// Whether the handler should be removed automatically when the element is disposed of,
    /// such as when an UpdatePanel refreshes, or Sys.Application.disposeElement is called.
    /// </param>
    #if DEBUG
    Sys.UI.DomEvent._ensureDomNode(element);
    if (eventName === "error") throw Error.invalidOperation(Sys.Res.addHandlerCantBeUsedForError);
    #endif
    if (!element._events) {
        element._events = {};
    }
    var eventCache = element._events[eventName];
    if (!eventCache) {
        element._events[eventName] = eventCache = [];
    }
    var browserHandler;
    if (element.addEventListener) {
        browserHandler = function(e) {
            return handler.call(element, new Sys.UI.DomEvent(e));
        }
        element.addEventListener(eventName, browserHandler, false);
    }
    else if (element.attachEvent) {
        browserHandler = function() {
            // window.event can be denied access in some rare circumstances (DevDiv 68929)
            var e = {};
            // We want to use the window for the event element, not the window for this script (DevDiv 63167)
            try {e = Sys.UI.DomElement._getWindow(element).event} catch(ex) {}
            return handler.call(element, new Sys.UI.DomEvent(e));
        }
        element.attachEvent('on' + eventName, browserHandler);
    }
    eventCache[eventCache.length] = {handler: handler, browserHandler: browserHandler, autoRemove: autoRemove };
    if (autoRemove) {
        var d = element.dispose;
        if (d !== Sys.UI.DomEvent._disposeHandlers) {
            // element.dispose called when an updatepanel refreshes or disposeElement called.
            element.dispose = Sys.UI.DomEvent._disposeHandlers;
            if (typeof(d) !== "undefined") {
                element._chainDispose = d;
            }
        }
    }
}

var $addHandlers = Sys.UI.DomEvent.addHandlers = function(element, events, handlerOwner, autoRemove) {
    /// <summary>
    ///   Adds a list of event handlers to an element.
    ///   If a handlerOwner is specified, delegates are created with each of the handlers.
    /// </summary>
    /// <param name="element">The element or text node that exposes the event.</param>
    /// <param name="events" type="Object">A dictionary of event handlers.</param>
    /// <param name="handlerOwner" optional="true">
    ///   The owner of the event handlers that will be the this pointer
    ///   for the delegates that will be created from the handlers.
    /// </param>
    /// <param name="autoRemove" type="Boolean" optional="true">
    /// Whether the handler should be removed automatically when the element is disposed of,
    /// such as when an UpdatePanel refreshes, or when Sys.Application.disposeElement is called.
    /// </param>
    ##DEBUG Sys.UI.DomEvent._ensureDomNode(element);
    for (var name in events) {
        var handler = events[name];
        ##DEBUG if (typeof(handler) !== 'function') throw Error.invalidOperation(Sys.Res.cantAddNonFunctionhandler);
        if (handlerOwner) {
            handler = Function.createDelegate(handlerOwner, handler);
        }
        $addHandler(element, name, handler, autoRemove || false);
    }
}

var $clearHandlers = Sys.UI.DomEvent.clearHandlers = function(element) {
    /// <summary>
    ///   Clears all the event handlers that were added to the element.
    /// </summary>
    /// <param name="element">The element or text node.</param>
    ##DEBUG Sys.UI.DomEvent._ensureDomNode(element);
    Sys.UI.DomEvent._clearHandlers(element, false);
}

Sys.UI.DomEvent._clearHandlers = function(element, autoRemoving) {
    if (element._events) {
        var cache = element._events;
        for (var name in cache) {
            var handlers = cache[name];
            for (var i = handlers.length - 1; i >= 0; i--) {
                var entry = handlers[i];
                if (!autoRemoving || entry.autoRemove) {
                    $removeHandler(element, name, entry.handler);
                }
            }
        }
        element._events = null;
    }
}

Sys.UI.DomEvent._disposeHandlers = function() {
    Sys.UI.DomEvent._clearHandlers(this, true);
    var d = this._chainDispose, type = typeof(d);
    if (type !== "undefined") {
        this.dispose = d;
        this._chainDispose = null;
        if (type === "function") {
            this.dispose();
        }
    }
}

var $removeHandler = Sys.UI.DomEvent.removeHandler = function(element, eventName, handler) {
    /// <summary>A cross-browser way to remove a DOM event handler from an element.</summary>
    /// <param name="element">The element or text node that exposes the event.</param>
    /// <param name="eventName" type="String">
    ///   The name of the event. Do not include the 'on' prefix, for example, 'click' instead of 'onclick'.
    /// </param>
    /// <param name="handler" type="Function">The event handler to remove.</param>
    Sys.UI.DomEvent._removeHandler(element, eventName, handler);
}
Sys.UI.DomEvent._removeHandler = function(element, eventName, handler) {
    ##DEBUG Sys.UI.DomEvent._ensureDomNode(element);
    var browserHandler = null;
    ##DEBUG if ((typeof(element._events) !== 'object') || !element._events) throw Error.invalidOperation(Sys.Res.eventHandlerInvalid);
    var cache = element._events[eventName];
    ##DEBUG if (!(cache instanceof Array)) throw Error.invalidOperation(Sys.Res.eventHandlerInvalid);
    for (var i = 0, l = cache.length; i < l; i++) {
        if (cache[i].handler === handler) {
            browserHandler = cache[i].browserHandler;
            break;
        }
    }
    ##DEBUG if (typeof(browserHandler) !== 'function') throw Error.invalidOperation(Sys.Res.eventHandlerInvalid);
    if (element.removeEventListener) {
        element.removeEventListener(eventName, browserHandler, false);
    }
    else if (element.detachEvent) {
        element.detachEvent('on' + eventName, browserHandler);
    }
    cache.splice(i, 1);
}

#if DEBUG
Sys.UI.DomEvent._ensureDomNode = function(element) {
    // DevDiv Bugs 100697: Accessing element.document causes dynamic script nodes to load prematurely.
    // DevDiv Bugs 124696: Firefox warns on undefined property element.tagName, added first part of IF
    // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
    if (element.tagName && (element.tagName.toUpperCase() === "SCRIPT")) return;
    
    var doc = element.ownerDocument || element.document || element;
    // Can't use _getWindow here and compare to the element to check if it's a window
    // because the object Safari exposes as document.defaultView is not the window (DevDiv 100229)
    // Looking at the document property instead to include window in DOM nodes, then comparing to the
    // document for this element and finally look for the nodeType property.
    if ((typeof(element.document) !== 'object') && (element != doc) && (typeof(element.nodeType) !== 'number')) {
        throw Error.argument("element", Sys.Res.argumentDomNode);
    }
}
#endif
