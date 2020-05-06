#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="EventArgs.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.EventArgs = function() {
    /// <summary>EventArgs is the base class for classes containing event data.</summary>
}
Sys.EventArgs.registerClass('Sys.EventArgs');

Sys.EventArgs.Empty = new Sys.EventArgs();
