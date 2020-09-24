#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="JavaScriptSerializer.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.Serialization.JavaScriptSerializer = function() {
    /// <summary>Provides serialization from JavaScript object to JavaScript object notation.</summary>
    // DevDiv #62350: Considered making all methods static and removing this constructor,
    // but this would have been a breaking change from Atlas 1.0 to Atlas Orcas so was rejected.
}
Sys.Serialization.JavaScriptSerializer.registerClass('Sys.Serialization.JavaScriptSerializer');

Sys.Serialization.JavaScriptSerializer._charsToEscapeRegExs = [];
Sys.Serialization.JavaScriptSerializer._charsToEscape = [];
Sys.Serialization.JavaScriptSerializer._dateRegEx = new RegExp('(^|[^\\\\])\\"\\\\/Date\\((-?[0-9]+)(?:[a-zA-Z]|(?:\\+|-)[0-9]{4})?\\)\\\\/\\"', 'g');
Sys.Serialization.JavaScriptSerializer._escapeChars = {};
// DevDiv Bugs 139383:
// Escape the backslashes in escapeRegEx so that we pass an escaped sequence to the RegExp,
// instead of literal characters. Safari does not support literal characters as it fails on iPhone 1.01.
Sys.Serialization.JavaScriptSerializer._escapeRegEx = new RegExp('["\\\\\\x00-\\x1F]', 'i');
Sys.Serialization.JavaScriptSerializer._escapeRegExGlobal = new RegExp('["\\\\\\x00-\\x1F]', 'g');
Sys.Serialization.JavaScriptSerializer._jsonRegEx = new RegExp('[^,:{}\\[\\]0-9.\\-+Eaeflnr-u \\n\\r\\t]', 'g');
Sys.Serialization.JavaScriptSerializer._jsonStringRegEx = new RegExp('"(\\\\.|[^"\\\\])*"', 'g');
Sys.Serialization.JavaScriptSerializer._serverTypeFieldName = '__type';

Sys.Serialization.JavaScriptSerializer._init = function() {
    var replaceChars = ['\\u0000','\\u0001','\\u0002','\\u0003','\\u0004','\\u0005','\\u0006','\\u0007',
                        '\\b','\\t','\\n','\\u000b','\\f','\\r','\\u000e','\\u000f','\\u0010','\\u0011',
                        '\\u0012','\\u0013','\\u0014','\\u0015','\\u0016','\\u0017','\\u0018','\\u0019',
                        '\\u001a','\\u001b','\\u001c','\\u001d','\\u001e','\\u001f'];
    // Backslash needs to be put at the beginning of the charsToEscape array so that it
    // gets replaced first in the serializeStringWithBuilder method to avoid adding
    // an extra backslash for the newly added backslash of other escaped chars.
    Sys.Serialization.JavaScriptSerializer._charsToEscape[0] = '\\';
    Sys.Serialization.JavaScriptSerializer._charsToEscapeRegExs['\\'] = new RegExp('\\\\', 'g');
    Sys.Serialization.JavaScriptSerializer._escapeChars['\\'] = '\\\\';
    Sys.Serialization.JavaScriptSerializer._charsToEscape[1] = '"';
    Sys.Serialization.JavaScriptSerializer._charsToEscapeRegExs['"'] = new RegExp('"', 'g');
    Sys.Serialization.JavaScriptSerializer._escapeChars['"'] = '\\"';
    for (var i = 0; i < 32; i++) {
        var c = String.fromCharCode(i);
        Sys.Serialization.JavaScriptSerializer._charsToEscape[i+2] = c;
        Sys.Serialization.JavaScriptSerializer._charsToEscapeRegExs[c] = new RegExp(c, 'g');
        Sys.Serialization.JavaScriptSerializer._escapeChars[c] = replaceChars[i];
    }
}

Sys.Serialization.JavaScriptSerializer._serializeBooleanWithBuilder = function(object, stringBuilder) {
    stringBuilder.append(object.toString());
}

Sys.Serialization.JavaScriptSerializer._serializeNumberWithBuilder = function(object, stringBuilder) {
    if (isFinite(object)) {
        stringBuilder.append(String(object));
    }
    else {
        throw Error.invalidOperation(Sys.Res.cannotSerializeNonFiniteNumbers);
    }
}

