#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="StringBuilder.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.StringBuilder = function(initialText) {
    /// <summary>Provides an optimized mechanism to concatenate a sequence of strings.</summary>
    /// <param name="initialText" optional="true" mayBeNull="true">The initial text for the StringBuilder.</param>
    this._parts = (typeof(initialText) !== 'undefined' && initialText !== null && initialText !== '') ?
        [initialText.toString()] : [];
    this._value = {};
    this._len = 0;
}

Sys.StringBuilder.prototype = {
    append: function(text) {
        /// <summary>Appends a new string at the end of the StringBuilder.</summary>
        /// <param name="text" mayBeNull="true">The string to append.</param>
        this._parts[this._parts.length] = text;
    },

    appendLine: function(text) {
        /// <summary>Appends a new string as a line of text at the end of the StringBuilder.</summary>
        /// <param name="text" optional="true" mayBeNull="true">The string to append.</param>
        this._parts[this._parts.length] =
            ((typeof(text) === 'undefined') || (text === null) || (text === '')) ?
            '\r\n' : text + '\r\n';
    },

    clear: function() {
        /// <summary>Clears the StringBuilder of its current contents.</summary>
        this._parts = [];
        this._value = {};
        this._len = 0;
    },

    isEmpty: function() {
        /// <summary>Use this method to determine if the StringBuilder has contents.</summary>
        /// <returns type="Boolean">True if the StringBuilder has any contents.</returns>
        if (this._parts.length === 0) return true;
        return this.toString() === '';
    },

    // separator may be null, to match behavior of ECMA Array.join(separator) and
    // .NET String.Join(separator, value)
    toString: function(separator) {
        /// <summary>Creates a string from the contents of the StringBuilder.</summary>
        /// <param name="separator" type="String" optional="true" mayBeNull="true">
        ///     The separator to insert between the elements of the StringBuilder.
        /// </param>
        /// <returns type="String">The string built from the StringBuilder.</returns>
        separator = separator || '';
        var parts = this._parts;
        if (this._len !== parts.length) {
            this._value = {};
            this._len = parts.length;
        }
        var val = this._value;
        if (typeof(val[separator]) === 'undefined') {
            // Need to remove empty elements before joining in the separator case
            if (separator !== '') {
                for (var i = 0; i < parts.length;) {
                    if ((typeof(parts[i]) === 'undefined') || (parts[i] === '') || (parts[i] === null)) {
                        parts.splice(i, 1);
                    }
                    else {
                        i++;
                    }
                }
            }
            val[separator] = this._parts.join(separator);
        }
        return val[separator];
    }
}
Sys.StringBuilder.registerClass('Sys.StringBuilder');
