//---------------------------------------------------------------------
// <copyright file="Common.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//  Common defintions and functions for the server and client lib
// </summary>
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    using System.Linq;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// Common defintions and functions for the server and client lib
    /// </summary>
    internal static class CommonUtil
    {
        /// <summary>
        /// List of types unsupported by the client
        /// </summary>
        private static readonly Type[] unsupportedTypes = new Type[]
        {
#if !ASTORIA_LIGHT  // System.Dynamic & tuples not available (as of SL 2.0)
                typeof(System.Dynamic.IDynamicMetaObjectProvider),
                typeof(System.Tuple<>),         // 1-Tuple
                typeof(System.Tuple<,>),        // 2-Tuple
                typeof(System.Tuple<,,>),       // 3-Tuple
                typeof(System.Tuple<,,,>),      // 4-Tuple
                typeof(System.Tuple<,,,,>),     // 5-Tuple
                typeof(System.Tuple<,,,,,>),    // 6-Tuple
                typeof(System.Tuple<,,,,,,>),   // 7-Tuple
                typeof(System.Tuple<,,,,,,,>)   // 8-Tuple
#endif
        };

        /// <summary>
        /// Test whether a type is unsupported by the client lib
        /// </summary>
        /// <param name="type">The type to test</param>
        /// <returns>Returns true if the type is not supported</returns>
        internal static bool IsUnsupportedType(Type type)
        {
            if (type.IsGenericType)
            {
                type = type.GetGenericTypeDefinition();
            }

            if (unsupportedTypes.Any(t => t.IsAssignableFrom(type)))
            {
                return true;
            }

            Debug.Assert(!type.FullName.StartsWith("System.Tuple", StringComparison.Ordinal), "System.Tuple is not blocked by unsupported type check");
            return false;
        }

        /// <summary>
        /// Turn Uri instance into string representation
        /// This is needed because Uri.ToString unescapes the string
        /// </summary>
        /// <param name="uri">The uri instance</param>
        /// <returns>The string representation of the uri</returns>
        internal static string UriToString(Uri uri)
        {
            return uri.IsAbsoluteUri ? uri.AbsoluteUri : uri.OriginalString;
        }
    }
}
