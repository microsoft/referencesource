#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Application.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys._Application = function() {
    /// <summary locid="M:J#Sys.Application.#ctor"/>
    Sys._Application.initializeBase(this);

    this._disposableObjects = [];
    this._components = {};
    this._createdComponents = [];
    this._secondPassComponents = [];

    // dispose the app in window.unload
    this._unloadHandlerDelegate = Function.createDelegate(this, this._unloadHandler);
    Sys.UI.DomEvent.addHandler(window, "unload", this._unloadHandlerDelegate);
    // automatically initialize when the dom is ready
    this._domReady();
}
Sys._Application.prototype = {
    _creatingComponents: false,
    _disposing: false,
    _deleteCount: 0,

    get_isCreatingComponents: function() {
        /// <value type="Boolean" locid="P:J#Sys.Application.isCreatingComponents"/>
        return this._creatingComponents;
    },
    get_isDisposing: function() {
        /// <value type="Boolean" locid="P:J#Sys.Application.isDisposing"/>
        return this._disposing;
    },
    add_init: function(handler) {
        /// <summary locid="E:J#Sys.Application.init"/>
        if (this._initialized) {
            handler(this, Sys.EventArgs.Empty);
        }
        else {
            this.get_events().addHandler("init", handler);
        }
    },
    remove_init: function(handler) {
        this.get_events().removeHandler("init", handler);
    },
    add_load: function(handler) {
        /// <summary locid="E:J#Sys.Application.load"/>
        this.get_events().addHandler("load", handler);
    },
    remove_load: function(handler) {
        this.get_events().removeHandler("load", handler);
    },
    add_unload: function(handler) {
        /// <summary locid="E:J#Sys.Application.unload"/>
        this.get_events().addHandler("unload", handler);
    },
    remove_unload: function(handler) {
        this.get_events().removeHandler("unload", handler);
    },
    addComponent: function(component) {
        /// <summary locid="M:J#Sys.Application.addComponent">Adds a top-level component to the application.</summary>
        /// <param name="component" type="Sys.Component">The component to add.</param>
        #if DEBUG
        var id = component.get_id();
        if (!id) throw Error.invalidOperation(Sys.Res.cantAddWithoutId);
        if (typeof(this._components[id]) !== 'undefined') throw Error.invalidOperation(String.format(Sys.Res.appDuplicateComponent, id));
        this._components[id] = component;
        #else
        this._components[component.get_id()] = component;
        #endif
    },
    beginCreateComponents: function() {
        /// <summary locid="M:J#Sys.Application.beginCreateComponents"/>
        this._creatingComponents = true;
    },
    dispose: function() {
        /// <summary locid="M:J#Sys.Application.dispose"/>
        if (!this._disposing) {
            this._disposing = true;
            if (this._timerCookie) {
                window.clearTimeout(this._timerCookie);
                delete this._timerCookie;
            }
            if (this._endRequestHandler) {
                Sys.WebForms.PageRequestManager.getInstance().remove_endRequest(this._endRequestHandler);
                delete this._endRequestHandler;
            }
            if (this._beginRequestHandler) {
                Sys.WebForms.PageRequestManager.getInstance().remove_beginRequest(this._beginRequestHandler);
                delete this._beginRequestHandler;
            }
            if (window.pageUnload) {
                window.pageUnload(this, Sys.EventArgs.Empty);
            }
            var unloadHandler = this.get_events().getHandler("unload");
            if (unloadHandler) {
                unloadHandler(this, Sys.EventArgs.Empty);
            }
            var disposableObjects = Array.clone(this._disposableObjects);
            for (var i = 0, l = disposableObjects.length; i < l; i++) {
                var object = disposableObjects[i];
                // some entries are undefined, but we keep the density such that no more than 1000 are.
                if (typeof(object) !== "undefined") {
                    object.dispose();
                }
            }
            Array.clear(this._disposableObjects);

            Sys.UI.DomEvent.removeHandler(window, "unload", this._unloadHandlerDelegate);

            if (Sys._ScriptLoader) {
                var sl = Sys._ScriptLoader.getInstance();
                if(sl) {
                    sl.dispose();
                }
            }

            Sys._Application.callBaseMethod(this, 'dispose');
        }
    },
    disposeElement: function(element, childNodesOnly) {
        /// <summary>Disposes of control and behavior resources associated with an element and its child nodes.</summary>
        /// <param name="element">The element to dispose.</param>
        /// <param name="childNodesOnly" type="Boolean">Whether to dispose of the element and its child nodes or only its child nodes.</param>
        // note: cannot use domElement="true" for parameter because it fails for text nodes which we want to
        // allow here.
        if (element.nodeType === 1) {
            var i, allElements = element.getElementsByTagName("*"),
                length = allElements.length,
                children = new Array(length);
            // we must clone the array first as it is a reference to the actual live tree
            // and will change if disposed components modify the DOM
            // Also we cannot use Array.clone() because it is not actually a javascript array
            for (i = 0; i < length; i++) {
                children[i] = allElements[i];
            }
            for (i = length - 1; i >= 0; i--) {
                var child = children[i];
                // disposes of controls and behaviors attached to an element
                // logic adapted from PageRequestManager._destroyTree
                // This logic inlined because with a large number of DOM elements, the
                // overhead of calling a different method for each element adds up
                var d = child.dispose;
                if (d && typeof(d) === "function") {
                    child.dispose();
                }
                else {
                    var c = child.control;
                    if (c && typeof(c.dispose) === "function") {
                        c.dispose();
                    }
                }
                var list = child._behaviors;
                if (list) {
                    this._disposeComponents(list);
                }
                list = child._components;
                if (list) {
                    this._disposeComponents(list);
                    child._components = null;
                }
            }
            if (!childNodesOnly) {
                var d = element.dispose;
                if (d && typeof(d) === "function") {
                    element.dispose();
                }
                else {
                    var c = element.control;
                    if (c && typeof(c.dispose) === "function") {
                        c.dispose();
                    }
                }
                var list = element._behaviors;
                if (list) {
                    this._disposeComponents(list);
                }
                list = element._components;
                if (list) {
                    this._disposeComponents(list);
                    element._components = null;
                }
            }
        }
    },
    endCreateComponents: function() {
        /// <summary locid="M:J#Sys.Application.endCreateComponents"/>
        var components = this._secondPassComponents;
        for (var i = 0, l = components.length; i < l; i++) {
            var component = components[i].component;
            Sys$Component$_setReferences(component, components[i].references);
            component.endUpdate();
        }
        this._secondPassComponents = [];
        this._creatingComponents = false;
    },
    findComponent: function(id, parent) {
        /// <summary locid="M:J#Sys.Application.findComponent">
        ///   Finds top-level components that were added through addComponent if no parent is specified
        ///   or children of the specified parent. If parent is a component
        /// </summary>
        /// <param name="id" type="String">The id of the component to find.</param>
        /// <param name="parent" optional="true" mayBeNull="true">
        ///   The component or element that contains the component to find.
        ///   If not specified or null, the search is made on Application.
        /// </param>
        /// <returns type="Sys.Component" mayBeNull="true">The component, or null if it wasn't found.</returns>
        // Need to reference the application singleton directly beause the $find alias
        // points to the instance function without context. The 'this' pointer won't work here.
        return (parent ?
            ((Sys.IContainer.isInstanceOfType(parent)) ?
                parent.findComponent(id) :
                parent[id] || null) :
            Sys.Application._components[id] || null);
    },
    getComponents: function() {
        /// <summary locid="M:J#Sys.Application.getComponents"/>
        /// <returns type="Array" elementType="Sys.Component"/>
        var res = [];
        var components = this._components;
        for (var name in components) {
            res[res.length] = components[name];
        }
        return res;
    },
    initialize: function() {
        /// <summary locid="M:J#Sys.Application.initialize"/>
        if(!this.get_isInitialized() && !this._disposing) {
            Sys._Application.callBaseMethod(this, 'initialize');
            this._raiseInit();
            if (this.get_stateString) {
                // only execute if history has been imported
                if (Sys.WebForms && Sys.WebForms.PageRequestManager) {
                    // Subscribe to begin and end request events
                    this._beginRequestHandler = Function.createDelegate(this, this._onPageRequestManagerBeginRequest);
                    Sys.WebForms.PageRequestManager.getInstance().add_beginRequest(this._beginRequestHandler);
                    this._endRequestHandler = Function.createDelegate(this, this._onPageRequestManagerEndRequest);
                    Sys.WebForms.PageRequestManager.getInstance().add_endRequest(this._endRequestHandler);
                }
                var loadedEntry = this.get_stateString();
                if (loadedEntry !== this._currentEntry) {
                    this._navigate(loadedEntry);
                }
                else {
                    // Dev10 Bug: 599356
                    // necessary to ensure history is initialized, or there wont be any timer running to check for changes in the
                    // hash. For example, if the user navigates a few times, then goes back to the first state and refreshes the
                    // page, we will be loading the page with no state (this else case), yet they can click forward -- but we
                    // never initialized history so it goes unnoticed.
                    this._ensureHistory();
                }
            }
            this.raiseLoad();
        }
    },
    notifyScriptLoaded: function() {
        /// <summary locid="M:J#Sys.Application.notifyScriptLoaded">Called by referenced scripts to indicate that they have completed loading. [Obsolete]</summary>
    },
    registerDisposableObject: function(object) {
        /// <summary locid="M:J#Sys.Application.registerDisposableObject">Registers a disposable object with the application.</summary>
        /// <param name="object" type="Sys.IDisposable">The object to register.</param>
        if (!this._disposing) {
            var objects = this._disposableObjects,
                i = objects.length;
            objects[i] = object;
            object.__msdisposeindex = i;
        }
    },
    raiseLoad: function() {
        /// <summary locid="M:J#Sys.Application.raiseLoad"/>
        var h = this.get_events().getHandler("load");
        var args = new Sys.ApplicationLoadEventArgs(Array.clone(this._createdComponents), !!this._loaded);
        this._loaded = true;
        if (h) {
            h(this, args);
        }

        if (window.pageLoad) {
            window.pageLoad(this, args);
        }
        this._createdComponents = [];
    },
    removeComponent: function(component) {
        /// <summary locid="M:J#Sys.Application.removeComponent">Removes a top-level component from the application.</summary>
        /// <param name="component" type="Sys.Component">The component to remove.</param>
        var id = component.get_id();
        if (id) delete this._components[id];
    },
    unregisterDisposableObject: function(object) {
        /// <summary locid="M:J#Sys.Application.unregisterDisposableObject">Unregisters a disposable object from the application.</summary>
        /// <param name="object" type="Sys.IDisposable">The object to unregister.</param>
        if (!this._disposing) {
            var i = object.__msdisposeindex;
            if (typeof(i) === "number") {
                // delete it from the array instead of removing it, so the msdisposeindex
                // remains correct on the other existing objects
                // When the array is enumerated we use for/in to skip over the deleted entries.
                var disposableObjects = this._disposableObjects;
                delete disposableObjects[i];
                delete object.__msdisposeindex;
                if (++this._deleteCount > 1000) {
                    // periodically rebuild the array to remove the sparse elements
                    // to put a cap on the amount of memory it can consume
                    var newArray = [];
                    for (var j = 0, l = disposableObjects.length; j < l; j++) {
                        object = disposableObjects[j];
                        if (typeof(object) !== "undefined") {
                            object.__msdisposeindex = newArray.length;
                            newArray.push(object);
                        }
                    }
                    this._disposableObjects = newArray;
                    this._deleteCount = 0;
                }
            }
        }
    },
    _addComponentToSecondPass: function(component, references) {
        this._secondPassComponents[this._secondPassComponents.length] = {component: component, references: references};
    },
    _disposeComponents: function(list) {
        if (list) {
            for (var i = list.length - 1; i >= 0; i--) {
                var item = list[i];
                if (typeof(item.dispose) === "function") {
                    item.dispose();
                }
            }
        }
    },
    _domReady: function() {
        // note that the DOM might be ready immediately, in which case the application fires its
        // init and load events immediately during the constructor: new Sys._Application().
        // Since the instance of Sys.Application is set to that value, Sys.Application does not yet
        // exist when _domReady is called. No script here or called by any of this scripts should use
        // Sys.Application. Use 'app' or 'this' instead.
        var check, er, app = this;
        function init() { app.initialize(); }

        // window.onload is the safe fallback. The rest is to try and initialize sooner, since onload
        // only fires once all images and other binary content is downloaded.
        var onload = function() {
            Sys.UI.DomEvent.removeHandler(window, "load", onload);
            init();
        }
        Sys.UI.DomEvent.addHandler(window, "load", onload);
        
        if (document.addEventListener) {
            try {
                // try/catch in case the browser does not support DOMContentLoaded
                document.addEventListener("DOMContentLoaded", check = function() {
                    document.removeEventListener("DOMContentLoaded", check, false);
                    init();
                }, false);
            }
            catch (er) { }
        }
        else if (document.attachEvent) {
            if ((window == window.top) && document.documentElement.doScroll) {
                // timer/doscroll trick works only when not in a frame
                var timeout, el = document.createElement("div");
                check = function() {
                    try {
                        el.doScroll("left");
                    }
                    catch (er) {
                        timeout = window.setTimeout(check, 0);
                        return;
                    }
                    el = null;
                    init();
                }
                check();
            }
            else {
                // in a frame this is the only reliable way to fire before onload, however
                // testing has shown it is not much better than onload if at all better.
                // using a <script> element with defer="true" is much better, but you have to
                // document.write it for the 'defer' to work, and that wouldnt work if this
                // script is being loaded dynamically, a reasonable possibility.
                // There is no known way of detecting whether the script is loaded dynamically or not.
		document.attachEvent("onreadystatechange", check = function() {
                    if (document.readyState === "complete") {
                        document.detachEvent("onreadystatechange", check);
                        init();
                    }
                });


            }
        }

    },
    _raiseInit: function() {
        var handler = this.get_events().getHandler("init");
        if (handler) {
            this.beginCreateComponents();
            handler(this, Sys.EventArgs.Empty);
            this.endCreateComponents();
        }
    },
    _unloadHandler: function(event) {
        this.dispose();
    }
}
Sys._Application.registerClass('Sys._Application', Sys.Component, Sys.IContainer);

Sys.Application = new Sys._Application();

var $find = Sys.Application.findComponent;
