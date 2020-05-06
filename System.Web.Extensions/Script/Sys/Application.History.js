#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Application.History.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

// History fields ("entry" in the code always designates a string representation of the state):

Sys.Application._appLoadHandler = null;
Sys.Application._beginRequestHandler = null;
Sys.Application._clientId = null;
Sys.Application._currentEntry = '';
Sys.Application._endRequestHandler = null;
Sys.Application._history = null;
Sys.Application._enableHistory = false;
#if DEBUG
Sys.Application._historyEnabledInScriptManager = false;
#endif
Sys.Application._historyFrame = null;
Sys.Application._historyInitialized = false;
Sys.Application._historyPointIsNew = false;
Sys.Application._ignoreTimer = false;
Sys.Application._initialState = null;
Sys.Application._state = {};
Sys.Application._timerCookie = 0;
Sys.Application._timerHandler = null;
Sys.Application._uniqueId = null;

// Add methods to support history

Sys._Application.prototype.get_stateString = function() {
    /// <value type="String" locid="P:J#Sys.Application.stateString"/>
    var hash = null;
    
    // Need to do an additional check because of a bug in Firefox
    // (https://bugzilla.mozilla.org/show_bug.cgi?id=378962).
    // Firefox incorrectly calls decodeURIComponent on window.hash
    // before returning it.
    if (Sys.Browser.agent === Sys.Browser.Firefox) {
        var href = window.location.href;
        var hashIndex = href.indexOf('#');
        if (hashIndex !== -1) {
            hash = href.substring(hashIndex + 1);
        }
        else {
            hash = "";
        }
        return hash;
    }
    else {
        hash = window.location.hash;
    }
    
    if ((hash.length > 0) && (hash.charAt(0) === '#')) {
        hash = hash.substring(1);
    }

    return hash;
};

Sys._Application.prototype.get_enableHistory = function() {
    /// <value type="Boolean" locid="P:J#Sys.Application.enableHistory"/>
    return this._enableHistory;
};

Sys._Application.prototype.set_enableHistory = function(value) {
    #if DEBUG
    if (this._initialized && !this._initializing) {
        throw Error.invalidOperation(Sys.Res.historyCannotEnableHistory);
    }
    else if (this._historyEnabledInScriptManager && !value) {
        throw Error.invalidOperation(Sys.Res.invalidHistorySettingCombination);
    }
    #endif
    this._enableHistory = value;
};

Sys._Application.prototype.add_navigate = function(handler) {
    /// <summary locid="E:J#Sys.Application.navigate"/>
    /// <param name="handler" type="Function"></param>
    this.get_events().addHandler("navigate", handler);
};

Sys._Application.prototype.remove_navigate = function(handler) {
    /// <param name="handler" type="Function"></param>
    this.get_events().removeHandler("navigate", handler);
};

Sys._Application.prototype.addHistoryPoint = function(state, title) {
    /// <summary locid="M:J#Sys.Application.addHistoryPoint"/>
    /// <param name="state" type="Object">
    ///     A dictionary of state bits that will be added to the main state
    ///     to form the global state of the new history point.
    ///     The state must be a string dictionary. The application is responsible
    ///     for converting the state bits from and into the relevant types.
    /// </param>
    /// <param name="title" type="String" optional="true" mayBeNull="true">The title for the new history point.</param>
    #if DEBUG 
    if (!this._enableHistory) throw Error.invalidOperation(Sys.Res.historyCannotAddHistoryPointWithHistoryDisabled);
    for (var n in state) {
        var v = state[n];
        var t = typeof(v);
        if ((v !== null) && ((t === 'object') || (t === 'function') || (t === 'undefined'))) {
            throw Error.argument('state', Sys.Res.stateMustBeStringDictionary);
        }
    }
    #endif
    this._ensureHistory();
    var initialState = this._state;
    for (var key in state) {
        var value = state[key];
        if (value === null) {
            if (typeof(initialState[key]) !== 'undefined') {
                delete initialState[key];
            }
        }
        else {
            initialState[key] = value;
        }
    }
    var entry = this._serializeState(initialState);
    this._historyPointIsNew = true;
    this._setState(entry, title);
    // Raising navigate even when creating a history point so that independant
    // observers such as PermaLink can get all state changes.
    this._raiseNavigate();
};

