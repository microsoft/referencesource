#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="PageRequestManager.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.WebForms.PageRequestManager = function() {
    this._form = null;
    this._activeDefaultButton = null;
    this._activeDefaultButtonClicked = false;
    this._updatePanelIDs = null;
    this._updatePanelClientIDs = null;
    this._updatePanelHasChildrenAsTriggers = null;
    this._asyncPostBackControlIDs = null;
    this._asyncPostBackControlClientIDs = null;
    this._postBackControlIDs = null;
    this._postBackControlClientIDs = null;
    this._scriptManagerID = null;
    this._pageLoadedHandler = null;

    this._additionalInput = null;
    this._onsubmit = null;
    this._onSubmitStatements = [];
    this._originalDoPostBack = null;
    this._originalDoPostBackWithOptions = null;
    this._originalFireDefaultButton = null;
    this._originalDoCallback = null;
    this._isCrossPost = false;
    this._postBackSettings = null;
    this._request = null;
    this._onFormSubmitHandler = null;
    this._onFormElementClickHandler = null;
    this._onWindowUnloadHandler = null;
    this._asyncPostBackTimeout = null;

    this._controlIDToFocus = null;
    this._scrollPosition = null;
    this._processingRequest = false;
    this._scriptDisposes = {};
    
    // DevDiv Bugs 161922, 138251:
    // List of hidden fields that should be removed if an async update does not
    // explictly define it.
    this._transientFields = ["__VIEWSTATEENCRYPTED", "__VIEWSTATEFIELDCOUNT"];

    this._textTypes = /^(text|password|hidden|search|tel|url|email|number|range|color|datetime|date|month|week|time|datetime-local)$/i;
}

