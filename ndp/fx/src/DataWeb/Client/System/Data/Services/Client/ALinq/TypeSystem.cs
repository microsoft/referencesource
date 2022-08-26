//---------------------------------------------------------------------
// <copyright file="TypeSystem.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Utility functions for processing Expression trees
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;

    /// <summary>Utility functions for processing Expression trees</summary>
    internal static class TypeSystem
    {
        /// <summary> Method map for methods in URI query options</summary>
        private static readonly Dictionary<MethodInfo, string> expressionMethodMap;

        /// <summary> VB Method map for methods in URI query options</summary>
        private static readonly Dictionary<string, string> expressionVBMethodMap;

        /// <summary> Properties that should be represented as methods</summary>
        private static readonly Dictionary<PropertyInfo, MethodInfo> propertiesAsMethodsMap;

        /// <summary> VB Assembly name</summary>
#if !ASTORIA_LIGHT
        private const string VisualBasicAssemblyFullName = "Microsoft.VisualBasic, Version=10.0.0.0, Culture=neutral, PublicKeyToken=" + AssemblyRef.MicrosoftPublicKeyToken;
#else
        private const string VisualBasicAssemblyFullName = "Microsoft.VisualBasic, Version=2.0.5.0, Culture=neutral, PublicKeyToken=" + AssemblyRef.MicrosoftSilverlightPublicKeyToken;
#endif

        /// <summary>
        /// Initializes method map
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1810:InitializeReferenceTypeStaticFieldsInline", Justification = "Cleaner code")]
        static TypeSystem()
        {
            // string functions
#if !ASTORIA_LIGHT
            const int ExpectedCount = 24;
#else
            const int ExpectedCount = 22;
#endif
            expressionMethodMap = new Dictionary<MethodInfo, string>(ExpectedCount, EqualityComparer<MethodInfo>.Default);
            expressionMethodMap.Add(typeof(string).GetMethod("Contains", new Type[] { typeof(string) }), @"substringof");
            expressionMethodMap.Add(typeof(string).GetMethod("EndsWith", new Type[] { typeof(string) }), @"endswith");
            expressionMethodMap.Add(typeof(string).GetMethod("StartsWith", new Type[] { typeof(string) }), @"startswith");
            expressionMethodMap.Add(typeof(string).GetMethod("IndexOf", new Type[] { typeof(string) }), @"indexof");
            expressionMethodMap.Add(typeof(string).GetMethod("Replace", new Type[] { typeof(string), typeof(string) }), @"replace");
            expressionMethodMap.Add(typeof(string).GetMethod("Substring", new Type[] { typeof(int) }), @"substring");
            expressionMethodMap.Add(typeof(string).GetMethod("Substring", new Type[] { typeof(int), typeof(int) }), @"substring");
            expressionMethodMap.Add(typeof(string).GetMethod("ToLower", Type.EmptyTypes), @"tolower");
            expressionMethodMap.Add(typeof(string).GetMethod("ToUpper", Type.EmptyTypes), @"toupper");
            expressionMethodMap.Add(typeof(string).GetMethod("Trim", Type.EmptyTypes), @"trim");
            expressionMethodMap.Add(typeof(string).GetMethod("Concat", new Type[] { typeof(string), typeof(string) }, null), @"concat");   
            expressionMethodMap.Add(typeof(string).GetProperty("Length", typeof(int)).GetGetMethod(), @"length");

            // datetime methods
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Day", typeof(int)).GetGetMethod(), @"day");
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Hour", typeof(int)).GetGetMethod(), @"hour");
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Month", typeof(int)).GetGetMethod(), @"month");
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Minute", typeof(int)).GetGetMethod(), @"minute");
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Second", typeof(int)).GetGetMethod(), @"second");
            expressionMethodMap.Add(typeof(DateTime).GetProperty("Year", typeof(int)).GetGetMethod(), @"year");

            // math methods
            expressionMethodMap.Add(typeof(Math).GetMethod("Round", new Type[] { typeof(double) }), @"round");
            expressionMethodMap.Add(typeof(Math).GetMethod("Round", new Type[] { typeof(decimal) }), @"round");
            expressionMethodMap.Add(typeof(Math).GetMethod("Floor", new Type[] { typeof(double) }), @"floor");
#if !ASTORIA_LIGHT // Math.Floor(Decimal) not available
            expressionMethodMap.Add(typeof(Math).GetMethod("Floor", new Type[] { typeof(decimal) }), @"floor");
#endif
            expressionMethodMap.Add(typeof(Math).GetMethod("Ceiling", new Type[] { typeof(double) }), @"ceiling");
#if !ASTORIA_LIGHT // Math.Ceiling(Decimal) not available
            expressionMethodMap.Add(typeof(Math).GetMethod("Ceiling", new Type[] { typeof(decimal) }), @"ceiling");
#endif

            Debug.Assert(expressionMethodMap.Count == ExpectedCount, "expressionMethodMap.Count == ExpectedCount");

            // vb methods
            // lookup these by type name + method name
            expressionVBMethodMap = new Dictionary<string, string>(EqualityComparer<string>.Default);

            expressionVBMethodMap.Add("Microsoft.VisualBasic.Strings.Trim", @"trim");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.Strings.Len", @"length");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.Strings.Mid", @"substring");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.Strings.UCase", @"toupper");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.Strings.LCase", @"tolower");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Year", @"year");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Month", @"month");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Day", @"day");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Hour", @"hour");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Minute", @"minute");
            expressionVBMethodMap.Add("Microsoft.VisualBasic.DateAndTime.Second", @"second");

            Debug.Assert(expressionVBMethodMap.Count == 11, "expressionVBMethodMap.Count == 11");

            propertiesAsMethodsMap = new Dictionary<PropertyInfo, MethodInfo>(EqualityComparer<PropertyInfo>.Default);
            propertiesAsMethodsMap.Add(
                typeof(string).GetProperty("Length", typeof(int)), 
                typeof(string).GetProperty("Length", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Day", typeof(int)), 
                typeof(DateTime).GetProperty("Day", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Hour", typeof(int)), 
                typeof(DateTime).GetProperty("Hour", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Minute", typeof(int)), 
                typeof(DateTime).GetProperty("Minute", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Second", typeof(int)), 
                typeof(DateTime).GetProperty("Second", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Month", typeof(int)),
                typeof(DateTime).GetProperty("Month", typeof(int)).GetGetMethod());
            propertiesAsMethodsMap.Add(
                typeof(DateTime).GetProperty("Year", typeof(int)), 
                typeof(DateTime).GetProperty("Year", typeof(int)).GetGetMethod());

            Debug.Assert(propertiesAsMethodsMap.Count == 7, "propertiesAsMethodsMap.Count == 7");
        }

        /// <summary>
        /// Sees if method has URI equivalent
        /// </summary>
        /// <param name="mi">The method info</param>
        /// <param name="methodName">uri method name</param>
        /// <returns>true/ false</returns>
        internal static bool TryGetQueryOptionMethod(MethodInfo mi, out string methodName)
        {
            return (expressionMethodMap.TryGetValue(mi, out methodName) ||
                (mi.DeclaringType.Assembly.FullName == VisualBasicAssemblyFullName &&
                 expressionVBMethodMap.TryGetValue(mi.DeclaringType.FullName + "." + mi.Name, out methodName)));
        }

        /// <summary>
        /// Sees if property can be represented as method for translation to URI
        /// </summary>
        /// <param name="pi">The property info</param>
        /// <param name="mi">get method for property</param>
        /// <returns>true/ false</returns>
        internal static bool TryGetPropertyAsMethod(PropertyInfo pi, out MethodInfo mi)
        {
            return propertiesAsMethodsMap.TryGetValue(pi, out mi);
        }

        /// <summary>
        /// Gets the elementtype for a sequence
        /// </summary>
        /// <param name="seqType">The sequence type</param>
        /// <returns>The element type</returns>
        internal static Type GetElementType(Type seqType)
        {
            Type ienum = FindIEnumerable(seqType);
            if (ienum == null) 
            {
                return seqType;
            }

            return ienum.GetGenericArguments()[0];
        }

        /// <summary>
        /// Determines whether a property is private
        /// </summary>
        /// <param name="pi">The PropertyInfo structure for the property</param>
        /// <returns>true/ false if property is private</returns>
        internal static bool IsPrivate(PropertyInfo pi)
        {
            MethodInfo mi = pi.GetGetMethod() ?? pi.GetSetMethod();
            if (mi != null)
            {
                return mi.IsPrivate;
            }

            return true;
        }

        /// <summary>
        /// Finds type that implements IEnumerable so can get elemtent type
        /// </summary>
        /// <param name="seqType">The Type to check</param>
        /// <returns>returns the type which implements IEnumerable</returns>
        internal static Type FindIEnumerable(Type seqType)
        {
            if (seqType == null || seqType == typeof(string))
            {
                return null;
            }

            if (seqType.IsArray)
            {
                return typeof(IEnumerable<>).MakeGenericType(seqType.GetElementType());
            }

            if (seqType.IsGenericType)
            {
                foreach (Type arg in seqType.GetGenericArguments())
                {
                    Type ienum = typeof(IEnumerable<>).MakeGenericType(arg);
                    if (ienum.IsAssignableFrom(seqType))
                    {
                        return ienum;
                    }
                }
            }

            Type[] ifaces = seqType.GetInterfaces();
            if (ifaces != null && ifaces.Length > 0)
            {
                foreach (Type iface in ifaces)
                {
                    Type ienum = FindIEnumerable(iface);
                    if (ienum != null)
                    {
                        return ienum;
                    }
                }
            }

            if (seqType.BaseType != null && seqType.BaseType != typeof(object))
            {
                return FindIEnumerable(seqType.BaseType);
            }

            return null;
        }
    }
}
