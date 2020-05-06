#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="XMLHttpExecutor.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Net.XMLDOM = function(markup) {
    /// <summary>Creates an XML document from an XML string.</summary>
    /// <param name="markup" type="String">The XML string to parse.</param>
    if (!window.DOMParser) {
        // DevDiv Bugs 150054: Msxml2.DOMDocument (version independent ProgID) required for mobile IE
        var progIDs = [ 'Msxml2.DOMDocument.3.0', 'Msxml2.DOMDocument' ];
        for (var i = 0, l = progIDs.length; i < l; i++) {
            try {
                var xmlDOM = new ActiveXObject(progIDs[i]);
                xmlDOM.async = false;
                xmlDOM.loadXML(markup);
                xmlDOM.setProperty('SelectionLanguage', 'XPath');
                return xmlDOM;
            }
            catch (ex) {
            }
        }
    }
    else {
        // Mozilla browsers have a DOMParser
        try {
            var domParser = new window.DOMParser();
            return domParser.parseFromString(markup, 'text/xml');
        }
        catch (ex) {
        }
    }
    return null;
}

Sys.Net.XMLHttpExecutor = function() {
    /// <summary>XMLHttpExecutor</summary>

    Sys.Net.XMLHttpExecutor.initializeBase(this);

    var _this = this;
    this._xmlHttpRequest = null;
    this._webRequest = null;
    this._responseAvailable = false;
    this._timedOut = false;
    this._timer = null;
    this._aborted = false;
    this._started = false;

    // Parentheses added around closure methods to work around a preprocessor bug
    // (the preprocessor loses context in that situation and uses the last closure as the name of the current
    // function even when its end has been reached and another function has started.)
    // DevDiv 169493
    this._onReadyStateChange = (function () {
        /*
            readyState values:
            0 = uninitialized
            1 = loading
            2 = loaded
            3 = interactive
            4 = complete
        */
        if (_this._xmlHttpRequest.readyState === 4 /*complete*/) {
            // DevDiv 58581:
            // When a request is pending when the page is closed (navigated away, postback, etc)
            // in FF and Safari, the request is aborted just as if abort() was called on the 
            // xmlhttprequest object.
            // However, even aborted requests have a readyState of 4, which we treat as successful.
            // This happened for example if a regular postback occurred during a partial update request.
            // In FF if you access the 'status' field on an aborted request, an error is thrown,
            // so the error console displayed an error when this happened.
            // On Safari it isn't an error, but status is undefined. That caused PRM to get the completed
            // event, and since the status is not 200, it raises an error.
            // IE and Opera ignore pending requests, or their readyState isn't 4.
            // Devdiv 983362:
            // Many webkit browsers now send a status code 0 when the request has been aborted. 
            try {
                if (typeof(_this._xmlHttpRequest.status) === "undefined" || _this._xmlHttpRequest.status === 0) {
                    // its an aborted request in webkit browsers(e.g. chrome,FF), ignore it
                    return;
                }
            }
            catch(ex) {
                // its an aborted request in Firefox, ignore it
                return;
            }
            
            _this._clearTimer();
            _this._responseAvailable = true;
            #if DEBUG
            #else
            try {
            #endif
                // DevDiv Bugs 148214: Use try/finally to ensure cleanup occurs even
                // if the completed callback causes an exception (such as with async
                // postbacks where a server-side exception occurred)
                _this._webRequest.completed(Sys.EventArgs.Empty);
            #if DEBUG
            #else
            }
            finally {
            #endif
                if (_this._xmlHttpRequest != null) {
                    _this._xmlHttpRequest.onreadystatechange = Function.emptyMethod;
                    _this._xmlHttpRequest = null;
                }
            #if DEBUG
            #else
            }
            #endif
        }
    });

    this._clearTimer = (function() {
        if (_this._timer != null) {
            window.clearTimeout(_this._timer);
            _this._timer = null;
        }
    });

    this._onTimeout = (function() {
        if (!_this._responseAvailable) {
            _this._clearTimer();
            _this._timedOut = true;
            _this._xmlHttpRequest.onreadystatechange = Function.emptyMethod;
            _this._xmlHttpRequest.abort();
            _this._webRequest.completed(Sys.EventArgs.Empty);
            _this._xmlHttpRequest = null;
        }
    });

}

