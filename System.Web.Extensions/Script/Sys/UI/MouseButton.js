#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="MouseButton.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.MouseButton = function() {
    /// <summary>
    ///   Describes mouse buttons. The values are those from the DOM standard, which are different from the IE values.
    /// </summary>
    /// <field name="leftButton" type="Number" integer="true" static="true"/>
    /// <field name="middleButton" type="Number" integer="true" static="true"/>
    /// <field name="rightButton" type="Number" integer="true" static="true"/>
    throw Error.notImplemented();
}
Sys.UI.MouseButton.prototype = {
    leftButton: 0,
    middleButton: 1,
    rightButton: 2
}
Sys.UI.MouseButton.registerEnum("Sys.UI.MouseButton");
