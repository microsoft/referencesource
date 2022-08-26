// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: SecurityResources
** 
** <OWNER>Microsoft</OWNER>
**
** Author: Rudi Martin
**
** Purpose: Internal class providing access to resources in
**          System.Security.dll.
**
** Date: July 14, 2000
**
============================================================*/

namespace System.Security {

    using System;
    using System.Resources;

    internal static class SecurityResources
    {
        private static volatile ResourceManager s_resMgr;

        internal static String GetResourceString(String key)
        {
            if (s_resMgr == null)
                s_resMgr = new ResourceManager("system.security", typeof(SecurityResources).Assembly);
            return s_resMgr.GetString(key, null);
        }
    }

}
