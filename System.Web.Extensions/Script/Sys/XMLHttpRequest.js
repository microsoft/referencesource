#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="XMLHttpRequest.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
if (!window.XMLHttpRequest) {
    window.XMLHttpRequest = function() {
        // DevDiv Bugs 150054: Msxml2.XMLHTTP (version independent ProgID) required for mobile IE
        var progIDs = [ 'Msxml2.XMLHTTP.3.0', 'Msxml2.XMLHTTP' ];
        for (var i = 0, l = progIDs.length; i < l; i++) {
            try {
                return new ActiveXObject(progIDs[i]);
            }
            catch (ex) {
            }
        }
        return null;
    }
}