// DevDiv 154418: for performance improvement, this function has been considered in different ways to come up
// with the following solutions, which depend on the length of the input strings and the browser types, implement
// one of the below three methods for scanning strings and performing escaped char replacements. Other solutions
// that had been considered and their performance comparisons are attached in DevDiv 154418.
Sys.Serialization.JavaScriptSerializer._serializeStringWithBuilder = function(string, stringBuilder) {
    stringBuilder.append('"');
    if (Sys.Serialization.JavaScriptSerializer._escapeRegEx.test(string)) {
        // Initialization the following arrays once: charsToEscapeRegExs, charsToEscape, and
        // escapeChars. These arrays are used for perf reasons, not neccessary required.
        if (Sys.Serialization.JavaScriptSerializer._charsToEscape.length === 0) {
            Sys.Serialization.JavaScriptSerializer._init();
        }
        // DevDiv154418: Depending on the string length, different methods for replacing escaped chars are used for
        // perf reason. Currently 128 is used as a cutoff point based on perf comparisons attached in DevDiv154418
        if (string.length < 128) {
            string = string.replace(Sys.Serialization.JavaScriptSerializer._escapeRegExGlobal,
                function(x) { return Sys.Serialization.JavaScriptSerializer._escapeChars[x]; });
        }
        else {
            for (var i = 0; i < 34; i++) {
                var c = Sys.Serialization.JavaScriptSerializer._charsToEscape[i];
                if (string.indexOf(c) !== -1) {
                    if (Sys.Browser.agent === Sys.Browser.Opera || Sys.Browser.agent === Sys.Browser.FireFox) {
                        string = string.split(c).join(Sys.Serialization.JavaScriptSerializer._escapeChars[c]);
                    }
                    else {
                        string = string.replace(Sys.Serialization.JavaScriptSerializer._charsToEscapeRegExs[c],
                            Sys.Serialization.JavaScriptSerializer._escapeChars[c]);
                    }
                }
            }
       }
    }
    stringBuilder.append(string);
    stringBuilder.append('"');
}

Sys.Serialization.JavaScriptSerializer._serializeWithBuilder = function(object, stringBuilder, sort, prevObjects) {
    var i;
    switch (typeof object) {
    case 'object':
        if (object) {
            #if DEBUG
            if (prevObjects){
                // The loop below makes serilzation O(n^2) worst case for linked list like struture
                // where in depth of graph is in linear proportion to number of elements.
                // However the depth of graph is limited by call stack size(less than 1000 in IE7) hence 
                // the performance hit is within reasonable bounds for debug mode
                for( var j = 0; j < prevObjects.length; j++) {
                    if (prevObjects[j] === object) {
                        throw Error.invalidOperation(Sys.Res.cannotSerializeObjectWithCycle);
                    }
                }
            }
            else {
                prevObjects = new Array();
            }
            try {
                Array.add(prevObjects, object);
            #endif
                
                if (Number.isInstanceOfType(object)){
                    Sys.Serialization.JavaScriptSerializer._serializeNumberWithBuilder(object, stringBuilder);
                }
                else if (Boolean.isInstanceOfType(object)){
                    Sys.Serialization.JavaScriptSerializer._serializeBooleanWithBuilder(object, stringBuilder);
                }
                else if (String.isInstanceOfType(object)){
                    Sys.Serialization.JavaScriptSerializer._serializeStringWithBuilder(object, stringBuilder);
                }
            
                // Arrays
                else if (Array.isInstanceOfType(object)) {
                    stringBuilder.append('[');
                   
                    for (i = 0; i < object.length; ++i) {
                        if (i > 0) {
                            stringBuilder.append(',');
                        }
                        Sys.Serialization.JavaScriptSerializer._serializeWithBuilder(object[i], stringBuilder,false,prevObjects);
                    }
                    stringBuilder.append(']');
                }
                else {
                    // DivDev 41125: Do not confuse atlas serialized strings with dates
                    // Currently it always serialize as \/Date({milliseconds from 1970/1/1})\/
                    // For example \/Date(123)\/
                    if (Date.isInstanceOfType(object)) {
                        stringBuilder.append('"\\/Date(');
                        stringBuilder.append(object.getTime());
                        stringBuilder.append(')\\/"');
                        break;
                    }

                    var properties = [];
                    var propertyCount = 0;
                    for (var name in object) {
                        // skip internal properties that should not be serialized.
                        if (name.startsWith('$')) {
                            continue;
                        }
                        //DevDiv 74427 : Need to make sure that _type is first item on JSON serialization
                        if (name === Sys.Serialization.JavaScriptSerializer._serverTypeFieldName && propertyCount !== 0){
                            // if current propery Name is __type, swap it with the first element on property array.
                            properties[propertyCount++] = properties[0];
                            properties[0] = name;
                        }
                        else{
                            properties[propertyCount++] = name;
                        }
                    }
                    if (sort) properties.sort();

                    stringBuilder.append('{');
                    var needComma = false;
                     
                    for (i=0; i<propertyCount; i++) {
                        var value = object[properties[i]];
                        if (typeof value !== 'undefined' && typeof value !== 'function') {
                            if (needComma) {
                                stringBuilder.append(',');
                            }
                            else {
                                needComma = true;
                            }
                           
                            // Serialize the name of the object property, then the value
                            Sys.Serialization.JavaScriptSerializer._serializeWithBuilder(properties[i], stringBuilder, sort, prevObjects);
                            stringBuilder.append(':');
                            Sys.Serialization.JavaScriptSerializer._serializeWithBuilder(value, stringBuilder, sort, prevObjects);
                          
                        }
                    }
                stringBuilder.append('}');
                }
            #if DEBUG
            }
            finally {
                Array.removeAt(prevObjects, prevObjects.length - 1);
            }
            #endif
        }
        else {
            stringBuilder.append('null');
        }
        break;

    case 'number':
        Sys.Serialization.JavaScriptSerializer._serializeNumberWithBuilder(object, stringBuilder);
        break;

    case 'string':
        Sys.Serialization.JavaScriptSerializer._serializeStringWithBuilder(object, stringBuilder);
        break;

    case 'boolean':
        Sys.Serialization.JavaScriptSerializer._serializeBooleanWithBuilder(object, stringBuilder);
        break;

    default:
        stringBuilder.append('null');
        break;
    }
}

