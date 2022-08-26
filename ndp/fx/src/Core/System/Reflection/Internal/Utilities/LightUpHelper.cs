// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;
using System.IO;

namespace System.Reflection.Internal
{
    internal static class LightUpHelper
    {
        internal static MethodInfo GetMethod(Type type, string name, params Type[] parameterTypes)
        {
            try
            {
                return type.GetRuntimeMethod(name, parameterTypes);
            }
            catch (AmbiguousMatchException)
            {
                // This is technically possible even when parameter types are passed
                // as the default binder allows for "widening conversions"
                // which can cause there to be more than one match. However, we
                // don't expect to hit this as the parameter types we pass are
                // specified to match known definitions precisely.

                Debug.Assert(false, "Current platform has ambiguous match for: " + type.FullName + "." + name);
                return null;
            }
        }
    }
}
