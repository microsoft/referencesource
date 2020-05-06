#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="WebRequest.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Net.WebRequest = function() {
    /// <summary>WebRequest class</summary>
    this._url = "";
    this._headers = { };
    this._body = null;
    this._userContext = null;
    this._httpVerb = null;
    this._executor = null;
    this._invokeCalled = false;
    this._timeout = 0;
}

// Properties about the request data
Sys.Net.WebRequest.prototype = {
    add_completed: function(handler) {
        this._get_eventHandlerList().addHandler("completed", handler);
    },
    remove_completed: function(handler) {
        this._get_eventHandlerList().removeHandler("completed", handler);
    },

    completed: function(eventArgs) {
        /// <summary>The completed method should be called when the request is completed.</summary>
        /// <param name="eventArgs" type="Sys.EventArgs">The event args to raise the event with.</param>
        var handler = Sys.Net.WebRequestManager._get_eventHandlerList().getHandler("completedRequest");
        if (handler) {
            handler(this._executor, eventArgs);
        }

        handler = this._get_eventHandlerList().getHandler("completed");
        if (handler) {
            handler(this._executor, eventArgs);
        }
    },

    _get_eventHandlerList: function() {
        if (!this._events) {
            this._events = new Sys.EventHandlerList();
        }
        return this._events;
    },

    get_url: function() {
        /// <summary>The get_url method returns the url property of the request.</summary>
        /// <value type="String">The url.</value>
        return this._url;
    },
    set_url: function(value) {
        this._url = value;
    },

    get_headers: function() {
        /// <summary>The get_headers method returns the headers dictionary for the request, set headers by adding to this dictionary.</summary>
        /// <value>The headers dictionary for the request.</value>
        return this._headers;
    },

    get_httpVerb: function() {
        /// <summary>The get_httpVerb method gets the httpVerb property of the request.</summary>
        /// <value type="String">The httpVerb for the request.</value>
        // Default is GET if no body, and POST otherwise
        if (this._httpVerb === null) {
            if (this._body === null) {
                return "GET";
            }
            return "POST";
        }
        return this._httpVerb;
    },
    set_httpVerb: function(value) {
        #if DEBUG
        if (value.length === 0) {
            throw Error.argument('value', Sys.Res.invalidHttpVerb);
        }
        #endif

        this._httpVerb = value;
    },

    get_body: function() {
        /// <summary>The get_body method gets the body property of the request.</summary>
        /// <value mayBeNull="true">The body of the request.</value>
        return this._body;
    },
    set_body: function(value) {
        this._body = value;
    },

    get_userContext: function() {
        /// <summary>The get_userContext method gets the userContext property of the request.</summary>
        /// <value mayBeNull="true">The userContext of the request.</value>
        return this._userContext;
    },
    set_userContext: function(value) {
        this._userContext = value;
    },

    get_executor: function() {
        /// <summary>The get_executor method gets the executor property of the request.</summary>
        /// <value type="Sys.Net.WebRequestExecutor">The executor for the request.</value>
        return this._executor;
    },
    set_executor: function(value) {
        #if DEBUG
        if (this._executor !== null && this._executor.get_started()) {
            throw Error.invalidOperation(Sys.Res.setExecutorAfterActive);
        }
        #endif

        this._executor = value;
        this._executor._set_webRequest(this);
    },

    get_timeout: function() {
        /// <summary>The get_timeout method gets the timeout property of the request.</summary>
        /// <value type="Number">The timeout in milliseconds for the request.</value>
        if (this._timeout === 0) {
            return Sys.Net.WebRequestManager.get_defaultTimeout();
        }
        return this._timeout;
    },
    set_timeout: function(value) {
        #if DEBUG
        if (value < 0) {
            throw Error.argumentOutOfRange("value", value, Sys.Res.invalidTimeout);
        }
        #endif

        this._timeout = value;
    },

    getResolvedUrl: function() {
        /// <summary>The getResolvedUrl method returns the url resolved against the base url of the page if set.</summary>
        /// <returns type="String">The resolved url for the request.</returns>
        return Sys.Net.WebRequest._resolveUrl(this._url);
    },

    invoke: function() {
        /// <summary>Invokes the request</summary>
        #if DEBUG
        if (this._invokeCalled) {
            throw Error.invalidOperation(Sys.Res.invokeCalledTwice);
        }
        #endif

        Sys.Net.WebRequestManager.executeRequest(this);
        this._invokeCalled = true;
    }
}

