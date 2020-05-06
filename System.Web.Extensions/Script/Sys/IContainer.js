#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="IContainer.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.IContainer = function() {
    ##DEBUG throw Error.notImplemented();
}
Sys.IContainer.prototype = {
    #if DEBUG
    addComponent: function(component) {
        /// <param name="component" type="Sys.Component"/>
        throw Error.notImplemented();
    },
    removeComponent: function(component) {
        /// <param name="component" type="Sys.Component"/>
        throw Error.notImplemented();
    },
    findComponent: function(id) {
        /// <param name="id" type="String"/>
        /// <returns type="Sys.Component"/>
        throw Error.notImplemented();
    },
    getComponents: function() {
        /// <returns type="Array" elementType="Sys.Component"/>
        throw Error.notImplemented();
    }
    #endif
}
Sys.IContainer.registerInterface("Sys.IContainer");

