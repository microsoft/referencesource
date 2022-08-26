//---------------------------------------------------------------------
// <copyright file="ProjectionRewriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Tries to remove transparent identifiers from selector expressions
//      so it can be used as a projection.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;

    #endregion Namespaces.

    internal class ProjectionRewriter : ALinqExpressionVisitor
    {
        #region Private fields.

        private readonly ParameterExpression newLambdaParameter;

        private ParameterExpression oldLambdaParameter;

        private bool sucessfulRebind;

        #endregion Private fields.

        private ProjectionRewriter(Type proposedParameterType)
        {
            Debug.Assert(proposedParameterType != null, "proposedParameterType != null");
            this.newLambdaParameter = Expression.Parameter(proposedParameterType, "it");
        }

        #region Internal methods.

        internal static LambdaExpression TryToRewrite(LambdaExpression le, Type proposedParameterType)
        {
            LambdaExpression result;
            if (!ResourceBinder.PatternRules.MatchSingleArgumentLambda(le, out le) ||  // can only rewrite single parameter Lambdas.
                ClientType.CheckElementTypeIsEntity(le.Parameters[0].Type) || // only attempt to rewrite if lambda parameter is not an entity type
                !(le.Parameters[0].Type.GetProperties().Any(p => p.PropertyType == proposedParameterType))) // lambda parameter must have public property that is same as proposed type.
            {
                result = le;
            }
            else
            {
                ProjectionRewriter rewriter = new ProjectionRewriter(proposedParameterType);
                result = rewriter.Rebind(le);
            }

            return result;
        }

        internal LambdaExpression Rebind(LambdaExpression lambda)
        {
            this.sucessfulRebind = true;
            this.oldLambdaParameter = lambda.Parameters[0];

            Expression body = this.Visit(lambda.Body);
            if (this.sucessfulRebind)
            {
                Type delegateType = typeof(Func<,>).MakeGenericType(new Type[] { newLambdaParameter.Type, lambda.Body.Type });
#if ASTORIA_LIGHT
                return ExpressionHelpers.CreateLambda(delegateType, body, new ParameterExpression[] { this.newLambdaParameter });
#else
                return Expression.Lambda(delegateType, body, new ParameterExpression[] { this.newLambdaParameter });
#endif
            }
            else
            {
                throw new NotSupportedException(Strings.ALinq_CanOnlyProjectTheLeaf);
            }
        }

        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            if (m.Expression == this.oldLambdaParameter)
            {
                if (m.Type == this.newLambdaParameter.Type)
                {
                    return this.newLambdaParameter;
                }
                else
                {
                    this.sucessfulRebind = false;
                }
            }

            return base.VisitMemberAccess(m);
        }

        #endregion Internal methods.
    }
}
