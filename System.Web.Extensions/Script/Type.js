#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Type.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
// Define the root object (for non-browser hosts)
if (!window) this.window = this;

// Alias Function as Type
window.Type = Function;

// This has undistinguishable perf from compiled a RegExp.
// The regexps here are kept a little too wide to allow for Unicode characters but still
// capture the most obvious developer errors. The JavaScript parser, as well as the checks for
// eval('name') === name will take care of the other errors.
// ********************************************************************************************
// NOTE: update ScriptComponentDescriptor.cs with any change to this expression
// so server and client-side are in sync.
##DEBUG Type.__fullyQualifiedIdentifierRegExp = new RegExp("^[^.0-9 \\s|,;:&*=+\\-()\\[\\]{}^%#@!~\\n\\r\\t\\f\\\\]([^ \\s|,;:&*=+\\-()\\[\\]{}^%#@!~\\n\\r\\t\\f\\\\]*[^. \\s|,;:&*=+\\-()\\[\\]{}^%#@!~\\n\\r\\t\\f\\\\])?$", "i");
##DEBUG Type.__identifierRegExp = new RegExp("^[^.0-9 \\s|,;:&*=+\\-()\\[\\]{}^%#@!~\\n\\r\\t\\f\\\\][^. \\s|,;:&*=+\\-()\\[\\]{}^%#@!~\\n\\r\\t\\f\\\\]*$", "i");

Type.prototype.callBaseMethod = function(instance, name, baseArguments) {
    /// <param name="instance">The instance for the base method. Usually 'this'.</param>
    /// <param name="name" type="String">The name of the base method.</param>
    /// <param name="baseArguments" type="Array" optional="true" mayBeNull="true" elementMayBeNull="true">
    ///     The arguments to pass to the base method.
    /// </param>
    /// <returns>The return value of the base method.</returns>
    var baseMethod = Sys._getBaseMethod(this, instance, name);
    #if DEBUG
    if (!baseMethod) throw Error.invalidOperation(String.format(Sys.Res.methodNotFound, name));
    #endif
    if (!baseArguments) {
        return baseMethod.apply(instance);
    }
    else {
        return baseMethod.apply(instance, baseArguments);
    }
}

Type.prototype.getBaseMethod = function(instance, name) {
    /// <summary>Use this method to get the base implementation of a method from the base class.</summary>
    /// <param name="instance">The instance for which the base method is needed. Usually 'this'.</param>
    /// <param name="name" type="String">The name of the method to get.</param>
    /// <returns type="Function" mayBeNull="true">The base method.</returns>
    return Sys._getBaseMethod(this, instance, name);
}

Type.prototype.getBaseType = function() {
    /// <returns type="Type" mayBeNull="true">The base type.</returns>
    return (typeof(this.__baseType) === "undefined") ? null : this.__baseType;
}

Type.prototype.getInterfaces = function() {
    /// <returns type="Array" elementType="Type" mayBeNull="false" elementMayBeNull="false">
    ///   A copy of the list of interfaces that the type implements.
    /// </returns>
    var result = [];
    var type = this;
    while(type) {
        var interfaces = type.__interfaces;
        if (interfaces) {
            for (var i = 0, l = interfaces.length; i < l; i++) {
                var interfaceType = interfaces[i];
                if (!Array.contains(result, interfaceType)) {
                    result[result.length] = interfaceType;
                }
            }
        }
        type = type.__baseType;
    }
    return result;
}

Type.prototype.getName = function() {
    /// <returns type="String">The name of the type.</returns>
    return (typeof(this.__typeName) === "undefined") ? "" : this.__typeName;
}

