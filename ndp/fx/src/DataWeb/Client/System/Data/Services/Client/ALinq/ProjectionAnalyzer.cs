//---------------------------------------------------------------------
// <copyright file="ProjectionAnalyzer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.ObjectModel;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Collections;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// Analyzes projection expressions to see if supported.
    /// To be writable, must follow these rules:
    /// 1)  Must be known Entity Type
    /// 2)  Must be a true narrowing of the source type.  Subset of properties + no transformations other then casts.
    /// 
    /// To be materializable (read-only), must follow these rules
    /// 1)  No transient object creation. (Entity and non-Entity types)
    /// 2)  No referencing of other DataService queries or contexts.
    /// </summary>
    internal static class ProjectionAnalyzer
    {
        #region Internal methods.

        /// <summary>
        /// Analyzes a lambda expression to check whether it can be satisfied with
        /// $select and client-side materialization.
        /// </summary>
        /// <param name="le">Lambda expression.</param>
        /// <param name="re">Resource expression in scope.</param>
        /// <param name="matchMembers">Whether member accesses are matched as top-level projections.</param>
        /// <returns>true if the lambda is a client-side projection; false otherwise.</returns>
        internal static bool Analyze(LambdaExpression le, ResourceExpression re, bool matchMembers)
        {
            Debug.Assert(le != null, "le != null");

            if (le.Body.NodeType == ExpressionType.Constant)
            {
                if (ClientType.CheckElementTypeIsEntity(le.Body.Type))
                {
                    throw new NotSupportedException(Strings.ALinq_CannotCreateConstantEntity);
                }

                re.Projection = new ProjectionQueryOptionExpression(le.Body.Type, le, new List<string>());
                return true;
            }

            if (le.Body.NodeType == ExpressionType.MemberInit || le.Body.NodeType == ExpressionType.New)
            {
                AnalyzeResourceExpression(le, re);
                return true;
            }

            if (matchMembers)
            {
                // Members can be projected standalone or type-casted.
                Expression withoutConverts = SkipConverts(le.Body);
                if (withoutConverts.NodeType == ExpressionType.MemberAccess)
                {
                    AnalyzeResourceExpression(le, re);
                    return true;
                }
            }

            return false;
        }

        internal static void Analyze(LambdaExpression e, PathBox pb)
        {
            bool knownEntityType = ClientType.CheckElementTypeIsEntity(e.Body.Type);
            pb.PushParamExpression(e.Parameters.Last());

            if (!knownEntityType)
            {
                NonEntityProjectionAnalyzer.Analyze(e.Body, pb);
            }
            else
            {
                switch (e.Body.NodeType)
                {
                    case ExpressionType.MemberInit:
                        EntityProjectionAnalyzer.Analyze((MemberInitExpression)e.Body, pb);
                        break;
                    case ExpressionType.New:
                        throw new NotSupportedException(Strings.ALinq_CannotConstructKnownEntityTypes);
                    case ExpressionType.Constant:
                        throw new NotSupportedException(Strings.ALinq_CannotCreateConstantEntity);
                    default:
                        // ExpressionType.MemberAccess as a top-level expression is correctly
                        // processed here, as the lambda isn't being member-initialized.
                        NonEntityProjectionAnalyzer.Analyze(e.Body, pb);
                        break;
                }
            }

            pb.PopParamExpression();
        }

        /// <summary>
        /// Checks whether the specified <see cref="MethodCallExpression"/> refers
        /// to a sequence method call allowed on entity types.
        /// </summary>
        /// <param name="call">Method call expression to check.</param>
        /// <returns>true if the method call is allowed; false otherwise.</returns>
        /// <remarks>The method won't check whether the call is made on actual entity types.</remarks>
        internal static bool IsMethodCallAllowedEntitySequence(MethodCallExpression call)
        {
            Debug.Assert(call != null, "call != null");
            return
                ReflectionUtil.IsSequenceMethod(call.Method, SequenceMethod.ToList) ||
                ReflectionUtil.IsSequenceMethod(call.Method, SequenceMethod.Select);
        }

        /// <summary>
        /// Checks whether the specified <see cref="MethodCallExpression"/> refers
        /// to a Select method call that works on the results of another Select call
        /// </summary>
        /// <param name="call">Method call expression to check.</param>
        /// <param name="type">Type of the projection</param>
        internal static void CheckChainedSequence(MethodCallExpression call, Type type)
        {
            if (ReflectionUtil.IsSequenceMethod(call.Method, SequenceMethod.Select))
            {
                // Chained Selects are not allowed
                // c.Orders.Select(...).Select(...)
                MethodCallExpression insideCall = ResourceBinder.StripTo<MethodCallExpression>(call.Arguments[0]);
                if (insideCall != null && ReflectionUtil.IsSequenceMethod(insideCall.Method, SequenceMethod.Select))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(type, call.ToString()));
                }
            }
        }

        /// <summary>
        /// Checks whether the specified expression creates a collection. 
        /// </summary>
        /// <param name="e">Expression to check.</param>
        /// <returns>true if given expression is collection producing.</returns>
        internal static bool IsCollectionProducingExpression(Expression e)
        {
            if (TypeSystem.FindIEnumerable(e.Type) != null)
            {
                Type elementType = TypeSystem.GetElementType(e.Type);
                Debug.Assert(elementType != null, "elementType == null");
                Type dscType = WebUtil.GetDataServiceCollectionOfT(elementType);
                if (typeof(List<>).MakeGenericType(elementType).IsAssignableFrom(e.Type) ||
                    (dscType != null && dscType.IsAssignableFrom(e.Type)))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Checks whether the specified expression is allowed in a MethodCall. Expressions that
        /// produce collections are not allowed. The only exception is when collection property 
        /// belongs to an entity e.g. c.Orders.Select(o => o), where we allow c.Orders.
        /// </summary>
        /// <param name="e">Expression to check.</param>
        /// <returns>true if expression is disallowed, false otherwise.</returns>
        internal static bool IsDisallowedExpressionForMethodCall(Expression e)
        {
            // If this is a collection hanging off a Entity, then that is fine.
            MemberExpression me = e as MemberExpression;
            if (me != null && ClientType.Create(me.Expression.Type, false).IsEntityType)
            {
                return false;
            }

            // All collection producing expressions are disallowed.
            return IsCollectionProducingExpression(e);
        }


        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Analyzes the specified expression with an entity-projection or 
        /// non-entity-projection analyzer.
        /// </summary>
        /// <param name="mie">Expression to analyze.</param>
        /// <param name="pb">Path box where select and expand paths are tracked.</param>
        private static void Analyze(MemberInitExpression mie, PathBox pb)
        {
            Debug.Assert(mie != null, "mie != null");
            Debug.Assert(pb != null, "pb != null");

            bool knownEntityType = ClientType.CheckElementTypeIsEntity(mie.Type);
            if (knownEntityType)
            {
                EntityProjectionAnalyzer.Analyze(mie, pb);
            }
            else
            {
                NonEntityProjectionAnalyzer.Analyze(mie, pb);
            }
        }

        /// <summary>
        /// Analyzes the specified <paramref name="lambda"/> for selection and updates 
        /// <paramref name="resource"/>.
        /// </summary>
        /// <param name="lambda">Lambda expression to analyze.</param>
        /// <param name="resource">Resource expression to update.</param>
        private static void AnalyzeResourceExpression(LambdaExpression lambda, ResourceExpression resource)
        {
            PathBox pb = new PathBox();
            ProjectionAnalyzer.Analyze(lambda, pb);
            resource.Projection = new ProjectionQueryOptionExpression(lambda.Body.Type, lambda, pb.ProjectionPaths.ToList());
            resource.ExpandPaths = pb.ExpandPaths.Union(resource.ExpandPaths, StringComparer.Ordinal).ToList();
        }

        /// <summary>Skips converts and returns the underlying expression.</summary>
        /// <param name="expression">Expression to dig into.</param>
        /// <returns>The original expression without converts.</returns>
        /// <remarks>
        /// IMPORTANT: This is fine for checks on underlying expressions where we
        /// want converts to be "mostly" transparent, but using the result in
        /// place of the given <paramref name="expression"/> loses information.
        /// </remarks>
        private static Expression SkipConverts(Expression expression)
        {
            Expression result = expression;
            while (result.NodeType == ExpressionType.Convert || result.NodeType == ExpressionType.ConvertChecked)
            {
                result = ((UnaryExpression)result).Operand;
            }

            return result;
        }

        #endregion Private methods.

        #region Inner types.

        private class EntityProjectionAnalyzer : ALinqExpressionVisitor
        {
            #region Private fields.

            /// <summary>Path-tracking object.</summary>
            private readonly PathBox box;

            /// <summary>Type being member-init'ed.</summary>
            private readonly Type type;

            #endregion Private fields.

            /// <summary>Initializes a new <see cref="EntityProjectionAnalyzer"/> instance.</summary>
            /// <param name="pb">Path-tracking object.</param>
            /// <param name="type">Type being member-init'ed.</param>
            private EntityProjectionAnalyzer(PathBox pb, Type type)
            {
                Debug.Assert(pb != null, "pb != null");
                Debug.Assert(type != null, "type != null");
                
                this.box = pb;
                this.type = type;
            }

            /// <summary>Analyzes the specified member-init expression.</summary>
            /// <param name="mie">Expression to analyze.</param>
            /// <param name="pb">Path-tracking object to store analysis in.</param>
            internal static void Analyze(MemberInitExpression mie, PathBox pb)
            {
                Debug.Assert(mie != null, "mie != null");

                var epa = new EntityProjectionAnalyzer(pb, mie.Type);

                MemberAssignmentAnalysis targetEntityPath = null;
                foreach (MemberBinding mb in mie.Bindings)
                {
                    MemberAssignment ma = mb as MemberAssignment;
                    epa.Visit(ma.Expression);
                    if (ma != null)
                    {
                        var analysis = MemberAssignmentAnalysis.Analyze(pb.ParamExpressionInScope, ma.Expression);
                        if (analysis.IncompatibleAssignmentsException != null)
                        {
                            throw analysis.IncompatibleAssignmentsException;
                        }

                        // Note that an "empty" assignment on the binding is not checked/handled,
                        // because the funcletizer would have turned that into a constant
                        // in the tree, the visit earlier in this method would have thrown
                        // an exception at finding a constant in an entity projection.
                        //
                        // We do account however for failing to find a reference off the
                        // parameter entry to detect errors like this: new ET() { Ref = e }
                        // Here it looks like the new ET should be the parent of 'e', but
                        // there is nothing in scope that represents that.
                        //
                        // This also explains while error messages might be a bit misleading
                        // in this case (because they reference a constant when the user
                        // hasn't included any).
                        Type targetType = GetMemberType(ma.Member);
                        Expression[] lastExpressions = analysis.GetExpressionsBeyondTargetEntity();
                        if (lastExpressions.Length == 0)
                        {
                            throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(targetType, ma.Expression));
                        }

                        MemberExpression lastExpression = lastExpressions[lastExpressions.Length - 1] as MemberExpression;
                        Debug.Assert(
                            !analysis.MultiplePathsFound, 
                            "!analysis.MultiplePathsFound -- the initilizer has been visited, and cannot be empty, and expressions that can combine paths should have thrown exception during initializer analysis");
                        Debug.Assert(
                            lastExpression != null,
                            "lastExpression != null -- the initilizer has been visited, and cannot be empty, and the only expressions that are allowed can be formed off the parameter, so this is always correlatd");
                        if (lastExpression != null && (lastExpression.Member.Name != ma.Member.Name))
                        {
                            throw new NotSupportedException(Strings.ALinq_PropertyNamesMustMatchInProjections(lastExpression.Member.Name, ma.Member.Name));
                        }

                        analysis.CheckCompatibleAssignments(mie.Type, ref targetEntityPath);

                        // Unless we're initializing an entity, we should not traverse into the parameter in scope.
                        bool targetIsEntity = ClientType.CheckElementTypeIsEntity(targetType);
                        bool sourceIsEntity = ClientType.CheckElementTypeIsEntity(lastExpression.Type);
                        if (sourceIsEntity && !targetIsEntity)
                        {
                            throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(targetType, ma.Expression));
                        }
                    }
                }
            }

            internal override Expression VisitUnary(UnaryExpression u)
            {
                Debug.Assert(u != null, "u != null");

                // Perfectly assignable conversions are OK. VB.NET compilers 
                // inserts these to exactly match method signatures, for example.
                if (ResourceBinder.PatternRules.MatchConvertToAssignable(u))
                {
                    return base.VisitUnary(u);
                }

                if ((u.NodeType == ExpressionType.Convert) || (u.NodeType == ExpressionType.ConvertChecked))
                {
                    Type sourceType = Nullable.GetUnderlyingType(u.Operand.Type) ?? u.Operand.Type;
                    Type targetType = Nullable.GetUnderlyingType(u.Type) ?? u.Type;

                    // when projecting known entity types, will allow convert expressions of primitive types.
                    if (ClientConvert.IsKnownType(sourceType) && ClientConvert.IsKnownType(targetType))
                    {
                        return base.Visit(u.Operand);
                    }
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, u.ToString()));
            }

            internal override Expression VisitBinary(BinaryExpression b)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, b.ToString()));
            }

            internal override Expression VisitTypeIs(TypeBinaryExpression b)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, b.ToString()));
            }

            internal override Expression VisitConditional(ConditionalExpression c)
            {
                var nullCheck = ResourceBinder.PatternRules.MatchNullCheck(this.box.ParamExpressionInScope, c);
                if (nullCheck.Match)
                {
                    this.Visit(nullCheck.AssignExpression);
                    return c;
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, c.ToString()));
            }

            internal override Expression VisitConstant(ConstantExpression c)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, c.ToString()));
            }

            internal override Expression VisitMemberAccess(MemberExpression m)
            {
                Debug.Assert(m != null, "m != null");

                // Only allowed to project entities
                if (!ClientType.CheckElementTypeIsEntity(m.Expression.Type) || IsCollectionProducingExpression(m.Expression))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, m.ToString()));
                }

                PropertyInfo pi = null;
                if (ResourceBinder.PatternRules.MatchNonPrivateReadableProperty(m, out pi))
                {
                    Expression e = base.VisitMemberAccess(m);
                    box.AppendToPath(pi);
                    return e;
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, m.ToString()));
            }

            internal override Expression VisitMethodCall(MethodCallExpression m)
            {
                if ((m.Object != null && IsDisallowedExpressionForMethodCall(m.Object))
                    || m.Arguments.Any(a => IsDisallowedExpressionForMethodCall(a)))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, m.ToString()));
                }

                if (ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(m))
                {
                    CheckChainedSequence(m, this.type);

                    // allow selects for following pattern:
                    // Orders = c.Orders.Select(o=> new NarrowOrder {...}).ToList();
                    return base.VisitMethodCall(m);
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, m.ToString()));
            }

            internal override Expression VisitInvocation(InvocationExpression iv)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, iv.ToString()));
            }

            internal override Expression VisitLambda(LambdaExpression lambda)
            {
                ProjectionAnalyzer.Analyze(lambda, this.box);
                return lambda;
            }

            internal override Expression VisitListInit(ListInitExpression init)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, init.ToString()));
            }

            internal override Expression VisitNewArray(NewArrayExpression na)
            {
                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, na.ToString()));
            }

            internal override Expression VisitMemberInit(MemberInitExpression init)
            {
                ProjectionAnalyzer.Analyze(init, this.box);
                return init;
            }

            internal override NewExpression VisitNew(NewExpression nex)
            {
                // Allow creation of DataServiceCollection<T> objects in projections
                if (ResourceBinder.PatternRules.MatchNewDataServiceCollectionOfT(nex))
                {
                    // It doesn't matter if the DSC is being tracked or not, that has no direct effect on the projections
                    // But it does matter if the T in DSC<T> is an entity or not. In here we only allow entity types to be used
                    //   for creation of DSC.
                    if (ClientType.CheckElementTypeIsEntity(nex.Type))
                    {
                        foreach (Expression e in nex.Arguments)
                        {
                            // no need to check the constant values here (DataServiceContext, funcs, etc).
                            if (e.NodeType != ExpressionType.Constant)
                            {
                                base.Visit(e);
                            }
                        }

                        return nex;
                    }
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjectionToEntity(this.type, nex.ToString()));
            }

            internal override Expression VisitParameter(ParameterExpression p)
            {
                if (p != box.ParamExpressionInScope)
                {
                    throw new NotSupportedException(Strings.ALinq_CanOnlyProjectTheLeaf);
                }
                
                this.box.StartNewPath();
                return p;
            }

            /// <summary>Gets the type of the specified <paramref name="member"/>.</summary>
            /// <param name="member">Member to get type of (typically PropertyInfo or FieldInfo).</param>
            /// <returns>The type of property or field type.</returns>
            private static Type GetMemberType(MemberInfo member)
            {
                Debug.Assert(member != null, "member != null");

                PropertyInfo propertyInfo = member as PropertyInfo;
                if (propertyInfo != null)
                {
                    return propertyInfo.PropertyType;
                }

                FieldInfo fieldInfo = member as FieldInfo;
                Debug.Assert(fieldInfo != null, "fieldInfo != null -- otherwise Expression.Member factory should have thrown an argument exception");
                return fieldInfo.FieldType;
            }
        }

        private class NonEntityProjectionAnalyzer : DataServiceALinqExpressionVisitor
        {
            private PathBox box;

            private Type type;

            private NonEntityProjectionAnalyzer(PathBox pb, Type type)
            {
                this.box = pb;
                this.type = type;
            }

            internal static void Analyze(Expression e, PathBox pb)
            {
                var nepa = new NonEntityProjectionAnalyzer(pb, e.Type);

                MemberInitExpression mie = e as MemberInitExpression;

                if (mie != null)
                {
                    foreach (MemberBinding mb in mie.Bindings)
                    {
                        MemberAssignment ma = mb as MemberAssignment;
                        if (ma != null)
                        {
                            nepa.Visit(ma.Expression);
                        }
                    }
                }
                else
                {
                    nepa.Visit(e);
                }
            }

            /// <summary>Visits a unary expression while initializing a non-entity type structure.</summary>
            /// <param name="u">Expression to visit.</param>
            /// <returns>The visited expression.</returns>
            internal override Expression VisitUnary(UnaryExpression u)
            {
                Debug.Assert(u != null, "u != null");

                if (!ResourceBinder.PatternRules.MatchConvertToAssignable(u))
                {
                    if (ClientType.CheckElementTypeIsEntity(u.Operand.Type))
                    {
                        throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, u.ToString()));
                    }
                }
                
                return base.VisitUnary(u);
            }

            internal override Expression VisitBinary(BinaryExpression b)
            {
                if (ClientType.CheckElementTypeIsEntity(b.Left.Type) || ClientType.CheckElementTypeIsEntity(b.Right.Type)
                    || IsCollectionProducingExpression(b.Left) || IsCollectionProducingExpression(b.Right))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, b.ToString()));
                }
                
                return base.VisitBinary(b);
            }

            internal override Expression VisitTypeIs(TypeBinaryExpression b)
            {
                if (ClientType.CheckElementTypeIsEntity(b.Expression.Type) || IsCollectionProducingExpression(b.Expression))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, b.ToString()));
                }
                
                return base.VisitTypeIs(b);
            }

            internal override Expression VisitConditional(ConditionalExpression c)
            {
                var nullCheck = ResourceBinder.PatternRules.MatchNullCheck(this.box.ParamExpressionInScope, c);
                if (nullCheck.Match)
                {
                    this.Visit(nullCheck.AssignExpression);
                    return c;
                }

                if (ClientType.CheckElementTypeIsEntity(c.Test.Type) || ClientType.CheckElementTypeIsEntity(c.IfTrue.Type) || ClientType.CheckElementTypeIsEntity(c.IfFalse.Type)
                    || IsCollectionProducingExpression(c.Test) || IsCollectionProducingExpression(c.IfTrue) || IsCollectionProducingExpression(c.IfFalse))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, c.ToString()));
                }
                
                return base.VisitConditional(c);
            }

            /// <summary>
            /// Visits a member access expression in non-entity projections, validating that
            /// it's correct and recording the path visit to include in a projection if necessary.
            /// </summary>
            /// <param name="m">Expression to visit.</param>
            /// <returns>The same expression.</returns>
            /// <remarks>
            /// The projection analyzer runs after funcletization, so a member expression
            /// rather than a constant expression implies that this is correlated to
            /// a parameter, by dotting through the argument in valid cases, and possibly
            /// more complex cases in others like new DSC(p.Orders)*.Foo* &lt;- .Foo is invalid.
            /// </remarks>
            internal override Expression VisitMemberAccess(MemberExpression m)
            {
                Debug.Assert(m != null, "m != null");

                // if primitive or nullable primitive, allow member access... i.e. calling Value on nullable<int>
                if (ClientConvert.IsKnownNullableType(m.Expression.Type))
                {
                    return base.VisitMemberAccess(m);
                }

                // Only allowed to project entities
                if (!ClientType.CheckElementTypeIsEntity(m.Expression.Type) || IsCollectionProducingExpression(m.Expression))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, m.ToString()));
                }

                PropertyInfo pi = null;
                if (ResourceBinder.PatternRules.MatchNonPrivateReadableProperty(m, out pi))
                {
                    Expression e = base.VisitMemberAccess(m);
                    box.AppendToPath(pi);
                    return e;
                }

                throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, m.ToString()));
            }

            internal override Expression VisitMethodCall(MethodCallExpression m)
            {
                if ((m.Object != null && IsDisallowedExpressionForMethodCall(m.Object))
                    || m.Arguments.Any(a => IsDisallowedExpressionForMethodCall(a)))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, m.ToString()));
                }

                if (ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(m))
                {
                    CheckChainedSequence(m, this.type);

                    // allow IEnum.Select and IEnum.ToList even if entity type.
                    return base.VisitMethodCall(m);
                }

                if ((m.Object != null ? ClientType.CheckElementTypeIsEntity(m.Object.Type) : false) 
                    || m.Arguments.Any(a => ClientType.CheckElementTypeIsEntity(a.Type)))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, m.ToString()));
                }

                return base.VisitMethodCall(m);
            }

            internal override Expression VisitInvocation(InvocationExpression iv)
            {
                if (ClientType.CheckElementTypeIsEntity(iv.Expression.Type) || IsCollectionProducingExpression(iv.Expression) 
                    || iv.Arguments.Any(a => ClientType.CheckElementTypeIsEntity(a.Type) || IsCollectionProducingExpression(a)))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, iv.ToString()));
                }

                return base.VisitInvocation(iv);
            }

            internal override Expression VisitLambda(LambdaExpression lambda)
            {
                ProjectionAnalyzer.Analyze(lambda, this.box);
                return lambda;
            }

            internal override Expression VisitMemberInit(MemberInitExpression init)
            {
                ProjectionAnalyzer.Analyze(init, this.box);
                return init;
            }

            internal override NewExpression VisitNew(NewExpression nex)
            {
                // Allow creation of DataServiceCollection<T> objects in projections, stop others that project entities
                if (ClientType.CheckElementTypeIsEntity(nex.Type) &&
                    !ResourceBinder.PatternRules.MatchNewDataServiceCollectionOfT(nex))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, nex.ToString()));
                }

                return base.VisitNew(nex);
            }

            internal override Expression VisitParameter(ParameterExpression p)
            {
                if (p != box.ParamExpressionInScope)
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, p.ToString()));
                }

                this.box.StartNewPath();
                return p;
            }

            internal override Expression VisitConstant(ConstantExpression c)
            {
                if (ClientType.CheckElementTypeIsEntity(c.Type))
                {
                    throw new NotSupportedException(Strings.ALinq_ExpressionNotSupportedInProjection(this.type, c.ToString()));
                }

                return base.VisitConstant(c);
            }
        }

        #endregion Inner types.
    }
}
