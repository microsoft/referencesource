#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Boolean.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Boolean.__typeName = 'Boolean';
Boolean.__class = true;

Boolean.parse = function(value) {
    /// <summary>Creates a bool from its string representation.</summary>
    /// <param name="value" type="String">"true" or "false".</param>
    /// <returns type="Boolean"/>
    /// <validationOptions validateCount="false"/>
    var v = value.trim().toLowerCase();
    if (v === 'false') return false;
    if (v === 'true') return true;
    ##DEBUG throw Error.argumentOutOfRange('value', value, Sys.Res.boolTrueOrFalse);
}
