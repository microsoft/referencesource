//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.IO;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;

    internal static class ToolConsole
    {
        static bool verbose = false;
        
        static public bool Verbose
        {
            set { verbose = value; }
        }
        
        internal static void WriteQueryLine(string str)
        {
            Console.WriteLine(str);
        }
        internal static void WriteLine(string str)
        {
            Console.WriteLine(str);
        }

        internal static void WriteError(Exception e)
        {
            WriteError(e, SR.GetString(SR.Error));
        }

        internal static void WriteWarning(string message)
        {
            if (verbose )
            {
                Console.Write(SR.GetString(SR.Warning));
                Console.WriteLine(message);
            }
        }

        internal static void WriteNonVerboseWarning(string message)
        {
            Console.Write(SR.GetString(SR.Warning));
            Console.WriteLine(message);
        }

        internal static void WriteError(string errMsg, string prefix)
        {
            Console.Error.Write(prefix);
            Console.Error.WriteLine(errMsg);
        }

        internal static void WriteError(Exception e, string prefix)
        {
            WriteError(e.Message, prefix);
            if (e.InnerException != null)
                WriteError(e.InnerException, "    ");
        }
        
        internal static void WriteDetailedException (Exception e, string prefix)
        {
            WriteError(e, prefix);
            if (e.InnerException != null)
                WriteError(e.InnerException, "    ");
        }
        
        
    }
}
