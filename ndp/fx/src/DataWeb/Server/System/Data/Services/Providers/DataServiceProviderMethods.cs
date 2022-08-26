//---------------------------------------------------------------------
// <copyright file="DataServiceProviderMethods.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      DataServiceProvider methods used for queries over IDSP providers.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Reflection;
    #endregion

    /// <summary>Use this class to perform late-bound operations on data service resource sets.</summary>    
    public static class DataServiceProviderMethods
    {
        #region Internal MethodInfos

        /// <summary>MethodInfo for object DataServiceProviderMethods.GetValue(this object value, string propertyName).</summary>
        internal static readonly MethodInfo GetValueMethodInfo = typeof(DataServiceProviderMethods).GetMethod(
            "GetValue",
            BindingFlags.Static | BindingFlags.Public,
            null,
            new Type[] { typeof(object), typeof(ResourceProperty) },
            null);

        /// <summary>MethodInfo for IEnumerable&lt;T&gt; DataServiceProviderMethods.GetSequenceValue(this object value, string propertyName).</summary>
        internal static readonly MethodInfo GetSequenceValueMethodInfo = typeof(DataServiceProviderMethods).GetMethod(
            "GetSequenceValue",
            BindingFlags.Static | BindingFlags.Public,
            null,
            new Type[] { typeof(object), typeof(ResourceProperty) },
            null);

        /// <summary>MethodInfo for Convert.</summary>
        internal static readonly MethodInfo ConvertMethodInfo = typeof(DataServiceProviderMethods).GetMethod(
            "Convert", 
            BindingFlags.Static | BindingFlags.Public);

        /// <summary>MethodInfo for TypeIs.</summary>
        internal static readonly MethodInfo TypeIsMethodInfo = typeof(DataServiceProviderMethods).GetMethod(
            "TypeIs", 
            BindingFlags.Static | BindingFlags.Public);

        #endregion

        #region GetValue, GetSequenceValue

        /// <summary>Gets a named value from the specified object.</summary>
        /// <param name='value'>Object to get value from.</param>
        /// <param name='property'>ResourceProperty describing the property whose value needs to be fetched.</param>
        /// <returns>The requested value.</returns>
        [Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", Justification = "Parameters will be used in the actual impl")]
        public static object GetValue(object value, ResourceProperty property)
        {
            throw new NotImplementedException();
        }

        /// <summary>Gets a named value from the specified object as a sequence.</summary>
        /// <param name='value'>Object to get value from.</param>
        /// <param name='property'>ResourceProperty describing the property whose value needs to be fetched.</param>
        /// <typeparam name="T">expected result type</typeparam>
        /// <returns>The requested value as a sequence; null if not found.</returns>
        [Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", Justification = "Parameters will be used in the actual impl")]
        [Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "need T for proper binding to collections")]
        public static IEnumerable<T> GetSequenceValue<T>(object value, ResourceProperty property)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region Type Conversions

        /// <summary>Performs an type cast on the specified value.</summary>
        /// <param name='value'>Value.</param>
        /// <param name='type'>Resource type to check for.</param>
        /// <returns>Casted value.</returns>
        [Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", Justification = "Parameters will be used in the actual impl")]
        public static object Convert(object value, ResourceType type)
        {
            throw new NotImplementedException();
        }

        /// <summary>Performs an type check on the specified value.</summary>
        /// <param name='value'>Value.</param>
        /// <param name='type'>Resource type to check for.</param>
        /// <returns>True if value is-a type; false otherwise.</returns>
        [Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", Justification = "Parameters will be used in the actual impl")]
        public static bool TypeIs(object value, ResourceType type)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region Type Comparers

        /// <summary>
        /// Compares 2 strings by ordinal, used to obtain MethodInfo for comparison operator expression parameter
        /// </summary>
        /// <param name="left">Left Parameter</param>
        /// <param name="right">Right Parameter</param>
        /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
        /// <remarks>
        /// Do not change the name of this function because LINQ to SQL is sensitive about the 
        /// method name, so is EF probably.
        /// </remarks>
        public static int Compare(String left, String right)
        {
            return Comparer<string>.Default.Compare(left, right);
        }

        /// <summary>
        /// Compares 2 booleans with true greater than false, used to obtain MethodInfo for comparison operator expression parameter
        /// </summary>
        /// <param name="left">Left Parameter</param>
        /// <param name="right">Right Parameter</param>
        /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
        /// <remarks>
        /// Do not change the name of this function because LINQ to SQL is sensitive about the 
        /// method name, so is EF probably.
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
        public static int Compare(bool left, bool right)
        {
            return Comparer<bool>.Default.Compare(left, right);
        }

        /// <summary>
        /// Compares 2 nullable booleans with true greater than false, used to obtain MethodInfo for comparison operator expression parameter
        /// </summary>
        /// <param name="left">Left Parameter</param>
        /// <param name="right">Right Parameter</param>
        /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
        /// <remarks>
        /// Do not change the name of this function because LINQ to SQL is sensitive about the 
        /// method name, so is EF probably.
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
        public static int Compare(bool? left, bool? right)
        {
            return Comparer<bool?>.Default.Compare(left, right);
        }

        /// <summary>
        /// Compares 2 guids by byte order, used to obtain MethodInfo for comparison operator expression parameter
        /// </summary>
        /// <param name="left">Left Parameter</param>
        /// <param name="right">Right Parameter</param>
        /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
        /// <remarks>
        /// Do not change the name of this function because LINQ to SQL is sensitive about the 
        /// method name, so is EF probably.
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
        public static int Compare(Guid left, Guid right)
        {
            return Comparer<Guid>.Default.Compare(left, right);
        }

        /// <summary>
        /// Compares 2 nullable guids by byte order, used to obtain MethodInfo for comparison operator expression parameter
        /// </summary>
        /// <param name="left">Left Parameter</param>
        /// <param name="right">Right Parameter</param>
        /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
        /// <remarks>
        /// Do not change the name of this function because LINQ to SQL is sensitive about the 
        /// method name, so is EF probably.
        /// </remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
        public static int Compare(Guid? left, Guid? right)
        {
            return Comparer<Guid?>.Default.Compare(left, right);
        }
        #endregion
    }
}
