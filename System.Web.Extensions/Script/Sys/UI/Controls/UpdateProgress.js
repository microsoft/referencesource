#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="UpdateProgress.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI._UpdateProgress = function(element) {
    Sys.UI._UpdateProgress.initializeBase(this,[element]);
    this._displayAfter = 500;
    this._dynamicLayout = true;
    this._associatedUpdatePanelId = null;
    this._beginRequestHandlerDelegate = null;
    this._startDelegate = null;
    this._endRequestHandlerDelegate = null;
    this._pageRequestManager = null;
    this._timerCookie = null;
}
Sys.UI._UpdateProgress.prototype = {
    get_displayAfter: function() {
        /// <value type="Number"/>
        return this._displayAfter;
    },
    set_displayAfter: function(value) {
        this._displayAfter = value;
    },
    get_dynamicLayout: function() {
        /// <value type="Boolean"/>
        return this._dynamicLayout;
    },
    set_dynamicLayout: function(value) {
        this._dynamicLayout = value;
    },
    get_associatedUpdatePanelId: function() {
        /// <value type="String" mayBeNull="true" />
        return this._associatedUpdatePanelId;
    },
    set_associatedUpdatePanelId: function(value) {
        this._associatedUpdatePanelId = value;
    },
    get_role: function() {
        /// <value type="String"></value>
        return "status";
    },
    _clearTimeout: function() {
        if (this._timerCookie) {
            window.clearTimeout(this._timerCookie);
            this._timerCookie = null;
        }
    },
    _getUniqueID: function(clientID) {
        var i = Array.indexOf(this._pageRequestManager._updatePanelClientIDs, clientID);
        return i === -1 ? null : this._pageRequestManager._updatePanelIDs[i];
    },
    _handleBeginRequest: function(sender, arg) {
        var curElem = arg.get_postBackElement(),
            showProgress = true,
            upID = this._associatedUpdatePanelId;
        if (this._associatedUpdatePanelId) {
            var updating = arg.get_updatePanelsToUpdate();
            if (updating && updating.length) {
                showProgress = (Array.contains(updating, upID) || Array.contains(updating, this._getUniqueID(upID)))
            }
            else {
                showProgress = false;
            }
        }
        while (!showProgress && curElem) {
            if (curElem.id && this._associatedUpdatePanelId === curElem.id) {
                showProgress = true; 
            }
            curElem = curElem.parentNode; 
        } 
        if (showProgress) {
            this._timerCookie = window.setTimeout(this._startDelegate, this._displayAfter);
        }
    },
    _startRequest: function() {
        if (this._pageRequestManager.get_isInAsyncPostBack()) {
            var element = this.get_element();
            if (this._dynamicLayout) {
                element.style.display = 'block';
            }
            else {
                element.style.visibility = 'visible';
            }
            if (this.get_role() === "status") {
                element.setAttribute("aria-hidden", "false");
            }
        }
        this._timerCookie = null;
    },
    _handleEndRequest: function(sender, arg) {
        var element = this.get_element();
        if (this._dynamicLayout) {
            element.style.display = 'none';
        }
        else {
            element.style.visibility = 'hidden';
        }
        if (this.get_role() === "status") {
            element.setAttribute("aria-hidden", "true");
        }
        this._clearTimeout();
    },
    dispose: function() {
        if (this._beginRequestHandlerDelegate !== null) {
            this._pageRequestManager.remove_beginRequest(this._beginRequestHandlerDelegate);
            this._pageRequestManager.remove_endRequest(this._endRequestHandlerDelegate);
            this._beginRequestHandlerDelegate = null;
            this._endRequestHandlerDelegate = null;
        }
        // DevDiv Bugs 172834: Should clear the timeout on dispose
        this._clearTimeout();
        Sys.UI._UpdateProgress.callBaseMethod(this,"dispose");
    },
    initialize: function() {
        Sys.UI._UpdateProgress.callBaseMethod(this, 'initialize');
        if (this.get_role() === "status") {
            this.get_element().setAttribute("aria-hidden", "true");
        }
    	this._beginRequestHandlerDelegate = Function.createDelegate(this, this._handleBeginRequest);
    	this._endRequestHandlerDelegate = Function.createDelegate(this, this._handleEndRequest);
    	this._startDelegate = Function.createDelegate(this, this._startRequest);
    	if (Sys.WebForms && Sys.WebForms.PageRequestManager) {
           this._pageRequestManager = Sys.WebForms.PageRequestManager.getInstance();
    	}
    	if (this._pageRequestManager !== null ) {
           // Review: should we throw if there's no pageRequestManager
    	    this._pageRequestManager.add_beginRequest(this._beginRequestHandlerDelegate);
    	    this._pageRequestManager.add_endRequest(this._endRequestHandlerDelegate);
    	}
    }
}
Sys.UI._UpdateProgress.registerClass('Sys.UI._UpdateProgress', Sys.UI.Control);