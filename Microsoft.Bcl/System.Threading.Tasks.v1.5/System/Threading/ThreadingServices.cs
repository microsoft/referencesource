using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Threading
{
    internal static class ThreadingServices
    {
        public static Type ThreadAbortExceptionType = Type.GetType("System.Threading.ThreadAbortException");
        public static Type AppDomainUnloadedExceptionType = Type.GetType("System.AppDomainUnloadedException");

        public static bool IsThreadAbort(Exception ex)
        {
            return ex.GetType() == ThreadAbortExceptionType;
        }
    }
}
