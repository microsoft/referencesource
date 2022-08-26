//---------------------------------------------------------------------
// <copyright file="UriWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Translates resource bound expression trees into URIs.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Translates resource bound expression trees into URIs.
    /// </summary>
    internal class UriWriter : DataServiceALinqExpressionVisitor
    {
        /// <summary>Data context used to generate type names for types.</summary>
        private readonly DataServiceContext context;

        /// <summary>stringbuilder for constructed URI</summary>
        private readonly StringBuilder uriBuilder;
        
        /// <summary>the data service version for the uri</summary>
        private Version uriVersion;

        /// <summary>the leaf resourceset for the URI being written</summary>
        private ResourceSetExpression leafResourceSet;
        
        /// <summary>
        /// Private constructor for creating UriWriter
        /// </summary>
        /// <param name='context'>Data context used to generate type names for types.</param>
        private UriWriter(DataServiceContext context)
        {
            Debug.Assert(context != null, "context != null");
            this.context = context;
            this.uriBuilder = new StringBuilder();
            this.uriVersion = Util.DataServiceVersion1;
        }

        /// <summary>
        /// Translates resource bound expression tree to a URI.
        /// </summary>
        /// <param name='context'>Data context used to generate type names for types.</param>
        /// <param name="addTrailingParens">flag to indicate whether generated URI should include () if leaf is ResourceSet</param>
        /// <param name="e">The expression to translate</param>
        /// <param name="uri">uri</param>
        /// <param name="version">version for query</param>    
        internal static void Translate(DataServiceContext context, bool addTrailingParens, Expression e, out Uri uri, out Version version)
        {
            var writer = new UriWriter(context);
            writer.leafResourceSet = addTrailingParens ? (e as ResourceSetExpression) : null;
            writer.Visit(e);
            uri = Util.CreateUri(context.BaseUriWithSlash, Util.CreateUri(writer.uriBuilder.ToString(), UriKind.Relative));
            version = writer.uriVersion;
        }

        /// <summary>
        /// MethodCallExpression visit method
        /// </summary>
        /// <param name="m">The MethodCallExpression expression to visit</param>
        /// <returns>The visited MethodCallExpression expression </returns>
        internal override Expression VisitMethodCall(MethodCallExpression m)
        {
            throw Error.MethodNotSupported(m);
        }

        /// <summary>
        /// UnaryExpression visit method
        /// </summary>
        /// <param name="u">The UnaryExpression expression to visit</param>
        /// <returns>The visited UnaryExpression expression </returns>
        internal override Expression VisitUnary(UnaryExpression u)
        {
            throw new NotSupportedException(Strings.ALinq_UnaryNotSupported(u.NodeType.ToString()));
        }

        /// <summary>
        /// BinaryExpression visit method
        /// </summary>
        /// <param name="b">The BinaryExpression expression to visit</param>
        /// <returns>The visited BinaryExpression expression </returns>
        internal override Expression VisitBinary(BinaryExpression b)
        {
            throw new NotSupportedException(Strings.ALinq_BinaryNotSupported(b.NodeType.ToString()));
        }

        /// <summary>
        /// ConstantExpression visit method
        /// </summary>
        /// <param name="c">The ConstantExpression expression to visit</param>
        /// <returns>The visited ConstantExpression expression </returns>
        internal override Expression VisitConstant(ConstantExpression c)
        {
            throw new NotSupportedException(Strings.ALinq_ConstantNotSupported(c.Value));
        }

        /// <summary>
        /// TypeBinaryExpression visit method
        /// </summary>
        /// <param name="b">The TypeBinaryExpression expression to visit</param>
        /// <returns>The visited TypeBinaryExpression expression </returns>
        internal override Expression VisitTypeIs(TypeBinaryExpression b)
        {
            throw new NotSupportedException(Strings.ALinq_TypeBinaryNotSupported);
        }

        /// <summary>
        /// ConditionalExpression visit method
        /// </summary>
        /// <param name="c">The ConditionalExpression expression to visit</param>
        /// <returns>The visited ConditionalExpression expression </returns>
        internal override Expression VisitConditional(ConditionalExpression c)
        {
            throw new NotSupportedException(Strings.ALinq_ConditionalNotSupported);
        }

        /// <summary>
        /// ParameterExpression visit method
        /// </summary>
        /// <param name="p">The ParameterExpression expression to visit</param>
        /// <returns>The visited ParameterExpression expression </returns>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            throw new NotSupportedException(Strings.ALinq_ParameterNotSupported);
        }

        /// <summary>
        /// MemberExpression visit method
        /// </summary>
        /// <param name="m">The MemberExpression expression to visit</param>
        /// <returns>The visited MemberExpression expression </returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            throw new NotSupportedException(Strings.ALinq_MemberAccessNotSupported(m.Member.Name));
        }

        /// <summary>
        /// LambdaExpression visit method
        /// </summary>
        /// <param name="lambda">The LambdaExpression to visit</param>
        /// <returns>The visited LambdaExpression</returns>
        internal override Expression VisitLambda(LambdaExpression lambda)
        {
            throw new NotSupportedException(Strings.ALinq_LambdaNotSupported);
        }

        /// <summary>
        /// NewExpression visit method
        /// </summary>
        /// <param name="nex">The NewExpression to visit</param>
        /// <returns>The visited NewExpression</returns>
        internal override NewExpression VisitNew(NewExpression nex)
        {
            throw new NotSupportedException(Strings.ALinq_NewNotSupported);
        }

        /// <summary>
        /// MemberInitExpression visit method
        /// </summary>
        /// <param name="init">The MemberInitExpression to visit</param>
        /// <returns>The visited MemberInitExpression</returns>
        internal override Expression VisitMemberInit(MemberInitExpression init)
        {
            throw new NotSupportedException(Strings.ALinq_MemberInitNotSupported);
        }

        /// <summary>
        /// ListInitExpression visit method
        /// </summary>
        /// <param name="init">The ListInitExpression to visit</param>
        /// <returns>The visited ListInitExpression</returns>
        internal override Expression VisitListInit(ListInitExpression init)
        {
            throw new NotSupportedException(Strings.ALinq_ListInitNotSupported);
        }

        /// <summary>
        /// NewArrayExpression visit method
        /// </summary>
        /// <param name="na">The NewArrayExpression to visit</param>
        /// <returns>The visited NewArrayExpression</returns>
        internal override Expression VisitNewArray(NewArrayExpression na)
        {
            throw new NotSupportedException(Strings.ALinq_NewArrayNotSupported);
        }

        /// <summary>
        /// InvocationExpression visit method
        /// </summary>
        /// <param name="iv">The InvocationExpression to visit</param>
        /// <returns>The visited InvocationExpression</returns>
        internal override Expression VisitInvocation(InvocationExpression iv)
        {
            throw new NotSupportedException(Strings.ALinq_InvocationNotSupported);
        }

        /// <summary>
        /// NavigationPropertySingletonExpression visit method.
        /// </summary>
        /// <param name="npse">NavigationPropertySingletonExpression expression to visit</param>
        /// <returns>Visited NavigationPropertySingletonExpression expression</returns>
        internal override Expression VisitNavigationPropertySingletonExpression(NavigationPropertySingletonExpression npse)
        {
            this.Visit(npse.Source);
            this.uriBuilder.Append(UriHelper.FORWARDSLASH).Append(this.ExpressionToString(npse.MemberExpression));
            this.VisitQueryOptions(npse);
            return npse;
        }

        /// <summary>
        /// ResourceSetExpression visit method.
        /// </summary>
        /// <param name="rse">ResourceSetExpression expression to visit</param>
        /// <returns>Visited ResourceSetExpression expression</returns>
        internal override Expression VisitResourceSetExpression(ResourceSetExpression rse)
        {
            if ((ResourceExpressionType)rse.NodeType == ResourceExpressionType.ResourceNavigationProperty)
            {
                this.Visit(rse.Source);
                this.uriBuilder.Append(UriHelper.FORWARDSLASH).Append(this.ExpressionToString(rse.MemberExpression));
            }
            else
            {
                this.uriBuilder.Append(UriHelper.FORWARDSLASH).Append((string)((ConstantExpression)rse.MemberExpression).Value);
            }

            if (rse.KeyPredicate != null)
            {
                this.uriBuilder.Append(UriHelper.LEFTPAREN);
                if (rse.KeyPredicate.Count == 1)
                {
                    this.uriBuilder.Append(this.ExpressionToString(rse.KeyPredicate.Values.First()));
                }
                else
                {
                    bool addComma = false;
                    foreach (var kvp in rse.KeyPredicate)
                    {
                        if (addComma)
                        {
                            this.uriBuilder.Append(UriHelper.COMMA);
                        }

                        this.uriBuilder.Append(kvp.Key.Name);
                        this.uriBuilder.Append(UriHelper.EQUALSSIGN);
                        this.uriBuilder.Append(this.ExpressionToString(kvp.Value));
                        addComma = true;
                    }
                }

                this.uriBuilder.Append(UriHelper.RIGHTPAREN);
            }
            else if (rse == this.leafResourceSet)
            {
                // if resourceset is on the leaf, append ()
                this.uriBuilder.Append(UriHelper.LEFTPAREN);
                this.uriBuilder.Append(UriHelper.RIGHTPAREN);
            }

            if (rse.CountOption == CountOption.ValueOnly)
            {
                // append $count segment: /$count
                this.uriBuilder.Append(UriHelper.FORWARDSLASH).Append(UriHelper.DOLLARSIGN).Append(UriHelper.COUNT);
                this.EnsureMinimumVersion(2, 0);
            }

            this.VisitQueryOptions(rse);
            return rse;
        }

        /// <summary>
        /// Visit Query options for Resource
        /// </summary>
        /// <param name="re">Resource Expression with query options</param>
        internal void VisitQueryOptions(ResourceExpression re)
        {
            bool needAmpersand = false;

            if (re.HasQueryOptions)
            {
                this.uriBuilder.Append(UriHelper.QUESTIONMARK);

                ResourceSetExpression rse = re as ResourceSetExpression;
                if (rse != null)
                {
                    IEnumerator options = rse.SequenceQueryOptions.GetEnumerator();
                    while (options.MoveNext())
                    {
                        if (needAmpersand)
                        {
                            this.uriBuilder.Append(UriHelper.AMPERSAND);
                        }

                        Expression e = ((Expression)options.Current);
                        ResourceExpressionType et = (ResourceExpressionType)e.NodeType;
                        switch (et)
                        {
                            case ResourceExpressionType.SkipQueryOption:
                                this.VisitQueryOptionExpression((SkipQueryOptionExpression)e);
                                break;
                            case ResourceExpressionType.TakeQueryOption:
                                this.VisitQueryOptionExpression((TakeQueryOptionExpression)e);
                                break;
                            case ResourceExpressionType.OrderByQueryOption:
                                this.VisitQueryOptionExpression((OrderByQueryOptionExpression)e);
                                break;
                            case ResourceExpressionType.FilterQueryOption:
                                this.VisitQueryOptionExpression((FilterQueryOptionExpression)e);
                                break;
                            default:
                                Debug.Assert(false, "Unexpected expression type " + (int)et);
                                break;
                        }

                        needAmpersand = true;
                    }
                }

                if (re.ExpandPaths.Count > 0)
                {
                    if (needAmpersand)
                    {
                        this.uriBuilder.Append(UriHelper.AMPERSAND);
                    }

                    this.VisitExpandOptions(re.ExpandPaths);
                    needAmpersand = true;
                }

                if (re.Projection != null && re.Projection.Paths.Count > 0)
                {
                    if (needAmpersand)
                    {
                        this.uriBuilder.Append(UriHelper.AMPERSAND);
                    }

                    this.VisitProjectionPaths(re.Projection.Paths);
                    needAmpersand = true;
                }

                if (re.CountOption == CountOption.InlineAll)
                {
                    if (needAmpersand)
                    {
                        this.uriBuilder.Append(UriHelper.AMPERSAND);
                    }

                    this.VisitCountOptions();
                    needAmpersand = true;
                }

                if (re.CustomQueryOptions.Count > 0)
                {
                    if (needAmpersand)
                    {
                        this.uriBuilder.Append(UriHelper.AMPERSAND);
                    }

                    this.VisitCustomQueryOptions(re.CustomQueryOptions);
                    needAmpersand = true;
                }
            }
        }

        /// <summary>
        /// SkipQueryOptionExpression visit method.
        /// </summary>
        /// <param name="sqoe">SkipQueryOptionExpression expression to visit</param>
        internal void VisitQueryOptionExpression(SkipQueryOptionExpression sqoe)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONSKIP);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);
            this.uriBuilder.Append(this.ExpressionToString(sqoe.SkipAmount));
        }

        /// <summary>
        /// TakeQueryOptionExpression visit method.
        /// </summary>
        /// <param name="tqoe">TakeQueryOptionExpression expression to visit</param>
        internal void VisitQueryOptionExpression(TakeQueryOptionExpression tqoe)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONTOP);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);
            this.uriBuilder.Append(this.ExpressionToString(tqoe.TakeAmount));
        }

        /// <summary>
        /// FilterQueryOptionExpression visit method.
        /// </summary>
        /// <param name="fqoe">FilterQueryOptionExpression expression to visit</param>
        internal void VisitQueryOptionExpression(FilterQueryOptionExpression fqoe)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONFILTER);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);
            this.uriBuilder.Append(this.ExpressionToString(fqoe.Predicate));
        }

        /// <summary>
        /// OrderByQueryOptionExpression visit method.
        /// </summary>
        /// <param name="oboe">OrderByQueryOptionExpression expression to visit</param>
        internal void VisitQueryOptionExpression(OrderByQueryOptionExpression oboe)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONORDERBY);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);

            int ii = 0;
            while (true)
            {
                var selector = oboe.Selectors[ii];

                this.uriBuilder.Append(this.ExpressionToString(selector.Expression));
                if (selector.Descending)
                {
                    this.uriBuilder.Append(UriHelper.SPACE);
                    this.uriBuilder.Append(UriHelper.OPTIONDESC);
                }

                if (++ii == oboe.Selectors.Count)
                {
                    break;
                }

                this.uriBuilder.Append(UriHelper.COMMA);
            }
        }

        /// <summary>
        /// VisitExpandOptions visit method.
        /// </summary>
        /// <param name="paths">Expand Paths</param>
        internal void VisitExpandOptions(List<string> paths)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONEXPAND);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);

            int ii = 0;
            while (true)
            {
                this.uriBuilder.Append(paths[ii]);

                if (++ii == paths.Count)
                {
                    break;
                }

                this.uriBuilder.Append(UriHelper.COMMA);
            }
        }

        /// <summary>
        /// ProjectionPaths visit method.
        /// </summary>
        /// <param name="paths">Projection Paths</param>
        internal void VisitProjectionPaths(List<string> paths)
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONSELECT);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);

            int ii = 0;
            while (true)
            {
                string path = paths[ii];

                this.uriBuilder.Append(path);

                if (++ii == paths.Count)
                {
                    break;
                }

                this.uriBuilder.Append(UriHelper.COMMA);
            }

            this.EnsureMinimumVersion(2, 0);
        }

        /// <summary>
        /// VisitCountOptions visit method.
        /// </summary>
        internal void VisitCountOptions()
        {
            this.uriBuilder.Append(UriHelper.DOLLARSIGN);
            this.uriBuilder.Append(UriHelper.OPTIONCOUNT);
            this.uriBuilder.Append(UriHelper.EQUALSSIGN);
            this.uriBuilder.Append(UriHelper.COUNTALL);
            this.EnsureMinimumVersion(2, 0);
        }

        /// <summary>
        /// VisitCustomQueryOptions visit method.
        /// </summary>
        /// <param name="options">Custom query options</param>
        internal void VisitCustomQueryOptions(Dictionary<ConstantExpression, ConstantExpression> options)
        {
            List<ConstantExpression> keys = options.Keys.ToList();
            List<ConstantExpression> values = options.Values.ToList();

            int ii = 0;
            while (true)
            {
                this.uriBuilder.Append(keys[ii].Value);
                this.uriBuilder.Append(UriHelper.EQUALSSIGN);
                this.uriBuilder.Append(values[ii].Value);

                if (keys[ii].Value.ToString().Equals(UriHelper.DOLLARSIGN + UriHelper.OPTIONCOUNT, StringComparison.OrdinalIgnoreCase))
                {
                    this.EnsureMinimumVersion(2, 0);
                }

                if (++ii == keys.Count)
                {
                    break;
                }

                this.uriBuilder.Append(UriHelper.AMPERSAND);
            }
        }

        /// <summary>Serializes an expression to a string.</summary>
        /// <param name="expression">Expression to serialize</param>
        /// <returns>The serialized expression.</returns>
        private string ExpressionToString(Expression expression)
        {
            return ExpressionWriter.ExpressionToString(this.context, expression);
        }

        /// <summary>
        /// Ensure that the translated uri has the required minimum version associated with it
        /// </summary>
        /// <param name="major">The major number for the version. </param>
        /// <param name="minor">The minor number for the version. </param>
        private void EnsureMinimumVersion(int major, int minor)
        {
            if (major > this.uriVersion.Major ||
                (major == this.uriVersion.Major && minor > this.uriVersion.Minor))
            {
                this.uriVersion = new Version(major, minor);
            }
        }
    }
}
