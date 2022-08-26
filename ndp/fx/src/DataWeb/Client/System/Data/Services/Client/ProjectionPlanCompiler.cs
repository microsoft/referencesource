//---------------------------------------------------------------------
// <copyright file="ProjectionPlanCompiler.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can create a materialization plan for a
// projection.
// </summary>
//---------------------------------------------------------------------

//// Uncomment the following line to trace projection building activity.
////#define TRACE_CLIENT_PROJECTIONS

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
    /// Use this class to create a <see cref="ProjectionPlan"/> for a given projection lambda.
    /// </summary>
    internal class ProjectionPlanCompiler : ALinqExpressionVisitor
    {
        #region Private fields.

        /// <summary>Annotations being tracked on this tree.</summary>
        private readonly Dictionary<Expression, ExpressionAnnotation> annotations;

        /// <summary>Expression that refers to the materializer.</summary>
        private readonly ParameterExpression materializerExpression;

        /// <summary>Tracks rewrite-to-source rewrites introduced by expression normalizer.</summary>
        private readonly Dictionary<Expression, Expression> normalizerRewrites;

        /// <summary>Number to suffix to identifiers to help with debugging.</summary>
        private int identifierId;

        /// <summary>Path builder used to help with tracking state while compiling.</summary>
        private ProjectionPathBuilder pathBuilder;

        /// <summary>Whether the top level projection has been found.</summary>
        private bool topLevelProjectionFound;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Initializes a new <see cref="ProjectionPlanCompiler"/> instance.
        /// </summary>
        /// <param name="normalizerRewrites">Rewrites introduces by normalizer.</param>
        private ProjectionPlanCompiler(Dictionary<Expression, Expression> normalizerRewrites)
        {
            this.annotations = new Dictionary<Expression, ExpressionAnnotation>(ReferenceEqualityComparer<Expression>.Instance);
            this.materializerExpression = Expression.Parameter(typeof(object), "mat");
            this.normalizerRewrites = normalizerRewrites;
            this.pathBuilder = new ProjectionPathBuilder();
        }

        #endregion Constructors.

        #region Internal methods.

        /// <summary>Creates a projection plan from the specified <paramref name="projection"/>.</summary>
        /// <param name="projection">Projection expression.</param>
        /// <param name="normalizerRewrites">Tracks rewrite-to-source rewrites introduced by expression normalizer.</param>
        /// <returns>A new <see cref="ProjectionPlan"/> instance.</returns>
        internal static ProjectionPlan CompilePlan(LambdaExpression projection, Dictionary<Expression, Expression> normalizerRewrites)
        {
            Debug.Assert(projection != null, "projection != null");
            Debug.Assert(projection.Parameters.Count == 1, "projection.Parameters.Count == 1");
            Debug.Assert(
                projection.Body.NodeType == ExpressionType.Constant ||
                projection.Body.NodeType == ExpressionType.MemberInit ||
                projection.Body.NodeType == ExpressionType.MemberAccess ||
                projection.Body.NodeType == ExpressionType.Convert ||
                projection.Body.NodeType == ExpressionType.ConvertChecked ||
                projection.Body.NodeType == ExpressionType.New,
                "projection.Body.NodeType == Constant, MemberInit, MemberAccess, Convert(Checked) New");

            ProjectionPlanCompiler rewriter = new ProjectionPlanCompiler(normalizerRewrites);
#if TRACE_CLIENT_PROJECTIONS
            Trace.WriteLine("Projection: " + projection);
#endif

            Expression plan = rewriter.Visit(projection);
#if TRACE_CLIENT_PROJECTIONS
            Trace.WriteLine("Becomes: " + plan);
#endif

            ProjectionPlan result = new ProjectionPlan();
            result.Plan = (Func<object, object, Type, object>)((LambdaExpression)plan).Compile();
            result.ProjectedType = projection.Body.Type;
#if DEBUG
            result.SourceProjection = projection;
            result.TargetProjection = plan;
#endif
            return result;
        }

        /// <summary>Binary visit method.</summary>
        /// <param name="b">Binary expression to visit.</param>
        /// <returns>(Possibly rewritten) binary expression.</returns>
        /// <remarks>
        /// This override is introduced because binary expressions are one of 
        /// the scopes at which normalization happens.
        /// </remarks>
        internal override Expression VisitBinary(BinaryExpression b)
        {
            Expression original = this.GetExpressionBeforeNormalization(b);
            if (original == b)
            {
                return base.VisitBinary(b);
            }
            else
            {
                return this.Visit(original);
            }
        }

        /// <summary>Visits the specified <paramref name="conditional"/> expression.</summary>
        /// <param name="conditional">Expression to check.</param>
        /// <returns>The visited expression.</returns>
        /// <remarks>
        /// This override allows us to check for rewrites created by
        /// ExpressionNormalizer.CreateCompareExpression.
        /// </remarks>
        internal override Expression VisitConditional(ConditionalExpression conditional)
        {
            Debug.Assert(conditional != null, "conditional != null");
            Expression original = this.GetExpressionBeforeNormalization(conditional);
            if (original != conditional)
            {
                return this.Visit(original);
            }

            var nullCheck = ResourceBinder.PatternRules.MatchNullCheck(this.pathBuilder.LambdaParameterInScope, conditional);
            if (!nullCheck.Match || !ClientType.CheckElementTypeIsEntity(nullCheck.AssignExpression.Type))
            {
                return base.VisitConditional(conditional);
            }

            return this.RebindConditionalNullCheck(conditional, nullCheck);
        }

        /// <summary>Unary visit method.</summary>
        /// <param name="u">Unary expression to visit.</param>
        /// <returns>(Possibly rewritten) unary expression.</returns>
        /// <remarks>
        /// This override is introduced because unary expressions are one of 
        /// the scopes at which normalization happens.
        /// </remarks>
        internal override Expression VisitUnary(UnaryExpression u)
        {
            Expression original = this.GetExpressionBeforeNormalization(u);
            Expression result;
            if (original == u)
            {
                result = base.VisitUnary(u);
                UnaryExpression unaryResult = result as UnaryExpression;
                if (unaryResult != null)
                {
                    ExpressionAnnotation annotation;
                    if (this.annotations.TryGetValue(unaryResult.Operand, out annotation))
                    {
                        this.annotations[result] = annotation;
                    }
                }
            }
            else
            {
                result = this.Visit(original);
            }

            return result;
        }

        /// <summary>
        /// MemberExpression visit method
        /// </summary>
        /// <param name="m">The MemberExpression expression to visit</param>
        /// <returns>The visited MemberExpression expression </returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            Debug.Assert(m != null, "m != null");

            Expression result;
            Expression baseSourceExpression = m.Expression;

            // if primitive or nullable primitive, allow member access... i.e. calling Value on nullable<int>
            if (ClientConvert.IsKnownNullableType(baseSourceExpression.Type))
            {
                result = base.VisitMemberAccess(m);
            }
            else
            {
                Expression baseTargetExpression = this.Visit(baseSourceExpression);
                ExpressionAnnotation annotation;
                if (this.annotations.TryGetValue(baseTargetExpression, out annotation))
                {
                    result = this.RebindMemberAccess(m, annotation);
                }
                else
                {
                    result = Expression.MakeMemberAccess(baseTargetExpression, m.Member);
                }
            }

            return result;
        }

        /// <summary>Parameter visit method.</summary>
        /// <param name="p">Parameter to visit.</param>
        /// <returns>Resulting expression.</returns>
        /// <remarks>
        /// The parameter may get rewritten as a materializing projection if
        /// it refers to an entity outside of member binding. In this case,
        /// it becomes a standalone tracked entity.
        /// </remarks>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            Debug.Assert(p != null, "p != null");

            // If this parameter isn't interesting, we're not doing any rewrites.
            Expression result;
            ExpressionAnnotation annotation;
            if (this.annotations.TryGetValue(p, out annotation))
            {
                result = this.RebindParameter(p, annotation);
            }
            else
            {
                result = base.VisitParameter(p);
            }

            return result;
        }

        /// <summary>
        /// MemberInitExpression visit method
        /// </summary>
        /// <param name="init">The MemberInitExpression to visit</param>
        /// <returns>The visited MemberInitExpression</returns>
        /// <remarks>
        /// A MemberInitExpression on a knownEntityType implies that we
        /// want to materialize the thing.
        /// </remarks>
        internal override Expression VisitMemberInit(MemberInitExpression init)
        {
            this.pathBuilder.EnterMemberInit(init);
            
            Expression result = null;
            if (this.pathBuilder.CurrentIsEntity && init.Bindings.Count > 0)
            {
                result = this.RebindEntityMemberInit(init);
            }
            else
            {
                result = base.VisitMemberInit(init);
            }

            this.pathBuilder.LeaveMemberInit();
            return result;
        }

        /// <summary>Visits a method call expression.</summary>
        /// <param name="m">Expression to visit.</param>
        /// <returns>A (possibly rewritten) expression for <paramref name="m"/>.</returns>
        internal override Expression VisitMethodCall(MethodCallExpression m)
        {
            Debug.Assert(m != null, "m != null");

            Expression original = this.GetExpressionBeforeNormalization(m);
            if (original != m)
            {
                return this.Visit(original);
            }

            Expression result;
            if (this.pathBuilder.CurrentIsEntity)
            {
                Debug.Assert(
                    ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(m) || ResourceBinder.PatternRules.MatchReferenceEquals(m),
                    "ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(m) || ResourceBinder.PatternRules.MatchReferenceEquals(m) -- otherwise ProjectionAnalyzer should have blocked this for entities");
                if (m.Method.Name == "Select")
                {
                    result = this.RebindMethodCallForMemberSelect(m);
                }
                else if (m.Method.Name == "ToList")
                {
                    result = this.RebindMethodCallForMemberToList(m);
                }
                else
                {
                    Debug.Assert(m.Method.Name == "ReferenceEquals", "We don't know how to handle this method, ProjectionAnalyzer updated?");
                    result = base.VisitMethodCall(m);
                }
            }
            else
            {
                if (ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(m))
                {
                    result = this.RebindMethodCallForNewSequence(m);
                }
                else
                {
                    result = base.VisitMethodCall(m);
                }
            }

            return result;
        }

        /// <summary>Visits a new expression</summary>
        /// <param name="nex">Expression to visit</param>
        /// <returns>A (possibly rewritten) expression for <paramref name="nex"/>.</returns>
        internal override NewExpression VisitNew(NewExpression nex)
        {
            Debug.Assert(nex != null, "nex != null");

            // Special case DataServiceCollection creation so context instance
            // and paging (continuations) propperly flow through
            if (ResourceBinder.PatternRules.MatchNewDataServiceCollectionOfT(nex))
            {
                return this.RebindNewExpressionForDataServiceCollectionOfT(nex);
            }

            return base.VisitNew(nex);
        }

        /// <summary>LambdaExpression visit method.</summary>
        /// <param name="lambda">The LambdaExpression to visit</param>
        /// <returns>The visited LambdaExpression</returns>
        internal override Expression VisitLambda(LambdaExpression lambda)
        {
            Debug.Assert(lambda != null, "lambda != null");

            Expression result;
            if (!this.topLevelProjectionFound || lambda.Parameters.Count == 1 && ClientType.CheckElementTypeIsEntity(lambda.Parameters[0].Type))
            {
                this.topLevelProjectionFound = true;

                ParameterExpression expectedTypeParameter = Expression.Parameter(typeof(Type), "type" + this.identifierId);
                ParameterExpression entryParameter = Expression.Parameter(typeof(object), "entry" + this.identifierId);
                this.identifierId++;

                this.pathBuilder.EnterLambdaScope(lambda, entryParameter, expectedTypeParameter);
                ProjectionPath parameterPath = new ProjectionPath(lambda.Parameters[0], expectedTypeParameter, entryParameter);
                ProjectionPathSegment parameterSegment = new ProjectionPathSegment(parameterPath, null, null);
                parameterPath.Add(parameterSegment);
                this.annotations[lambda.Parameters[0]] = new ExpressionAnnotation() { Segment = parameterSegment };

                Expression body = this.Visit(lambda.Body);

                // Value types must be boxed explicitly; the lambda initialization
                // won't do it for us (type-compatible types still work, so all
                // references will work fine with System.Object).
                if (body.Type.IsValueType)
                {
                    body = Expression.Convert(body, typeof(object));
                }

                result = Expression.Lambda<Func<object, object, Type, object>>(
                    body,
                    this.materializerExpression,
                    entryParameter,
                    expectedTypeParameter);

                this.pathBuilder.LeaveLambdaScope();
            }
            else
            {
                result = base.VisitLambda(lambda);
            }

            return result;
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>Generates a call to a static method on AtomMaterializer.</summary>
        /// <param name="methodName">Name of method to invoke.</param>
        /// <param name="arguments">Arguments to pass to method.</param>
        /// <returns>The constructed expression.</returns>
        /// <remarks>
        /// There is no support for overload resolution - method names in AtomMaterializer
        /// must be unique.
        /// </remarks>
        private static Expression CallMaterializer(string methodName, params Expression[] arguments)
        {
            return CallMaterializerWithType(methodName, null, arguments);
        }

        /// <summary>Generates a call to a static method on AtomMaterializer.</summary>
        /// <param name="methodName">Name of method to invoke.</param>
        /// <param name="typeArguments">Type arguments for method (possibly null).</param>
        /// <param name="arguments">Arguments to pass to method.</param>
        /// <returns>The constructed expression.</returns>
        /// <remarks>
        /// There is no support for overload resolution - method names in AtomMaterializer
        /// must be unique.
        /// </remarks>
        private static Expression CallMaterializerWithType(string methodName, Type[] typeArguments, params Expression[] arguments)
        {
            Debug.Assert(methodName != null, "methodName != null");
            Debug.Assert(arguments != null, "arguments != null");

            MethodInfo method = typeof(AtomMaterializerInvoker).GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            Debug.Assert(method != null, "method != null - found " + methodName);
            if (typeArguments != null)
            {
                method = method.MakeGenericMethod(typeArguments);
            }

            return Expression.Call(method, arguments);
        }

        /// <summary>Creates an expression that calls ProjectionCheckValueForPathIsNull.</summary>
        /// <param name="entry">Expression for root entry for paths.</param>
        /// <param name="entryType">Expression for expected type for entry.</param>
        /// <param name="path">Path to check null value for.</param>
        /// <returns>A new expression with the call instance.</returns>
        private Expression CallCheckValueForPathIsNull(Expression entry, Expression entryType, ProjectionPath path)
        {
            Expression result = CallMaterializer("ProjectionCheckValueForPathIsNull", entry, entryType, Expression.Constant(path, typeof(object)));
            this.annotations.Add(result, new ExpressionAnnotation() { Segment = path[path.Count - 1] });
            return result;
        }

        /// <summary>Creates an expression that calls ProjectionValueForPath.</summary>
        /// <param name="entry">Expression for root entry for paths.</param>
        /// <param name="entryType">Expression for expected type for entry.</param>
        /// <param name="path">Path to pull value from.</param>
        /// <returns>A new expression with the call instance.</returns>
        private Expression CallValueForPath(Expression entry, Expression entryType, ProjectionPath path)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(path != null, "path != null");

            Expression result = CallMaterializer("ProjectionValueForPath", this.materializerExpression, entry, entryType, Expression.Constant(path, typeof(object)));
            this.annotations.Add(result, new ExpressionAnnotation() { Segment = path[path.Count - 1] });
            return result;
        }

        /// <summary>Creates an expression that calls ProjectionValueForPath.</summary>
        /// <param name="entry">Expression for root entry for paths.</param>
        /// <param name="entryType">Expression for expected type for entry.</param>
        /// <param name="path">Path to pull value from.</param>
        /// <param name="type">Path to convert result for.</param>
        /// <returns>A new expression with the call instance.</returns>
        private Expression CallValueForPathWithType(Expression entry, Expression entryType, ProjectionPath path, Type type)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(path != null, "path != null");
            
            Expression value = this.CallValueForPath(entry, entryType, path);
            Expression result = Expression.Convert(value, type);
            this.annotations.Add(result, new ExpressionAnnotation() { Segment = path[path.Count - 1] });
            return result;
        }

        /// <summary>
        /// Rebinds a conditional that performs a null check on an entity.
        /// </summary>
        /// <param name="conditional">Conditional expression.</param>
        /// <param name="nullCheck">Results of null check analysis.</param>
        /// <returns>The rebound expression.</returns>
        /// <remarks>
        /// Do a rewrite to avoid creating a type in the null check:
        ///   a.b == null ? null : [a.b]-based expression
        /// becomes
        ///   ProjectionIsNull(a.b) ? null : [a.b]-based expression
        /// </remarks>
        private Expression RebindConditionalNullCheck(ConditionalExpression conditional, ResourceBinder.PatternRules.MatchNullCheckResult nullCheck)
        {
            Debug.Assert(conditional != null, "conditional != null");
            Debug.Assert(nullCheck.Match, "nullCheck.Match -- otherwise no reason to call this rebind method");

            Expression testToNullForProjection = this.Visit(nullCheck.TestToNullExpression);
            Expression assignForProjection = this.Visit(nullCheck.AssignExpression);
            ExpressionAnnotation testToNullAnnotation;
            if (!this.annotations.TryGetValue(testToNullForProjection, out testToNullAnnotation))
            {
                return base.VisitConditional(conditional);
            }

            ProjectionPathSegment testToNullSegment = testToNullAnnotation.Segment;

            Expression testToNullThroughMethod = this.CallCheckValueForPathIsNull(
                testToNullSegment.StartPath.RootEntry,
                testToNullSegment.StartPath.ExpectedRootType,
                testToNullSegment.StartPath);

            Expression test = testToNullThroughMethod;
            Expression iftrue = Expression.Constant(null, assignForProjection.Type);
            Expression iffalse = assignForProjection;
            Expression result = Expression.Condition(test, iftrue, iffalse);
            return result;
        }

        /// <summary>
        /// Rebinds the specified <paramref name="init"/> expression by gathering 
        /// annotated paths and returning an expression that calls the
        /// ProjectionGetEntity method.
        /// </summary>
        /// <param name="init">Member initialization expression.</param>
        /// <returns>A new expression suitable for materialization.</returns>
        private Expression RebindEntityMemberInit(MemberInitExpression init)
        {
            Debug.Assert(init != null, "init != null");
            Debug.Assert(init.Bindings.Count > 0, "init.Bindings.Count > 0 -- otherwise this is just empty construction");

            // We "jump" into entities only if we're not already materializing an entity.
            Expression[] expressions;
            if (!this.pathBuilder.HasRewrites)
            {
                MemberAssignmentAnalysis propertyAnalysis = MemberAssignmentAnalysis.Analyze(
                    this.pathBuilder.LambdaParameterInScope,
                    ((MemberAssignment)init.Bindings[0]).Expression);
                expressions = propertyAnalysis.GetExpressionsToTargetEntity();
                Debug.Assert(expressions.Length != 0, "expressions.Length != 0 -- otherwise there is no correlation to parameter in entity member init");
            }
            else
            {
                expressions = MemberAssignmentAnalysis.EmptyExpressionArray;
            }

            Expression entryParameterAtMemberInit = this.pathBuilder.ParameterEntryInScope;
            List<string> propertyNames = new List<string>();
            List<Func<object, object, Type, object>> propertyFunctions = new List<Func<object, object, Type, object>>();
            Type projectedType = init.NewExpression.Type;
            Expression projectedTypeExpression = Expression.Constant(projectedType, typeof(Type));

            // We may need to materialize from deeper in the entity tree for anonymous types.
            // t => new { nested = new Nested() { nid = t.nested.nid }
            //
            // We do the same kind of rewriting we'd do for a nested entity
            // but at the initializing scope (rather than at the member assignment scope).
            //
            // t=> new { nested = ProjInit(GetEntry(entry0, "Nested"), "nid", *->nid) }
            Expression entryToInitValue;    // Expression that yields value for entry in target tree.
            Expression expectedParamValue;  // Expression that yield expectedType in target tree.
            ParameterExpression entryParameterForMembers;       // Parameter expression members think of as "entry".
            ParameterExpression expectedParameterForMembers;    // Parameter expression members think of as "expectedType" for entry.
            string[] expressionNames = expressions.Skip(1).Select(e => ((MemberExpression)e).Member.Name).ToArray();
            if (expressions.Length <= 1)
            {
                entryToInitValue = this.pathBuilder.ParameterEntryInScope;
                expectedParamValue = this.pathBuilder.ExpectedParamTypeInScope;
                entryParameterForMembers = (ParameterExpression)this.pathBuilder.ParameterEntryInScope;
                expectedParameterForMembers = (ParameterExpression)this.pathBuilder.ExpectedParamTypeInScope;
            }
            else
            {
                entryToInitValue = this.GetDeepestEntry(expressions);
                expectedParamValue = projectedTypeExpression;
                entryParameterForMembers = Expression.Parameter(typeof(object), "subentry" + this.identifierId++);
                expectedParameterForMembers = (ParameterExpression)this.pathBuilder.ExpectedParamTypeInScope;

                // Annotate the entry expression with 'how we get to it' information.
                // The annotation on entryToInitiValue is picked up
                // The annotation on entryParameterForMembers is picked up to build nested member-init on entities.
                ProjectionPath entryPath = new ProjectionPath(
                    (ParameterExpression)this.pathBuilder.LambdaParameterInScope, 
                    this.pathBuilder.ExpectedParamTypeInScope, 
                    this.pathBuilder.ParameterEntryInScope,
                    expressions.Skip(1));

                this.annotations.Add(entryToInitValue, new ExpressionAnnotation() { Segment = entryPath[entryPath.Count - 1] });
                this.annotations.Add(entryParameterForMembers, new ExpressionAnnotation() { Segment = entryPath[entryPath.Count - 1] });
                this.pathBuilder.RegisterRewrite(this.pathBuilder.LambdaParameterInScope, expressionNames, entryParameterForMembers);
            }

            for (int i = 0; i < init.Bindings.Count; i++)
            {
                MemberAssignment assignment = (MemberAssignment)init.Bindings[i];
                propertyNames.Add(assignment.Member.Name);

                LambdaExpression propertyLambda;

                // Here are the rewrites we do for member inits:
                // new T { id = t.id }
                // => ProjInit(pt, "id", f(t -> *.id));
                //
                // new T { t2 = new T2 { id2 = *.t2.id2 } }
                // => ProjInit(pt, "t2", f(ProjInit(pt->t2), "id2", *.id2)))
                if ((ClientType.CheckElementTypeIsEntity(assignment.Member.ReflectedType) &&
                     assignment.Expression.NodeType == ExpressionType.MemberInit))
                {
                    Expression nestedEntry = CallMaterializer(
                        "ProjectionGetEntry",
                        entryParameterAtMemberInit,
                        Expression.Constant(assignment.Member.Name, typeof(string)));
                    ParameterExpression nestedEntryParameter = Expression.Parameter(
                        typeof(object),
                        "subentry" + this.identifierId++);

                    // Register the rewrite from the top to the entry if necessary.
                    ProjectionPath entryPath;
                    ExpressionAnnotation entryAnnotation;
                    if (this.annotations.TryGetValue(this.pathBuilder.ParameterEntryInScope, out entryAnnotation))
                    {
                        entryPath = new ProjectionPath(
                            (ParameterExpression)this.pathBuilder.LambdaParameterInScope,
                            this.pathBuilder.ExpectedParamTypeInScope,
                            entryParameterAtMemberInit);
                        entryPath.AddRange(entryAnnotation.Segment.StartPath);
                    }
                    else
                    {
                        entryPath = new ProjectionPath(
                            (ParameterExpression)this.pathBuilder.LambdaParameterInScope,
                            this.pathBuilder.ExpectedParamTypeInScope,
                            entryParameterAtMemberInit,
                            expressions.Skip(1));
                    }

                    ProjectionPathSegment nestedSegment = new ProjectionPathSegment(
                        entryPath,
                        assignment.Member.Name,
                        assignment.Member.ReflectedType);

                    entryPath.Add(nestedSegment);

                    string[] names = (entryPath.Where(m => m.Member != null).Select(m => m.Member)).ToArray();

                    this.annotations.Add(nestedEntryParameter, new ExpressionAnnotation() { Segment = nestedSegment });
                    this.pathBuilder.RegisterRewrite(this.pathBuilder.LambdaParameterInScope, names, nestedEntryParameter);
                    Expression e = this.Visit(assignment.Expression);
                    this.pathBuilder.RevokeRewrite(this.pathBuilder.LambdaParameterInScope, names);
                    this.annotations.Remove(nestedEntryParameter);

                    e = Expression.Convert(e, typeof(object));
                    ParameterExpression[] parameters =
                        new ParameterExpression[] 
                        {
                            this.materializerExpression,
                            nestedEntryParameter,
                            expectedParameterForMembers,
                        };
                    propertyLambda = Expression.Lambda(e, parameters);

                    Expression[] nestedParams =
                        new Expression[]
                        {
                            this.materializerExpression, 
                            nestedEntry,
                            expectedParameterForMembers,
                        };
                    var invokeParameters =
                        new ParameterExpression[] 
                        {
                            this.materializerExpression,
                            (ParameterExpression)entryParameterAtMemberInit,
                            expectedParameterForMembers,
                        };
                    propertyLambda = Expression.Lambda(Expression.Invoke(propertyLambda, nestedParams), invokeParameters);
                }
                else
                {
                    // We need an expression of object, which might require boxing.
                    Expression e = this.Visit(assignment.Expression);
                    e = Expression.Convert(e, typeof(object));
                    ParameterExpression[] parameters =
                        new ParameterExpression[] 
                        {
                            this.materializerExpression,
                            entryParameterForMembers,
                            expectedParameterForMembers,
                        };
                    propertyLambda = Expression.Lambda(e, parameters);
                }

#if TRACE_CLIENT_PROJECTIONS
                Trace.WriteLine("Compiling lambda for " + assignment.Member.Name + ": " + propertyLambda);
#endif
                propertyFunctions.Add((Func<object, object, Type, object>) propertyLambda.Compile());
            }

            // Revoke rewrites used for nested initialization.
            for (int i = 1; i < expressions.Length; i++)
            {
                this.pathBuilder.RevokeRewrite(this.pathBuilder.LambdaParameterInScope, expressionNames);
                this.annotations.Remove(entryToInitValue);
                this.annotations.Remove(entryParameterForMembers);
            }

            Expression reboundExpression = CallMaterializer(
                "ProjectionInitializeEntity",
                this.materializerExpression,
                entryToInitValue,
                expectedParamValue,
                projectedTypeExpression,
                Expression.Constant(propertyNames.ToArray()),
                Expression.Constant(propertyFunctions.ToArray()));

            return Expression.Convert(reboundExpression, projectedType);
        }

        /// <summary>
        /// Creates an expression that gets the deepest entry that will be found on the 
        /// specified <paramref name="path"/> (for the target tree).
        /// </summary>
        /// <param name="path">Path of expressions to walk.</param>
        /// <returns>An expression that invokes ProjectionGetEntry on the target tree.</returns>
        private Expression GetDeepestEntry(Expression[] path)
        {
            Debug.Assert(path.Length > 1, "path.Length > 1");
            
            Expression result = null;
            int pathIndex = 1;
            do
            {
                result = CallMaterializer(
                    "ProjectionGetEntry",
                    result ?? this.pathBuilder.ParameterEntryInScope,
                    Expression.Constant(((MemberExpression)path[pathIndex]).Member.Name, typeof(string)));
                pathIndex++;
            }
            while (pathIndex < path.Length);

            return result;
        }

        /// <summary>Gets an expression before its rewrite.</summary>
        /// <param name="expression">Expression to check.</param>
        /// <returns>The expression before normalization.</returns>
        private Expression GetExpressionBeforeNormalization(Expression expression)
        {
            Debug.Assert(expression != null, "expression != null");
            if (this.normalizerRewrites != null)
            {
                Expression original;
                if (this.normalizerRewrites.TryGetValue(expression, out original))
                {
                    expression = original;
                }
            }

            return expression;
        }

        /// <summary>Rebinds the specified parameter expression as a path-based access.</summary>
        /// <param name="expression">Expression to rebind.</param>
        /// <param name='annotation'>Annotation for the expression to rebind.</param>
        /// <returns>The rebound expression.</returns>
        private Expression RebindParameter(Expression expression, ExpressionAnnotation annotation)
        {
            Debug.Assert(expression != null, "expression != null");
            Debug.Assert(annotation != null, "annotation != null");

            Expression result;
            result = this.CallValueForPathWithType(
                annotation.Segment.StartPath.RootEntry,
                annotation.Segment.StartPath.ExpectedRootType,
                annotation.Segment.StartPath,
                expression.Type);

            // Refresh the annotation so the next one that comes along
            // doesn't start off with an already-written path.
            ProjectionPath parameterPath = new ProjectionPath(
                annotation.Segment.StartPath.Root,
                annotation.Segment.StartPath.ExpectedRootType,
                annotation.Segment.StartPath.RootEntry);
            ProjectionPathSegment parameterSegment = new ProjectionPathSegment(parameterPath, null, null);
            parameterPath.Add(parameterSegment);
            this.annotations[expression] = new ExpressionAnnotation() { Segment = parameterSegment };

            return result;
        }

        /// <summary>Rebinds the specified member access expression into a path-based value retrieval method call.</summary>
        /// <param name='m'>Member expression.</param>
        /// <param name='baseAnnotation'>Annotation for the base portion of the expression.</param>
        /// <returns>A rebound expression.</returns>
        private Expression RebindMemberAccess(MemberExpression m, ExpressionAnnotation baseAnnotation)
        {
            Debug.Assert(m != null, "m != null");
            Debug.Assert(baseAnnotation != null, "baseAnnotation != null");

            ProjectionPathSegment memberSegment;

            // If we are in nested member-init, we rewrite the property
            // accessors that are in the form of top.nested.id to
            // nested.id.
            Expression baseSourceExpression = m.Expression;
            Expression result = this.pathBuilder.GetRewrite(baseSourceExpression);
            if (result != null)
            {
                Expression baseTypeExpression = Expression.Constant(baseSourceExpression.Type, typeof(Type));
                ProjectionPath nestedPath = new ProjectionPath(result as ParameterExpression, baseTypeExpression, result);
                ProjectionPathSegment nestedSegment = new ProjectionPathSegment(nestedPath, m.Member.Name, m.Type);
                nestedPath.Add(nestedSegment);
                result = this.CallValueForPathWithType(result, baseTypeExpression, nestedPath, m.Type);
            }
            else
            {
                // This actually modifies the path for the underlying
                // segments, but that shouldn't be a problem. Actually
                // we should be able to remove it from the dictionary.
                // There should be no aliasing problems, because
                // annotations always come from target expression
                // that are generated anew (except parameters,
                // but those)
                memberSegment = new ProjectionPathSegment(baseAnnotation.Segment.StartPath, m.Member.Name, m.Type);
                baseAnnotation.Segment.StartPath.Add(memberSegment);
                result = this.CallValueForPathWithType(
                    baseAnnotation.Segment.StartPath.RootEntry,
                    baseAnnotation.Segment.StartPath.ExpectedRootType,
                    baseAnnotation.Segment.StartPath,
                    m.Type);
            }

            return result;
        }

        /// <summary>Rewrites a new statement for DataServiceCollection so the paging information
        /// is preserved in the materializer.</summary>
        /// <param name="nex">NewExpression to create a collection</param>
        /// <returns>The rewritten expression.</returns>
        private NewExpression RebindNewExpressionForDataServiceCollectionOfT(NewExpression nex)
        {
            Debug.Assert(nex != null, "nex != null");
            Debug.Assert(
                ResourceBinder.PatternRules.MatchNewDataServiceCollectionOfT(nex),
                "Called should have checked that the 'new' was for our collection type");

            NewExpression result = base.VisitNew(nex);

            ExpressionAnnotation annotation = null;

            if (result != null)
            {
                ConstructorInfo constructorInfo = 
                    nex.Type.GetConstructors(BindingFlags.NonPublic | BindingFlags.Instance).First(
                        c => c.GetParameters().Length == 7 && c.GetParameters()[0].ParameterType == typeof(object));

                Type enumerable = typeof(IEnumerable<>).MakeGenericType(nex.Type.GetGenericArguments()[0]);

                if (result.Arguments.Count == 1 && result.Constructor == nex.Type.GetConstructor(new[] { enumerable }) &&
                    this.annotations.TryGetValue(result.Arguments[0], out annotation))
                {
                    // DataServiceCollection<T>(
                    //     IEnumerable<T> items)
                    // -> 
                    // DataServiceCollection<T>(materializer, null, items, TrackingMode.AutoChangeTracking, null, null, null)
                    result = Expression.New(
                        constructorInfo,
                        this.materializerExpression,
                        Expression.Constant(null, typeof(DataServiceContext)),
                        result.Arguments[0],
                        Expression.Constant(TrackingMode.AutoChangeTracking, typeof(TrackingMode)),
                        Expression.Constant(null, typeof(string)),
                        Expression.Constant(null, typeof(Func<EntityChangedParams, bool>)),
                        Expression.Constant(null, typeof(Func<EntityCollectionChangedParams, bool>)));
                }
                else if (result.Arguments.Count == 2 &&
                         this.annotations.TryGetValue(result.Arguments[0], out annotation))
                {
                    // DataServiceCollection<T>(
                    //     IEnumerable<T> items, 
                    //     TrackingMode trackingMode)
                    // ->
                    // DataServiceCollection<T>(materializer, null, items, trackingMode, null, null, null)
                    result = Expression.New(
                        constructorInfo, 
                        this.materializerExpression,
                        Expression.Constant(null, typeof(DataServiceContext)),
                        result.Arguments[0], // items
                        result.Arguments[1], // TrackingMode
                        Expression.Constant(null, typeof(string)),
                        Expression.Constant(null, typeof(Func<EntityChangedParams, bool>)),
                        Expression.Constant(null, typeof(Func<EntityCollectionChangedParams, bool>)));
                }
                else if (result.Arguments.Count == 5 &&
                         this.annotations.TryGetValue(result.Arguments[0], out annotation))
                {
                    // DataServiceCollection<T>(
                    //     IEnumerable<T> items, 
                    //     TrackingMode trackingMode, 
                    //     string entitySet, 
                    //     Func<> entityChangedCallback, 
                    //     Func<> entityCollectionChangedCallback)
                    // ->
                    // DataServiceCollection<T>(materializer, null, items, trackingMode,
                    //     entitySet, entityChangedCallback, entityCollectionChangedCallback)
                    result = Expression.New(
                        constructorInfo, 
                        this.materializerExpression,
                        Expression.Constant(null, typeof(DataServiceContext)),
                        result.Arguments[0],   // items
                        result.Arguments[1],   // TrackingMode
                        result.Arguments[2],   // entityset name
                        result.Arguments[3],   // entity changed cb
                        result.Arguments[4]);  // collection changed cb
                }
                else if (result.Arguments.Count == 6 &&
                         typeof(DataServiceContext).IsAssignableFrom(result.Arguments[0].Type) &&
                         this.annotations.TryGetValue(result.Arguments[1], out annotation))
                {
                    // DataServiceCollection<T>(
                    //     DataServiceContext context,
                    //     IEnumerable<T> items, 
                    //     TrackingMode trackingMode, 
                    //     string entitySet, 
                    //     Func<> entityChangedCallback, 
                    //     Func<> entityCollectionChangedCallback)
                    // ->
                    // DataServiceCollection<T>(materializer, context, items, trackingMode,
                    //     entitySet, entityChangedCallback, entityCollectionChangedCallback)
                    result = Expression.New(
                        constructorInfo,
                        this.materializerExpression,
                        result.Arguments[0],   // context
                        result.Arguments[1],   // items
                        result.Arguments[2],   // trackingMode
                        result.Arguments[3],   // entityset name
                        result.Arguments[4],   // entity changed cb
                        result.Arguments[5]);  // collection changed cb
                }
            }

            if (annotation != null)
            {
                // Propagate the annotation of the "items" parameter (the enumerable) as the annotation
                //   of the DataServiceCollection, since it now represents the same thing.
                this.annotations.Add(result, annotation);
            }

            // Note that we end up just falling through without changing anything for 
            // constructors of DataServiceCollection<T> that aren't correlated (i.e. don't take
            // an enumerable in the input).
            return result;
        }

        /// <summary>Rewrites a call to Select() by adding to the current paths to project out.</summary>
        /// <param name="call">Call expression.</param>
        /// <returns>Expression with annotated path to include in member binding.</returns>
        private Expression RebindMethodCallForMemberSelect(MethodCallExpression call)
        {
            Debug.Assert(call != null, "call != null");
            Debug.Assert(call.Method.Name == "Select", "call.Method.Name == 'Select'");
            Debug.Assert(call.Object == null, "call.Object == null -- otherwise this isn't a call to a static Select method");
            Debug.Assert(call.Arguments.Count == 2, "call.Arguments.Count == 2 -- otherwise this isn't the expected Select() call on IQueryable");

            // Get the path for the parameter value that will be used in the Select lambda.
            Expression result = null;
            Expression parameterSource = this.Visit(call.Arguments[0]);
            ExpressionAnnotation annotation;
            this.annotations.TryGetValue(parameterSource, out annotation);

            // It's possibly that we haven't annotated this argument, in which
            // case we don't care about this select, eg new { a = new int[] {1}.Select(i=>i+1).First() }
            if (annotation != null)
            {
                // With this information from an annotation:
                // {t->*.Players}
                //
                // Call this lambda:
                // {(mat, entry1, type1) => Convert(ProjectionValueForPath(mat, entry1, type1, p->*.FirstName))}
                //
                // Annotating the intermediate expressions so we mark them
                // as rewrites and we know to route them through materializer
                // helpers, eg to preserve paging information.
                Expression selectorExpression = this.Visit(call.Arguments[1]);
                Type returnElementType = call.Method.ReturnType.GetGenericArguments()[0];
                result = CallMaterializer(
                    "ProjectionSelect",
                    this.materializerExpression,
                    this.pathBuilder.ParameterEntryInScope,
                    this.pathBuilder.ExpectedParamTypeInScope,
                    Expression.Constant(returnElementType, typeof(Type)),
                    Expression.Constant(annotation.Segment.StartPath, typeof(object)),
                    selectorExpression);
                this.annotations.Add(result, annotation);
                result = CallMaterializerWithType(
                    "EnumerateAsElementType",
                    new Type[] { returnElementType },
                    result);
                this.annotations.Add(result, annotation);
            }

            if (result == null)
            {
                result = base.VisitMethodCall(call);
            }

            return result;
        }

        /// <summary>Rewrites a call to ToList in the specified method.</summary>
        /// <param name="call">Call expression.</param>
        /// <returns>Expression with annotated path to include in member binding.</returns>
        /// <remarks>
        /// All that is necessary here is to rewrite the call to Select() and indicate
        /// that the target type is a given List&lt;T&gt;.
        /// 
        /// 

        private Expression RebindMethodCallForMemberToList(MethodCallExpression call)
        {
            Debug.Assert(call != null, "call != null");
            Debug.Assert(call.Object == null, "call.Object == null -- otherwise this isn't a call to a static ToList method");
            Debug.Assert(call.Method.Name == "ToList", "call.Method.Name == 'ToList'");

            // Wrap the source to the .ToList() call if it's our rewrite.
            Debug.Assert(call.Arguments.Count == 1, "call.Arguments.Count == 1 -- otherwise this isn't the expected ToList() call on IEnumerable");

            Expression result = this.Visit(call.Arguments[0]);
            ExpressionAnnotation annotation;
            if (this.annotations.TryGetValue(result, out annotation))
            {
                result = this.TypedEnumerableToList(result, call.Type);
                this.annotations.Add(result, annotation);
            }

            return result;
        }

        /// <summary>Rewrites a method call used in a sequence method (possibly over entity types).</summary>
        /// <param name="call">Call expression.</param>
        /// <returns>Expression that can be called directly to yield the expected value.</returns>
        private Expression RebindMethodCallForNewSequence(MethodCallExpression call)
        {
            Debug.Assert(call != null, "call != null");
            Debug.Assert(ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(call), "ProjectionAnalyzer.IsMethodCallAllowedEntitySequence(call)");
            Debug.Assert(call.Object == null, "call.Object == null -- otherwise this isn't the supported Select or ToList methods");

            // The only expressions that require rewriting are:
            // - [null].Select(entity-type-based-expression, lambda)
            // - [null].ToList(entity-type-enumeration)
            //
            // All other expressions can be visited normally.
            Expression result = null;

            if (call.Method.Name == "Select")
            {
                Debug.Assert(call.Arguments.Count == 2, "call.Arguments.Count == 2 -- otherwise this isn't the argument we expected");

                // Get the path for the parameter value that will be used in the Select lambda.
                Expression parameterSource = this.Visit(call.Arguments[0]);
                ExpressionAnnotation annotation;
                this.annotations.TryGetValue(parameterSource, out annotation);

                // It's possibly that we haven't annotated this argument, in which
                // case we don't care about this select, eg new { a = new int[] {1}.Select(i=>i+1).First() }
                if (annotation != null)
                {
                    // With this information from an annotation:
                    // {t->*.Players}
                    //
                    // Call this lambda:
                    // {(mat, entry1, type1) => Convert(ProjectionValueForPath(mat, entry1, type1, p->*.FirstName))}
                    Expression selectorExpression = this.Visit(call.Arguments[1]);
                    Type returnElementType = call.Method.ReturnType.GetGenericArguments()[0];
                    result = CallMaterializer(
                        "ProjectionSelect",
                        this.materializerExpression,
                        this.pathBuilder.ParameterEntryInScope,
                        this.pathBuilder.ExpectedParamTypeInScope,
                        Expression.Constant(returnElementType, typeof(Type)),
                        Expression.Constant(annotation.Segment.StartPath, typeof(object)),
                        selectorExpression);
                    this.annotations.Add(result, annotation);
                    result = CallMaterializerWithType(
                        "EnumerateAsElementType",
                        new Type[] { returnElementType },
                        result);
                    this.annotations.Add(result, annotation);
                }
            }
            else
            {
                Debug.Assert(call.Method.Name == "ToList", "call.Method.Name == 'ToList'");

                // Annotate the source to the .ToList() call.
                Expression source = this.Visit(call.Arguments[0]);
                ExpressionAnnotation annotation;
                if (this.annotations.TryGetValue(source, out annotation))
                {
                    result = this.TypedEnumerableToList(source, call.Type);
                    this.annotations.Add(result, annotation);
                }
            }

            if (result == null)
            {
                result = base.VisitMethodCall(call);
            }

            return result;
        }

        /// <summary>Returns a method call that returns a list from a typed enumerable.</summary>
        /// <param name="source">Expression to convert.</param>
        /// <param name="targetType">Target type to return.</param>
        /// <returns>The new expression.</returns>
        private Expression TypedEnumerableToList(Expression source, Type targetType)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(targetType != null, "targetType != null");

            // 

            Type enumeratedType = source.Type.GetGenericArguments()[0];
            Type listElementType = targetType.GetGenericArguments()[0];

            // Return the annotated expression.
            Expression result = CallMaterializerWithType(
                "ListAsElementType",
                new Type[] { enumeratedType, listElementType },
                this.materializerExpression,
                source);

            return result;
        }

        #endregion Private methods.

        #region Inner types.

        /// <summary>Annotates an expression, typically from the target tree.</summary>
        internal class ExpressionAnnotation
        {
            /// <summary>Segment that marks the path found to an expression.</summary>
            internal ProjectionPathSegment Segment 
            { 
                get; 
                set; 
            }
        }

        #endregion Inner types.
    }
}
