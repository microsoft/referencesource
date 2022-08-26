//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

using System.Reflection;
using System.Resources;

internal static class CommonResStrings
{
    private static ResourceManager resmgr = new ResourceManager("CommonResStrings", Assembly.GetExecutingAssembly());

    internal static string GetString(string id)
    {
        return resmgr.GetString(id);
    }
    
        
    internal static string CopyrightForCmdLine
    {
        get
        {
            return GetString("Microsoft_Copyright_CommandLine_Logo");
        }
    }
 
    internal static string WcfTrademarkForCmdLine
    {
        get
        {
            return GetString("WCF_Trademark_CommandLine_Logo");
        }
    }
}
