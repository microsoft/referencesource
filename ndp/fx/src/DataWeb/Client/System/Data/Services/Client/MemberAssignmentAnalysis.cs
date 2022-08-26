//---------------------------------------------------------------------
// <copyright file="MemberAssignmentAnalysis.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can analyze a member assignment to determine
// how deep / which entity types the assignment is coming from.
// </summary>
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
    /// Use this class to analyze a member assignment and figure out the 
    /// target path for a member-init on an entity type.
    /// </summary>
    /// <remarks>
    /// This class will also detect cases in which the assignment
    /// expression refers to cases which we shouldn't handle during
    /// materialization, such as references to multiple entity types
    /// as sources (or refering to no source at all).
    /// </remarks>
    internal class MemberAssignmentAnalysis : ALinqExpressionVisitor
    {
        #region Fields.

        /// <summary>Empty expression array; immutable.</summary>
        internal static readonly Expression[] EmptyExpressionArray = new Expression[0];

        /// <summary>Entity in scope for the lambda that's providing the parameter.</summary>
        private readonly Expression entity;

        /// <summary>A non-null value when incompatible paths were found for an entity initializer.</summary>
        private Exception incompatibleAssignmentsException;

        /// <summary>Whether multiple paths were found for this analysis.</summary>
        private bool multiplePathsFound;

        /// <summary>Path traversed from the entry field.</summary>
        private List<Expression> pathFromEntity;

        #endregion Fields.

        #region Constructor.

        /// <summary>Initializes a new <see cref="MemberAssignmentAnalysis"/> instance.</summary>
        /// <param name="entity">Entity in scope for the lambda that's providing the parameter.</param>
        private MemberAssignmentAnalysis(Expression entity)
        {
            Debug.Assert(entity != null, "entity != null");

            this.entity = entity;
            this.pathFromEntity = new List<Expression>();
        }

        #endregion Constructor.

        #region Properties.

        /// <summary>A non-null value when incompatible paths were found for an entity initializer.</summary>
        internal Exception IncompatibleAssignmentsException
        {
            get { return this.incompatibleAssignmentsException; }
        }

        /// <summary>Whether multiple paths were found during analysis.</summary>
        internal bool MultiplePathsFound
        {
            get { return this.multiplePathsFound; }
        }

        #endregion Properites.

        #region Methods.

        /// <summary>Analyzes an assignment from a member-init expression.</summary>
        /// <param name="entityInScope">Entity in scope for the lambda that's providing the parameter.</param>
        /// <param name="assignmentExpression">The expression to analyze.</param>
        /// <returns>The analysis results.</returns>
        internal static MemberAssignmentAnalysis Analyze(Expression entityInScope, Expression assignmentExpression)
        {
            Debug.Assert(entityInScope != null, "entityInScope != null");
            Debug.Assert(assignmentExpression != null, "assignmentExpression != null");

            MemberAssignmentAnalysis result = new MemberAssignmentAnalysis(entityInScope);
            result.Visit(assignmentExpression);
            return result;
        }

        /// <summary>
        /// Checks whether the this and a <paramref name="previous"/>
        /// paths for assignments are compatible.
        /// </summary>
        /// <param name="targetType">Type being initialized.</param>
        /// <param name="previous">Previously seen member accesses (null if this is the first).</param>
        /// <returns>An exception to be thrown if assignments are not compatible; null otherwise.</returns>
        /// <remarks>
        /// This method does not set the IncompatibleAssignmentsException property on either
        /// analysis instance.
        /// </remarks>
        internal Exception CheckCompatibleAssignments(Type targetType, ref MemberAssignmentAnalysis previous)
        {
            if (previous == null)
            {
                previous = this;
                return null;
            }

            Expression[] previousExpressions = previous.GetExpressionsToTargetEntity();
            Expression[] candidateExpressions = this.GetExpressionsToTargetEntity();
            return CheckCompatibleAssignments(targetType, previousExpressions, candidateExpressions);
        }

        /// <summary>Visits the specified <paramref name="expression"/>.</summary>
        /// <param name="expression">Expression to visit.</param>
        /// <returns>The visited expression.</returns>
        /// <remarks>This method is overriden to short-circuit analysis once an error is found.</remarks>
        internal override Expression Visit(Expression expression)
        {
            if (this.multiplePathsFound || this.incompatibleAssignmentsException != null)
            {
                return expression;
            }

            return base.Visit(expression);
        }

        /// <summary>Visits a conditional expression.</summary>
        /// <param name="c">Expression to visit.</param>
        /// <returns>The same expression.</returns>
        /// <remarks>
        /// There are three expressions of interest: the Test, the IfTrue 
        /// branch, and the IfFalse branch. If this is a NullCheck expression,
        /// then we can traverse the non-null branch, which will be the
        /// correct path of the resulting value.
        /// </remarks>
        internal override Expression VisitConditional(ConditionalExpression c)
        {
            Expression result;

            var nullCheck = ResourceBinder.PatternRules.MatchNullCheck(this.entity, c);
            if (nullCheck.Match)
            {
                this.Visit(nullCheck.AssignExpression);
                result = c;
            }
            else
            {
                result = base.VisitConditional(c);
            }

            return result;
        }

        /// <summary>Parameter visit method.</summary>
        /// <param name="p">Parameter to visit.</param>
        /// <returns>The same expression.</returns>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            if (p == this.entity)
            {
                if (this.pathFromEntity.Count != 0)
                {
                    this.multiplePathsFound = true;
                }
                else
                {
                    this.pathFromEntity.Add(p);
                }
            }

            return p;
        }

        /// <summary>Visits a nested member init.</summary>
        /// <param name="init">Expression to visit.</param>
        /// <returns>The same expression.</returns>
        internal override Expression VisitMemberInit(MemberInitExpression init)
        {
            Expression result = init;
            MemberAssignmentAnalysis previousNested = null;
            foreach (var binding in init.Bindings)
            {
                MemberAssignment assignment = binding as MemberAssignment;
                if (assignment == null)
                {
                    continue;
                }

                MemberAssignmentAnalysis nested = MemberAssignmentAnalysis.Analyze(this.entity, assignment.Expression);
                if (nested.MultiplePathsFound)
                {
                    this.multiplePathsFound = true;
                    break;
                }

                // When we're visitng a nested entity initializer, we're exactly one level above that.
                Exception incompatibleException = nested.CheckCompatibleAssignments(init.Type, ref previousNested);
                if (incompatibleException != null)
                {
                    this.incompatibleAssignmentsException = incompatibleException;
                    break;
                }

                if (this.pathFromEntity.Count == 0)
                {
                    this.pathFromEntity.AddRange(nested.GetExpressionsToTargetEntity());
                }
            }

            return result;
        }

        /// <summary>Visits a member access expression.</summary>
        /// <param name="m">Access to visit.</param>
        /// <returns>The same expression.</returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            Expression result = base.VisitMemberAccess(m);
            if (this.pathFromEntity.Contains(m.Expression))
            {
                this.pathFromEntity.Add(m);
            }

            return result;
        }

        /// <summary>Visits a method call expression.</summary>
        /// <param name="call">Method call to visit.</param>
        /// <returns>The same call.</returns>
        internal override Expression VisitMethodCall(MethodCallExpression call)
        {
            // When we .Select(), the source of the enumeration is what we contribute
            // eg: p => p.Cities.Select(c => c) ::= Select(p.Cities, c => c);
            if (ReflectionUtil.IsSequenceMethod(call.Method, SequenceMethod.Select))
            {
                this.Visit(call.Arguments[0]);
                return call;
            }

            return base.VisitMethodCall(call);
        }

        /// <summary>Gets the expressions that go beyond the last entity.</summary>
        /// <returns>An array of member expressions coming after the last entity.</returns>
        /// <remarks>Currently a single member access is supported.</remarks>
        internal Expression[] GetExpressionsBeyondTargetEntity()
        {
            Debug.Assert(!this.multiplePathsFound, "this.multiplePathsFound -- otherwise GetExpressionsToTargetEntity won't return reliable (consistent) results");

            if (this.pathFromEntity.Count <= 1)
            {
                return EmptyExpressionArray;
            }

            Expression[] result = new Expression[1];
            result[0] = this.pathFromEntity[this.pathFromEntity.Count - 1];
            return result;
        }

        /// <summary>Gets the expressions that "walk down" to the last entity.</summary>
        /// <returns>An array of member expressions down to the last entity.</returns>
        internal Expression[] GetExpressionsToTargetEntity()
        {
            Debug.Assert(!this.multiplePathsFound, "this.multiplePathsFound -- otherwise GetExpressionsToTargetEntity won't return reliable (consistent) results");

            if (this.pathFromEntity.Count <= 1)
            {
                return EmptyExpressionArray;
            }

            Expression[] result = new Expression[this.pathFromEntity.Count - 1];
            for (int i = 0; i < result.Length; i++)
            {
                result[i] = this.pathFromEntity[i];
            }

            return result;
        }

        /// <summary>
        /// Checks whether the <paramref name="previous"/> and <paramref name="candidate"/>
        /// paths for assignments are compatible.
        /// </summary>
        /// <param name="targetType">Type being initialized.</param>
        /// <param name="previous">Previously seen member accesses.</param>
        /// <param name="candidate">Member assignments under evaluate.</param>
        /// <returns>An exception to be thrown if assignments are not compatible; null otherwise.</returns>
        private static Exception CheckCompatibleAssignments(Type targetType, Expression[] previous, Expression[] candidate)
        {
            Debug.Assert(targetType != null, "targetType != null");
            Debug.Assert(previous != null, "previous != null");
            Debug.Assert(candidate != null, "candidate != null");

            if (previous.Length != candidate.Length)
            {
                throw CheckCompatibleAssignmentsFail(targetType, previous, candidate);
            }

            for (int i = 0; i < previous.Length; i++)
            {
                Expression p = previous[i];
                Expression c = candidate[i];
                if (p.NodeType != c.NodeType)
                {
                    throw CheckCompatibleAssignmentsFail(targetType, previous, candidate);
                }

                if (p == c)
                {
                    continue;
                }

                if (p.NodeType != ExpressionType.MemberAccess)
                {
                    return CheckCompatibleAssignmentsFail(targetType, previous, candidate);
                }

                if (((MemberExpression)p).Member.Name != ((MemberExpression)c).Member.Name)
                {
                    return CheckCompatibleAssignmentsFail(targetType, previous, candidate);
                }
            }

            return null;
        }

        /// <summary>Creates an exception to be used when CheckCompatibleAssignment fails.</summary>
        /// <param name="targetType">Type being initialized.</param>
        /// <param name="previous">Previously seen member accesses.</param>
        /// <param name="candidate">Member assignments under evaluate.</param>
        /// <returns>A new exception with diagnostic information.</returns>
        private static Exception CheckCompatibleAssignmentsFail(Type targetType, Expression[] previous, Expression[] candidate)
        {
            Debug.Assert(targetType != null, "targetType != null");
            Debug.Assert(previous != null, "previous != null");
            Debug.Assert(candidate != null, "candidate != null");

            string message = Strings.ALinq_ProjectionMemberAssignmentMismatch(targetType.FullName, previous.LastOrDefault(), candidate.LastOrDefault());
            return new NotSupportedException(message);
        }

        #endregion Methods.
    }
}
