#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="ScriptLoader.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
// This ScriptLoader works by injecting script tags into the DOM sequentially, waiting for each script
// to finish loading before proceeding to the next one.
// It supports a timeout which applies to ALL scripts.

Sys._ScriptLoader = function() {
    this._scriptsToLoad = null;
    this._sessions = [];
    this._scriptLoadedDelegate = Function.createDelegate(this, this._scriptLoadedHandler);
}
Sys._ScriptLoader.prototype = {
    dispose: function() {
        this._stopSession();
        this._loading = false;
        if(this._events) {
            delete this._events;
        }
        this._sessions = null;
        this._currentSession = null;
        this._scriptLoadedDelegate = null;        
    },
    
    loadScripts: function(scriptTimeout, allScriptsLoadedCallback, scriptLoadFailedCallback, scriptLoadTimeoutCallback) {
        /// <summary>Begins loading scripts that have been queued.</summary>
        /// <param name="scriptTimeout" type="Number" integer="true">Timeout in seconds for loading all scripts.</param>
        /// <param name="allScriptsLoadedCallback" type="Function" mayBeNull="true">Callback for notification when all scripts have successfully loaded.</param>
        /// <param name="scriptLoadFailedCallback" type="Function" mayBeNull="true">Callback for notification when a script fails to load.</param>
        /// <param name="scriptLoadTimeoutCallback" type="Function" mayBeNull="true">Callback for notification when scripts have not finished loading within the given timeout.</param>
        var session = {
            allScriptsLoadedCallback: allScriptsLoadedCallback,
            scriptLoadFailedCallback: scriptLoadFailedCallback,
            scriptLoadTimeoutCallback: scriptLoadTimeoutCallback,
            scriptsToLoad: this._scriptsToLoad,
            scriptTimeout: scriptTimeout };
        this._scriptsToLoad = null;
        this._sessions[this._sessions.length] = session;
        
        if (!this._loading) {
            this._nextSession();
        }
    },
    
    queueCustomScriptTag: function(scriptAttributes) {
        /// <summary>Queues a script reference with the given set of custom script element attributes.</summary>
        /// <param name="scriptAttributes" mayBeNull="false">A JSON object that describtes the attributes to apply to the script element.</param>
        if(!this._scriptsToLoad) {
            this._scriptsToLoad = [];
        }
        Array.add(this._scriptsToLoad, scriptAttributes);
    },

    queueScriptBlock: function(scriptContent) {
        /// <summary>Queues a script reference with literal script.</summary>
        /// <param name="scriptContent" type="String" mayBeNull="false">Literal script to execute.</param>
        if(!this._scriptsToLoad) {
            this._scriptsToLoad = [];
        }
        Array.add(this._scriptsToLoad, {text: scriptContent});
    },

    queueScriptReference: function(scriptUrl, fallback) {
        /// <summary>Queues a script reference to the given script URL.</summary>
        /// <param name="scriptUrl" type="String" mayBeNull="false">URL to the script to reference.</param>
        /// <param name="fallback" mayBeNull="true" optional="true">Fallback path.</param>
        if(!this._scriptsToLoad) {
            this._scriptsToLoad = [];
        }
        Array.add(this._scriptsToLoad, {src: scriptUrl, fallback: fallback});
    },
    
    _createScriptElement: function(queuedScript) {
        var scriptElement = document.createElement('script');

        // Initialize default script type to JavaScript - but it might get overwritten
        // if a custom script tag has a different type attribute.
        scriptElement.type = 'text/javascript';

        // Apply script element attributes
        for (var attr in queuedScript) {
            scriptElement[attr] = queuedScript[attr];
        }
        
        return scriptElement;
    },
    
    _loadScriptsInternal: function() {
        var session = this._currentSession;
        // Load up the next script in the list
        if (session.scriptsToLoad && session.scriptsToLoad.length > 0) {
            var nextScript = Array.dequeue(session.scriptsToLoad);

            var onLoad = this._scriptLoadedDelegate;
            if (nextScript.fallback) {
                var fallback = nextScript.fallback;
                delete nextScript.fallback;
                
                var self = this;
                onLoad = function(scriptElement, loaded) {
                    loaded || (function() {
                        var fallbackScriptElement = self._createScriptElement({src: fallback});
                        self._currentTask = new Sys._ScriptLoaderTask(fallbackScriptElement, self._scriptLoadedDelegate);
                        self._currentTask.execute();
                    })();
                };
            }            

            // Inject a script element into the DOM
            var scriptElement = this._createScriptElement(nextScript);
            
            if (scriptElement.text && Sys.Browser.agent === Sys.Browser.Safari) {
                // Safari requires the inline script to be in the innerHTML attribute
                scriptElement.innerHTML = scriptElement.text;
                delete scriptElement.text;
            }            

            // AtlasWhidbey 36149: If they queue an empty script block "", we can't tell the difference between
            //                     a script block queue entry and a src entry with just if(!element.text).
            // dont use scriptElement.src --> FF resolves that to the current directory, IE leaves it blank.
            // nextScript.src is always a string if it's a non block script.
            if (typeof(nextScript.src) === "string") {
                // We only need to worry about timing out and loading if the script tag has a 'src'.
                this._currentTask = new Sys._ScriptLoaderTask(scriptElement, onLoad);
                // note: task is responsible for disposing of _itself_. This is necessary so that the ScriptLoader can continue
                //       with script loading after a script notifies it has loaded. The task sticks around until the dom element finishes
                //       completely, and disposes itself automatically.
                // note: its possible for notify to occur before this method even returns in IE! So it should remain the last possible statement.
                this._currentTask.execute();
            }
            else {
                // script is literal script, so just load the script by adding the new element to the DOM
                // DevDiv Bugs 146697: use lowercase names on getElementsByTagName to work with xhtml content type
#if DEBUG
                // DevDiv Bugs 146327: In debug mode, report useful error message for pages without <head> element
                var headElements = document.getElementsByTagName('head');
                if (headElements.length === 0) {
                     throw new Error.invalidOperation(Sys.Res.scriptLoadFailedNoHead);
                }
                else {
                     headElements[0].appendChild(scriptElement);
                }
#else
                document.getElementsByTagName('head')[0].appendChild(scriptElement);
#endif
                
                // DevDiv 157097: Removed setTimeout, assuming the script executed synchronously.
                // Previously the setTimeout worked around a Firefox bug where the script was not
                // executed immediately when the element is appended. However, that bug was fixed,
                // and the timeout causes a significant hit to performance. The timeout also causes
                // an inconsistency in the order of events between regular and async requests. With the
                // timeout, it would be possible for delayed operations in component initialize methods
                // to complete before the app loaded event is raised, where that is not possible on a 
                // GET because the loaded event is raised within the same execution chain that executes
                // the initialize methods.
                
                // cleanup (removes the script element in release mode).
                Sys._ScriptLoaderTask._clearScript(scriptElement);
                // Resume script loading progress.
                this._loadScriptsInternal();
            }
        }
        else {
            // When there are no more scripts to load, call the final event
            this._stopSession();
            var callback = session.allScriptsLoadedCallback;
            if(callback) {
                callback(this);
            }
            // and move on to the next session, if any
            this._nextSession();
        }
    },

    _nextSession: function() {
        if (this._sessions.length === 0) {
            this._loading = false;
            this._currentSession = null;
            return;
        }
        this._loading = true;
        
        var session = Array.dequeue(this._sessions);
        this._currentSession = session;
        #if DEBUG
        #else
        if(session.scriptTimeout > 0) {
            this._timeoutCookie = window.setTimeout(
                Function.createDelegate(this, this._scriptLoadTimeoutHandler), session.scriptTimeout * 1000);
        }
        #endif
        this._loadScriptsInternal();
    },

    _raiseError: function() {
        // Abort script loading and raise an error.
        var callback = this._currentSession.scriptLoadFailedCallback;
        var scriptElement = this._currentTask.get_scriptElement();
        this._stopSession();
        
        if(callback) {
            callback(this, scriptElement);
            this._nextSession();
        }
        else {
            this._loading = false;
            throw Sys._ScriptLoader._errorScriptLoadFailed(scriptElement.src);
        }
    },
    
    _scriptLoadedHandler: function(scriptElement, loaded) {
        // called by the ScriptLoaderTask when the script element has finished loading, which could be because it loaded or
        // errored out (for browsers that support the error event).
        if (loaded) {
            // script loaded and contained a single notify callback, move on to next script
            // DevDiv Bugs 123213: Note that scriptElement.src is read as un-htmlencoded, even if it was html encoded originally
            Array.add(Sys._ScriptLoader._getLoadedScripts(), scriptElement.src);
            this._currentTask.dispose();
            this._currentTask = null;
            this._loadScriptsInternal();
        }
        else {
            // script loaded with an error
            this._raiseError();
        }
    },
    #if DEBUG
    #else
    _scriptLoadTimeoutHandler: function() {
        var callback = this._currentSession.scriptLoadTimeoutCallback;
        this._stopSession();

        if(callback) {
            callback(this);
        }
        this._nextSession();
    },
    #endif
    _stopSession: function() {
        #if DEBUG
        #else
        if(this._timeoutCookie) {
            window.clearTimeout(this._timeoutCookie);
            this._timeoutCookie = null;
        }
        #endif
        if(this._currentTask) {
            this._currentTask.dispose();
            this._currentTask = null;
        }
    }    
}
Sys._ScriptLoader.registerClass('Sys._ScriptLoader', null, Sys.IDisposable);

