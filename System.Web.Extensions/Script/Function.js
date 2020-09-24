#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Function.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Function.__typeName = 'Function';
Function.__class = true;

Function.createCallback = function(method, context) {
    /// <summary>
    ///     Creates a callback function that retains the parameter initially used during its creation.
    ///     The callback is used without parameter but this will call the actual method with the parameter.
    ///     This is especially useful when setting up a handler for a DOM event that must retain a parameter
    ///     despite the DOM event handler needing to be a function with the event object as the only parameter.
    ///     In this case, the function will be called with the event as the first parameter and the context
    ///     as the second.
    ///     If the callback is called with an arbitrary list of parameters, the context is appended.
    /// </summary>
    /// <param name="method" type="Function">The function for which the callback is created.</param>
    /// <param name="context" mayBeNull="true">The parameter for the function.</param>
    /// <returns type="Function">The callback function.</returns>

    // The method still makes sense for null context, but not if the context is omitted altogether
    // (omitted context makes the callback equivalent to the method itself, with one more level of indirection).

    return function() {
        var l = arguments.length;
        if (l > 0) {
            // arguments is not a real array, need to build a real one from it so we can add
            var args = [];
            for (var i = 0; i < l; i++) {
                args[i] = arguments[i];
            }
            args[l] = context;
            return method.apply(this, args);
        }
        return method.call(this, context);
    }
}

Function.createDelegate = function(instance, method) {
    /// <summary>
    ///     Creates a delegate function that retains the context from its creation
    ///     (i.e. what 'this' means from within its scope).
    ///     This is especially useful when setting up an event handler to point to an object method
    ///     that needs to use the 'this' pointer from within its scope.
    /// </summary>
    /// <param name="instance" mayBeNull="true">
    ///     The object instance that will be the context for the function (i.e. what 'this' means from within its scope).
    /// </param>
    /// <param name="method" type="Function">The function from which the delegate is created.</param>
    /// <returns type="Function">The delegate function.</returns>

    // The method still makes some sense with a null instance, in the same way that createCallback still
    // makes sense with a null context.

    return function() {
        return method.apply(instance, arguments);
    }
}

Function.emptyFunction = Function.emptyMethod = function() {
    /// <summary>A function that does nothing.</summary>
    /// <validationOptions enabled="false"/>
}

Function.validateParameters = function(parameters, expectedParameters, validateParameterCount) {
    /// <summary locid="M:J#Function.validateParameters">
    ///     Validates the parameters to a method are as expected.
    /// </summary>
    /// <param name="parameters"/>
    /// <param name="expectedParameters"/>
    /// <param name="validateParameterCount" type="Boolean" optional="true">True if extra parameters are prohibited, false if they should be ignored. The default is true.</param>
    /// <returns type="Error" mayBeNull="true"/>
    /// <example>
    ///     function foo(anyParam, stringParam, anyArrayParam, stringArrayParam,
    ///                  interfaceParam, optionalStringParam) {
    ///         #if DEBUG
    ///         var e = Function.validateParameters(arguments, [
    ///             { name: "anyParam" },
    ///             { name: "mayBeNullParam", mayBeNull: true },
    ///             { name: "stringParam", type: String },
    ///             { name: "floatParam", type: Number },
    ///             { name: "intParam", type: Number, integer: true },
    ///             { name: "domParam", domElement: true },
    ///             { name: "anyArrayParam", type: Array },
    ///             { name: "mayBeNullArrayParam", type: Array, elementMayBeNull: true },
    ///             { name: "stringArrayParam", type: Array, elementType: String },
    ///             { name: "intArrayParam", type: Array, elementType: Number, elementInteger: true },
    ///             { name: "domElementArrayParam", type: Array, elementDomElement: true },
    ///             { name: "interfaceParam", type: Sys.IFoo }
    ///             { name: "optionalStringParam", type: String, optional: true }
    ///             { name: "stringParamArray", type: String, parameterArray: true }
    ///             { name: "mayBeNullParamArray", parameterArray: true, mayBeNull: true }
    ///         ]);
    ///         if (e) throw e;
    ///         #endif
    ///     }
    /// </example>    
    // type info omitted from parameters so that the parameter-parameters are not enumerated and each entry validated
    return Function._validateParams(parameters, expectedParameters, validateParameterCount);
}

