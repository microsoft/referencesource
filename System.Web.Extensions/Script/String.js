#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="String.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
String.__typeName = 'String';
String.__class = true;

String.prototype.endsWith = function(suffix) {
    /// <summary>Determines whether the end of this instance matches the specified string.</summary>
    /// <param name="suffix" type="String">A string to compare to.</param>
    /// <returns type="Boolean">true if suffix matches the end of this instance; otherwise, false.</returns>
    return (this.substr(this.length - suffix.length) === suffix);
}

String.prototype.startsWith = function(prefix) {
    /// <summary>Determines whether the beginning of this instance matches the specified string.</summary>
    /// <param name="prefix" type="String">The String to compare.</param>
    /// <returns type="Boolean">true if prefix matches the beginning of this string; otherwise, false.</returns>
    return (this.substr(0, prefix.length) === prefix);
}

String.prototype.trim = function() {
    /// <summary>
    ///     Removes all leading and trailing white-space characters from the current String object.
    /// </summary>
    /// <returns type="String">
    ///     The string that remains after all white-space characters are removed from the start and end of the current String object.
    /// </returns>
    return this.replace(/^\s+|\s+$/g, '');
}

String.prototype.trimEnd = function() {
    /// <summary>
    ///     Removes all trailing white spaces from the current String object.
    /// </summary>
    /// <returns type="String">
    ///     The string that remains after all white-space characters are removed from the end of the current String object.
    /// </returns>
    return this.replace(/\s+$/, '');
}

String.prototype.trimStart = function() {
    /// <summary>
    ///     Removes all leading white spaces from the current String object.
    /// </summary>
    /// <returns type="String">
    ///     The string that remains after all white-space characters are removed from the start of the current String object.
    /// </returns>
    return this.replace(/^\s+/, '');
}

String.format = function(format, args) {
    /// <summary>
    ///     Replaces the format items in a specified String with the text equivalents of the values of
    ///     corresponding object instances.  The invariant culture will be used to format dates and numbers.
    /// </summary>
    /// <param name="format" type="String">A format string.</param>
    /// <param name="args" parameterArray="true" mayBeNull="true">The objects to format.</param>
    /// <returns type="String">
    ///     A copy of format in which the format items have been replaced by the
    ///     string equivalent of the corresponding instances of object arguments.
    /// </returns>
    return String._toFormattedString(false, arguments);
}

String._toFormattedString = function(useLocale, args) {
    var result = '';
    var format = args[0];

    for (var i=0;;) {
        // Find the next opening or closing brace
        var open = format.indexOf('{', i);
        var close = format.indexOf('}', i);
        if ((open < 0) && (close < 0)) {
            // Not found: copy the end of the string and break
            result += format.slice(i);
            break;
        }
        if ((close > 0) && ((close < open) || (open < 0))) {
            #if DEBUG
            // Closing brace before opening is an error
            if (format.charAt(close + 1) !== '}') {
                throw Error.argument('format', Sys.Res.stringFormatBraceMismatch);
            }
            #endif
            result += format.slice(i, close + 1);
            i = close + 2;
            continue;
        }

        // Copy the string before the brace
        result += format.slice(i, open);
        i = open + 1;

        // Check for double braces (which display as one and are not arguments)
        if (format.charAt(i) === '{') {
            result += '{';
            i++;
            continue;
        }

        #if DEBUG
        // at this point we have a valid opening brace, which should be matched by a closing brace.
        if (close < 0) throw Error.argument('format', Sys.Res.stringFormatBraceMismatch);
        #else
        // This test is just done to break a potential infinite loop for invalid format strings.
        // The code here is minimal because this is an error condition in debug mode anyway.
        if (close < 0) break;
        #endif

        // Find the closing brace

        // Get the string between the braces, and split it around the ':' (if any)
        var brace = format.substring(i, close);
        var colonIndex = brace.indexOf(':');
        var argNumber = parseInt((colonIndex < 0)? brace : brace.substring(0, colonIndex), 10) + 1;
        ##DEBUG if (isNaN(argNumber)) throw Error.argument('format', Sys.Res.stringFormatInvalid);
        var argFormat = (colonIndex < 0)? '' : brace.substring(colonIndex + 1);

        var arg = args[argNumber];
        if (typeof(arg) === "undefined" || arg === null) {
            arg = '';
        }

        // If it has a toFormattedString method, call it.  Otherwise, call toString()
        if (arg.toFormattedString) {
            result += arg.toFormattedString(argFormat);
        }
        else if (useLocale && arg.localeFormat) {
            result += arg.localeFormat(argFormat);
        }
        else if (arg.format) {
            result += arg.format(argFormat);
        }
        else
            result += arg.toString();

        i = close + 1;
    }

    return result;
}