Sys.WebForms.PageRequestManager.prototype = {

    _get_eventHandlerList: function() {
        if (!this._events) {
            this._events = new Sys.EventHandlerList();
        }
        return this._events;
    },

    get_isInAsyncPostBack : function() {
        /// <summary>Indicates that an async postback is in progress.</summary>
        /// <value type="Boolean"/>
        return this._request !== null;
    },

    // Events
    add_beginRequest: function(handler) {
        /// <summary>Adds a beginRequest event handler.</summary>
        this._get_eventHandlerList().addHandler("beginRequest", handler);
    },
    remove_beginRequest: function(handler) {
        this._get_eventHandlerList().removeHandler("beginRequest", handler);
    },

    add_endRequest: function(handler) {
        /// <summary>Adds a endRequest event handler.</summary>
        this._get_eventHandlerList().addHandler("endRequest", handler);
    },
    remove_endRequest: function(handler) {
        this._get_eventHandlerList().removeHandler("endRequest", handler);
    },

    add_initializeRequest: function(handler) {
        /// <summary>Adds a initializeRequest event handler.</summary>
        this._get_eventHandlerList().addHandler("initializeRequest", handler);
    },
    remove_initializeRequest: function(handler) {
        this._get_eventHandlerList().removeHandler("initializeRequest", handler);
    },

    add_pageLoaded: function(handler) {
        /// <summary>Adds a pageLoaded event handler.</summary>
        this._get_eventHandlerList().addHandler("pageLoaded", handler);
    },
    remove_pageLoaded: function(handler) {
        this._get_eventHandlerList().removeHandler("pageLoaded", handler);
    },

    add_pageLoading: function(handler) {
        /// <summary>Adds a pageLoading event handler.</summary>
        this._get_eventHandlerList().addHandler("pageLoading", handler);
    },
    remove_pageLoading: function(handler) {
        this._get_eventHandlerList().removeHandler("pageLoading", handler);
    },

    abortPostBack: function() {
        if (!this._processingRequest && this._request) {
            // cancel the request if a response hasn't already been received
            this._request.get_executor().abort();
            this._request = null;
        }
    },

    beginAsyncPostBack: function(updatePanelsToUpdate, eventTarget, eventArgument, causesValidation, validationGroup) {
        /// <summary>Begins an asynchronous postback.</summary>
        /// <param name="updatePanelsToUpdate" type="Array" elementType="String" mayBeNull="true" optional="true">
        ///     A list of UniqueIDs or ClientIDs of UpdatePanel controls that should have their rendering updated.
        /// </param>
        /// <param name="eventTarget" type="String" mayBeNull="true" optional="true"/>
        /// <param name="eventArgument" type="String" mayBeNull="true" optional="true"/>
        /// <param name="causesValidation" type="Boolean" mayBeNull="true" optional="true"/>
        /// <param name="validationGroup" type="String" mayBeNull="true" optional="true"/>
        if (causesValidation && (typeof(Page_ClientValidate) === 'function') && !Page_ClientValidate(validationGroup || null)) {
            return;
        }
        this._postBackSettings = this._createPostBackSettings(true, updatePanelsToUpdate, eventTarget);
        var form = this._form;
        form.__EVENTTARGET.value = (eventTarget || "");
        form.__EVENTARGUMENT.value = (eventArgument || "");
        this._isCrossPost = false;
        this._additionalInput = null;
        this._onFormSubmit();
    },
    
    _cancelPendingCallbacks : function() {
        // DevDiv Bugs 125825: To avoid EVENTVALIDATION corruption we must cancel pending callbacks when an async postback begins
        // to cancel callbacks, we run logic similar to WebForm_CallbackComplete,
        // except we do not run WebForm_ExecuteCallback for them. This code is exactly
        // WebForm_CallbackComplete except without the call to WebForm_ExecuteCallback.
        // We are basically treating each callback as completed, ignoring the response if any.
        for (var i = 0, l = window.__pendingCallbacks.length; i < l; i++) {
            var callback = window.__pendingCallbacks[i];
            if (callback) {
                if (!callback.async) {
                    // we just cancelled the single allowed instance of a synchronous callback
                    window.__synchronousCallBackIndex = -1;
                }
                window.__pendingCallbacks[i] = null;
                var callbackFrameID = "__CALLBACKFRAME" + i;
                var xmlRequestFrame = document.getElementById(callbackFrameID);
                if (xmlRequestFrame) {
                    xmlRequestFrame.parentNode.removeChild(xmlRequestFrame);
                }
            }
        }
    },
    
    _commitControls: function(updatePanelData, asyncPostBackTimeout) {
        // DevDiv Bugs 154403:
        // commits context data parsed from a delta. This is called after an async response
        // has proceeded past the script include loading phase, which when it can no longer
        // be cancelled and the HTML DOM will be updated.
        // _processUpdatePanelArrays is the method that creates these arrays..
        // DevDiv Bugs 188564: Update may not have an async postback timeout node and/or updatepanel nodes.
        if (updatePanelData) {
            this._updatePanelIDs = updatePanelData.updatePanelIDs;
            this._updatePanelClientIDs = updatePanelData.updatePanelClientIDs;
            this._updatePanelHasChildrenAsTriggers = updatePanelData.updatePanelHasChildrenAsTriggers;
            this._asyncPostBackControlIDs = updatePanelData.asyncPostBackControlIDs;
            this._asyncPostBackControlClientIDs = updatePanelData.asyncPostBackControlClientIDs;
            this._postBackControlIDs = updatePanelData.postBackControlIDs;
            this._postBackControlClientIDs = updatePanelData.postBackControlClientIDs;
        }
        if (typeof(asyncPostBackTimeout) !== 'undefined' && asyncPostBackTimeout !== null) {
            this._asyncPostBackTimeout = asyncPostBackTimeout * 1000;
        }
    },
    
    _createHiddenField: function(id, value) {
        // DevDiv Bugs 27075: Creates a hidden field via innerHTML to workaround a caching issue.
        var container, field = document.getElementById(id);

        if (field) {
            // the field already exists
            if (!field._isContained) {
                // but it is not contained within a SPAN container, so we must create one
                // in order to recreate it with innerHTML.
                field.parentNode.removeChild(field);
            }
            else {
                // and it already has a container, we'll just set the container innerHTML to replace it,
                // which is much faster than removing the element, recreating it, and then setting innerHTML
                container = field.parentNode;
            }
        }
        if (!container) {
            container = document.createElement('span');
            // set display none in case this SPAN would be styled
            container.style.cssText = "display:none !important";
            this._form.appendChild(container);
        }
        // now create/replace the input by setting innerHTML
        container.innerHTML = "<input type='hidden' />";
        field = container.childNodes[0];
        // flag it as contained, so if it needs to be removed we know to remove the container, too.
        field._isContained = true;
        field.id = field.name = id;
        field.value = value;
    },

    _createPageRequestManagerTimeoutError : function() {
        // Creates a PageRequestManagerTimeoutException representing a request that timed out.
        var displayMessage = "Sys.WebForms.PageRequestManagerTimeoutException: " + Sys.WebForms.Res.PRM_TimeoutError;
        var e = Error.create(displayMessage, {name: 'Sys.WebForms.PageRequestManagerTimeoutException'});
        e.popStackFrame();
        return e;
    },

    _createPageRequestManagerServerError : function(httpStatusCode, message) {
        // Creates a PageRequestManagerServerErrorException representing an error that occurred on the server.
        var displayMessage = "Sys.WebForms.PageRequestManagerServerErrorException: " +
            (message || String.format(Sys.WebForms.Res.PRM_ServerError, httpStatusCode));
        var e = Error.create(displayMessage, {
            name: 'Sys.WebForms.PageRequestManagerServerErrorException',
            httpStatusCode: httpStatusCode
        });
        e.popStackFrame();
        return e;
    },

    _createPageRequestManagerParserError : function(parserErrorMessage) {
        // Creates a PageRequestManagerParserErrorException representing a parser error that occurred while processing a response from the server.
        var displayMessage = "Sys.WebForms.PageRequestManagerParserErrorException: " + String.format(Sys.WebForms.Res.PRM_ParserError, parserErrorMessage);
        var e = Error.create(displayMessage, {name: 'Sys.WebForms.PageRequestManagerParserErrorException'});
        e.popStackFrame();
        return e;
    },

    _createPanelID: function(panelsToUpdate, postBackSettings) {
        var asyncTarget = postBackSettings.asyncTarget,
            toUpdate = this._ensureUniqueIds(panelsToUpdate || postBackSettings.panelsToUpdate),
            panelArg = (toUpdate instanceof Array)
                ? toUpdate.join(',')
                : (toUpdate || this._scriptManagerID);
        if (asyncTarget) {
            panelArg += "|" + asyncTarget;
        }
        return encodeURIComponent(this._scriptManagerID) + '=' + encodeURIComponent(panelArg) + '&';
    },

    _createPostBackSettings : function(async, panelsToUpdate, asyncTarget, sourceElement) {
        return { async:async, asyncTarget: asyncTarget, panelsToUpdate: panelsToUpdate, sourceElement: sourceElement };
    },

    _convertToClientIDs : function(source, destinationIDs, destinationClientIDs, version4) {
        if (source) {
            for (var i = 0, l = source.length; i < l; i += (version4 ? 2 : 1)) {
                var uniqueID = source[i],
                    clientID = (version4 ? source[i+1] : "") || this._uniqueIDToClientID(uniqueID);
                Array.add(destinationIDs, uniqueID);
                Array.add(destinationClientIDs, clientID);
            }
        }
    },

    dispose : function() {
        if (this._form) {
            Sys.UI.DomEvent.removeHandler(this._form, 'submit', this._onFormSubmitHandler);
            Sys.UI.DomEvent.removeHandler(this._form, 'click', this._onFormElementClickHandler);
            Sys.UI.DomEvent.removeHandler(window, 'unload', this._onWindowUnloadHandler);
            Sys.UI.DomEvent.removeHandler(window, 'load', this._pageLoadedHandler);
        }

        if (this._originalDoPostBack) {
            window.__doPostBack = this._originalDoPostBack;
            this._originalDoPostBack = null;
        }
        if (this._originalDoPostBackWithOptions) {
            window.WebForm_DoPostBackWithOptions = this._originalDoPostBackWithOptions;
            this._originalDoPostBackWithOptions = null;
        }
        if (this._originalFireDefaultButton) {
            window.WebForm_FireDefaultButton = this._originalFireDefaultButton;
            this._originalFireDefaultButton = null;
        }
        if (this._originalDoCallback) {
            window.WebForm_DoCallback = this._originalDoCallback;
            this._originalDoCallback = null;
        }

        this._form = null;
        this._updatePanelIDs = null;
        this._updatePanelClientIDs = null;
        this._asyncPostBackControlIDs = null;
        this._asyncPostBackControlClientIDs = null;
        this._postBackControlIDs = null;
        this._postBackControlClientIDs = null;
        this._asyncPostBackTimeout = null;
        this._scrollPosition = null;
        this._activeElement = null;
    },
    
    _doCallback : function(eventTarget, eventArgument, eventCallback, context, errorCallback, useAsync) {
        // DevDiv Bugs 125825: Do not allow callbacks to begin while an async postback is in progress to prevent EVENTVALIDATION corruption
        if (!this.get_isInAsyncPostBack()) {
            this._originalDoCallback(eventTarget, eventArgument, eventCallback, context, errorCallback, useAsync);
        }
    },

    // New implementation of __doPostBack
    _doPostBack : function(eventTarget, eventArgument) {
        var event = window.event;
        if (!event) {
            // try to find the event by looking up the call stack. Two possible code paths bring us here
            // (1) domevent->__doPostBackDelegate->_doPostBack
            // (2) domevent->WebForm_DoPostBackWithOptionsDelegate->_doPostBackWithOptions->OriginalWebForm_DoPostBackWithOptions->_doPostBackDelegate->_doPostBack
            // Any other code paths are custom code that calls __doPostBack
            // Find the top of the stack and get its first argument, if any, which may or may not be the DOM event.
            // Use a recursionLimit to protect against recursive callstacks, which may occur in custom controls that call
            // __doPostBack. It also occurred in Firefox before the copy of DPWO was created (see comment in that method),
            // hinting at a bug that could cause a recurisve stack when the stack is not actually recursive.
            // 30 is used as sufficiently large enough to allow for any complex stacks in complex scenarios while small
            // enough not to slow the postback from occurring.
            var caller = arguments.callee ? arguments.callee.caller : null;
            if (caller) {
                var recursionLimit = 30;
                while (caller.arguments.callee.caller && --recursionLimit) {
                    caller = caller.arguments.callee.caller;
                }
                event = (recursionLimit && caller.arguments.length) ? caller.arguments[0] : null;
            }
        }

        this._additionalInput = null;
        var form = this._form;
        if ((eventTarget === null) || (typeof(eventTarget) === "undefined") || (this._isCrossPost)) {
            // Allow the default form submit to take place. Since it's a cross-page postback.
            // DevDiv 80942: we should fall to a full postback if event target is null or undefined
            this._postBackSettings = this._createPostBackSettings(false);
            // set to false so subsequent posts that don't go through DPWO aren't considered cross post
            this._isCrossPost = false;
        }
        else {
            var mpUniqueID = this._masterPageUniqueID;
            // If it's not a cross-page post, see if we can find the DOM element that caused the postback
            var clientID = this._uniqueIDToClientID(eventTarget);
            var postBackElement = document.getElementById(clientID);
            if (!postBackElement && mpUniqueID) {
                if (eventTarget.indexOf(mpUniqueID + "$") === 0) {
                    // With ClientIDMode=Predictable, the MasterPageID is missing from the beginning
                    // of the client ID.
                    postBackElement = document.getElementById(clientID.substr(mpUniqueID.length + 1));
                }
            }
            if (!postBackElement) {
                // If the control has no matching DOM element we look for an exact
                // match from RegisterAsyncPostBackControl or RegisterPostBackControl.
                // If we can't find anything about it then we do a search based on
                // naming containers to still try and find a match.
                if (Array.contains(this._asyncPostBackControlIDs, eventTarget)) {
                    // Exact match for async postback
                    this._postBackSettings = this._createPostBackSettings(true, null, eventTarget);
                }
                else {
                    if (Array.contains(this._postBackControlIDs, eventTarget)) {
                        // Exact match for regular postback
                        this._postBackSettings = this._createPostBackSettings(false);
                    }
                    else {
                        // Find nearest element based on UniqueID in case the element calling
                        // __doPostBack doesn't have an ID. GridView does this for its Update
                        // button and without this we can't do async postbacks.
                        var nearestUniqueIDMatch = this._findNearestElement(eventTarget);
                        if (nearestUniqueIDMatch) {
                            // We found a related parent element, so walk up the DOM to find out what kind
                            // of postback we should do.
                            this._postBackSettings = this._getPostBackSettings(nearestUniqueIDMatch, eventTarget);
                        }
                        else {
                            // the control may have rendered without the master page id prefix due to ClientIDMode = Predictable
                            // For example "ctl00$ctl00$foo$Button1" may have rendered an id of "foo_button1" if "ctl00$ctl00" 
                            // is the ID for a 2-level nested master page.
                            // try stripping it from the beginning of the ID and trying again
                            if (mpUniqueID) {
                                mpUniqueID += "$";
                                if (eventTarget.indexOf(mpUniqueID) === 0) {
                                    nearestUniqueIDMatch = this._findNearestElement(eventTarget.substr(mpUniqueID.length));
                                }
                            }
                            if (nearestUniqueIDMatch) {
                                // We found a related parent element, so walk up the DOM to find out what kind
                                // of postback we should do.
                                this._postBackSettings = this._getPostBackSettings(nearestUniqueIDMatch, eventTarget);
                            }
                            else {
                                // if we still didn't find it, see if the given UniqueID corresponds to the last clicked
                                // form element. Find that by looking at the source of the current dom event, if any.
                                // In the case of an href="javascript" link, there is no event, so use the saved
                                // last clicked element in _activeElement.
                                // Then examine the element to see if it looks like it is targetting itself.
                                // If not, then the clicked element might either be causing a postback on a different
                                // element, or it is a saved _activeElement that could be stale from a previous click.
                                var activeElement;
                                try {
                                    activeElement = event ? (event.target || event.srcElement) : null;
                                }
                                catch(ex) {
                                }
                                activeElement = activeElement || this._activeElement;
                                var causesPostback = /__doPostBack\(|WebForm_DoPostBackWithOptions\(/;
                                function testCausesPostBack(attr) {
                                    attr = attr ? attr.toString() : "";
                                    return (causesPostback.test(attr) &&
                                        (attr.indexOf("'" + eventTarget + "'") !== -1) || (attr.indexOf('"' + eventTarget + '"') !== -1));
                                }
                                if (activeElement && (
                                        (activeElement.name === eventTarget) ||
                                        testCausesPostBack(activeElement.href) ||
                                        testCausesPostBack(activeElement.onclick) ||
                                        testCausesPostBack(activeElement.onchange)
                                        )) {
                                    // walk up the DOM to find out what kind of postback we should do.
                                    this._postBackSettings = this._getPostBackSettings(activeElement, eventTarget);
                                }
                                else {
                                    // Can't find any DOM element at all related to the eventTarget,
                                    // so we just give up and do a regular postback.
                                    this._postBackSettings = this._createPostBackSettings(false);
                                }
                            }
                        }
                    }
                }
            }
            else {
                // The element was found, so walk up the DOM to find out what kind
                // of postback we should do.
                this._postBackSettings = this._getPostBackSettings(postBackElement, eventTarget);
            }
        }

        if (!this._postBackSettings.async) {
            // Temporarily restore the form's onsubmit handler expando while calling
            // the original ASP.NET 2.0 __doPostBack() function.
            form.onsubmit = this._onsubmit;
            this._originalDoPostBack(eventTarget, eventArgument);
            form.onsubmit = null;
            return;
        }

        form.__EVENTTARGET.value = eventTarget;
        form.__EVENTARGUMENT.value = eventArgument;
        this._onFormSubmit();
    },

    _doPostBackWithOptions: function(options) {
        this._isCrossPost = options && options.actionUrl;
        // note that when DoPostBackWithOptions is used, _doPostBack or _onFormSubmit, one of the two,
        // are guaranteed to be called next.
        // In both of those methods it is important to clear the isCrossPost flag so subsequent posts that
        // don't use DoPostBackWithOptions are not considered cross page posts.

        // The following is copied from WebForms.js/DoPostBackWithOptions.
        // We do not call the _originalDoPostBackWith options as this breaks the stack walk in
        // _doPostBack in Firefox, because it considers the new DPWO to be the same as the original
        // when looking at fn.caller.
        var validationResult = true;
        if (options.validation) {
            if (typeof(Page_ClientValidate) == 'function') {
                validationResult = Page_ClientValidate(options.validationGroup);
            }
        }
        if (validationResult) {
            if ((typeof(options.actionUrl) != "undefined") && (options.actionUrl != null) && (options.actionUrl.length > 0)) {
                theForm.action = options.actionUrl;
            }
            if (options.trackFocus) {
                var lastFocus = theForm.elements["__LASTFOCUS"];
                if ((typeof(lastFocus) != "undefined") && (lastFocus != null)) {
                    if (typeof(document.activeElement) == "undefined") {
                        lastFocus.value = options.eventTarget;
                    }
                    else {
                        var active = document.activeElement;
                        if ((typeof(active) != "undefined") && (active != null)) {
                            if ((typeof(active.id) != "undefined") && (active.id != null) && (active.id.length > 0)) {
                                lastFocus.value = active.id;
                            }
                            else if (typeof(active.name) != "undefined") {
                                lastFocus.value = active.name;
                            }
                        }
                    }
                }
            }
        }
        if (options.clientSubmit) {
            // this 'this._doPostBack' instead of '__doPostBack'
            this._doPostBack(options.eventTarget, options.eventArgument);
        }
    },

    _elementContains : function(container, element) {
        while (element) {
            if (element === container) {
                return true;
            }
            element = element.parentNode;
        }
        return false;
    },

    _endPostBack: function(error, executor, data) {
        if (this._request === executor.get_webRequest()) {
            // the postback being ended is the one being processed
            this._processingRequest = false;
            this._additionalInput = null;
            this._request = null;
        }

        var handler = this._get_eventHandlerList().getHandler("endRequest");
        var errorHandled = false;
        if (handler) {
            var eventArgs = new Sys.WebForms.EndRequestEventArgs(error, data ? data.dataItems : {}, executor);
            handler(this, eventArgs);
            errorHandled = eventArgs.get_errorHandled();
        }
        if (error && !errorHandled) {
            // DevDiv 89485: throw, don't alert()
            throw error;
        }
    },

    _ensureUniqueIds: function(ids) {
        // given a single ID or an array of IDs that might be ClientIDs or UniqueIDs,
        // returns a list of the UniqueIDs. This is used from createPanelID and makes
        // it so beginAsyncPostBack and the panelstoUpdate property of the event args
        // supports a list of update panels by UniqueID or ClientID.
        // If an ID is not a ClientID we assume it is a UniqueID even though it could just be
        // that it does not exist.
        if (!ids) return ids;
        ids = ids instanceof Array ? ids : [ids];
        var uniqueIds = [];
        for (var i = 0, l = ids.length; i < l; i++) {
            var id = ids[i], index = Array.indexOf(this._updatePanelClientIDs, id);
            uniqueIds.push(index > -1 ? this._updatePanelIDs[index] : id);
        }
        return uniqueIds;
    },

    // Finds the nearest element to the given UniqueID. If an element is not
    // found for the exact UniqueID, it walks up the parent chain to look for it.
    _findNearestElement : function(uniqueID) {
        while (uniqueID.length > 0) {
            var clientID = this._uniqueIDToClientID(uniqueID);
            var element = document.getElementById(clientID);
            if (element) {
                return element;
            }
            var indexOfLastDollar = uniqueID.lastIndexOf('$');
            if (indexOfLastDollar === -1) {
                return null;
            }
            uniqueID = uniqueID.substring(0, indexOfLastDollar);
        }
        return null;
    },

    _findText : function(text, location) {
        var startIndex = Math.max(0, location - 20);
        var endIndex = Math.min(text.length, location + 20);
        return text.substring(startIndex, endIndex);
    },
    
    _fireDefaultButton: function(event, target) {
        // This is a copy of the function WebForm_FireDefaultButton as defined in WebForms.js.
        // The purpose is to hook into the WebForm_FireDefaultButton call with the code commented in the middle.
        // Other than that, there have been a few minor changes to the code but the logic is the same.
        if (event.keyCode === 13) {
            var src = event.srcElement || event.target;
            if (!src || (src.tagName.toLowerCase() !== "textarea")) {
                var defaultButton = document.getElementById(target);

                if (defaultButton && (typeof(defaultButton.click) !== "undefined")) {
                    
                    // Beginning of new code...
                    
                    // In all but FF this causes the form.onclick event to fire with the button as the event target.
                    // In FF the the form.onclick event has the current focus control as the target, which prevents the
                    // default button's server-side click event from firing. So we ensure the correct control is determined
                    // to have caused the postback by saving the default button before clicking on it. The code in
                    // onFormSubmit looks for this field and ensures the postback target is the button.
                    this._activeDefaultButton = defaultButton;
                    this._activeDefaultButtonClicked = false;
                    try {
                        // click is synchronous -- it will immediately cause a form onclick event and then a form onsubmit event
                        // assuming nothing uses preventDefault() to cancel the event.
                        defaultButton.click();
                    }
                    finally {
                        // form submission may or may not be occuring after this point
                        this._activeDefaultButton = null;
                    }
                    
                    // ...End of new code
                    
                    // cancel submission caused by hitting enter in the input control
                    event.cancelBubble = true;
                    if (typeof(event.stopPropagation) === "function") {
                        event.stopPropagation();
                    }
                    return false;
                }
            }
        }
        return true;
    },

    _getPageLoadedEventArgs: function(initialLoad, data) {
        // -------------+------------------------------------+-----------------------
        // Situation    | In ID collections                  | In eventArg property
        // -------------+------------------------------------+-----------------------
        // Update (exp) | in panelsToRefresh                 | updated
        // Update (imp) | in new, in old, in childUP         | created
        // Create (exp) | in new, not in old, not in childUP | created
        // Create (imp) | in new, not in old, in childUP     | created
        // Delete (exp) | not in new, in old, not in childUP | ---
        // Delete (imp) | not in new, in old, in childUP     | ---
        // -------------+------------------------------------+-----------------------
        // (exp) = explicit
        // (imp) = implicit (happened as result of parent UpdatePanel updating)
        // --------------------------------------------------------------------------
        // in panelsToRefresh = updated
        // not updated, in new = created
        // else = don't care
        // --------------------------------------------------------------------------

        var updated = [];
        var created = [];
        var version4 = data ? data.version4 : false;
        var upData = data ? data.updatePanelData : null;

        // All panels before update,
        // All panels after update,
        // Child panels created after update,
        // Parent panels created after update
        var newIDs, newClientIDs, childIDs, refreshedIDs;

        if (!upData) {
            // this is the initial load, consider the initialized update panels
            // to be the newly created ones
            newIDs = this._updatePanelIDs;
            newClientIDs = this._updatePanelClientIDs;
            childIDs = null;
            refreshedIDs = null;
        }
        else {
            newIDs = upData.updatePanelIDs;
            newClientIDs = upData.updatePanelClientIDs;
            childIDs = upData.childUpdatePanelIDs;
            refreshedIDs = upData.panelsToRefreshIDs;
        }

        var i, l, uniqueID, clientID;
        // in panelsToRefresh = updated
        if (refreshedIDs) {
            for (i = 0, l = refreshedIDs.length; i < l; i += (version4 ? 2 : 1)) {
                uniqueID = refreshedIDs[i];
                clientID = (version4 ? refreshedIDs[i+1] : "") || this._uniqueIDToClientID(uniqueID);
                Array.add(updated, document.getElementById(clientID));
            }
        }

        // If the panel is in the new list and it is either the initial load
        // of the page a refreshed child, it is 'created'.
        for (i = 0, l = newIDs.length; i < l; i++) {
            if (initialLoad || Array.indexOf(childIDs, newIDs[i]) !== -1) {
                Array.add(created, document.getElementById(newClientIDs[i]));
            }
        }

        return new Sys.WebForms.PageLoadedEventArgs(updated, created, data ? data.dataItems : {});
    },

    _getPageLoadingEventArgs: function(data) {
        // -------------+------------------------------------+-----------------------
        // Situation    | In ID collections                  | In eventArg property
        // -------------+------------------------------------+-----------------------
        // Update (exp) | in panelsToRefresh                 | updated
        // Update (imp) | in old, in new, in childUP         | deleted
        // Create (exp) | not in old, in new, not in childUP | ---
        // Create (imp) | not in old, in new, in childUP     | ---
        // Delete (exp) | in old, not in new, not in childUP | deleted
        // Delete (imp) | in old, not in new, in childUP     | deleted
        // -------------+------------------------------------+-----------------------
        // (exp) = explicit
        // (imp) = implicit (happened as result of parent UpdatePanel updating)
        // --------------------------------------------------------------------------
        // in panelsToRefresh = updated
        // not updated, (not in new or in childUP) = deleted
        // else = don't care
        // --------------------------------------------------------------------------

        var updated = [],
            deleted = [],
            upData = data.updatePanelData,
            oldIDs = upData.oldUpdatePanelIDs,
            oldClientIDs = upData.oldUpdatePanelClientIDs,
            newIDs = upData.updatePanelIDs,
            childIDs = upData.childUpdatePanelIDs,
            refreshedIDs = upData.panelsToRefreshIDs,
            i, l, uniqueID, clientID,
            version4 = data.version4;
        // in panelsToRefresh = updated
        for (i = 0, l = refreshedIDs.length; i < l; i += (version4 ? 2 : 1)) {
            uniqueID = refreshedIDs[i];
            clientID = (version4 ? refreshedIDs[i+1] : "") || this._uniqueIDToClientID(uniqueID);
            Array.add(updated, document.getElementById(clientID));
        }

        // not in new or in childUP = deleted
        for (i = 0, l = oldIDs.length; i < l; i++) {
            uniqueID = oldIDs[i];
            if (Array.indexOf(refreshedIDs, uniqueID) === -1 &&
                (Array.indexOf(newIDs, uniqueID) === -1 || Array.indexOf(childIDs, uniqueID) > -1)) {
                Array.add(deleted, document.getElementById(oldClientIDs[i]));
            }
        }

        return new Sys.WebForms.PageLoadingEventArgs(updated, deleted, data.dataItems);
    },

    _getPostBackSettings : function(element, elementUniqueID) {
##DEBUGINTERNAL Sys.Debug.assert(element ? true : false, 'panelID should be specified if async is true');

        var originalElement = element;

        // Keep track of whether we have an AsyncPostBackControl but still
        // want to see if we're inside an UpdatePanel anyway.
        var proposedSettings = null;

        // Walk up DOM hierarchy to find out the nearest container of
        // the element that caused the postback.
        while (element) {
            if (element.id) {
                // First try an exact match for async postback, regular postback, or UpdatePanel
                if (!proposedSettings && Array.contains(this._asyncPostBackControlClientIDs, element.id)) {
                    // The element explicitly causes an async postback
                    proposedSettings = this._createPostBackSettings(true, null, elementUniqueID, originalElement);
                }
                else {
                    if (!proposedSettings && Array.contains(this._postBackControlClientIDs, element.id)) {
                        // The element explicitly doesn't cause an async postback
                        return this._createPostBackSettings(false);
                    }
                    else {
                        var indexOfPanel = Array.indexOf(this._updatePanelClientIDs, element.id);
                        if (indexOfPanel !== -1) {
                            // The element causes an async postback because it is inside an UpdatePanel
                            if (this._updatePanelHasChildrenAsTriggers[indexOfPanel]) {
                                // If it was in an UpdatePanel and the panel has ChildrenAsTriggers=true, then
                                // we do an async postback and refresh the given panel

                                // Although we do the search by looking at ClientIDs, we end
                                // up sending a UniqueID back to the server so that we can
                                // call FindControl() with it.
                                return this._createPostBackSettings(true, [this._updatePanelIDs[indexOfPanel]], elementUniqueID, originalElement);
                            }
                            else {
                                // The element was inside an UpdatePanel so we do an async postback,
                                // but because it has ChildrenAsTriggers=false we don't update this panel.
                                return this._createPostBackSettings(true, null, elementUniqueID, originalElement);
                            }
                        }
                    }
                }

                // Then try near matches
                if (!proposedSettings && this._matchesParentIDInList(element.id, this._asyncPostBackControlClientIDs)) {
                    // The element explicitly causes an async postback
                    proposedSettings = this._createPostBackSettings(true, null, elementUniqueID, originalElement);
                }
                else {
                    if (!proposedSettings && this._matchesParentIDInList(element.id, this._postBackControlClientIDs)) {
                        // The element explicitly doesn't cause an async postback
                        return this._createPostBackSettings(false);
                    }
                }
            }

            element = element.parentNode;
        }

        // If we have proposed settings that means we found a match for an
        // AsyncPostBackControl but were still searching for an UpdatePanel.
        // If we got here that means we didn't find the UpdatePanel so we
        // just fall back to the original AsyncPostBackControl settings that
        // we created.
        if (!proposedSettings) {
            // The element doesn't cause an async postback
            return this._createPostBackSettings(false);
        }
        else {
            return proposedSettings;
        }
    },

    _getScrollPosition : function() {
        var d = document.documentElement;
        if (d && (this._validPosition(d.scrollLeft) || this._validPosition(d.scrollTop))) {
            return {
                x: d.scrollLeft,
                y: d.scrollTop
            };
        }
        else {
            d = document.body;
            if (d && (this._validPosition(d.scrollLeft) || this._validPosition(d.scrollTop))) {
                return {
                    x: d.scrollLeft,
                    y: d.scrollTop
                };
            }
            else {
                if (this._validPosition(window.pageXOffset) || this._validPosition(window.pageYOffset)) {
                    return {
                        x: window.pageXOffset,
                        y: window.pageYOffset
                    };
                }
                else {
                    return {
                        x: 0,
                        y: 0
                    };
                }
            }
        }
    },

    _initializeInternal : function(scriptManagerID, formElement, updatePanelIDs, asyncPostBackControlIDs, postBackControlIDs, asyncPostBackTimeout, masterPageUniqueID) {
        if (this._prmInitialized) {
            throw Error.invalidOperation(Sys.WebForms.Res.PRM_CannotRegisterTwice);
        }
        this._prmInitialized = true;
        this._masterPageUniqueID = masterPageUniqueID;
        this._scriptManagerID = scriptManagerID;
        this._form = Sys.UI.DomElement.resolveElement(formElement);
        this._onsubmit = this._form.onsubmit;
        this._form.onsubmit = null;
        this._onFormSubmitHandler = Function.createDelegate(this, this._onFormSubmit);
        this._onFormElementClickHandler = Function.createDelegate(this, this._onFormElementClick);
        this._onWindowUnloadHandler = Function.createDelegate(this, this._onWindowUnload);
        Sys.UI.DomEvent.addHandler(this._form, 'submit', this._onFormSubmitHandler);
        Sys.UI.DomEvent.addHandler(this._form, 'click', this._onFormElementClickHandler);
        Sys.UI.DomEvent.addHandler(window, 'unload', this._onWindowUnloadHandler);

        this._originalDoPostBack = window.__doPostBack;
        if (this._originalDoPostBack) {
            window.__doPostBack = Function.createDelegate(this, this._doPostBack);
        }
        this._originalDoPostBackWithOptions = window.WebForm_DoPostBackWithOptions;
        if (this._originalDoPostBackWithOptions) {
            window.WebForm_DoPostBackWithOptions = Function.createDelegate(this, this._doPostBackWithOptions);
        }
        this._originalFireDefaultButton = window.WebForm_FireDefaultButton;
        if (this._originalFireDefaultButton) {
            window.WebForm_FireDefaultButton = Function.createDelegate(this, this._fireDefaultButton);
        }
        this._originalDoCallback = window.WebForm_DoCallback;
        if (this._originalDoCallback) {
            window.WebForm_DoCallback = Function.createDelegate(this, this._doCallback);
        }

        this._pageLoadedHandler = Function.createDelegate(this, this._pageLoadedInitialLoad);
        Sys.UI.DomEvent.addHandler(window, 'load', this._pageLoadedHandler);
        if (updatePanelIDs) {
            this._updateControls(updatePanelIDs, asyncPostBackControlIDs, postBackControlIDs, asyncPostBackTimeout, true);
        }
    },

    _matchesParentIDInList : function(clientID, parentIDList) {
        for (var i = 0, l = parentIDList.length; i < l; i++) {
            if (clientID.startsWith(parentIDList[i] + "_")) {
                return true;
            }
        }
        return false;
    },
    
    _onFormElementActive : function(element, offsetX, offsetY) {
        // element: the form element that is active
        // offsetX/Y: if the element is an image button, the coordinates of the click
        if (element.disabled) {
            return;
        }
        this._activeElement = element;

        // Check if the element that was clicked on should cause an async postback
        this._postBackSettings = this._getPostBackSettings(element, element.name);

        if (element.name) {
            // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
            var tagName = element.tagName.toUpperCase();
            if (tagName === 'INPUT') {
                var type = element.type;
                if (type === 'submit') {
                    // DevDiv Bugs 109456: Encode the name as well as the value
                    this._additionalInput = encodeURIComponent(element.name) + '=' + encodeURIComponent(element.value);
                }
                else if (type === 'image') {
                    // DevDiv Bugs 109456: Encode the name as well as the value
                    this._additionalInput = encodeURIComponent(element.name) + '.x=' + offsetX + '&' + encodeURIComponent(element.name) + '.y=' + offsetY;
                }
            }
            else if ((tagName === 'BUTTON') && (element.name.length !== 0) && (element.type === 'submit')) {
                // DevDiv Bugs 109456: Encode the name as well as the value
                this._additionalInput = encodeURIComponent(element.name) + '=' + encodeURIComponent(element.value);
            }
        }
    },

    _onFormElementClick : function(evt) {
        // flag used by fireDefaultButton to know whether calling click() on the default button raised this event.
        this._activeDefaultButtonClicked = (evt.target === this._activeDefaultButton);
        this._onFormElementActive(evt.target, evt.offsetX, evt.offsetY);
    },

    _onFormSubmit : function(evt) {
        var i, l, continueSubmit = true,
            isCrossPost = this._isCrossPost;
        // set to false so subsequent posts that don't go through DPWO aren't considered cross post
        this._isCrossPost = false;

        // Call the statically declared form onsubmit statement if there was one
        if (this._onsubmit) {
            continueSubmit = this._onsubmit();
        }

        // If necessary, call dynamically added form onsubmit statements
        if (continueSubmit) {
            for (i = 0, l = this._onSubmitStatements.length; i < l; i++) {
                if (!this._onSubmitStatements[i]()) {
                    continueSubmit = false;
                    break;
                }
            }
        }

        if (!continueSubmit) {
            if (evt) {
                evt.preventDefault();
            }
            return;
        }

        var form = this._form;
        if (isCrossPost) {
            // Allow the default form submit to take place. Since it's a cross-page postback.
            return;
        }

        // DevDiv Bugs 123782
        if (this._activeDefaultButton && !this._activeDefaultButtonClicked) {
            // we are submitting because a default button's click method was called by _fireDefaultButton
            // but calling click() explicitly did not cause a click event or raised it for a different element,
            // so we must manually create the correct postback options.
            // The button was clicked programmatically, so there are no offsetX or offsetY coordinates.
            this._onFormElementActive(this._activeDefaultButton, 0, 0);
        }

        // If the postback happened from outside an update panel, fall back
        // and do a normal postback.
        // Dev10 546632: postBackSettings may not exist in certain scenarios where
        // the form click events are cancelled.
        if (!this._postBackSettings || !this._postBackSettings.async) {
            return;
        }

        // Construct the form body
        var formBody = new Sys.StringBuilder(),
            formElements = form.elements,
            count = formElements.length,
            panelID = this._createPanelID(null, this._postBackSettings);
        // DevDiv Bugs 109456: ScriptManager and UpdatePanel IDs should be encoded as well
        formBody.append(panelID);

        for (i = 0; i < count; i++) {
            var element = formElements[i];
            var name = element.name;
            if (typeof(name) === "undefined" || (name === null) || (name.length === 0) || (name === this._scriptManagerID)) {
                continue;
            }

            // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
            var tagName = element.tagName.toUpperCase();

            if (tagName === 'INPUT') {
                var type = element.type;

                // Handle input types that should be treated as text
                if (this._textTypes.test(type)
                    || ((type === 'checkbox' || type === 'radio') && element.checked)) {
                    // DevDiv Bugs 109456: Encode the name as well as the value
                    formBody.append(encodeURIComponent(name));
                    formBody.append('=');
                    formBody.append(encodeURIComponent(element.value));
                    formBody.append('&');
                }
            }
            else if (tagName === 'SELECT') {
                var optionCount = element.options.length;
                for (var j = 0; j < optionCount; j++) {
                    var option = element.options[j];
                    if (option.selected) {
                        // DevDiv Bugs 109456: Encode the name as well as the value
                        formBody.append(encodeURIComponent(name));
                        formBody.append('=');
                        formBody.append(encodeURIComponent(option.value));
                        formBody.append('&');
                    }
                }
            }
            else if (tagName === 'TEXTAREA') {
                // DevDiv Bugs 109456: Encode the name as well as the value
                formBody.append(encodeURIComponent(name));
                formBody.append('=');
                formBody.append(encodeURIComponent(element.value));
                formBody.append('&');
            }
        }

        // DevDiv Bugs 188713: Some firewalls strip the X-MicrosoftAjax header, so send a custom form field as well.
        formBody.append("__ASYNCPOST=true&");

        if (this._additionalInput) {
            formBody.append(this._additionalInput);
            this._additionalInput = null;
        }
        
        var request = new Sys.Net.WebRequest();
        var action = form.action;
        if (Sys.Browser.agent === Sys.Browser.InternetExplorer) {
            // DevDiv Bugs 85367: In IE we must encode the path portion of the request because XHR doesn't do it for us.
            // First, we remove the url fragment, which can appear in history-related scenarios
            // but is not relevant to async postbacks.
            var fragmentIndex = action.indexOf('#');
            if (fragmentIndex !== -1) {
                action = action.substr(0, fragmentIndex);
            }
            // We only want to encode the path fragment, not the querystring.
            // decodeURI() is run before encodeURI() to avoid double encoding characters
            // that are already encoded. We only want to encode characters that need encoding
            // but are not already encoded. This is what happens with international characters.
            // IE will encode a space to "%20", for example, but a turkish i remains as-is.
            // We are safe from doube-decoding. No browser uri decodes the uri in form.action.
            var domain = "", query = "", queryIndex = action.indexOf('?');
            if (queryIndex !== -1) {
                // tear off the query
                query = action.substr(queryIndex);
                action = action.substr(0, queryIndex);
            }
            // for IDN paths we must NOT encode the domain name.
            // domain may or may not be present (e.g. action of "foo.aspx" vs "http://domain/foo.aspx").
            if (/^https?\:\/\/.*$/gi.test(action)) {
                var domainPartIndex = action.indexOf("//") + 2,
                    slashAfterDomain = action.indexOf("/", domainPartIndex);
                if (slashAfterDomain === -1) {
                    // entire url is the domain (e.g. "http://foo.com")
                    domain = action;
                    action = "";
                }
                else {
                    domain = action.substr(0, slashAfterDomain);
                    action = action.substr(slashAfterDomain);
                }
            }
            action = domain + encodeURI(decodeURI(action)) + query;
        }
        request.set_url(action);
        request.get_headers()['X-MicrosoftAjax'] = 'Delta=true';
        request.get_headers()['Cache-Control'] = 'no-cache';
        request.set_timeout(this._asyncPostBackTimeout);
        request.add_completed(Function.createDelegate(this, this._onFormSubmitCompleted));
        request.set_body(formBody.toString());
        // eventArgs var reused for beginRequest below
        var panelsToUpdate, eventArgs, handler = this._get_eventHandlerList().getHandler("initializeRequest");
        if (handler) {
            panelsToUpdate = this._postBackSettings.panelsToUpdate;
            eventArgs = new Sys.WebForms.InitializeRequestEventArgs(request, this._postBackSettings.sourceElement, panelsToUpdate);
            handler(this, eventArgs);
            continueSubmit = !eventArgs.get_cancel();
        }

        if (!continueSubmit) {
            if (evt) {
                evt.preventDefault();
            }
            return;
        }
        
        if (eventArgs && eventArgs._updated) {
            // the initializeRequest event has changed the update panels to update
            panelsToUpdate = eventArgs.get_updatePanelsToUpdate();
            request.set_body(request.get_body().replace(panelID, this._createPanelID(panelsToUpdate, this._postBackSettings)));
        }

        // Save the scroll position
        this._scrollPosition = this._getScrollPosition();


        // If we're going on to make a new request (i.e. the user didn't cancel), and
        // there's still an ongoing request, we have to abort it. If we don't then it
        // will exhaust the browser's two connections per server limit very quickly.
        this.abortPostBack();

        handler = this._get_eventHandlerList().getHandler("beginRequest");
        if (handler) {
            eventArgs = new Sys.WebForms.BeginRequestEventArgs(request, this._postBackSettings.sourceElement,
                panelsToUpdate || this._postBackSettings.panelsToUpdate);
            handler(this, eventArgs);
        }
        
        // DevDiv Bugs 125825: Cancel any pending callbacks when an async postback begins
        if (this._originalDoCallback) {
            this._cancelPendingCallbacks();
        }

        this._request = request;
        // DevDiv Bugs 154403: A previous request may be loading scripts at this point.
        // If so, once its scripts finish, it will not be processed any further. 
        // We must clear _processingRequest, otherwise in this scenario the 2nd post would
        // not be abortable because it would appear that its response is already received.
        this._processingRequest = false;
        request.invoke();

        // Suppress the default form submit functionality
        if (evt) {
            evt.preventDefault();
        }
    },

    _onFormSubmitCompleted : function(sender, eventArgs) {
        // setting this true means the request can no longer be aborted
        this._processingRequest = true;

        // sender is the executor object

        if (sender.get_timedOut()) {
            this._endPostBack(this._createPageRequestManagerTimeoutError(), sender, null);
            return;
        }

        if (sender.get_aborted()) {
            this._endPostBack(null, sender, null);
            return;
        }

        // If the response isn't the response to the latest request, ignore it (last one wins)
        // This can happen if a 2nd async post begins before the response for the 1st is received
        if (!this._request || (sender.get_webRequest() !== this._request)) {
            return;
        }

        // If we have an invalid status code, go into error mode
        if (sender.get_statusCode() !== 200) {
            this._endPostBack(this._createPageRequestManagerServerError(sender.get_statusCode()), sender, null);
            return;
        }

        var data = this._parseDelta(sender);
        if (!data) return;
        
        var i, l;

        if (data.asyncPostBackControlIDsNode && data.postBackControlIDsNode &&
            data.updatePanelIDsNode && data.panelsToRefreshNode && data.childUpdatePanelIDsNode) {
            
            var oldUpdatePanelIDs = this._updatePanelIDs,
                oldUpdatePanelClientIDs = this._updatePanelClientIDs;
            var childUpdatePanelIDsString = data.childUpdatePanelIDsNode.content;
            var childUpdatePanelIDs = childUpdatePanelIDsString.length ? childUpdatePanelIDsString.split(',') : [];

            var asyncPostBackControlIDsArray = this._splitNodeIntoArray(data.asyncPostBackControlIDsNode);
            var postBackControlIDsArray = this._splitNodeIntoArray(data.postBackControlIDsNode);
            var updatePanelIDsArray = this._splitNodeIntoArray(data.updatePanelIDsNode);
            var panelsToRefreshIDs = this._splitNodeIntoArray(data.panelsToRefreshNode);

            // Validate that all the top level UpdatePanels that we plan to update exist
            // in the active document. We do this early so that we can later assume that
            // all referenced UpdatePanels exist.
            var v4 = data.version4;
            for (i = 0, l = panelsToRefreshIDs.length; i < l; i+= (v4 ? 2 : 1)) {
                var panelClientID = (v4 ? panelsToRefreshIDs[i+1] : "") || this._uniqueIDToClientID(panelsToRefreshIDs[i]);
                if (!document.getElementById(panelClientID)) {
                    this._endPostBack(Error.invalidOperation(String.format(Sys.WebForms.Res.PRM_MissingPanel, panelClientID)), sender, data);
                    return;
                }
            }
            
            var updatePanelData = this._processUpdatePanelArrays(
                updatePanelIDsArray,
                asyncPostBackControlIDsArray,
                postBackControlIDsArray, v4);
            updatePanelData.oldUpdatePanelIDs = oldUpdatePanelIDs;
            updatePanelData.oldUpdatePanelClientIDs = oldUpdatePanelClientIDs;
            updatePanelData.childUpdatePanelIDs = childUpdatePanelIDs;
            updatePanelData.panelsToRefreshIDs = panelsToRefreshIDs;
            data.updatePanelData = updatePanelData;
        }

        // Process data items
        data.dataItems = {};
        var node;
        for (i = 0, l = data.dataItemNodes.length; i < l; i++) {
            node = data.dataItemNodes[i];
            data.dataItems[node.id] = node.content;
        }
        for (i = 0, l = data.dataItemJsonNodes.length; i < l; i++) {
            node = data.dataItemJsonNodes[i];
            data.dataItems[node.id] = Sys.Serialization.JavaScriptSerializer.deserialize(node.content);
        }

        var handler = this._get_eventHandlerList().getHandler("pageLoading");
        if (handler) {
            handler(this, this._getPageLoadingEventArgs(data));
        }

        // DevDiv Bugs 127756: Load script includes before updating the HTML.
        // After updating the HTML, load script blocks, startup scripts, hidden fields, arrays, expandos, and onsubmit statements
        // Includes must be loaded first because the new DOM may have inline event handlers declared that depend on new
        // resources in the include files. They may also depend on resources in client script BLOCKS,
        // but script blocks can still load after updating the HTML because they load synchronously,
        // and therefore will still be loaded before any event handlers could fire. They can't be loaded before updating the HTML,
        // because dispose scripts must be executed first in case they dispose of resources declared by the script blocks.
        // Neither can the HTML be disposed of, then scripts loaded, and then the HTML updated, because that would cause a flicker.
        // This means ClientScriptIncludes and ClientScriptBlocks will load disjunct from one another during async updates,
        // but script includes should not depend on script blocks. This mechanism allows inline event handlers, a likely scenario,
        // whereas complex dependencies between script includes and script blocks is uncommon and not recommended.
        // Finally -- startup scripts could contain more script includes, so there are two calls to the script loader, which breaks the
        // completion handling of an async update into 3 separate functions: _onFormSubmitCompleted, _scriptIncludesLoadComplete, _scriptsLoadComplete.

        // DevDiv Bugs 154403: Race condition
        // The fix for 127756 (see above) introduced a race condition. It made it possible for a 2nd async post to
        // begin after the response for the 1st is received but before the HTML is updated, because the page is
        // interactive while scripts are loading. Previously, the HTML was updated immediately when a response was
        // received. This extra gap made it possible for the 2nd post to send stale ViewState and EventValidation data
        // to the server, which would ultimately be replaced by the processing of the 1st response once its scripts
        // have finished loading. A queuing mechanism for async posts cannot work -- because a queued post may
        // originate from HTML that is going to be destroyed or altered by an earlier post still being processed.
        // Instead -- the processing of an async response is altered slightly. Before this fix, once a response is
        // received it was definitely going to be fully processed (all scripts loaded, dom updated, etc). Now, if a
        // 2nd async post begins while scripts are loading for the first, the first response is effectively cancelled.
        // This means it might be possible to load script includes that aren't needed, but this should be harmless.
        // Example:
        // Click [Button1], async post sent to the server. The async response includes a script include [Script1.js],
        // and updated ViewState. PRM begins using the ScriptLoader to load [Script1.js] (it has not updated ViewState
        // yet). While the script is downloading, Click [Button2], which is in a different update panel. Async post is
        // sent to the server with the same ViewState as the first post.
        // Without this fix, Request #1 would continue processing after [Script1.js] has loaded, updating the DOM and
        // ViewState. But then Request #2 would complete with more updated HTML and ViewState. The result would be a
        // page with updated HTML for both UpdatePanels, but only the updated ViewState from the 2nd post.
        // The fix is that when [Script1.js] finishes loading during the processing for Request #1, it is detected
        // that a new request has been made, and no further processing occurs. The only thing Request #1 has caused is
        // its Script Includes to be downloaded, which should be harmless. This also brings the model closer to what it
        // originally was, before the fix for 127756, where it was not possible to perform a post before the viewstate
        // update from a previous post was committed. Note that as always, a 2nd post still cancels the 1st if the
        // response from the 1st is not yet received.
        
        // Read existing script elements (user code may have manually inserted a script element, this will ensure we know about those).
        // This is used to detect duplicates so we don't reload scripts that have already loaded.
        Sys._ScriptLoader.readLoadedScripts();

        // Starting batch mode for component creation to allow for
        // two-pass creation and components that reference each other.
        // endCreateComponents called from _scriptsLoadComplete.
        Sys.Application.beginCreateComponents();

        // First load ClientScriptIncludes
        var scriptLoader = Sys._ScriptLoader.getInstance();
        this._queueScripts(scriptLoader, data.scriptBlockNodes, true, false);
        
        // Save context into a member so that we can later get it from the completion callback
        this._processingRequest = true;

        // PRM does not support load timeout
        //                      timeout, completeCallback, failedCallback, timeoutCallback
        scriptLoader.loadScripts(0,
            Function.createDelegate(this, Function.createCallback(this._scriptIncludesLoadComplete, data)),
            Function.createDelegate(this, Function.createCallback(this._scriptIncludesLoadFailed, data)),
            null);        
    },
    
    _onWindowUnload : function(evt) {
        this.dispose();
    },

    _pageLoaded: function(initialLoad, data) {
        var handler = this._get_eventHandlerList().getHandler("pageLoaded");
        if (handler) {
            handler(this, this._getPageLoadedEventArgs(initialLoad, data));
        }
        if (!initialLoad) {
            // If this isn't the first page load (i.e. we are doing an async postback), we
            // need to re-raise the Application's load event.
            Sys.Application.raiseLoad();
        }
    },

    _pageLoadedInitialLoad : function(evt) {
        this._pageLoaded(true, null);
    },
    
    _parseDelta: function(executor) {
        // General format: length|type|id|content|
        var reply = executor.get_responseData();
        var delimiterIndex, len, type, id, content;
        var replyIndex = 0;
        var parserErrorDetails = null;
        var delta = [];

        while (replyIndex < reply.length) {
            // length| - from index to next delimiter
            delimiterIndex = reply.indexOf('|', replyIndex);
            if (delimiterIndex === -1) {
                parserErrorDetails = this._findText(reply, replyIndex);
                break;
            }
            len = parseInt(reply.substring(replyIndex, delimiterIndex), 10);
            if ((len % 1) !== 0) {
                parserErrorDetails = this._findText(reply, replyIndex);
                break;
            }
            replyIndex = delimiterIndex + 1;

            // type| - from index to next delimiter
            delimiterIndex = reply.indexOf('|', replyIndex);
            if (delimiterIndex === -1) {
                parserErrorDetails = this._findText(reply, replyIndex);
                break;
            }
            type = reply.substring(replyIndex, delimiterIndex);
            replyIndex = delimiterIndex + 1;

            // id| - from index to next delimiter
            delimiterIndex = reply.indexOf('|', replyIndex);
            if (delimiterIndex === -1) {
                parserErrorDetails = this._findText(reply, replyIndex);
                break;
            }
            id = reply.substring(replyIndex, delimiterIndex);
            replyIndex = delimiterIndex + 1;

            // content - the next 'len' characters after index
            if ((replyIndex + len) >= reply.length) {
                parserErrorDetails = this._findText(reply, reply.length);
                break;
            }
            // DevDiv 75383: We no longer encode null characters in the response content, so we no longer decode them either.
            // See comment in server side PageRequestManager for why.
            content = reply.substr(replyIndex, len);
            replyIndex += len;

            // terminating delimiter
            if (reply.charAt(replyIndex) !== '|') {
                parserErrorDetails = this._findText(reply, replyIndex);
                break;
            }

            replyIndex++;

            Array.add(delta, {type: type, id: id, content: content});
        }

        // If there was a parser error, go into error mode
        if (parserErrorDetails) {
            this._endPostBack(this._createPageRequestManagerParserError(String.format(Sys.WebForms.Res.PRM_ParserErrorDetails, parserErrorDetails)), executor, null);
            return null;
        }

        var updatePanelNodes = [];
        var hiddenFieldNodes = [];
        var arrayDeclarationNodes = [];
        var scriptBlockNodes = [];
        var scriptStartupNodes = [];
        var expandoNodes = [];
        var onSubmitNodes = [];
        var dataItemNodes = [];
        var dataItemJsonNodes = [];
        var scriptDisposeNodes = [];
        var asyncPostBackControlIDsNode, postBackControlIDsNode,
            updatePanelIDsNode, asyncPostBackTimeoutNode,
            childUpdatePanelIDsNode, panelsToRefreshNode, formActionNode,
            versionNode;

        // Sort delta by type
        for (var i = 0, l = delta.length; i < l; i++) {
            var deltaNode = delta[i];
            switch (deltaNode.type) {
                case "#":
                    versionNode = deltaNode;
                    break;
                case "updatePanel":
                    Array.add(updatePanelNodes, deltaNode);
                    break;
                case "hiddenField":
                    Array.add(hiddenFieldNodes, deltaNode);
                    break;
                case "arrayDeclaration":
                    Array.add(arrayDeclarationNodes, deltaNode);
                    break;
                case "scriptBlock":
                    Array.add(scriptBlockNodes, deltaNode);
                    break;
                case "fallbackScript":
                    // The previous script in the scriptBlock array must be an include script.
                    scriptBlockNodes[scriptBlockNodes.length - 1].fallback = deltaNode.id;
                case "scriptStartupBlock":
                    Array.add(scriptStartupNodes, deltaNode);
                    break;
                case "expando":
                    Array.add(expandoNodes, deltaNode);
                    break;
                case "onSubmit":
                    Array.add(onSubmitNodes, deltaNode);
                    break;
                case "asyncPostBackControlIDs":
                    asyncPostBackControlIDsNode = deltaNode;
                    break;
                case "postBackControlIDs":
                    postBackControlIDsNode = deltaNode;
                    break;
                case "updatePanelIDs":
                    updatePanelIDsNode = deltaNode;
                    break;
                case "asyncPostBackTimeout":
                    asyncPostBackTimeoutNode = deltaNode;
                    break;
                case "childUpdatePanelIDs":
                    childUpdatePanelIDsNode = deltaNode;
                    break;
                case "panelsToRefreshIDs":
                    panelsToRefreshNode = deltaNode;
                    break;
                case "formAction":
                    formActionNode = deltaNode;
                    break;
                case "dataItem":
                    Array.add(dataItemNodes, deltaNode);
                    break;
                case "dataItemJson":
                    Array.add(dataItemJsonNodes, deltaNode);
                    break;
                case "scriptDispose":
                    Array.add(scriptDisposeNodes, deltaNode);
                    break;
                case "pageRedirect":
                    if (versionNode && parseFloat(versionNode.content) >= 4) {
                        // asp.net 4+ will uri encode the entire value
                        deltaNode.content = unescape(deltaNode.content);
                    }
                    // DevDiv Bugs 100201: IE does not set referrer header on redirect if you set window.location, inject anchor node instead
                    // dynamic anchor technique only works on IE
                    if (Sys.Browser.agent === Sys.Browser.InternetExplorer) {
                        var anchor = document.createElement("a");
                        anchor.style.display = 'none';
                        // cancel bubble so body.onclick is not raised
                        anchor.attachEvent("onclick", cancelBubble);
                        anchor.href = deltaNode.content;
                        // do not append it to the body since that can cause the IE Operation Aborted error.
                        // do not append it to the form since that can have the same problem.
                        // appending it as a sibling _before_ the form is acceptable.
                        this._form.parentNode.insertBefore(anchor, this._form);
                        anchor.click();
                        anchor.detachEvent("onclick", cancelBubble);
                        this._form.parentNode.removeChild(anchor);
                        
                        function cancelBubble(e) {
                            e.cancelBubble = true;
                        }
                    }
                    else {
                        window.location.href = deltaNode.content;
                    }
                    return null;
                case "error":
                    // The id contains the HTTP status code and the content contains the message
                    this._endPostBack(this._createPageRequestManagerServerError(Number.parseInvariant(deltaNode.id), deltaNode.content), executor, null);
                    return null;
                case "pageTitle":
                    document.title = deltaNode.content;
                    break;
                case "focus":
                    this._controlIDToFocus = deltaNode.content;
                    break;
                default:
                    // If there was an unknown message, go into error mode
                    this._endPostBack(this._createPageRequestManagerParserError(String.format(Sys.WebForms.Res.PRM_UnknownToken, deltaNode.type)), executor, null);
                    return null;
            } // switch
        } // for (var i = 0, l = delta.length; i < l; i++)
        return {
            version4: versionNode ? (parseFloat(versionNode.content) >= 4) : false,
            executor: executor,
            updatePanelNodes: updatePanelNodes,
            hiddenFieldNodes: hiddenFieldNodes,
            arrayDeclarationNodes: arrayDeclarationNodes,
            scriptBlockNodes: scriptBlockNodes,
            scriptStartupNodes: scriptStartupNodes,
            expandoNodes: expandoNodes,
            onSubmitNodes: onSubmitNodes,
            dataItemNodes: dataItemNodes,
            dataItemJsonNodes: dataItemJsonNodes,
            scriptDisposeNodes: scriptDisposeNodes,
            asyncPostBackControlIDsNode: asyncPostBackControlIDsNode,
            postBackControlIDsNode: postBackControlIDsNode,
            updatePanelIDsNode: updatePanelIDsNode,
            asyncPostBackTimeoutNode: asyncPostBackTimeoutNode,
            childUpdatePanelIDsNode: childUpdatePanelIDsNode,
            panelsToRefreshNode: panelsToRefreshNode,
            formActionNode: formActionNode };
    },
    
    _processUpdatePanelArrays: function(updatePanelIDs, asyncPostBackControlIDs, postBackControlIDs, version4) {
        var newUpdatePanelIDs, newUpdatePanelClientIDs, newUpdatePanelHasChildrenAsTriggers;
        // DevDiv Bugs 154403:
        // Processes the lists of update panel data into their meaningful arrays.
        // This was factored as part of the fix for 154403 to allow the arrays to be
        // used in a temporary context before being committed to the PRM's instance
        // variables. _commitControls copies the neccessary arrays when the temporary
        // context can be committed (after all scripts have loaded).
        
        if (updatePanelIDs) {
            // Parse the array that has the UniqueIDs and split the data out.
            // The array contains UniqueIDs with either a 't' or 'f' prefix
            // indicating whether the panel has ChildrenAsTriggers enabled.
            var l = updatePanelIDs.length,
                m = version4 ? 2 : 1;
            newUpdatePanelIDs = new Array(l/m);
            newUpdatePanelClientIDs = new Array(l/m);
            newUpdatePanelHasChildrenAsTriggers = new Array(l/m);
            
            for (var i = 0, j = 0; i < l; i += m, j++) {
                var ct,
                    uniqueID = updatePanelIDs[i],
                    clientID = version4 ? updatePanelIDs[i+1] : "";
                // UpdatePanel's ClientID may be set. If so, both the uniqueID and clientID are given
                // otherwise only the uniqueID is given, which we can convert to a clientID
                ct = (uniqueID.charAt(0) === 't');
                uniqueID = uniqueID.substr(1);
                if (!clientID) {
                    clientID = this._uniqueIDToClientID(uniqueID);
                }
                // The three arrays are kept in sync by index
                newUpdatePanelHasChildrenAsTriggers[j] = ct;
                newUpdatePanelIDs[j] = uniqueID;
                newUpdatePanelClientIDs[j] = clientID;
            }
        }
        else {
            newUpdatePanelIDs = [];
            newUpdatePanelClientIDs = [];
            newUpdatePanelHasChildrenAsTriggers = [];
        }

        var newAsyncPostBackControlIDs = [];
        var newAsyncPostBackControlClientIDs = [];
        this._convertToClientIDs(asyncPostBackControlIDs, newAsyncPostBackControlIDs, newAsyncPostBackControlClientIDs, version4);

        var newPostBackControlIDs = [];
        var newPostBackControlClientIDs = [];
        this._convertToClientIDs(postBackControlIDs, newPostBackControlIDs, newPostBackControlClientIDs, version4);
        
        return {
            updatePanelIDs: newUpdatePanelIDs,
            updatePanelClientIDs: newUpdatePanelClientIDs,
            updatePanelHasChildrenAsTriggers: newUpdatePanelHasChildrenAsTriggers,
            asyncPostBackControlIDs: newAsyncPostBackControlIDs,
            asyncPostBackControlClientIDs: newAsyncPostBackControlClientIDs,
            postBackControlIDs: newPostBackControlIDs,
            postBackControlClientIDs: newPostBackControlClientIDs
        };
    },
    
    _queueScripts : function(scriptLoader, scriptBlockNodes, queueIncludes, queueBlocks) {
        ##DEBUGINTERNAL Sys.Debug.assert(queueIncludes || queueBlocks);
        for (var i = 0, l = scriptBlockNodes.length; i < l; i++) {
            var scriptBlockType = scriptBlockNodes[i].id;
            switch (scriptBlockType) {
                case "ScriptContentNoTags":
                    if (!queueBlocks) {
                        continue;
                    }
                    // The content contains raw JavaScript
                    scriptLoader.queueScriptBlock(scriptBlockNodes[i].content);
                    break;
                case "ScriptContentWithTags":
                    // The content contains serialized attributes for the script tag
                    var scriptTagAttributes;
                    eval("scriptTagAttributes = " + scriptBlockNodes[i].content);

                    if (scriptTagAttributes.src) {
                        // Don't reload a script that's already in the DOM
                        // or if not queuing includes
                        if (!queueIncludes || Sys._ScriptLoader.isScriptLoaded(scriptTagAttributes.src)) {
                            continue;
                        }
                    }
                    else if (!queueBlocks) {
                        // its a script block
                        continue;
                    }

                    scriptLoader.queueCustomScriptTag(scriptTagAttributes);
                    break;
                case "ScriptPath":
                    var script = scriptBlockNodes[i];
                    // Don't reload a script that's already in the DOM
                    // only if we aren't loading includes
                    if (!queueIncludes || Sys._ScriptLoader.isScriptLoaded(script.content)) {
                        continue;
                    }

                    // The content contains the URL reference of the script to load
                    scriptLoader.queueScriptReference(script.content, script.fallback);
                    break;
            }
        }        
    },

    _registerDisposeScript : function(panelID, disposeScript) {
        if (!this._scriptDisposes[panelID]) {
            this._scriptDisposes[panelID] = [disposeScript];
        }
        else {
            Array.add(this._scriptDisposes[panelID], disposeScript);
        }
    },
    
    _scriptIncludesLoadComplete: function(scriptLoader, data) {
        // script includes have loaded, completing the asynchronous portion of the update
        // processing. At this point, it is not possible for another async post to begin
        // before we are finished updating the HTML and processing the delta's hidden fields
        // and inline script blocks.
        // But another async post may have started while we were loading script includes.
        // If so, we abort processing this response, because the new request takes priority
        // and has already sent viewstate and event validation to the server. If we were to process
        // this old response further we could corrupt viewstate and event validation.
        // At worst we may have loaded some unnecessary script includes, but its likely they would
        // have been required for the new update anyway, and if not they should be harmless libraries.
        ##DEBUGINTERNAL Sys.Debug.assert(!!data, "Data must be provided as the first parameter.");
        ##DEBUGINTERNAL Sys.Debug.assert(!!data.executor, "data.executor is missing.");
        if (data.executor.get_webRequest() !== this._request) {
            return;
        }
        
        // DevDiv Bugs 188564: Update may not have an async postback timeout node.
        this._commitControls(data.updatePanelData,
            data.asyncPostBackTimeoutNode ? data.asyncPostBackTimeoutNode.content : null);

        // Update the form action (it may have changed due to cookieless session, etc.)
        if (data.formActionNode) {
            this._form.action = data.formActionNode.content;
        }
        
        var i, l, node;

        // Update the rendering for each delta panel and dispose all the contents.
        // The dispose can happen either through DOM elements that have dispose
        // support or through direct dispose registrations done on the server.
        for (i = 0, l = data.updatePanelNodes.length; i < l; i++) {
            node = data.updatePanelNodes[i];
            var updatePanelElement = document.getElementById(node.id);

            if (!updatePanelElement) {
                this._endPostBack(Error.invalidOperation(String.format(Sys.WebForms.Res.PRM_MissingPanel, node.id)), data.executor, data);
                return;
            }

            this._updatePanel(updatePanelElement, node.content);
        }

        // Update the dispose entries
        // We have to do this after we disposed all the panels since otherwise
        // we would run the dispose scripts on the brand new markup.
        for (i = 0, l = data.scriptDisposeNodes.length; i < l; i++) {
            node = data.scriptDisposeNodes[i];
            this._registerDisposeScript(node.id, node.content);
        }

        // Update the hidden fields
        // DevDiv Bugs 161922, 138251:
        // First remove transient fields that might be omitted in the response
        // If they are explictly defined they will be recreated.
        for (i = 0, l = this._transientFields.length; i < l; i++) {
            var field = document.getElementById(this._transientFields[i]);
            if (field) {
                // when removing the field, be sure to remove its parent container, if any
                var toRemove = field._isContained ? field.parentNode : field;
                toRemove.parentNode.removeChild(toRemove);
            }
        }
        for (i = 0, l = data.hiddenFieldNodes.length; i < l; i++) {
            node = data.hiddenFieldNodes[i];
            this._createHiddenField(node.id, node.content);
        }
        
        if (data.scriptsFailed) {
            // the script includes in the first step failed to load. Raise the error now that the DOM is updated.
            throw Sys._ScriptLoader._errorScriptLoadFailed(data.scriptsFailed.src, data.scriptsFailed.multipleCallbacks);
        }
        
        // continue on to loading literal client script blocks, arrays, expandos, startup scripts, and onSubmitStatements (in that order)

        // Load literal script blocks
        this._queueScripts(scriptLoader, data.scriptBlockNodes, false, true);

        // Update array declarations
        var arrayScript = '';
        for (i = 0, l = data.arrayDeclarationNodes.length; i < l; i++) {
            node = data.arrayDeclarationNodes[i];
            arrayScript += "Sys.WebForms.PageRequestManager._addArrayElement('" + node.id + "', " + node.content + ");\r\n";
        }

        // Update expandos
        var expandoScript = '';
        for (i = 0, l = data.expandoNodes.length; i < l; i++) {
            node = data.expandoNodes[i];
            expandoScript += node.id + " = " + node.content + "\r\n";
        }

        // Execute these dynamically created scripts through the ScriptLoader so that
        // they get executed in the global window context. If we execute them through
        // calls to eval() then they will evaluate in this function's context, which
        // is incorrect.
        if (arrayScript.length) {
            scriptLoader.queueScriptBlock(arrayScript);
        }
        if (expandoScript.length) {
            scriptLoader.queueScriptBlock(expandoScript);
        }
        
        this._queueScripts(scriptLoader, data.scriptStartupNodes, true, true);

        // Update onsubmit statements
        // Create a function that calls the submit statement and otherwise returns true;
        var onSubmitStatementScript = '';
        for (i = 0, l = data.onSubmitNodes.length; i < l; i++) {
            if (i === 0) {
                onSubmitStatementScript = 'Array.add(Sys.WebForms.PageRequestManager.getInstance()._onSubmitStatements, function() {\r\n';
            }
            onSubmitStatementScript += data.onSubmitNodes[i].content + "\r\n";
        }
        if (onSubmitStatementScript.length) {
            onSubmitStatementScript += "\r\nreturn true;\r\n});\r\n";
            scriptLoader.queueScriptBlock(onSubmitStatementScript);
        }

        // PRM does not support load timeout
        //                      timeout, completeCallback, failedCallback, timeoutCallback
        // no failed callback -- if there is a failure ScriptLoader will throw.
        scriptLoader.loadScripts(0,
            Function.createDelegate(this, Function.createCallback(this._scriptsLoadComplete, data)), null, null);

        // Do not add code after the call to loadScripts(). If you need to do extra
        // processing after scripts are loaded, do it in _scriptsLoadComplete.
    },
    
    _scriptIncludesLoadFailed : function(scriptLoader, scriptElement, multipleCallbacks, data) {
        // called when script includes fail to load from _onFormSubmitComplete
        // save error details so we can raise the error later
        data.scriptsFailed = { src: scriptElement.src, multipleCallbacks: multipleCallbacks };
        this._scriptIncludesLoadComplete(scriptLoader, data);
    },

    _scriptsLoadComplete: function(scriptLoader, data) {
        ##DEBUGINTERNAL Sys.Debug.assert(!!data, "Data must be provided as the first parameter.");
        ##DEBUGINTERNAL Sys.Debug.assert(!!data.executor, "data.executor is missing.");
        // This function gets called after all scripts have been loaded by the PRM.
        // It might also get called directly if there aren't any scripts to load.
        // Its purpose is to finish off the processing of a postback.
        var response = data.executor;

        // These two variables are used by ASP.net callbacks.
        // Because of how callbacks work, we have to re-initialize the
        // variables to an empty state so that their values don't keep
        // growing on every async postback. Then we have to re-initialize
        // the callback process.
        if (window.__theFormPostData) {
            window.__theFormPostData = "";
        }
        if (window.__theFormPostCollection) {
            window.__theFormPostCollection = [];
        }
        if (window.WebForm_InitCallback) {
            window.WebForm_InitCallback();
        }

        // Restore scroll position
        if (this._scrollPosition) {
            // window.scrollTo() is supported by IE and Firefox (and possibly Safari)
            if (window.scrollTo) {
                window.scrollTo(this._scrollPosition.x, this._scrollPosition.y);
            }
            this._scrollPosition = null;
        }

        Sys.Application.endCreateComponents();

        // Raise completion events
        this._pageLoaded(false, data);

        this._endPostBack(null, response, data);

        // Set focus
        if (this._controlIDToFocus) {
            var focusTarget;
            var oldContentEditableSetting;
            if (Sys.Browser.agent === Sys.Browser.InternetExplorer) {
                // IE6 and IE7 have a bug where you can't focus certain elements
                // if they've been changed in the DOM. To work around this they
                // suggested turning off contentEditable temporarily while focusing
                // the target element.
                var targetControl = $get(this._controlIDToFocus);

                focusTarget = targetControl;
                // If the focus control isn't focusable, default to the first focusable child
                if (targetControl && (!WebForm_CanFocus(targetControl))) {
                    focusTarget = WebForm_FindFirstFocusableChild(targetControl);
                }
                // If we found the focus target and it supports contentEditable then
                // turn it off. Otherwise forget we ever tried to disable content editing.
                if (focusTarget && (typeof(focusTarget.contentEditable) !== "undefined")) {
                    oldContentEditableSetting = focusTarget.contentEditable;
                    focusTarget.contentEditable = false;
                }
                else {
                    focusTarget = null;
                }
            }
            WebForm_AutoFocus(this._controlIDToFocus);
            if (focusTarget) {
                // If we did the contentEditable hack, reset the value
                focusTarget.contentEditable = oldContentEditableSetting;
            }
            this._controlIDToFocus = null;
        }
    },

    _splitNodeIntoArray : function(node) {
        var str = node.content;
        var arr = str.length ? str.split(',') : [];
        return arr;
    },

    _uniqueIDToClientID : function(uniqueID) {
        // Convert unique IDs to client IDs by replacing all '$' with '_'
        return uniqueID.replace(/\$/g, '_');
    },
    
    _updateControls: function(updatePanelIDs, asyncPostBackControlIDs, postBackControlIDs, asyncPostBackTimeout, version4) {
        this._commitControls(
            this._processUpdatePanelArrays(updatePanelIDs, asyncPostBackControlIDs, postBackControlIDs, version4),
            asyncPostBackTimeout);
    },
    
    _updatePanel: function(updatePanelElement, rendering) {
        for (var updatePanelID in this._scriptDisposes) {
            if (this._elementContains(updatePanelElement, document.getElementById(updatePanelID))) {
                // Run all the dispose scripts for this panel
                var disposeScripts = this._scriptDisposes[updatePanelID];
                for (var i = 0, l = disposeScripts.length; i < l; i++) {
                    eval(disposeScripts[i]);
                }

                // Remove the dispose entries for this panel
                delete this._scriptDisposes[updatePanelID];
            }
        }

        Sys.Application.disposeElement(updatePanelElement, true);

        // Update the region with the new UpdatePanel content
        updatePanelElement.innerHTML = rendering;
    },

    _validPosition : function(position) {
        return (typeof(position) !== "undefined") && (position !== null) && (position !== 0);
    }
}

Sys.WebForms.PageRequestManager.getInstance = function() {
    /// <summary>Gets the current instance of the PageRequestManager.</summary>
    /// <returns type="Sys.WebForms.PageRequestManager"/>
    var prm = Sys.WebForms.PageRequestManager._instance;
    if (!prm) {
        prm = Sys.WebForms.PageRequestManager._instance = new Sys.WebForms.PageRequestManager();
    }
    return prm;
}

Sys.WebForms.PageRequestManager._addArrayElement = function(arrayName) {
    if (!window[arrayName]) {
        // Create array if doesn't already exist
        window[arrayName] = new Array();
    }

    // add the argument list, not counting arrayName
    // note: cannot use Array.addRange or Array.dequeue
    // because 'arguments' is not actually an instance of an Array.
    for (var i = 1, l = arguments.length; i < l; i++) {
        Array.add(window[arrayName], arguments[i]);
    }
}

Sys.WebForms.PageRequestManager._initialize = function() {
    var prm = Sys.WebForms.PageRequestManager.getInstance();
    prm._initializeInternal.apply(prm, arguments);
}

Sys.WebForms.PageRequestManager.registerClass('Sys.WebForms.PageRequestManager');
