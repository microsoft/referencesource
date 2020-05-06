#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="IDisposable.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.IDisposable = function() {
    ##DEBUG throw Error.notImplemented();
}
Sys.IDisposable.prototype = {
    #if DEBUG
    dispose: function() {
        throw Error.notImplemented();
    }
    #endif
}
Sys.IDisposable.registerInterface('Sys.IDisposable');
