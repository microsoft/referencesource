#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="ProfileService.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.Services._ProfileService = function() {
    /// <summary locid="M:J#Sys.Net.ProfileService.#ctor"/>
    Sys.Services._ProfileService.initializeBase(this);
    this.properties = {};
}
Sys.Services._ProfileService.DefaultWebServicePath = '';

Sys.Services._ProfileService.prototype = {
    _defaultLoadCompletedCallback: null,
    _defaultSaveCompletedCallback: null,
    _path: '',
    _timeout: 0,

    get_defaultLoadCompletedCallback: function() {
        /// <value type="Function" mayBeNull="true" locid="P:J#Sys.Services.ProfileService.defaultLoadCompletedCallback">
        /// Default callback to call when loading is complete.</value>
        return this._defaultLoadCompletedCallback;
    },
    set_defaultLoadCompletedCallback: function(value) {
        this._defaultLoadCompletedCallback = value;
    },

    get_defaultSaveCompletedCallback: function() {
        /// <value type="Function" mayBeNull="true" locid="P:J#Sys.Services.ProfileService.defaultSaveCompletedCallback">
        /// Default callback to call when saving is complete.</value>
        return this._defaultSaveCompletedCallback;
    },
    set_defaultSaveCompletedCallback: function(value) {
        this._defaultSaveCompletedCallback = value;
    },
    
    get_path: function() {
        /// <value type="String" mayBeNull="true" locid="P:J#Sys.Services.ProfileService.path">
        /// Path to a profile webservice.</value>    
        // override from base to ensure returned value is '' even if usercode sets to null.
        // also refactored from v1 to ensure empty string on getter instead of setter.
        return this._path || '';
    },
        
    load: function(propertyNames, loadCompletedCallback, failedCallback, userContext) {
        /// <summary locid="M:J#Sys.Services.ProfileService.load"/>
        /// <param name="propertyNames" type="Array" elementType="String" optional="true" elementMayBeNull="false" mayBeNull="true">A string array of profile properties to load.</param>
        /// <param name="loadCompletedCallback" type="Function" optional="true" mayBeNull="true">Callback to call when loading has completed.</param>
        /// <param name="failedCallback" type="Function" optional="true" mayBeNull="true">Callback to call if loading fails.</param>
        /// <param name="userContext" optional="true" mayBeNull="true">Custom context passed to the completed or failed callback.</param>
        var parameters;
        var methodName;
        if (!propertyNames) {
            methodName = "GetAllPropertiesForCurrentUser";
            parameters = { authenticatedUserOnly: false };
        }
        else {
            methodName = "GetPropertiesForCurrentUser";
            parameters = { properties: this._clonePropertyNames(propertyNames), authenticatedUserOnly: false };
        }

        this._invoke(this._get_path(),
                                        methodName,
                                        false,
                                        parameters,
                                        Function.createDelegate(this, this._onLoadComplete),
                                        Function.createDelegate(this, this._onLoadFailed),
                                        [loadCompletedCallback, failedCallback, userContext]);
    },

    save: function(propertyNames, saveCompletedCallback, failedCallback, userContext) {
        /// <summary locid="M:J#Sys.Services.ProfileService.save"/>
        /// <param name="propertyNames" type="Array" elementType="String" optional="true" elementMayBeNull="false" mayBeNull="true">A string array of profile properties to save.</param>
        /// <param name="saveCompletedCallback" type="Function" optional="true" mayBeNull="true">Callback to call when saving has completed.</param>
        /// <param name="failedCallback" type="Function" optional="true" mayBeNull="true">Callback to call if saving fails.</param>
        /// <param name="userContext" optional="true" mayBeNull="true">Custom context passed to the completed or failed callback.</param>
        var flattenedProperties = this._flattenProperties(propertyNames, this.properties);
        this._invoke(this._get_path(),
                                        "SetPropertiesForCurrentUser",
                                        false,
                                        { values: flattenedProperties.value, authenticatedUserOnly: false },
                                        Function.createDelegate(this, this._onSaveComplete),
                                        Function.createDelegate(this, this._onSaveFailed),
                                        [saveCompletedCallback, failedCallback, userContext, flattenedProperties.count]);
    },
    
    // DevDiv 31283: calling load with two of the same property names throws an error, so we strip dups
    _clonePropertyNames: function(arr) {
        var nodups = [];
        var seen = {};
        for (var i=0; i < arr.length; i++) {
            var prop = arr[i];
            if(!seen[prop]) { Array.add(nodups, prop); seen[prop]=true; };
        }
        return nodups;
    },    

    // convert properties like properties.ProfileGroup.ProfileSetting to properties["ProfileGroup.ProfileSetting"].
    // propertyNames: list of properties that should be included in the flattened list (others are excluded)
    // properties: object containing properties to flatten
    // groupName: current group name used for recursion
    _flattenProperties: function(/*string[]*/propertyNames, properties, groupName) {
        var flattenedProperties = {};
        var val;
        var key;
        var count = 0;
        if (propertyNames && propertyNames.length === 0) {
            return { value: flattenedProperties, count: 0 };
        }

        for (var property in properties) {
            val = properties[property];
            key = groupName ? groupName + "." + property : property;
            // is it a property group?
            if(Sys.Services.ProfileGroup.isInstanceOfType(val)) {
                var obj = this._flattenProperties(propertyNames, val, key);
                var groupProperties = obj.value;
                count += obj.count; // count all the group's properties we're about to merge in
                // merge in group's properties
                // NOTE: We don't use Array.addRange because flattenedProperties is not an Array.
                //       It can't be an array because it polutes the associative array and we need it to be purely properties.
                //       Array.prototype.addRange.apply() doesn't work either.
                // NOTE: In the case where a group exists but has no inner properties of its own, the for loop will short out
                //       and there will be no keys added to the collection, as expected.
                for(var subKey in groupProperties) {
                    var subVal = groupProperties[subKey];
                    flattenedProperties[subKey] = subVal;
                }
            }
            else {
                // is this a specified property (or use all properties)?
                if(!propertyNames || Array.indexOf(propertyNames, key) !== -1) {
                    flattenedProperties[key] = val;
                    count++; // keep track of how many properties are in the flattened dictionary
                }
            }
        }
        return { value: flattenedProperties, count: count };
    },
    
    _get_path: function() {
        var path = this.get_path();
        if (!path.length) {
            path = Sys.Services._ProfileService.DefaultWebServicePath;
        }
        if (!path || !path.length) {
            throw Error.invalidOperation(Sys.Res.servicePathNotSet);
        }
        return path;
    },    

    _onLoadComplete: function(result, context, methodName) {
        if (typeof(result) !== "object") {
            throw Error.invalidOperation(String.format(Sys.Res.webServiceInvalidReturnType, methodName, "Object"));
        }

        var unflattened = this._unflattenProperties(result);
        for (var name in unflattened) {
            this.properties[name] = unflattened[name];
        }
        
        var callback = context[0] || this.get_defaultLoadCompletedCallback() || this.get_defaultSucceededCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();        
            callback(result.length, userContext, "Sys.Services.ProfileService.load");
        }
    },
    
    _onLoadFailed: function(err, context, methodName) {
        var callback = context[1] || this.get_defaultFailedCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();        
            callback(err, userContext, "Sys.Services.ProfileService.load");
        }
        #if DEBUG
        else {
            Sys.Net.WebServiceProxy._defaultFailedCallback(err, methodName);
        }
        #endif
    },
    
    _onSaveComplete: function(result, context, methodName) {
        // context[3] is the number of properties we sent to the server.
        var count = context[3];
        if (result !== null) { // dont use if(result), might be number 0
            if (result instanceof Array) {
                // result is a list of properties that failed. Subtract the count to get the # succeeded
                count -= result.length;
            }
            else if (typeof(result) === 'number') {
                // legacy server API -- the number of successful properties is returned directly
                count = result;
            }
            else {
                // no other types allowed
                throw Error.invalidOperation(String.format(Sys.Res.webServiceInvalidReturnType, methodName, "Array"));
            }
        }
        // else: if result is null, treat as an empty array (no failures)
        
        var callback = context[0] || this.get_defaultSaveCompletedCallback() || this.get_defaultSucceededCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();
            callback(count, userContext, "Sys.Services.ProfileService.save");
        }
    },
    
    _onSaveFailed: function(err, context, methodName) {
        var callback = context[1] || this.get_defaultFailedCallback();
        if (callback) {
            var userContext = context[2] || this.get_defaultUserContext();
            callback(err, userContext, "Sys.Services.ProfileService.save");
        }
        #if DEBUG
        else {
            Sys.Net.WebServiceProxy._defaultFailedCallback(err, methodName);
        }
        #endif
    },
    
    _unflattenProperties: function(properties) {
        var unflattenedProperties = {};
        var dotIndex;
        var val;
        var count = 0;
        for (var key in properties) {
            count++;
            val = properties[key];

            dotIndex = key.indexOf('.');
            if (dotIndex !== -1) {
                var groupName = key.substr(0, dotIndex);
                key = key.substr(dotIndex+1);
                var group = unflattenedProperties[groupName];
                if (!group || !Sys.Services.ProfileGroup.isInstanceOfType(group)) {
                    group = new Sys.Services.ProfileGroup();
                    unflattenedProperties[groupName] = group;
                }
                group[key] = val;
            }
            else {
                unflattenedProperties[key] = val;
            }
        }
        properties.length = count;
        return unflattenedProperties;
    }
}
Sys.Services._ProfileService.registerClass('Sys.Services._ProfileService', Sys.Net.WebServiceProxy);
Sys.Services.ProfileService = new Sys.Services._ProfileService();

Sys.Services.ProfileGroup = function(properties) {
    /// <param name="properties" optional="true" mayBeNull="true">An object containing the settings for this profile group.</param>
    if (properties) {
        for (var property in properties) {
            this[property] = properties[property];
        }
    }
}
Sys.Services.ProfileGroup.registerClass('Sys.Services.ProfileGroup');
