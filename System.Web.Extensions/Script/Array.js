#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Array.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Array.__typeName = 'Array';
Array.__class = true;

Array.add = Array.enqueue = function(array, item) {
    /// <summary>Adds an element at the end of the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to add to.</param>
    /// <param name="item" mayBeNull="true">The object to add.</param>

    // Setting Array[Array.length] is faster than Array.push() for a single element.
    array[array.length] = item;
}

Array.addRange = function(array, items) {
    /// <summary>Adds a range of items at the end of the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to add to.</param>
    /// <param name="items" type="Array" elementMayBeNull="true">The array of items to append.</param>

    // Array.push() for multiple elements is faster than setting Array[Array.length] in a loop.
    array.push.apply(array, items);
}

Array.clear = function(array) {
    /// <summary>Clears the array of its elements.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to clear.</param>
    array.length = 0;
}

Array.clone = function(array) {
    /// <summary>Makes a clone of the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to clone.</param>
    /// <returns type="Array" elementMayBeNull="true">A clone of the array.</returns>
    if (array.length === 1) {
        return [array[0]];
    }
    else {
        // When the Array ctor is called with 0 or 2 or more arguments, it creates a new
        // Array with the elements from the argument list.
        return Array.apply(null, array);
    }
}

Array.contains = function(array, item) {
    /// <summary>Use this method to determine if an array contains the specified element.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to look into.</param>
    /// <param name="item" mayBeNull="true">The object to find in the array.</param>
    /// <returns type="Boolean">True if the object was found.</returns>
    return (Sys._indexOf(array, item) >= 0);
}

Array.dequeue = function(array) {
    /// <param name="array" type="Array" elementMayBeNull="true">Removes and returns the object at the beginning of the array.</param>
    /// <returns mayBeNull="true">The object that is removed from the beginning of the array.</returns>
    return array.shift();
}

Array.forEach = function(array, method, instance) {
    /// <summary>Calls the specified function on each element of the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to enumerate.</param>
    /// <param name="method" type="Function">The method to call.
    ///     The method should take the array element, the index of the element and
    ///     the array itself as its parameters.
    /// </param>
    /// <param name="instance" optional="true" mayBeNull="true">The context under which the function must run (i.e. what 'this' means inside the function).</param>
    for (var i = 0, l = array.length; i < l; i++) {
        var elt = array[i];
        if (typeof(elt) !== 'undefined') method.call(instance, elt, i, array);
    }
}

Array.indexOf = function(array, item, start) {
    /// <summary>Finds the index in the array of the provided item.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to look into.</param>
    /// <param name="item" optional="true" mayBeNull="true">The object to find.</param>
    /// <param name="start" optional="true" mayBeNull="true">The index where the search begins.</param>
    /// <returns type="Number">The index of the item or -1 if it wasn't found.</returns>
    return Sys._indexOf(array, item, start);
}

Array.insert = function(array, index, item) {
    /// <summary>Inserts an item at the specified index.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to insert into.</param>
    /// <param name="index" mayBeNull="true">The index where the item will be inserted.</param>
    /// <param name="item" mayBeNull="true">The item to insert.</param>
    array.splice(index, 0, item);
}

Array.parse = function(value) {
    /// <summary>Creates an array from a string representation of the form "[elt1, elt2, elt3]".</summary>
    /// <param name="value" type="String" mayBeNull="true">The string representation of the array.</param>
    /// <returns type="Array" elementMayBeNull="true">An array built from the string representation.</returns>
    if (!value) return [];
    #if DEBUG
    var v = eval(value);
    if (!Array.isInstanceOfType(v)) throw Error.argument('value', Sys.Res.arrayParseBadFormat);
    return v;
    #else
    return eval(value);
    #endif
}

Array.remove = function(array, item) {
    /// <summary>Removes the first occurence of an item from the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to remove from.</param>
    /// <param name="item" mayBeNull="true">The item to remove.</param>
    /// <returns type="Boolean">True if the item was found.</returns>
    var index = Sys._indexOf(array, item);
    if (index >= 0) {
        array.splice(index, 1);
    }
    return (index >= 0);
}

Array.removeAt = function(array, index) {
    /// <summary>Removes the item at the specified index from the array.</summary>
    /// <param name="array" type="Array" elementMayBeNull="true">The array to remove from.</param>
    /// <param name="index" mayBeNull="true">The index of the item to remove.</param>
    array.splice(index, 1);
}

// a private _indexOf allows for other public Array members to perform indexOf() without
// causing a double validation of its public parameters, a huge source of slowness in debug mode
// for code that uses any of the Array members that call indexOf (Array.remove and Array.contains).
// It is Sys._indexOf instead of Array._indexOf to avoid adding another member to a built-in type.
Sys._indexOf = function(array, item, start) {
    if (typeof(item) === "undefined") return -1;
    var length = array.length;
    if (length !== 0) {
        // Coerce into number ("1a" will become NaN, which is consistent with the built-in behavior of similar Array methods)
        start = start - 0;
        // NaN becomes zero
        if (isNaN(start)) {
            start = 0;
        }
        else {
            // If start is positive or negative infinity, don't try to truncate it.
            // The infinite values will be handled correctly by the subsequent code.
            if (isFinite(start)) {
                // This is faster than doing Math.floor or Math.ceil
                start = start - (start % 1);
            }
            // Negative start indices start from the end
            if (start < 0) {
                start = Math.max(0, length + start);
            }
        }

        // A do/while loop seems to have equal performance to a for loop in this scenario
        for (var i = start; i < length; i++) {
            if ((typeof(array[i]) !== "undefined") && (array[i] === item)) {
                return i;
            }
        }
    }
    return -1;
}