Sys._Application.prototype.setServerId = function(clientId, uniqueId) {
    /// <summary locid="M:J#Sys.Application.setServerId"/>
    /// <param name="clientId" type="String"/>
    /// <param name="uniqueId" type="String"/>
    this._clientId = clientId;
    this._uniqueId = uniqueId;
};

Sys._Application.prototype.setServerState = function(value) {
    /// <summary locid="M:J#Sys.Application.setServerState"/>
    /// <param name="value" type="String"/>
    this._ensureHistory();
    this._state.__s = value;
    this._updateHiddenField(value);
};

Sys._Application.prototype._deserializeState = function(entry) {
    // <summary>Deserializes a querystring to a string dictionary.</summary>
    // <param name="entry" type="String" mayBeNull="true">The serialized dictionary.</returns>
    // <returns type="Object">The deserialized string dictionary.</param>
    var result = {};
    entry = entry || '';
    // Extract the server part (everything after &&)
    var serverSeparator = entry.indexOf('&&');
    if ((serverSeparator !== -1) && (serverSeparator + 2 < entry.length)) {
        result.__s = entry.substr(serverSeparator + 2);
        entry = entry.substr(0, serverSeparator);
    }
    // Deserialize the client part
    var tokens = entry.split('&');
    for (var i = 0, l = tokens.length; i < l; i++) {
        var token = tokens[i];
        var equal = token.indexOf('=');
        if ((equal !== -1) && (equal + 1 < token.length)) {
            var name = token.substr(0, equal);
            var value = token.substr(equal + 1);
            result[name] = decodeURIComponent(value);
        }
    }
    return result;
};

Sys._Application.prototype._enableHistoryInScriptManager = function() {
    this._enableHistory = true;
    #if DEBUG
    this._historyEnabledInScriptManager = true;
    #endif
};

Sys._Application.prototype._ensureHistory = function() {
    if (!this._historyInitialized && this._enableHistory) {
        if ((Sys.Browser.agent === Sys.Browser.InternetExplorer) && 
            ((!document.documentMode) || document.documentMode < 8)) {
            this._historyFrame = document.getElementById('__historyFrame');
            ##DEBUG if (!this._historyFrame) throw Error.invalidOperation(Sys.Res.historyMissingFrame);
            this._ignoreIFrame = true;
        }
        this._timerHandler = Function.createDelegate(this, this._onIdle);
        this._timerCookie = window.setTimeout(this._timerHandler, 100);
        
        try {
            this._initialState = this._deserializeState(this.get_stateString());
        } catch(e) {}
        
        this._historyInitialized = true;
    }
};

Sys._Application.prototype._navigate = function(entry) {
    this._ensureHistory();

    var state = this._deserializeState(entry);
    
    if (this._uniqueId) {
        var oldServerEntry = this._state.__s || '';
        var newServerEntry = state.__s || '';
        if (newServerEntry !== oldServerEntry) {
            this._updateHiddenField(newServerEntry);
            __doPostBack(this._uniqueId, newServerEntry);
            this._state = state;
            return;
        }
    }
    this._setState(entry);
    this._state = state;
    this._raiseNavigate();
};

Sys._Application.prototype._onIdle = function() {
    delete this._timerCookie;
    
    var entry = this.get_stateString();
    if (entry !== this._currentEntry) {
        if (!this._ignoreTimer) {
            this._historyPointIsNew = false;
            this._navigate(entry);
        }
    }
    else {
        this._ignoreTimer = false;
    }
    this._timerCookie = window.setTimeout(this._timerHandler, 100);
};