Type.prototype.implementsInterface = function(interfaceType) {
    /// <param name="interfaceType" type="Type">The interface to test.</param>
    /// <returns type="Boolean">True if the type implements the interface.</returns>
    this.resolveInheritance();

    var interfaceName = interfaceType.getName();
    var cache = this.__interfaceCache;
    if (cache) {
        var cacheEntry = cache[interfaceName];
        if (typeof(cacheEntry) !== 'undefined') return cacheEntry;
    }
    else {
        cache = this.__interfaceCache = {};
    }

    var baseType = this;
    while (baseType) {
        var interfaces = baseType.__interfaces;
        if (interfaces) {
            if (Array.indexOf(interfaces, interfaceType) !== -1) {
                return cache[interfaceName] = true;
            }
        }

        baseType = baseType.__baseType;
    }

    return cache[interfaceName] = false;
}

Type.prototype.inheritsFrom = function(parentType) {
    /// <param name="parentType" type="Type">The type to test.</param>
    /// <returns type="Boolean">True if the type inherits from parentType.</returns>
    this.resolveInheritance();
    var baseType = this.__baseType;
    while (baseType) {
        if (baseType === parentType) {
            return true;
        }
        baseType = baseType.__baseType;
    }

    return false;
}

Type.prototype.initializeBase = function(instance, baseArguments) {
    /// <summary>
    ///     This method initializes the base type in the context
    ///     of a given instance object (to keep track of the base type, and to
    ///     effectively inherit the object model of the base class, and
    ///     initializing members of the base class).
    ///     This should be called from the derived class constructor.
    /// </summary>
    /// <param name="instance">The object to initialize base types for. Usually 'this'.</param>
    /// <param name="baseArguments" type="Array" optional="true" mayBeNull="true" elementMayBeNull="true">
    ///     The arguments for the base constructor.
    /// </param>
    /// <returns>The instance.</returns>
    #if DEBUG
    if (!Sys._isInstanceOfType(this, instance)) throw Error.argumentType('instance', Object.getType(instance), this);
    #endif

    this.resolveInheritance();
    if (this.__baseType) {
        if (!baseArguments) {
            this.__baseType.apply(instance);
        }
        else {
            this.__baseType.apply(instance, baseArguments);
        }
    }

    return instance;
}

Type.prototype.isImplementedBy = function(instance) {
    /// <param name="instance" mayBeNull="true">The object on which the interface must be tested.</param>
    /// <returns type="Boolean">True if the instance implements the interface.</returns>
    if (typeof(instance) === "undefined" || instance === null) return false;

    var instanceType = Object.getType(instance);
    return !!(instanceType.implementsInterface && instanceType.implementsInterface(this));
}

Type.prototype.isInstanceOfType = function(instance) {
    /// <param name="instance" mayBeNull="true">The object on which the type must be tested.</param>
    /// <returns type="Boolean">True if the object is an instance of the type or one of its derived types.</returns>
    return Sys._isInstanceOfType(this, instance);
}

