#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Object.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Object.__typeName = 'Object';
Object.__class = true;

Object.getType = function(instance) {
    /// <param name="instance">The object for which the type must be returned.</param>
    /// <returns type="Type">The type of the object.</returns>
    var ctor = instance.constructor;
    if (!ctor || (typeof(ctor) !== "function") || !ctor.__typeName || (ctor.__typeName === 'Object')) {
        return Object;
    }
    return ctor;
}

Object.getTypeName = function(instance) {
    /// <param name="instance">The object for which the type name must be returned.</param>
    /// <returns type="String">The name of the type of the object.</returns>
    return Object.getType(instance).getName();
}