// devdiv 617545: Ajax history iframe rendered in IE8+ after .NET 4.5 installed
// the fix of DevDiv 13920 enables iframe rendering on server side for IE8+ 
// as the latter could be in a lower document mode. On client side, 
// in case documentMode >= 8, make sure that the _onIframeLoad handler does nothing
// Note document.documentMode for IE7 is undefined 
Sys._Application.prototype._onIFrameLoad = function(entry) {
    if ((!document.documentMode) || document.documentMode < 8 ) {
        this._ensureHistory();
        if (!this._ignoreIFrame) {
            this._historyPointIsNew = false;
            this._navigate(entry);
        }
        this._ignoreIFrame = false;
    }
};

Sys._Application.prototype._onPageRequestManagerBeginRequest = function(sender, args) {
    this._ignoreTimer = true;
    this._originalTitle = document.title;
};

Sys._Application.prototype._onPageRequestManagerEndRequest = function(sender, args) {
    var dataItem = args.get_dataItems()[this._clientId];
    var originalTitle = this._originalTitle;
    this._originalTitle = null;

    // Reset event target if set by history
    var eventTarget = document.getElementById("__EVENTTARGET");
    if (eventTarget && eventTarget.value === this._uniqueId) {
        eventTarget.value = '';
    }
    if (typeof(dataItem) !== 'undefined') {
        this.setServerState(dataItem);
        this._historyPointIsNew = true;
    }
    else {
        this._ignoreTimer = false;
    }
    var entry = this._serializeState(this._state);
    if (entry !== this._currentEntry) {
        this._ignoreTimer = true;
        if (typeof(originalTitle) === "string") {
            // At this point the PRM has already updated the document title.
            // We want the title of the target entry will be the new page title
            // but since the title has been set, it has been applied to the current
            // entry. Temporarily restoring the old title while setting the hash.
            // However, this has the opposite effect in IE7.
            if (Sys.Browser.agent !== Sys.Browser.InternetExplorer || Sys.Browser.version > 7) {
                var newTitle = document.title;
                document.title = originalTitle;
                this._setState(entry);
                document.title = newTitle;
            }
            else {
                this._setState(entry);
            }
            this._raiseNavigate();
        }
        else {
            this._setState(entry);
            this._raiseNavigate();
        }
    }
};

Sys._Application.prototype._raiseNavigate = function() {
    var isNew = this._historyPointIsNew;
    var h = this.get_events().getHandler("navigate");
    var stateClone = {};
    for (var key in this._state) {
        if (key !== '__s') {
            stateClone[key] = this._state[key];
        }
    }
    var args = new Sys.HistoryEventArgs(stateClone);
    if (h) {
        h(this, args);
    }
    // Dev10 475308 
    // History: Firefox 3.0: Entries on History dropdown button are shifted after navigation
    // The entry in the history dropdown ends up with the title of the page you came from rather than
    // the title of the page you navigated to. This is just how it works. In order for the title
    // of the entry in the dropdown to be correct, the title must be set before the hash is changed,
    // which obviously is not possible since we only set the title in response to the hash changing.
    // So, we re-do another hash change, to force FF to re-think the title of the entry. Since the value
    // is the same, it doesn't actually cause a navigation. 
    // The only problem is that doing this causes a page refresh if there isn't a hash at all.
    // There is no other known workaround for this problem in that scenario, so this bug remains 
    // when you navigate from a certain state to one with no hash. Setting a blank hash like "#" 
    // makes things worse as it is considered a new state and resets the history.
    // See Mozilla bug: https://bugzilla.mozilla.org/show_bug.cgi?id=442060
    // Dev10 622219: don't use the go() workaround if the document is an iframe and the top window
    // does not have a hash, because that will cause the page to refresh.
    // If the top document is in a different domain we can't detect that due to the same origin policy,
    // so the try/catch prevents an error. In that scenario it is the best we can do.
    if (!isNew) {
        // !isNew means we raised the navigate event because the user navigated with the browser,
        // not because a history point was added. In this scenario, the history point is navigated
        // to first, then the title is changed in reaction. In FF that causes the history point navigated
        // to to get the title of the page we were on before. Applying this hack fixes it. Unfortunately
        // the hack required varies in 3.0 and 3.5.
        var err;
        try {
            if ((Sys.Browser.agent === Sys.Browser.Firefox) && window.location.hash &&
                (!window.frameElement || window.top.location.hash)) {
                (Sys.Browser.version < 3.5) ?
                    window.history.go(0) :
                    location.hash = this.get_stateString();
            }
        }
        catch(err) {
        }
    }
};