// Given a url and an optional base url, return an absolute url combining the url and base url
Sys.Net.WebRequest._resolveUrl = function(url, baseUrl) {
    // If the url contains a host, we are done
    if (url && url.indexOf('://') !== -1) {
        return url;
    }

    // If a base url isn't passed in, we use either the base element if specified or the URL from the browser
    if (!baseUrl || baseUrl.length === 0) {
        var baseElement = document.getElementsByTagName('base')[0];
        if (baseElement && baseElement.href && baseElement.href.length > 0) {
            baseUrl = baseElement.href;
        }
        else {
            baseUrl = document.URL;
        }
    }

    // strip off any querystrings
    var qsStart = baseUrl.indexOf('?');
    if (qsStart !== -1) {
        baseUrl = baseUrl.substr(0, qsStart);
    }
    // and hashes
    qsStart = baseUrl.indexOf('#');
    if (qsStart !== -1) {
        baseUrl = baseUrl.substr(0, qsStart);
    }
    baseUrl = baseUrl.substr(0, baseUrl.lastIndexOf('/') + 1);

    // If a url wasn't specified, we just use the base
    if (!url || url.length === 0) {
        return baseUrl;
    }

    // For absolute path url, we need to rebase it against the base url, stripping off everything after the http://host
    if (url.charAt(0) === '/') {
        var slashslash = baseUrl.indexOf('://');
        #if DEBUG
        if (slashslash === -1) {
            throw Error.argument("baseUrl", Sys.Res.badBaseUrl1);
        }
        #endif

        var nextSlash = baseUrl.indexOf('/', slashslash + 3);
        #if DEBUG
        if (nextSlash === -1) {
            throw Error.argument("baseUrl", Sys.Res.badBaseUrl2);
        }
        #endif

        return baseUrl.substr(0, nextSlash) + url;
    }
    // Otherwise for relative urls we just combine with the base url stripping off the last path component (filename typically)
    // Note the app path always contains a trailing slash so when resolving app paths, we never strip off anything important
    else {
        var lastSlash = baseUrl.lastIndexOf('/');
        #if DEBUG
        if (lastSlash === -1) {
            throw Error.argument("baseUrl", Sys.Res.badBaseUrl3);
        }
        #endif

        return baseUrl.substr(0, lastSlash+1) + url;
    }
}

Sys.Net.WebRequest._createQueryString = function(queryString, encodeMethod, addParams) {
    // By default, use URI encoding
    encodeMethod = encodeMethod || encodeURIComponent;
    var i = 0, obj, val, arg, sb = new Sys.StringBuilder();
    if (queryString) {
        for (arg in queryString) {
            obj = queryString[arg];
            if (typeof(obj) === "function") continue;
            val = Sys.Serialization.JavaScriptSerializer.serialize(obj);
            if (i++) {
                sb.append('&');
            }
            sb.append(arg);
            sb.append('=');
            sb.append(encodeMethod(val));
        }
    }
    if (addParams) {
        if (i) {
            sb.append('&');
        }
        sb.append(addParams);
    }
    return sb.toString();
}

Sys.Net.WebRequest._createUrl = function(url, queryString, addParams) {
    if (!queryString && !addParams) {
        return url;
    }
    var qs = Sys.Net.WebRequest._createQueryString(queryString, null, addParams);
    return qs.length
        ? url + ((url && url.indexOf('?') >= 0) ? "&" : "?") + qs
        : url;
}

Sys.Net.WebRequest.registerClass('Sys.Net.WebRequest');
