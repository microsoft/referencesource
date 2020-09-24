#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="VisibilityMode.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.VisibilityMode = function() {
    /// <summary>
    ///   Describes how a DOM element should disappear when its visible property is set to false.
    /// </summary>
    /// <field name="hide" type="Number" integer="true" static="true">
    ///   The element disappears but its space remains
    /// </field>
    /// <field name="collapse" type="Number" integer="true" static="true">
    ///   The element disappears and the space it occupied is collapsed.
    /// </field>
    throw Error.notImplemented();
}
Sys.UI.VisibilityMode.prototype = {
    hide: 0,
    collapse: 1
}
Sys.UI.VisibilityMode.registerEnum("Sys.UI.VisibilityMode");

