#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Key.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.Key = function() {
    /// <summary>
    ///   Describes key codes.
    /// </summary>
    /// <field name="backspace" type="Number" integer="true" static="true"/>
    /// <field name="tab" type="Number" integer="true" static="true"/>
    /// <field name="enter" type="Number" integer="true" static="true"/>
    /// <field name="esc" type="Number" integer="true" static="true"/>
    /// <field name="space" type="Number" integer="true" static="true"/>
    /// <field name="pageUp" type="Number" integer="true" static="true"/>
    /// <field name="pageDown" type="Number" integer="true" static="true"/>
    /// <field name="end" type="Number" integer="true" static="true"/>
    /// <field name="home" type="Number" integer="true" static="true"/>
    /// <field name="left" type="Number" integer="true" static="true"/>
    /// <field name="up" type="Number" integer="true" static="true"/>
    /// <field name="right" type="Number" integer="true" static="true"/>
    /// <field name="down" type="Number" integer="true" static="true"/>
    /// <field name="del" type="Number" integer="true" static="true"/>
    throw Error.notImplemented();
}
Sys.UI.Key.prototype = {
    backspace: 8,
    tab: 9,
    enter: 13,
    esc: 27,
    space: 32,
    pageUp: 33,
    pageDown: 34,
    end: 35,
    home: 36,
    left: 37,
    up: 38,
    right: 39,
    down: 40,
    del: 127
}
Sys.UI.Key.registerEnum("Sys.UI.Key");
