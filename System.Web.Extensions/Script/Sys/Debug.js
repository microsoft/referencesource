#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Debug.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys._Debug = function() {
    /// <summary locid="M:J#Sys.Debug.#ctor">Provides a set of methods that help debug your code.</summary>
    /// <field name="isDebug" type="Boolean" locid="F:J#Sys.Debug.isDebug"/>
}
Sys._Debug.prototype = {

    _appendConsole: function(text) {
        // VS script debugger output window.
        if ((typeof(Debug) !== 'undefined') && Debug.writeln) {
            Debug.writeln(text);
        }
        // FF firebug and Safari console.
        if (window.console && window.console.log) {
            window.console.log(text);
        }
        // Opera console.
        if (window.opera) {
            window.opera.postError(text);
        }
        // WebDevHelper console.
        if (window.debugService) {
            window.debugService.trace(text);
        }
    },

    _appendTrace: function(text) {
        var traceElement = document.getElementById('TraceConsole');
        if (traceElement && (traceElement.tagName.toUpperCase() === 'TEXTAREA')) {
            traceElement.value += text + '\n';
        }
    },

    assert: function(condition, message, displayCaller) {
        /// <summary locid="M:J#Sys.Debug.assert">
        ///     Checks for a condition, displays a message and prompts the user to break
        ///      into the debugger if the condition is false.
        /// </summary>
        /// <param name="condition" type="Boolean">true to prevent a message being displayed; otherwise, false.</param>
        /// <param name="message" type="String" optional="true" mayBeNull="true">A message to display.</param>
        /// <param name="displayCaller" type="Boolean" optional="true">True if the function calling assert should be displayed in the message.</param>
        if (!condition) {
            message = (displayCaller && this.assert.caller) ?
                String.format(Sys.Res.assertFailedCaller, message, this.assert.caller) :
                String.format(Sys.Res.assertFailed, message);

            if (confirm(String.format(Sys.Res.breakIntoDebugger, message))) {
                this.fail(message);
            }
        }
    },

    clearTrace: function() {
        /// <summary locid="M:J#Sys.Debug.clearTrace" />
        var traceElement = document.getElementById('TraceConsole');
        if (traceElement && (traceElement.tagName.toUpperCase() === 'TEXTAREA')) {
            traceElement.value = '';
        }
    },

    fail: function(message) {
        /// <summary locid="M:J#Sys.Debug.fail">Displays a message in the debugger's output window and breaks into the debugger.</summary>
        /// <param name="message" type="String" mayBeNull="true">A message to display.</param>
        this._appendConsole(message);

        // Cannot execute eval('debugger') in browsers that don't have a debugger statement, since it causes a parse error.
        if (Sys.Browser.hasDebuggerStatement) {
            eval('debugger');
        }
    },

    trace: function(text) {
        /// <summary locid="M:J#Sys.Debug.trace">Appends a text line to the debugger console and the TraceConsole textarea element if available.</summary>
        /// <param name="text">Text for trace.</param>
        this._appendConsole(text);
        this._appendTrace(text);
    },

    traceDump: function(object, name) {
        /// <summary locid="M:J#Sys.Debug.traceDump">Dumps an object to the debugger console and the TraceConsole textarea element if available.</summary>
        /// <param name="object" mayBeNull="true">Object for trace dump.</param>
        /// <param name="name" type="String" mayBeNull="true" optional="true">Object name.</param>
        var text = this._traceDump(object, name, true);
    },

    _traceDump: function(object, name, recursive, indentationPadding, loopArray) {
        name = name? name : 'traceDump';
        indentationPadding = indentationPadding? indentationPadding : '';
        if (object === null) {
            this.trace(indentationPadding + name + ': null');
            return;
        }
        switch(typeof(object)) {
            case 'undefined':
                this.trace(indentationPadding + name + ': Undefined');
                break;
            case 'number': case 'string': case 'boolean':
                this.trace(indentationPadding + name + ': ' + object);
                break;
            default:
                if (Date.isInstanceOfType(object) || RegExp.isInstanceOfType(object)) {
                    this.trace(indentationPadding + name + ': ' + object.toString());
                    break;
                }
                if (!loopArray) {
                    loopArray = [];
                }
                else if (Array.contains(loopArray, object)) {
                    this.trace(indentationPadding + name + ': ...');
                    return;
                }
                Array.add(loopArray, object);

                // don't recurse into dom elements.
                // trace dump has to use '==' for window when it's passed as event arg in IE.
                // i.e., body onLoad="Sys.Debug.traceDump(window)"
                if ((object == window) || (object === document) ||
                    (window.HTMLElement && (object instanceof HTMLElement)) ||
                    (typeof(object.nodeName) === 'string')) {
                    var tag = object.tagName? object.tagName : 'DomElement';
                    if (object.id) {
                        tag += ' - ' + object.id;
                    }
                    this.trace(indentationPadding + name + ' {' +  tag + '}');
                }
                // objects and arrays
                else {
                    var typeName = Object.getTypeName(object);
                    this.trace(indentationPadding + name + (typeof(typeName) === 'string' ? ' {' + typeName + '}' : ''));
                    if ((indentationPadding === '') || recursive) {
                        indentationPadding += "    ";
                        var i, length, properties, p, v;
                        if (Array.isInstanceOfType(object)) {
                            length = object.length;
                            for (i = 0; i < length; i++) {
                                this._traceDump(object[i], '[' + i + ']', recursive, indentationPadding, loopArray);
                            }
                        }
                        else {
                            for (p in object) {
                                v = object[p];
                                if (!Function.isInstanceOfType(v)) {
                                    this._traceDump(v, p, recursive, indentationPadding, loopArray);
                                }
                            }
                        }
                    }
                }
                Array.remove(loopArray, object);
        }
    }
}
Sys._Debug.registerClass('Sys._Debug');

Sys.Debug = new Sys._Debug();
#if DEBUG
    Sys.Debug.isDebug = true;
#else
    Sys.Debug.isDebug = false;
#endif
