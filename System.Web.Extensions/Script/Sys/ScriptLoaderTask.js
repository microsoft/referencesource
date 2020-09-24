#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="ScriptLoaderTask.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
// ScriptLoaderTask loads a single script by injecting a dynamic script tag into the DOM.
// It calls the completed callback when the script element's load/readystatechange or error event occus.
// The task should be disposed of after use, as it contains references to the script element.

Sys._ScriptLoaderTask = function(scriptElement, completedCallback) {
    /// <param name="scriptElement" domElement="true">The script element to add to the DOM.</param>
    /// <param name="completedCallback" type="Function">Callback to call when the script has loaded or failed to load.</param>
    this._scriptElement = scriptElement;
    this._completedCallback = completedCallback;
}
Sys._ScriptLoaderTask.prototype = {
    get_scriptElement: function() {
        /// <value domElement="true">The script element.</value>
        return this._scriptElement;
    },
    
    dispose: function() {
        // disposes of the task by removing the load handlers, aborting the window timeout, and releasing the ref to the dom element
        if(this._disposed) {
            // already disposed
            return;
        }
        this._disposed = true;
        this._removeScriptElementHandlers();
        // remove script element from DOM
        Sys._ScriptLoaderTask._clearScript(this._scriptElement);
        this._scriptElement = null;
    },
        
    execute: function() {
        /// <summary>Begins loading the given script element.</summary>
        if (this._ensureReadyStateLoaded()) {
            this._executeInternal();
        }
    },

    _executeInternal: function() {
        this._addScriptElementHandlers();
        // DevDiv Bugs 146697: use lowercase names on getElementsByTagName to work with xhtml content type
#if DEBUG
        // DevDiv Bugs 146327: In debug mode, report useful error message for pages without <head> element
        var headElements = document.getElementsByTagName('head');
        if (headElements.length === 0) {
             throw new Error.invalidOperation(Sys.Res.scriptLoadFailedNoHead);
        }
        else {
             headElements[0].appendChild(this._scriptElement);
        }
#else
        document.getElementsByTagName('head')[0].appendChild(this._scriptElement);
#endif
    },

    _ensureReadyStateLoaded: function() {
        // If using IE8 or earlier, we want to do a two-stage script load.  The first stage is
        // to set the 'src' attribute on the script element and then wait for IE to finish
        // downloading it before adding it to the DOM.
        if (this._useReadyState() && this._scriptElement.readyState !== 'loaded' && this._scriptElement.readyState !== 'complete') {
            this._scriptDownloadDelegate = Function.createDelegate(this, this._executeInternal);
            $addHandler(this._scriptElement, 'readystatechange', this._scriptDownloadDelegate);
            return false;
        }

        return true;
    },
       
    _addScriptElementHandlers: function() {
        // adds the necessary event handlers to the script node to know when it is finished loading

        // First, remove the download handler if we used one
        if (this._scriptDownloadDelegate) {
            $removeHandler(this._scriptElement, 'readystatechange', this._scriptDownloadDelegate);
            this._scriptDownloadDelegate = null;
        }

        // Then add a handler to fire when the script is loaded in the DOM
        this._scriptLoadDelegate = Function.createDelegate(this, this._scriptLoadHandler);
        if (this._useReadyState()) {
            $addHandler(this._scriptElement, 'readystatechange', this._scriptLoadDelegate);
        } else {
            $addHandler(this._scriptElement, 'load', this._scriptLoadDelegate);
        }

        // FF throws onerror if the script doesn't exist, not loaded.
        // DevDev Bugs 86101 -- cant use DomElement.addHandler because it throws for 'error' events.
        if (this._scriptElement.addEventListener) {
            this._scriptErrorDelegate = Function.createDelegate(this, this._scriptErrorHandler);
            this._scriptElement.addEventListener('error', this._scriptErrorDelegate, false);
        }
    },    
    
    _removeScriptElementHandlers: function() {
        // removes the load and error handlers from the script element
        if(this._scriptLoadDelegate) {
            var scriptElement = this.get_scriptElement();

            // First, remove the download handler if we used one
            if (this._scriptDownloadDelegate) {
                $removeHandler(this._scriptElement, 'readystatechange', this._scriptDownloadDelegate);
                this._scriptDownloadDelegate = null;
            }

            if (this._useReadyState() && this._scriptLoadDelegate) {
                $removeHandler(scriptElement, 'readystatechange', this._scriptLoadDelegate);
            }
            else {
                $removeHandler(scriptElement, 'load', this._scriptLoadDelegate);
            }
            if (this._scriptErrorDelegate) {
                // DevDev Bugs 86101 -- cant use DomElement.removeHandler because addHandler throws for 'error' events.
                this._scriptElement.removeEventListener('error', this._scriptErrorDelegate, false);
                this._scriptErrorDelegate = null;
            }
            this._scriptLoadDelegate = null;
        }
    },    

    _scriptErrorHandler: function() {
        // handler for when the script element's error event occurs
        if(this._disposed) {
            return;
        }
        
        // false == did not load successfully (404, etc)
        this._completedCallback(this.get_scriptElement(), false);
    },
           
    _scriptLoadHandler: function() {
        // handler for when the script element's load/readystatechange event occurs
        if(this._disposed) {
            return;
        }

        var scriptElement = this.get_scriptElement();
        if (this._useReadyState() && scriptElement.readyState !== 'complete') {
            return;
        }

        this._completedCallback(scriptElement, true);
    },

    _useReadyState: function() {
        return (Sys.Browser.agent === Sys.Browser.InternetExplorer && (Sys.Browser.version < 9 || ((document.documentMode || 0) < 9)));
    }
}
Sys._ScriptLoaderTask.registerClass("Sys._ScriptLoaderTask", null, Sys.IDisposable);

Sys._ScriptLoaderTask._clearScript = function(scriptElement) {
    if (!Sys.Debug.isDebug && scriptElement.parentNode) {
        // In release mode we clear out the script elements that we add
        // so that they don't clutter up the DOM.
        scriptElement.parentNode.removeChild(scriptElement);
    }
}