Sys._Application.prototype._serializeState = function(state) {
    // <summary>Serializes a string dictionary to a querystring form.</summary>
    // <param type="Object" name="state">The dictionary to serialize.</param>
    // <returns type="String">The serialized dictionary.</returns>
    var serialized = [];
    for (var key in state) {
        var value = state[key];
        if (key === '__s') {
            var serverState = value;
        }
        else {
            ##DEBUG if (key.indexOf('=') !== -1) throw Error.argument('state', Sys.Res.stateFieldNameInvalid);
            serialized[serialized.length] = key + '=' + encodeURIComponent(value);
        }
    }
    return serialized.join('&') + (serverState ? '&&' + serverState : '');
};

Sys._Application.prototype._setState = function(entry, title) {
    if (this._enableHistory) {
        entry = entry || '';
        if (entry !== this._currentEntry) {
            // Replace the hash on the current ASP.NET form if it exists
            if (window.theForm) {
                var action = window.theForm.action;
                var hashIndex = action.indexOf('#');
                window.theForm.action = ((hashIndex !== -1) ? action.substring(0, hashIndex) : action) + '#' + entry;
            }
        
            if (this._historyFrame && this._historyPointIsNew) {
                // need to HTML-encode the document title
                var newDiv = document.createElement("div");
                newDiv.appendChild(document.createTextNode(title || document.title));
                var htmlEncodedTitle = newDiv.innerHTML;

                this._ignoreIFrame = true;
                var frameDoc = this._historyFrame.contentWindow.document;
                frameDoc.open("javascript:'<html></html>'");
                frameDoc.write("<html><head><title>" + htmlEncodedTitle +
                    "</title><scri" + "pt type=\"text/javascript\">parent.Sys.Application._onIFrameLoad(" + 
                    Sys.Serialization.JavaScriptSerializer.serialize(entry) +
                    ");</scri" + "pt></head><body></body></html>");
                frameDoc.close();
            }
            this._ignoreTimer = false;
            this._currentEntry = entry;
            if (this._historyFrame || this._historyPointIsNew) {
                // Dev 10 Bug: 594690
                // dont set the hash unless this is a new history point, since that's pointless, and it
                // causes an occassional loss of forward state in IE8.
                // unless the iframe is in use, we need to set it to keep the hash in sync with it.
                var currentHash = this.get_stateString();
                if (entry !== currentHash) {
                    // Check length before setting it
                    #if DEBUG
                    var loc = document.location;
                    if (loc.href.length - loc.hash.length + entry.length > 2048) {
                        throw Error.invalidOperation(String.format(Sys.Res.urlTooLong, 2048));
                    }
                    #endif
                    window.location.hash = entry;
                    // reset currentEntry to the statestring in case the browser has
                    // encoded any characters in it
                    this._currentEntry = this.get_stateString();
                    if ((typeof(title) !== 'undefined') && (title !== null)) {
                        document.title = title;
                    }
                }
            }
            this._historyPointIsNew = false;
        }
    }
};

Sys._Application.prototype._updateHiddenField = function(value) {
    if (this._clientId) {
        var serverStateField = document.getElementById(this._clientId);
        if (serverStateField) {
            serverStateField.value = value;
        }
    }
};
