//---------------------------------------------------------------------
// <copyright file="DataServiceQueryProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Query Provider for Linq to URI translatation
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// QueryProvider implementation
    /// </summary>
    internal sealed class DataServiceQueryProvider : IQueryProvider
    {
        /// <summary>DataServiceContext for query provider</summary>
        internal readonly DataServiceContext Context;

        /// <summary>Constructs a query provider based on the context passed in </summary>
        /// <param name="context">The context for the query provider</param>
        internal DataServiceQueryProvider(DataServiceContext context)
        {
            this.Context = context;
        }

        #region IQueryProvider implementation

        /// <summary>Factory method for creating DataServiceOrderedQuery based on expression </summary>
        /// <param name="expression">The expression for the new query</param>
        /// <returns>new DataServiceQuery</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        public IQueryable CreateQuery(Expression expression)
        {
            Util.CheckArgumentNull(expression, "expression");
            Type et = TypeSystem.GetElementType(expression.Type);
            Type qt = typeof(DataServiceQuery<>.DataServiceOrderedQuery).MakeGenericType(et);
            object[] args = new object[] { expression, this };

            ConstructorInfo ci = qt.GetConstructor(
                BindingFlags.NonPublic | BindingFlags.Instance, 
                null, 
                new Type[] { typeof(Expression), typeof(DataServiceQueryProvider) }, 
                null);

            return (IQueryable)Util.ConstructorInvoke(ci, args);
        }

        /// <summary>Factory method for creating DataServiceOrderedQuery based on expression </summary>
        /// <typeparam name="TElement">generic type</typeparam>
        /// <param name="expression">The expression for the new query</param>
        /// <returns>new DataServiceQuery</returns>
        public IQueryable<TElement> CreateQuery<TElement>(Expression expression)
        {
            Util.CheckArgumentNull(expression, "expression");
            return new DataServiceQuery<TElement>.DataServiceOrderedQuery(expression, this);
        }

        /// <summary>Creates and executes a DataServiceQuery based on the passed in expression</summary>
        /// <param name="expression">The expression for the new query</param>
        /// <returns>the results</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1062:Validate arguments of public methods", MessageId = "0", Justification = "parameters are validated against null via CheckArgumentNull")]
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        public object Execute(Expression expression)
        {
            Util.CheckArgumentNull(expression, "expression");

            MethodInfo mi = typeof(DataServiceQueryProvider).GetMethod("ReturnSingleton", BindingFlags.NonPublic | BindingFlags.Instance);
            return mi.MakeGenericMethod(expression.Type).Invoke(this, new object[] { expression });
        }

        /// <summary>Creates and executes a DataServiceQuery based on the passed in expression</summary>
        /// <typeparam name="TResult">generic type</typeparam>
        /// <param name="expression">The expression for the new query</param>
        /// <returns>the results</returns>
        public TResult Execute<TResult>(Expression expression)
        {
            Util.CheckArgumentNull(expression, "expression");
            return ReturnSingleton<TResult>(expression);
        }

        #endregion

        /// <summary>Creates and executes a DataServiceQuery based on the passed in expression which results a single value</summary>
        /// <typeparam name="TElement">generic type</typeparam>
        /// <param name="expression">The expression for the new query</param>
        /// <returns>single valued results</returns>
        internal TElement ReturnSingleton<TElement>(Expression expression)
        {
            IQueryable<TElement> query = new DataServiceQuery<TElement>.DataServiceOrderedQuery(expression, this);

            MethodCallExpression mce = expression as MethodCallExpression;
            Debug.Assert(mce != null, "mce != null");

            SequenceMethod sequenceMethod;
            if (ReflectionUtil.TryIdentifySequenceMethod(mce.Method, out sequenceMethod))
            {
                switch (sequenceMethod)
                {
                    case SequenceMethod.Single:
                        return query.AsEnumerable().Single();
                    case SequenceMethod.SingleOrDefault:
                        return query.AsEnumerable().SingleOrDefault();
                    case SequenceMethod.First:
                        return query.AsEnumerable().First();
                    case SequenceMethod.FirstOrDefault:
                        return query.AsEnumerable().FirstOrDefault();
#if !ASTORIA_LIGHT
                    case SequenceMethod.LongCount:
                    case SequenceMethod.Count:
                        return (TElement)Convert.ChangeType(((DataServiceQuery<TElement>)query).GetQuerySetCount(this.Context), typeof(TElement), System.Globalization.CultureInfo.InvariantCulture.NumberFormat);
#endif
                    default:
                        throw Error.MethodNotSupported(mce);
                }
            }

            // Should never get here - should be caught by expression compiler.
            Debug.Assert(false, "Not supported singleton operator not caught by Resource Binder");
            throw Error.MethodNotSupported(mce);
        }

        /// <summary>Builds the Uri for the expression passed in.</summary>
        /// <param name="e">The expression to translate into a Uri</param>
        /// <returns>Query components</returns>
        internal QueryComponents Translate(Expression e)
        {
            Uri uri;
            Version version;
            bool addTrailingParens = false;
            Dictionary<Expression, Expression> normalizerRewrites = null;

            // short cut analysis if just a resource set
            // note - to be backwards compatible with V1, will only append trailing () for queries
            // that include more then just a resource set.
            if (!(e is ResourceSetExpression))
            {
                normalizerRewrites = new Dictionary<Expression, Expression>(ReferenceEqualityComparer<Expression>.Instance);
                e = Evaluator.PartialEval(e);
                e = ExpressionNormalizer.Normalize(e, normalizerRewrites);
                e = ResourceBinder.Bind(e);
                addTrailingParens = true;
            }

            UriWriter.Translate(this.Context, addTrailingParens, e, out uri, out version);
            ResourceExpression re = e as ResourceExpression;
            Type lastSegmentType = re.Projection == null ? re.ResourceType : re.Projection.Selector.Parameters[0].Type;
            LambdaExpression selector = re.Projection == null ? null : re.Projection.Selector;
            return new QueryComponents(uri, version, lastSegmentType, selector, normalizerRewrites); 
        }
    }
}
