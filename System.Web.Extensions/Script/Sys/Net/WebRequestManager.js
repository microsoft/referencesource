#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="WebRequestManager.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.Net._WebRequestManager = function() {
    /// <summary locid="P:J#Sys.Net.WebRequestManager.#ctor"/>
    this._defaultTimeout = 0;
    this._defaultExecutorType = "Sys.Net.XMLHttpExecutor";
}

Sys.Net._WebRequestManager.prototype = {
    add_invokingRequest: function(handler) {
        /// <summary locid="E:J#Sys.Net.WebRequestManager.invokingRequest"/>
        this._get_eventHandlerList().addHandler("invokingRequest", handler);
    },
    remove_invokingRequest: function(handler) {
        this._get_eventHandlerList().removeHandler("invokingRequest", handler);
    },

    add_completedRequest: function(handler) {
        /// <summary locid="E:J#Sys.Net.WebRequestManager.completedRequest"/>
        this._get_eventHandlerList().addHandler("completedRequest", handler);
    },
    remove_completedRequest: function(handler) {
        this._get_eventHandlerList().removeHandler("completedRequest", handler);
    },

    _get_eventHandlerList: function() {
        if (!this._events) {
            this._events = new Sys.EventHandlerList();
        }
        return this._events;
    },

    get_defaultTimeout: function() {
        /// <value type="Number" locid="P:J#Sys.Net.WebRequestManager.defaultTimeout">The default timeout for requests in milliseconds.</value>
        return this._defaultTimeout;
    },
    set_defaultTimeout: function(value) {
        #if DEBUG
        if (value < 0) {
            throw Error.argumentOutOfRange("value", value, Sys.Res.invalidTimeout);
        }
        #endif

        this._defaultTimeout = value;
    },

    get_defaultExecutorType: function() {
        /// <value type="String" locid="P:J#Sys.Net.WebRequestManager.defaultExecutorType">The default executor type name.</value>
        return this._defaultExecutorType;
    },
    set_defaultExecutorType: function(value) {
        this._defaultExecutorType = value;
    },

    executeRequest: function(webRequest) {
        /// <summary locid="M:J#Sys.Net.WebRequestManager.executeRequest">Executes a request.</summary>
        /// <param name="webRequest" type="Sys.Net.WebRequest">The webRequest to execute.</param>
        var executor = webRequest.get_executor();
        // if the request didn't set an executor, use the request manager default executor
        if (!executor) {
            // TODO: Optimize this by caching the type

            var failed = false;
            try {
                var executorType = eval(this._defaultExecutorType);
                executor = new executorType();
            } catch (e) {
                failed = true;
            }

            #if DEBUG
            if (failed  || !Sys.Net.WebRequestExecutor.isInstanceOfType(executor) || !executor) {
                throw Error.argument("defaultExecutorType", String.format(Sys.Res.invalidExecutorType, this._defaultExecutorType));
            }
            #endif

            webRequest.set_executor(executor);
        }

        // skip the request if it has been aborted;
        if (executor.get_aborted()) {
            return;
        }

        var evArgs = new Sys.Net.NetworkRequestEventArgs(webRequest);
        var handler = this._get_eventHandlerList().getHandler("invokingRequest");
        if (handler) {
            handler(this, evArgs);
        }

        if (!evArgs.get_cancel()) {
            executor.executeRequest();
        }
    }
}

Sys.Net._WebRequestManager.registerClass('Sys.Net._WebRequestManager');

// Create a single instance of the class
Sys.Net.WebRequestManager = new Sys.Net._WebRequestManager();
