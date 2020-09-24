#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="INotifyPropertyChange.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.INotifyPropertyChange = function() {
    /// <summary>Implement this interface to become a provider of property change notifications.</summary>
    ##DEBUG throw Error.notImplemented();
}
Sys.INotifyPropertyChange.prototype = {
    #if DEBUG
    add_propertyChanged: function(handler) {
        throw Error.notImplemented();
    },
    remove_propertyChanged: function(handler) {
        throw Error.notImplemented();
    }
    #endif
}
Sys.INotifyPropertyChange.registerInterface('Sys.INotifyPropertyChange');