Sys.Serialization.JavaScriptSerializer.serialize = function(object) {
    /// <summary>Generates a JSON string from an object.</summary>
    /// <param name="object" mayBeNull="true">The object to serialize.</param>
    /// <returns type="String">The JSON string representation of the object.</returns>
    var stringBuilder = new Sys.StringBuilder();
    Sys.Serialization.JavaScriptSerializer._serializeWithBuilder(object, stringBuilder, false);
    return stringBuilder.toString();
}

Sys.Serialization.JavaScriptSerializer.deserialize = function(data, secure) {
    /// <summary>Deserializes a JSON string.</summary>
    /// <param name="data" type="String">The JSON string to eval.</param>
    /// <param name="secure" type="Boolean" optional="true">
    ///     True if the method should perform JSON conformance checks before evaluating. False by default.
    /// </param>
    /// <returns>The results of eval applied to data.</returns>
    
    if (data.length === 0) throw Error.argument('data', Sys.Res.cannotDeserializeEmptyString);
    // DevDiv 41127: Never confuse atlas serialized strings with dates.
    // DevDiv 74430: JavasciptSerializer will need to handle date time offset - following WCF design
    // serilzed dates might look like "\/Date(123)\/" or "\/Date(123A)" or "Date(123+4567)" or Date(123-4567)"
    // the regex escaped version of this pattern is \"\\/Date\(123(?:[a-zA-Z]|(?:\+|-)[0-9]{4})?\)\\/\"
    // but we must also do js escaping to put it in the string. Escape all \ with \\
    // Result: \\"\\\\/Date\\(123(?:[a-zA-Z]|(?:\\+|-)[0-9]{4})?\\)\\\\/\\"
    // The 123 can really be any number with an optional -, and we want to capture it.
    // Regex for that is: (-?[0-9]+)
    // Result: \\"\\\\/Date\\((-?[0-9]+)(?:[a-zA-Z]|(?:\\+|-)[0-9]{4})?\\)\\\\/\\"
    // We want to avoid replacing serialized strings that happen to contain this string as a substring.
    // We can do that by excluding matches that start with a slash \ since that means the first quote is escaped.
    // The first quote of a real date string will never be escaped and so will never be preceeded with \
    // So we want to add regex pattern (^|[^\\]) to the beginning, which means "beginning of string or anything but slash".
    // JS Escaped version: (^|[^\\\\])
    // Result: (^|[^\\\\])\\"\\\\/Date\\((-?[0-9]+)(?:[a-zA-Z]|(?:\\+|-)[0-9]{4})?\\)\\\\/\\"
    // Finally, the replace string is $1new Date($2). We must include $1 so we put back the potentially matched character we captured.

    try {    
        var exp = data.replace(Sys.Serialization.JavaScriptSerializer._dateRegEx, "$1new Date($2)");
        
        if (secure && Sys.Serialization.JavaScriptSerializer._jsonRegEx.test(
             exp.replace(Sys.Serialization.JavaScriptSerializer._jsonStringRegEx, ''))) throw null;

        return eval('(' + exp + ')');
    }
    catch (e) {
         throw Error.argument('data', Sys.Res.cannotDeserializeInvalidJson);
    }
}