Function._validateParams = function(params, expectedParams, validateParameterCount) {
    // *DO NOT* triple-slash comment those. The double-slashes here are on purpose.
    // We don't need to document private functions and those will induce infinite loops
    // if the preprocessor generates validation code for these.
    // <summary>
    //     Validates the parameters to a method.
    // </summary>
    // <example>
    //     function foo(anyParam, stringParam, anyArrayParam, stringArrayParam,
    //                  interfaceParam, optionalStringParam) {
    //         #if DEBUG
    //         var e = Function._validateParams(arguments, [
    //             { name: "anyParam" },
    //             { name: "mayBeNullParam", mayBeNull: true },
    //             { name: "stringParam", type: String },
    //             { name: "floatParam", type: Number },
    //             { name: "intParam", type: Number, integer: true },
    //             { name: "domParam", domElement: true },
    //             { name: "anyArrayParam", type: Array },
    //             { name: "mayBeNullArrayParam", type: Array, elementMayBeNull: true },
    //             { name: "stringArrayParam", type: Array, elementType: String },
    //             { name: "intArrayParam", type: Array, elementType: Number, elementInteger: true },
    //             { name: "domElementArrayParam", type: Array, elementDomElement: true },
    //             { name: "interfaceParam", type: Sys.IFoo }
    //             { name: "optionalStringParam", type: String, optional: true }
    //             { name: "stringParamArray", type: String, parameterArray: true }
    //             { name: "mayBeNullParamArray", parameterArray: true, mayBeNull: true }
    //         ]);
    //         if (e) throw e;
    //         #endif
    //     }
    // </example>
    // <param name="params" type="Array">Array of parameter values passed to the method.</param>
    // <param name="expectedParams" type="Array" optional="true">Array of JSON objects describing the expected parameters.</param>
    var e, expectedLength = expectedParams.length;
    validateParameterCount = validateParameterCount || (typeof(validateParameterCount) === "undefined");
    e = Function._validateParameterCount(params, expectedParams, validateParameterCount);
    if (e) {
        e.popStackFrame();
        return e;
    }
    for (var i = 0, l = params.length; i < l; i++) {
        // If there are more params than expectedParams, then the last expectedParam
        // must be a paramArray.  Use the last expectedParam to validate the remaining
        // params.
        var expectedParam = expectedParams[Math.min(i, expectedLength - 1)],
            paramName = expectedParam.name;
        if (expectedParam.parameterArray) {
            // Append index of parameter in parameterArray
            paramName += "[" + (i - expectedLength + 1) + "]";
        }
        else if (!validateParameterCount && (i >= expectedLength)) {
            // there were more params than expected params.
            // count validation is disabled, and the last expected param
            // is not a parameter array. Just ignore the rest of the parameters.
            break;
        }
        e = Function._validateParameter(params[i], expectedParam, paramName);
        if (e) {
            e.popStackFrame();
            return e;
        }
    }
    return null;
}

Function._validateParameterCount = function(params, expectedParams, validateParameterCount) {
    var i, error,
        expectedLen = expectedParams.length,
        actualLen = params.length;
    if (actualLen < expectedLen) {
        // this might be ok if some parameters are optional or if theres a parameter array
        var minParams = expectedLen;
        for (i = 0; i < expectedLen; i++) {
            var param = expectedParams[i];
            if (param.optional || param.parameterArray) {
                minParams--;
            }
        }        
        if (actualLen < minParams) {
            error = true;
        }
    }
    else if (validateParameterCount && (actualLen > expectedLen)) {
        // this might be ok if a parameter is a parameterArray
        error = true;      
        for (i = 0; i < expectedLen; i++) {
            if (expectedParams[i].parameterArray) {
                error = false; // infinite parameters is ok
                break;
            }
        }  
    }

    if (error) {
        var e = Error.parameterCount();
        e.popStackFrame();
        return e;
    }

    return null;
}

