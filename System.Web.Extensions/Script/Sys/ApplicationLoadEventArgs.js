#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="ApplicationLoadEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.ApplicationLoadEventArgs = function(components, isPartialLoad) {
    /// <param name="components" type="Array" elementType="Sys.Component">
    ///   The list of components that were created since the last time the load event was raised.
    /// </param>
    /// <param name="isPartialLoad" type="Boolean">True if the page is partially loading.</param>
    Sys.ApplicationLoadEventArgs.initializeBase(this);
    this._components = components;
    this._isPartialLoad = isPartialLoad;
}
 Sys.ApplicationLoadEventArgs.prototype = {
    get_components: function() {
        /// <value type="Array" elementType="Sys.Component">
        ///   The list of components that were created since the last time the load event was raised.
        /// </value>
        return this._components;
    },
    get_isPartialLoad: function() {
        /// <value type="Boolean">True if the page is partially loading.</value>
        return this._isPartialLoad;
    }
}
Sys.ApplicationLoadEventArgs.registerClass('Sys.ApplicationLoadEventArgs', Sys.EventArgs);
