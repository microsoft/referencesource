#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Bounds.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.Bounds = function(x, y, width, height) {
    /// <param name="x" type="Number" integer="true"/>
    /// <param name="y" type="Number" integer="true"/>
    /// <param name="width" type="Number" integer="true"/>
    /// <param name="height" type="Number" integer="true"/>
    /// <field name="x" type="Number" integer="true"/>
    /// <field name="y" type="Number" integer="true"/>
    /// <field name="width" type="Number" integer="true"/>
    /// <field name="height" type="Number" integer="true"/>
    this.x = x;
    this.y = y;
    this.height = height;
    this.width = width;
}
Sys.UI.Bounds.registerClass('Sys.UI.Bounds');
