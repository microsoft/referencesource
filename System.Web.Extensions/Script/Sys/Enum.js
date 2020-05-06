#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Enum.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
function Sys$Enum$parse(value, ignoreCase) {
    /// <summary>
    ///     Converts the string representation of the name or numeric value of one or more enumerated
    ///     constants to an equivalent enumerated object.
    /// </summary>
    /// <example>
    ///     Sys.UI.LineType.parse("solid, wide");
    /// </example>
    /// <param name="value" type="String">A string containing the name or value to convert.</param>
    /// <param name="ignoreCase" type="Boolean" optional="true">
    ///   If true, the parsing will be done case-insensitively.
    ///   If omitted, the parsing is done case-sensitively.
    /// </param>
    /// <returns>An object of type enumType whose value is represented by value.</returns>
    var values, parsed, val;
    if (ignoreCase) {
        values = this.__lowerCaseValues;
        if (!values) {
            this.__lowerCaseValues = values = {};
            var prototype = this.prototype;
            for (var name in prototype) {
                values[name.toLowerCase()] = prototype[name];
            }
        }
    }
    else {
        values = this.prototype;
    }
    if (!this.__flags) {
        val = (ignoreCase ? value.toLowerCase() : value);
        parsed = values[val.trim()];
        if (typeof(parsed) !== 'number') throw Error.argument('value', String.format(Sys.Res.enumInvalidValue, value, this.__typeName));
        return parsed;
    }
    else {
        var parts = (ignoreCase ? value.toLowerCase() : value).split(',');
        var v = 0;

        for (var i = parts.length - 1; i >= 0; i--) {
            var part = parts[i].trim();
            parsed = values[part];
            if (typeof(parsed) !== 'number') throw Error.argument('value', String.format(Sys.Res.enumInvalidValue, value.split(',')[i].trim(), this.__typeName));
            v |= parsed;
        }
        return v;
    }
}

function Sys$Enum$toString(value) {
    /// <summary>Converts the value of an enum instance to its equivalent string representation.</summary>
    /// <example>
    ///     Sys.UI.LineType.toString(Sys.UI.LineType.solid | Sys.UI.LineType.wide)
    ///     returns "Sys.UI.LineType.solid, Sys.UI.LineType.wide".
    /// </example>
    /// <param name="value" optional="true" mayBeNull="true">The value of the enum instance for which the string representation must be constructed.</param>
    /// <returns type="String">The string representation of "value".</returns>
    // Need to do the type check manually instead of using parameter validation to be able to return
    // an error message that mentions the actual enum type that's expected instead of Number.
    if ((typeof(value) === 'undefined') || (value === null)) return this.__string;
    ##DEBUG if ((typeof(value) != 'number') || ((value % 1) !== 0)) throw Error.argumentType('value', Object.getType(value), this);
    var values = this.prototype;
    var i;
    if (!this.__flags || (value === 0)) {
        for (i in values) {
            if (values[i] === value) {
                return i;
            }
        }
    }
    else {
        var sorted = this.__sortedValues;
        if (!sorted) {
            sorted = [];
            for (i in values) {
                sorted[sorted.length] = {key: i, value: values[i]};
            }
            sorted.sort(function(a, b) {
                return a.value - b.value;
            });
            this.__sortedValues = sorted;
        }
        var parts = [];
        var v = value;
        for (i = sorted.length - 1; i >= 0; i--) {
            var kvp = sorted[i];
            var vali = kvp.value;
            if (vali === 0) continue;
            if ((vali & value) === vali) {
                parts[parts.length] = kvp.key;
                v -= vali;
                if (v === 0) break;
            }
        }
        if (parts.length && v === 0) return parts.reverse().join(', ');
    }
    #if DEBUG
    throw Error.argumentOutOfRange('value', value, String.format(Sys.Res.enumInvalidValue, value, this.__typeName));
    #else
    return '';
    #endif
}

Type.prototype.registerEnum = function(name, flags) {
    /// <summary>Registers an enum type.</summary>
    /// <example>
    ///     Sys.UI.LineType = function() {
    ///         /// &lt;field name="solid" type="Number" integer="true" static="true"/&gt;
    ///         /// &lt;field name="wide" type="Number" integer="true" static="true"/&gt;
    ///         /// &lt;field name="rounded" type="Number" integer="true" static="true"/&gt;
    ///     }
    ///     Sys.UI.LineType.prototype = {
    ///         solid:      1,
    ///         wide:       2,
    ///         rounded:    4,
    ///     }
    ///     Sys.UI.LineType.registerEnum("Sys.UI.LineType", true);
    /// </example>
    /// <param name="name" type="String">The fully-qualified name of the enum.</param>
    /// <param name="flags" type="Boolean" optional="true">True if the enum is a flags collection.</param>
    #if DEBUG
    if (!Type.__fullyQualifiedIdentifierRegExp.test(name)) throw Error.argument('name', Sys.Res.notATypeName);
    // Check if the type name parses to an existing object that matches this.
    var parsedName;
    try {
        parsedName = eval(name);
    }
    catch(e) {
        throw Error.argument('name', Sys.Res.argumentTypeName);
    }
    if (parsedName !== this) throw Error.argument('name', Sys.Res.badTypeName);
    if (Sys.__registeredTypes[name]) throw Error.invalidOperation(String.format(Sys.Res.typeRegisteredTwice, name));
    for (var j in this.prototype) {
        var val = this.prototype[j];
        if (!Type.__identifierRegExp.test(j)) throw Error.invalidOperation(String.format(Sys.Res.enumInvalidValueName, j));
        if (typeof(val) !== 'number' || (val % 1) !== 0) throw Error.invalidOperation(Sys.Res.enumValueNotInteger);
        if (typeof(this[j]) !== 'undefined') throw Error.invalidOperation(String.format(Sys.Res.enumReservedName, j));
    }
    #endif
    Sys.__upperCaseTypes[name.toUpperCase()] = this;

    for (var i in this.prototype) {
        this[i] = this.prototype[i];
    }
    this.__typeName = name;
    this.parse = Sys$Enum$parse;
    this.__string = this.toString();
    this.toString = Sys$Enum$toString;
    this.__flags = flags;
    this.__enum = true;
    ##DEBUG Sys.__registeredTypes[name] = true;
}

Type.isEnum = function(type) {
    /// <param name="type" mayBeNull="true">The type to test.</param>
    /// <returns type="Boolean">True if the type is an enum.</returns>
    if ((typeof(type) === 'undefined') || (type === null)) return false;
    return !!type.__enum;
}

Type.isFlags = function(type) {
    /// <param name="type" mayBeNull="true">The type to test.</param>
    /// <returns type="Boolean">True if the type is a set of flags.</returns>
    if ((typeof(type) === 'undefined') || (type === null)) return false;
    return !!type.__flags;
}