Type.prototype.registerClass = function(typeName, baseType, interfaceTypes) {
    /// <summary>
    ///     Registers a class (represented by its ctor function), and
    ///     optional base type, followed by any number of interfaces.
    /// </summary>
    /// <param name="typeName" type="String">The fully-qualified name of the type.</param>
    /// <param name="baseType" type="Type" optional="true" mayBeNull="true">The base type.</param>
    /// <param name="interfaceTypes" parameterArray="true" type="Type">
    ///     One or several interfaces that the type implements.
    /// </param>
    /// <returns type="Type">The registered type.</returns>
    #if DEBUG
    if (!Type.__fullyQualifiedIdentifierRegExp.test(typeName)) throw Error.argument('typeName', Sys.Res.notATypeName);
    // Check if the type name parses to an existing object that matches this.
    var parsedName;
    try {
        parsedName = eval(typeName);
    }
    catch(e) {
        throw Error.argument('typeName', Sys.Res.argumentTypeName);
    }
    if (parsedName !== this) throw Error.argument('typeName', Sys.Res.badTypeName);
    // Check for double registrations
    if (Sys.__registeredTypes[typeName]) throw Error.invalidOperation(String.format(Sys.Res.typeRegisteredTwice, typeName));

    // We never accept undefined for this parameter because this is the only way we can catch
    // registerClass("Sys.Foo", Sys.BArWithATypo, Sys.ISomeInterface).
    if ((arguments.length > 1) && (typeof(baseType) === 'undefined')) throw Error.argumentUndefined('baseType');
    if (baseType && !baseType.__class) throw Error.argument('baseType', Sys.Res.baseNotAClass);
    #endif

    this.prototype.constructor = this;
    this.__typeName = typeName;
    this.__class = true;
    if (baseType) {
        this.__baseType = baseType;
        this.__basePrototypePending = true;
    }
    // Saving a case-insensitive index of the registered types on each namespace
    Sys.__upperCaseTypes[typeName.toUpperCase()] = this;

    // It is more performant to check "if (interfaceTypes)" than "if (arguments.length > 2)".
    // Accessing the arguments array is relatively expensive, so we only want to do so if there
    // are actually interface parameters.
    if (interfaceTypes) {
        this.__interfaces = [];
        ##DEBUG this.resolveInheritance();
        for (var i = 2, l = arguments.length; i < l; i++) {
            var interfaceType = arguments[i];
            #if DEBUG
            if (!interfaceType.__interface) throw Error.argument('interfaceTypes[' + (i - 2) + ']', Sys.Res.notAnInterface);
            for (var methodName in interfaceType.prototype) {
                var method = interfaceType.prototype[methodName];
                if (!this.prototype[methodName]) {
                    this.prototype[methodName] = method;
                }
            }
            #endif
            this.__interfaces.push(interfaceType);
        }
    }
    ##DEBUG Sys.__registeredTypes[typeName] = true;

    return this;
}

Type.prototype.registerInterface = function(typeName) {
    /// <summary>Registers an interface (represented by its ctor function).</summary>
    /// <param name="typeName" type="String">The fully-qualified name of the interface.</param>
    /// <returns type="Type">The registered interface.</returns>
    #if DEBUG
    if (!Type.__fullyQualifiedIdentifierRegExp.test(typeName)) throw Error.argument('typeName', Sys.Res.notATypeName);
    // Check if the type name parses to an existing object that matches this.
    var parsedName;
    try {
        parsedName = eval(typeName);
    }
    catch(e) {
        throw Error.argument('typeName', Sys.Res.argumentTypeName);
    }
    if (parsedName !== this) throw Error.argument('typeName', Sys.Res.badTypeName);
    // Check for double registrations
    if (Sys.__registeredTypes[typeName]) throw Error.invalidOperation(String.format(Sys.Res.typeRegisteredTwice, typeName));
    #endif
    // Saving a case-insensitive index of the registered types on each namespace
    Sys.__upperCaseTypes[typeName.toUpperCase()] = this;

    this.prototype.constructor = this;
    this.__typeName = typeName;
    this.__interface = true;
    ##DEBUG Sys.__registeredTypes[typeName] = true;

    return this;
}

Type.prototype.resolveInheritance = function() {
    /// <summary>
    /// This method is called on the ctor function instance.
    /// It does three things:
    /// 1. It stores __baseType as a property of the constructor function
    /// 2. It copies members from the baseType's prototype into the
    ///    prototype associated with the type represented by this ctor,
    ///    if this type itself doesn't have the same member in its prototype,
    ///    i.e., it doesn't override the method.
    /// 3. It recurses up the inheritance chain to do the same for the base type.
    ///    Note that this logic runs only once per type, because it
    ///    is based on true value for __basePrototypePending property
    ///    off the ctor function.
    /// </summary>

    if (this.__basePrototypePending) {
        var baseType = this.__baseType;

        baseType.resolveInheritance();

        for (var memberName in baseType.prototype) {
            var memberValue = baseType.prototype[memberName];
            if (!this.prototype[memberName]) {
                this.prototype[memberName] = memberValue;
            }
        }
        delete this.__basePrototypePending;
    }
}

Type.getRootNamespaces = function() {
    /// <returns type="Array">Returns an array containing references to all the root namespaces</returns>
    return Array.clone(Sys.__rootNamespaces);
}

