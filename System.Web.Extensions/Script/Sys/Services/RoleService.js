#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="RoleService.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.Services._RoleService = function() {
    /// <summary locid="M:J#Sys.Services.RoleService.#ctor"/>
    Sys.Services._RoleService.initializeBase(this);
    this._roles = [];
}
Sys.Services._RoleService.DefaultWebServicePath = '';
Sys.Services._RoleService.prototype = {
    _defaultLoadCompletedCallback: null,
    _rolesIndex: null,
    _timeout: 0,
    _path: '',

    get_defaultLoadCompletedCallback: function() {
        /// <value type="Function" mayBeNull="true" locid="P:J#Sys.Services.RoleService.defaultLoadCompletedCallback">
        /// Default callback to call when loading is complete.</value>
        return this._defaultLoadCompletedCallback;
    },
    set_defaultLoadCompletedCallback: function(value) {
        this._defaultLoadCompletedCallback = value;
    },
    
    get_path: function() {
        /// <value type="String" mayBeNull="true" locid="P:J#Sys.Services.RoleService.path">Path to a role webservice.</value>    
        // override from base to ensure returned value is '' even if usercode sets to null, consistent with other appservices in v1.
        return this._path || '';
    },

    get_roles: function() {
        /// <value type="Array" elementType="String" mayBeNull="false" locid="P:J#Sys.Services.RoleService.roles">
        /// An array of role names for the current user.</value>
        return Array.clone(this._roles);
    },

    isUserInRole: function(role) {
        /// <summary locid="M:J#Sys.Services.RoleService.isUserInRole">Gets a value indicating whether the current user is in the specified role.</summary>
        /// <param name="role" type="String" mayBeNull="false">The name of the role to search in.</param>
        /// <returns type="Boolean"/>
        var v = this._get_rolesIndex()[role.trim().toLowerCase()];
        return !!v;
    },
    
    load: function(loadCompletedCallback, failedCallback, userContext) {
        /// <summary locid="M:J#Sys.Services.RoleService.load"/>
        /// <param name="loadCompletedCallback" type="Function" optional="true" mayBeNull="true">Callback to call when loading has completed.</param>
        /// <param name="failedCallback" type="Function" optional="true" mayBeNull="true">Callback to call if loading fails.</param>
        /// <param name="userContext" optional="true" mayBeNull="true">Custom context passed to the completed or failed callback.</param>
        Sys.Net.WebServiceProxy.invoke(
                    this._get_path(),
                    "GetRolesForCurrentUser",
                    false,
                    {} /* no params*/,
                    Function.createDelegate(this, this._onLoadComplete),
                    Function.createDelegate(this, this._onLoadFailed),
                    [loadCompletedCallback, failedCallback, userContext],
                    this.get_timeout());
    },

    _get_path: function() {
        var path = this.get_path();
        if(!path || !path.length) {
            path = Sys.Services._RoleService.DefaultWebServicePath;
        }
        if(!path || !path.length) {
            throw Error.invalidOperation(Sys.Res.servicePathNotSet);
        }
        return path;
    },  
    
    _get_rolesIndex: function() {
        if (!this._rolesIndex) {
            var index = {};
            for(var i=0; i < this._roles.length; i++) {
                index[this._roles[i].toLowerCase()] = true;
            }
            this._rolesIndex = index;
        }
        return this._rolesIndex;
    },

    _onLoadComplete: function(result, context, methodName) {
        if(result && !(result instanceof Array)) {
            throw Error.invalidOperation(String.format(Sys.Res.webServiceInvalidReturnType, methodName, "Array"));
        }

        this._roles = result;
        this._rolesIndex = null;

        var callback = context[0] || this.get_defaultLoadCompletedCallback() || this.get_defaultSucceededCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();
            var clonedResult = Array.clone(result);
            callback(clonedResult, userContext, "Sys.Services.RoleService.load");
        }
    },

    _onLoadFailed: function(err, context, methodName) {
        var callback = context[1] || this.get_defaultFailedCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();
            callback(err, userContext, "Sys.Services.RoleService.load");
        }
        #if DEBUG
        else {
            Sys.Net.WebServiceProxy._defaultFailedCallback(err, methodName);
        }
        #endif
    }
}

Sys.Services._RoleService.registerClass('Sys.Services._RoleService', Sys.Net.WebServiceProxy);
Sys.Services.RoleService = new Sys.Services._RoleService();