Function._validateParameter = function(param, expectedParam, paramName) {
    var e,
        expectedType = expectedParam.type,
        expectedInteger = !!expectedParam.integer,
        expectedDomElement = !!expectedParam.domElement,
        mayBeNull = !!expectedParam.mayBeNull;

    e = Function._validateParameterType(param, expectedType, expectedInteger, expectedDomElement, mayBeNull, paramName);
    if (e) {
        e.popStackFrame();
        return e;
    }

    // If parameter is an array, and not undefined or null, validate the type of its elements
    var expectedElementType = expectedParam.elementType,
        elementMayBeNull = !!expectedParam.elementMayBeNull;
    if (expectedType === Array && typeof(param) !== "undefined" && param !== null &&
        (expectedElementType || !elementMayBeNull)) {
        var expectedElementInteger = !!expectedParam.elementInteger,
            expectedElementDomElement = !!expectedParam.elementDomElement;
        for (var i=0; i < param.length; i++) {
            var elem = param[i];
            e = Function._validateParameterType(elem, expectedElementType,
                expectedElementInteger, expectedElementDomElement, elementMayBeNull,
                paramName + "[" + i + "]");
            if (e) {
                e.popStackFrame();
                return e;
            }
        }
    }

    return null;
}

Function._validateParameterType = function(param, expectedType, expectedInteger, expectedDomElement, mayBeNull, paramName) {
    var e, i;

    if (typeof(param) === "undefined") {
        if (mayBeNull) {
            return null;
        }
        else {
            e = Error.argumentUndefined(paramName);
            e.popStackFrame();
            return e;
        }
    }

    if (param === null) {
        if (mayBeNull) {
            return null;
        }
        else {
            e = Error.argumentNull(paramName);
            e.popStackFrame();
            return e;
        }
    }

    if (expectedType && expectedType.__enum) {
        if (typeof(param) !== 'number') {
            e = Error.argumentType(paramName, Object.getType(param), expectedType);
            e.popStackFrame();
            return e;
        }
        if ((param % 1) === 0) {
            var values = expectedType.prototype;
            if (!expectedType.__flags || (param === 0)) {
                for (i in values) {
                    if (values[i] === param) return null;
                }
            }
            else {
                var v = param;
                for (i in values) {
                    var vali = values[i];
                    if (vali === 0) continue;
                    if ((vali & param) === vali) {
                        v -= vali;
                    }
                    if (v === 0) return null;
                }
            }
        }
        e = Error.argumentOutOfRange(paramName, param, String.format(Sys.Res.enumInvalidValue, param, expectedType.getName()));
        e.popStackFrame();
        return e;
    }

    // Text nodes are not considered DOM elements here.
    if (expectedDomElement && (!Sys._isDomElement(param) || (param.nodeType === 3))) {
        e = Error.argument(paramName, Sys.Res.argumentDomElement);
        e.popStackFrame();
        return e;
    }

    // If there is no expected type, any type is allowed.
    if (expectedType && !Sys._isInstanceOfType(expectedType, param)) {
        e = Error.argumentType(paramName, Object.getType(param), expectedType);
        e.popStackFrame();
        return e;
    }

    if (expectedType === Number && expectedInteger) {
        // Modulo operator is 5x faster than Math.round().
        // Modulo returns Number.NaN for Number.NaN, Number.POSITIVE_INFINITY, and Number.NEGATIVE_INFINITY.
        if ((param % 1) !== 0) {
            e = Error.argumentOutOfRange(paramName, param, Sys.Res.argumentInteger);
            e.popStackFrame();
            return e;
        }
    }

    return null;
}