Type.isClass = function(type) {
    /// <param name="type" mayBeNull="true">The type to test.</param>
    /// <returns type="Boolean">True if the type is a class.</returns>
    if ((typeof(type) === 'undefined') || (type === null)) return false;
    return !!type.__class;
}

Type.isInterface = function(type) {
    /// <param name="type" mayBeNull="true">The type to test.</param>
    /// <returns type="Boolean">True if the type is an interface.</returns>
    if ((typeof(type) === 'undefined') || (type === null)) return false;
    return !!type.__interface;
}

Type.isNamespace = function(object) {
    /// <param name="object" mayBeNull="true">The type to test.</param>
    /// <returns type="Boolean">True if the object is a namespace.</returns>
    if ((typeof(object) === 'undefined') || (object === null)) return false;
    return !!object.__namespace;
}

Type.parse = function(typeName, ns) {
    /// <summary>
    ///   If a namespace is specified, the type name is searched for on this namespace in a
    ///   case-insensitive way.
    ///   If no namespace is specified, the fully-qualified, case-sensitive type name must be specified.
    /// </summary>
    /// <param name="typeName" type="String" mayBeNull="true">The name of the type.</param>
    /// <param name="ns" optional="true" mayBeNull="true">The namespace where to look for the type.</param>
    /// <returns type="Type" mayBeNull="true">The type or null.</returns>
    var fn;
    if (ns) {
        fn = Sys.__upperCaseTypes[ns.getName().toUpperCase() + '.' + typeName.toUpperCase()];
        return fn || null;
    }
    if (!typeName) return null;
    if (!Type.__htClasses) {
        Type.__htClasses = {};
    }
    fn = Type.__htClasses[typeName];
    if (!fn) {
        fn = eval(typeName);
        ##DEBUG if (typeof(fn) !== 'function') throw Error.argument('typeName', Sys.Res.notATypeName);
        Type.__htClasses[typeName] = fn;
    }
    return fn;
}

Type.registerNamespace = function(namespacePath) {
    /// <summary>Creates a namespace.</summary>
    /// <param name="namespacePath" type="String">The full path of the namespace.</param>
    #if DEBUG
    // in debug mode, the private version does all the work to enable bypassing
    // the parameter validation in debug mode when registering 'Sys'.
    Type._registerNamespace(namespacePath);
}
Type._registerNamespace = function(namespacePath) {
    if (!Type.__fullyQualifiedIdentifierRegExp.test(namespacePath)) throw Error.argument('namespacePath', Sys.Res.invalidNameSpace);
    #endif
    var rootObject = window;
    var namespaceParts = namespacePath.split('.');

    for (var i = 0; i < namespaceParts.length; i++) {
        var currentPart = namespaceParts[i];
        var ns = rootObject[currentPart];
        #if DEBUG
        var nsType = typeof(ns);
        if ((nsType !== "undefined") && (ns !== null)) {
            if (nsType === "function") {
                throw Error.invalidOperation(String.format(Sys.Res.namespaceContainsClass, namespaceParts.splice(0, i + 1).join('.')));
            }
            if ((typeof(ns) !== "object") || (ns instanceof Array)) {
                throw Error.invalidOperation(String.format(Sys.Res.namespaceContainsNonObject, namespaceParts.splice(0, i + 1).join('.')));
            }
        }
        #endif
        if (!ns) {
            ns = rootObject[currentPart] = {};
        }
        if (!ns.__namespace) {
            if ((i === 0) && (namespacePath !== "Sys")) {
                Sys.__rootNamespaces[Sys.__rootNamespaces.length] = ns;
            }
            ns.__namespace = true;
            ns.__typeName = namespaceParts.slice(0, i + 1).join('.');
            #if DEBUG
            var parsedName;
            try {
                parsedName = eval(ns.__typeName);
            }
            catch(e) {
                parsedName = null;
            }
            if (parsedName !== ns) {
                delete rootObject[currentPart];
                throw Error.argument('namespacePath', Sys.Res.invalidNameSpace);
            }
            #endif
            ns.getName = function() {return this.__typeName;}
        }
        rootObject = ns;
    }
}

