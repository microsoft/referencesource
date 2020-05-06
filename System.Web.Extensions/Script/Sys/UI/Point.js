#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Point.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.Point = function(x, y) {
    /// <param name="x" type="Number"/>
    /// <param name="y" type="Number"/>
    /// <field name="x" type="Number" integer="true"/>
    /// <field name="y" type="Number" integer="true"/>
    /// <field name="rawX" type="Number" />
    /// <field name="rawY" type="Number" />
    // DevDiv 398683: IE10 started returning floating points
    // Just round the inputs to protect against getting non integers
    this.rawX = x;
    this.rawY = y;
    this.x = Math.round(x);
    this.y = Math.round(y);
}
Sys.UI.Point.registerClass('Sys.UI.Point');
