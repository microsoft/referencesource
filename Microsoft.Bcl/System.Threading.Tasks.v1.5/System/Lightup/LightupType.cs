using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System
{
    internal static class LightupType
    {
        public static readonly Type ParameterizedThreadStart = GetExternallyVisibleType("System.Threading.ParameterizedThreadStart, mscorlib");
        public static readonly Type ExecutionContext = GetExternallyVisibleType("System.Threading.ExecutionContext, mscorlib");
        public static readonly Type ContextCallback = GetExternallyVisibleType("System.Threading.ContextCallback, mscorlib");
        public static readonly Type OperatingSystem = GetExternallyVisibleType("System.OperatingSystem, mscorlib");

        private static Type GetExternallyVisibleType(string typeName)
        {
            // Types such as ExecutionContext exist on Phone, but are not visible
            Type type = Type.GetType(typeName);
            if (type != null && type.IsVisible)
                return type;

            return null;
        }
    }
}