Type._checkDependency = function(dependency, featureName) {
    var scripts = Type._registerScript._scripts, isDependent = (scripts ? (!!scripts[dependency]) : false);
    if ((typeof(featureName) !== 'undefined') && !isDependent) {
        throw Error.invalidOperation(String.format(Sys.Res.requiredScriptReferenceNotIncluded, 
        featureName, dependency));
    }
    return isDependent;
}

Type._registerScript = function(scriptName, dependencies) {
    var scripts = Type._registerScript._scripts;
    if (!scripts) {
        Type._registerScript._scripts = scripts = {};
    }
    if (scripts[scriptName]) {
        throw Error.invalidOperation(String.format(Sys.Res.scriptAlreadyLoaded, scriptName));
    }
    scripts[scriptName] = true;
    if (dependencies) {
        for (var i = 0, l = dependencies.length; i < l; i++) {
            var dependency = dependencies[i];
            if (!Type._checkDependency(dependency)) {
                throw Error.invalidOperation(String.format(Sys.Res.scriptDependencyNotFound, scriptName, dependency));
            }
        }
    }
}

#if DEBUG
// bypass param validation in debug mode
Type._registerNamespace("Sys");
#else
Type.registerNamespace("Sys");
#endif
Sys.__upperCaseTypes = {};
Sys.__rootNamespaces = [Sys];
##DEBUG Sys.__registeredTypes = {};

// a private version of getBaseMethod and isInstanceOfType allows for other public APIs to call getBaseMethod without
// causing a re-validation of the arguments. It's only debug mode, but the perf of debug mode was so bad in some cases
// due to validation that it causes the browser to warn of a run-away script. Even in debug mode it's still important
// we have reasonable perf, and avoiding re-validation of public facing parameters is the primary way to do that.
// The private versions are on Sys to avoid adding another function to the prototype of every function.
Sys._isInstanceOfType = function(type, instance) {
    if (typeof(instance) === "undefined" || instance === null) return false;
    if (instance instanceof type) return true;
    var instanceType = Object.getType(instance);
    return !!(instanceType === type) ||
           (instanceType.inheritsFrom && instanceType.inheritsFrom(type)) ||
           (instanceType.implementsInterface && instanceType.implementsInterface(type));
}

// getBaseMethod's validateParameters was shown to be a huge portion of the time spent disposing of a large
// number of components on a page, where they all call type.callBaseMethod, which also calls getBaseMethod. 
Sys._getBaseMethod = function(type, instance, name) {
    #if DEBUG
    if (!Sys._isInstanceOfType(type, instance)) throw Error.argumentType('instance', Object.getType(instance), type);
    #endif
    var baseType = type.getBaseType();
    if (baseType) {
        var baseMethod = baseType.prototype[name];
        return (baseMethod instanceof Function) ? baseMethod : null;
    }
    return null;
}

Sys._isDomElement = function(obj) {
    // Using nodeType to check this is a DOM element for lack of a better test on IE and Safari.
    // This is not entirely foolproof ({nodeType: 1} would seem to be of type Sys.UI.DomElement)
    // but we need something that works cross-browser.
    // Opera and Firefox both have an HTMLElement type of which DOM elements are instances but
    // we're not using it here for consistency.
    var val = false;
    if (typeof (obj.nodeType) !== 'number') {
        // Windows and documents are considered elements even though they are not strictly speaking.
        // No node type may still be window or document.
        // Try to get the document for the element, revert to obj if not found:
        var doc = obj.ownerDocument || obj.document || obj;
        if (doc != obj) {
            // The parameter is not the document, but it may be window.
            // Try to get the window for the document:
            var w = doc.defaultView || doc.parentWindow;
            val = (w != obj);
        }
        else {
            // doc is equal to obj, but we still need to check that it's really a document.
            // Using the body property for lack of a better cross-browser test.
            val = (typeof (doc.body) === 'undefined');
        }
    }
    return !val;
}