Sys._ScriptLoader.getInstance = function() {
    var sl = Sys._ScriptLoader._activeInstance;
    if(!sl) {
        sl = Sys._ScriptLoader._activeInstance = new Sys._ScriptLoader();
    }
    return sl;
}

Sys._ScriptLoader.isScriptLoaded = function(scriptSrc) {
    // For Firefox we need to resolve the script src attribute
    // since the script elements already in the DOM are always
    // resolved. To do this we create a dummy element to see
    // what it would resolve to.
    // DevDiv Bugs 146697: Need to use lower-case tag name for xhtml content type.
    var dummyScript = document.createElement('script');
    dummyScript.src = scriptSrc;
    return Array.contains(Sys._ScriptLoader._getLoadedScripts(), dummyScript.src);
}

Sys._ScriptLoader.readLoadedScripts = function() {
    // enumerates the SCRIPT elements in the DOM and ensures we have their SRC's in the referencedScripts array.
    if(!Sys._ScriptLoader._referencedScripts) {
        var referencedScripts = Sys._ScriptLoader._referencedScripts = [];

        // DevDiv Bugs 146697: use lowercase names on getElementsByTagName to work with xhtml content type
        var existingScripts = document.getElementsByTagName('script');
        for (var i = existingScripts.length - 1; i >= 0; i--) {
            var scriptNode = existingScripts[i];
            var scriptSrc = scriptNode.src;
            if (scriptSrc.length) {
                if (!Array.contains(referencedScripts, scriptSrc)) {
                    Array.add(referencedScripts, scriptSrc);
                }
            }
        }
    }
}

Sys._ScriptLoader._errorScriptLoadFailed = function(scriptUrl) {
    var errorMessage;
    #if DEBUG
    // a much more detailed message is displayed in debug mode
    errorMessage = Sys.Res.scriptLoadFailedDebug;
    #else
    // a simplier error is displayed in release
    errorMessage = Sys.Res.scriptLoadFailed;
    #endif

    var displayMessage = "Sys.ScriptLoadFailedException: " + String.format(errorMessage, scriptUrl);
    var e = Error.create(displayMessage, {name: 'Sys.ScriptLoadFailedException', 'scriptUrl': scriptUrl });
    e.popStackFrame();
    return e;
}

Sys._ScriptLoader._getLoadedScripts = function() {
    if(!Sys._ScriptLoader._referencedScripts) {
        Sys._ScriptLoader._referencedScripts = [];
        Sys._ScriptLoader.readLoadedScripts();
    }
    return Sys._ScriptLoader._referencedScripts;
}