Sys.Net.XMLHttpExecutor.prototype = {

    get_timedOut: function() {
        /// <summary>Returns whether the executor has timed out.</summary>
        /// <value type="Boolean">True if the executor has timed out.</value>
        return this._timedOut;
    },

    get_started: function() {
        /// <summary>Returns whether the executor has started.</summary>
        /// <value type="Boolean">True if the executor has started.</value>
        return this._started;
    },

    get_responseAvailable: function() {
        /// <summary>Returns whether the executor has successfully completed.</summary>
        /// <value type="Boolean">True if a response is available.</value>
        return this._responseAvailable;
    },

    get_aborted: function() {
        /// <summary>Returns whether the executor has been aborted.</summary>
        /// <value type="Boolean">True if the executor has been aborted.</value>
        return this._aborted;
    },

    executeRequest: function() {
        /// <summary>Invokes the request.</summary>
        this._webRequest = this.get_webRequest();

        #if DEBUG
        if (this._started) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOnceStarted, 'executeRequest'));
        }
        if (this._webRequest === null) {
            throw Error.invalidOperation(Sys.Res.nullWebRequest);
        }
        #endif

        var body = this._webRequest.get_body();
        var headers = this._webRequest.get_headers();
        this._xmlHttpRequest = new XMLHttpRequest();
        this._xmlHttpRequest.onreadystatechange = this._onReadyStateChange;
        var verb = this._webRequest.get_httpVerb();
        this._xmlHttpRequest.open(verb, this._webRequest.getResolvedUrl(), true /*async*/);
        this._xmlHttpRequest.setRequestHeader("X-Requested-With", "XMLHttpRequest");
        if (headers) {
            for (var header in headers) {
                var val = headers[header];
                if (typeof(val) !== "function")
                    this._xmlHttpRequest.setRequestHeader(header, val);
            }
        }

        if (verb.toLowerCase() === "post") {
            // If it's a POST but no Content-Type was specified, default to application/x-www-form-urlencoded; charset=utf-8
            if ((headers === null) || !headers['Content-Type']) {
                // DevDiv 109456: Include charset=utf-8. Javascript encoding methods always use utf-8, server may be set to assume other encoding.
                this._xmlHttpRequest.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=utf-8');
            }

            // DevDiv 15893: If POST with no body, default to ""(FireFox needs this)
            if (!body) {
                body = "";
            }
        }

        var timeout = this._webRequest.get_timeout();
        if (timeout > 0) {
            this._timer = window.setTimeout(Function.createDelegate(this, this._onTimeout), timeout);
        }
        this._xmlHttpRequest.send(body);
        this._started = true;
    },

    getResponseHeader: function(header) {
        /// <summary>Returns a response header.</summary>
        /// <param name="header" type="String">The requested header.</param>
        /// <returns type="String">The value of the header.</returns>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'getResponseHeader'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'getResponseHeader'));
        }
        #endif

        var result;
        try {
            result = this._xmlHttpRequest.getResponseHeader(header);
        } catch (e) {
        }
        if (!result) result = "";
        return result;
    },

    getAllResponseHeaders: function() {
        /// <summary>Returns all the responses header.</summary>
        /// <returns type="String">The text of all the headers.</returns>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'getAllResponseHeaders'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'getAllResponseHeaders'));
        }
        #endif

        return this._xmlHttpRequest.getAllResponseHeaders();
    },

    get_responseData: function() {
        /// <summary>Returns the response data.</summary>
        /// <value type="String">The text of the response.</value>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'get_responseData'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'get_responseData'));
        }
        #endif

        return this._xmlHttpRequest.responseText;
    },

    get_statusCode: function() {
        /// <summary>Returns the status code for the response.</summary>
        /// <value type="Number">The status code of the response.</value>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'get_statusCode'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'get_statusCode'));
        }
        #endif
        var result = 0;
        try {
            result = this._xmlHttpRequest.status;
        }
        catch(ex) {
        }
        return result;
    },

    get_statusText: function() {
        /// <summary>Returns the status text for the response.</summary>
        /// <value type="String">The status text of the repsonse.</value>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'get_statusText'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'get_statusText'));
        }
        #endif

        return this._xmlHttpRequest.statusText;
    },

    get_xml: function() {
        /// <summary>Returns the response in xml format.</summary>
        /// <value>The response in xml format.</value>
        #if DEBUG
        if (!this._responseAvailable) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallBeforeResponse, 'get_xml'));
        }
        if (!this._xmlHttpRequest) {
            throw Error.invalidOperation(String.format(Sys.Res.cannotCallOutsideHandler, 'get_xml'));
        }
        #endif

        var xml = this._xmlHttpRequest.responseXML;
        if (!xml || !xml.documentElement) {

            // This happens if the server doesn't set the content type to text/xml.
            xml = Sys.Net.XMLDOM(this._xmlHttpRequest.responseText);

            // If we still couldn't get an XML DOM, the data is probably not XML
            if (!xml || !xml.documentElement)
                return null;
        }
        // REVIEW: todo this used to use Sys.Runtime get_hostType
        else if (navigator.userAgent.indexOf('MSIE') !== -1 && typeof(xml.setProperty) != 'undefined') {
            xml.setProperty('SelectionLanguage', 'XPath');
        }

        // For Firefox parser errors have document elements of parser error
        if (xml.documentElement.namespaceURI === "http://www.mozilla.org/newlayout/xml/parsererror.xml" &&
            xml.documentElement.tagName === "parsererror") {
            return null;
        }
        
        // For Safari, parser errors are always the first child of the root
        if (xml.documentElement.firstChild && xml.documentElement.firstChild.tagName === "parsererror") {
            return null;
        }
        
        return xml;
    },

    abort: function() {
        /// <summary>Aborts the request.</summary>
        #if DEBUG
        if (!this._started) {
            throw Error.invalidOperation(Sys.Res.cannotAbortBeforeStart);
        }
        #endif

        // aborts are no ops if we are done, timedout, or aborted already
        if (this._aborted || this._responseAvailable || this._timedOut)
            return;

        this._aborted = true;

        this._clearTimer();

        if (this._xmlHttpRequest && !this._responseAvailable) {

            // Remove the onreadystatechange first otherwise abort would trigger readyState to become 4
            this._xmlHttpRequest.onreadystatechange = Function.emptyMethod;
            this._xmlHttpRequest.abort();
            
            this._xmlHttpRequest = null;            

            // DevDiv 59229: Call completed on the request instead of raising the event directly
            this._webRequest.completed(Sys.EventArgs.Empty);
        }
    }
}
Sys.Net.XMLHttpExecutor.registerClass('Sys.Net.XMLHttpExecutor', Sys.Net.WebRequestExecutor);
