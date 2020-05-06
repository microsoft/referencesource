#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="PropertyChangedEventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.PropertyChangedEventArgs = function(propertyName) {
    /// <summary>Describes property changes.</summary>
    /// <param name="propertyName" type="String">The name of the property that changed.</param>
    Sys.PropertyChangedEventArgs.initializeBase(this);
    this._propertyName = propertyName;
}
 Sys.PropertyChangedEventArgs.prototype = {
    get_propertyName: function() {
        /// <value type="String">The name of the property that changed.</value>
        return this._propertyName;
    }
}
Sys.PropertyChangedEventArgs.registerClass('Sys.PropertyChangedEventArgs', Sys.EventArgs);
