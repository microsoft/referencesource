//---------------------------------------------------------------------
// <copyright file="InputBinder.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Expression visitor that converts ParameterReference or (MemberAccess*)\ParameterReference expressions
//      into references to one or more ResourceSetExpressions. After processing, the specified expression tree
//      will contain InputReferenceExpressions instead of these parameter or parameter and property references.
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
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// Replaces references to resource sets - represented as either ParameterExpressions or one or more
    /// MemberExpressions over a ParameterExpression - with an appropriate InputReferenceExpression that
    /// indicates which resource set is referenced; effective 'binds' the argument expression to the
    /// resource sets that it references.
    /// </summary>
    internal sealed class InputBinder : DataServiceALinqExpressionVisitor
    {
        #region Private fields.

        /// <summary>Tracks which resource sets are referenced by the argument expression</summary>
        private readonly HashSet<ResourceExpression> referencedInputs = new HashSet<ResourceExpression>(EqualityComparer<ResourceExpression>.Default);

        /// <summary>Resource from which valid references must start; if no set with a transparent scope is present, only direct references to this resource will be rebound</summary>
        private readonly ResourceExpression input;

        /// <summary>The input resource, as a resource set (may be null if the input is actually a NavigationPropertySingletonExpression)</summary>
        private readonly ResourceSetExpression inputSet;
        
        /// <summary>The ParameterExpression that, if encountered, indicates a reference to the input resource set</summary>
        private readonly ParameterExpression inputParameter;

        #endregion Private fields.

        /// <summary>
        /// Constructs a new InputBinder based on the specified input resource set, which is represented by the specified ParameterExpression.
        /// </summary>
        /// <param name="resource">The current input resource from which valid references must start</param>
        /// <param name="setReferenceParam">The parameter that must be referenced in order to refer to the specified input resource set</param>
        private InputBinder(ResourceExpression resource, ParameterExpression setReferenceParam)
        {
            this.input = resource;
            this.inputSet = resource as ResourceSetExpression;
            this.inputParameter = setReferenceParam;
        }

        /// <summary>
        /// Replaces Lambda parameter references or transparent scope property accesses over those Lambda
        /// parameter references with <see cref="InputReferenceExpression"/>s to the appropriate corresponding
        /// <see cref="ResourceSetExpression"/>s, based on the 'input' ResourceSetExpression to which the
        /// Lambda is logically applied and any enclosing transparent scope applied to that input resource set.
        /// </summary>
        /// <param name="e">The expression to rebind</param>
        /// <param name="currentInput">
        /// The 'current input' resource set - either the root resource set or the
        /// rightmost set in the navigation chain.</param>
        /// <param name="inputParameter">The Lambda parameter that represents a reference to the 'input' set</param>
        /// <param name="referencedInputs">A list that will be populated with the resource sets that were referenced by the rebound expression</param>
        /// <returns>
        /// The rebound version of <paramref name="e"/> where MemberExpression/ParameterExpressions that
        /// represent resource set references have been replaced with appropriate InputReferenceExpressions.
        /// </returns>
        internal static Expression Bind(Expression e, ResourceExpression currentInput, ParameterExpression inputParameter, List<ResourceExpression> referencedInputs)
        {
            Debug.Assert(e != null, "Expression cannot be null");
            Debug.Assert(currentInput != null, "A current input resource set is required");
            Debug.Assert(inputParameter != null, "The input lambda parameter is required");
            Debug.Assert(referencedInputs != null, "The referenced inputs list is required");

            InputBinder binder = new InputBinder(currentInput, inputParameter);
            Expression result = binder.Visit(e);
            referencedInputs.AddRange(binder.referencedInputs);
            return result;
        }
                
        /// <summary>
        /// Resolves member accesses that represent transparent scope property accesses to the corresponding resource set,
        /// iff the input resource set is enclosed in a transparent scope and the specified MemberExpression represents
        /// such a property access.
        /// </summary>
        /// <param name="m">MemberExpression expression to visit</param>
        /// <returns>
        /// An InputReferenceExpression if the member access represents a transparent scope property
        /// access that can be resolved to a resource set in the path that produces the input resource set;
        /// otherwise the same MemberExpression is returned.
        /// </returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            // If the current input resource set is not enclosed in a transparent scope, then this
            // MemberExpression cannot represent a valid transparent scope access based on the input parameter.
            if (this.inputSet == null ||
                !this.inputSet.HasTransparentScope)
            {
                return base.VisitMemberAccess(m);
            }

            ParameterExpression innerParamRef = null;
            Stack<PropertyInfo> nestedAccesses = new Stack<PropertyInfo>();
            MemberExpression memberRef = m;
            while (memberRef != null &&
                   memberRef.Member.MemberType == MemberTypes.Property &&
                   memberRef.Expression != null)
            {
                nestedAccesses.Push((PropertyInfo)memberRef.Member);

                if (memberRef.Expression.NodeType == ExpressionType.Parameter)
                {
                    innerParamRef = (ParameterExpression)memberRef.Expression;
                }

                memberRef = memberRef.Expression as MemberExpression;
            }

            // Only continue if the inner non-MemberExpression is the input reference ParameterExpression and
            // at least one property reference is present - otherwise this cannot be a transparent scope access.
            if (innerParamRef != this.inputParameter || nestedAccesses.Count == 0)
            {
                return m;
            }

            ResourceExpression target = this.input;
            ResourceSetExpression targetSet = this.inputSet;
            bool transparentScopeTraversed = false;

            // Process all the traversals through transparent scopes.
            while (nestedAccesses.Count > 0)
            {
                if (targetSet == null || !targetSet.HasTransparentScope)
                {
                    break;
                }

                // Peek the property; pop it once it's consumed
                // (it could be a non-transparent-identifier access).
                PropertyInfo currentProp = nestedAccesses.Peek();

                // If this is the accessor for the target, then the member
                // refers to the target itself.
                if (currentProp.Name.Equals(targetSet.TransparentScope.Accessor, StringComparison.Ordinal))
                {
                    target = targetSet;
                    nestedAccesses.Pop();
                    transparentScopeTraversed = true;
                    continue;
                }

                // This member could also be one of the in-scope sources of the target.
                Expression source;
                if (!targetSet.TransparentScope.SourceAccessors.TryGetValue(currentProp.Name, out source))
                {
                    break;
                }

                transparentScopeTraversed = true;
                nestedAccesses.Pop();
                Debug.Assert(source != null, "source != null -- otherwise ResourceBinder created an accessor to nowhere");
                InputReferenceExpression sourceReference = source as InputReferenceExpression;
                if (sourceReference == null)
                {
                    targetSet = source as ResourceSetExpression;
                    if (targetSet == null || !targetSet.HasTransparentScope)
                    {
                        target = (ResourceExpression)source;
                    }
                }
                else
                {
                    targetSet = sourceReference.Target as ResourceSetExpression;
                    target = targetSet;
                }
            }

            // If no traversals were made, the original expression is OK.
            if (!transparentScopeTraversed)
            {
                return m;
            }

            // Process traversals after the transparent scope.
            Expression result = this.CreateReference(target);
            while (nestedAccesses.Count > 0)
            {
                result = Expression.Property(result, nestedAccesses.Pop());
            }

            return result;
        }

        /// <summary>
        /// Converts a parameter reference to the input resource set into an InputReferenceExpression,
        /// iff the parameter reference is to the parameter expression that represents the input resource set
        /// and the input resource set is not enclosed in a transparent scope.
        /// </summary>
        /// <param name="p">The parameter reference expression</param>
        /// <returns>
        /// An InputReferenceExpression if the parameter reference is to the input parameter;
        /// otherwise the same parameter reference expression
        /// </returns>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            // If the input Resource Set is not enclosed in a transparent scope,
            // and the parameter reference is a reference to the Lambda parameter
            // that represents the input resource set, then return an InputReferenceExpression.
            if ((this.inputSet == null || !this.inputSet.HasTransparentScope) &&
               p == this.inputParameter)
            {
                return this.CreateReference(this.input);
            }
            else
            {
                return base.VisitParameter(p);
            }
        }

        /// <summary>
        /// Returns an <see cref="InputReferenceExpression"/> that references the specified resource set,
        /// and also adds the the resource set to the hashset of resource sets that were referenced by the
        /// expression that is being rebound.
        /// </summary>
        /// <param name="resource">The resource(set) for which a reference was found</param>
        /// <returns>An InputReferenceExpression that represents a reference to the specified resource set</returns>
        private Expression CreateReference(ResourceExpression resource)
        {
            this.referencedInputs.Add(resource);
            return resource.CreateReference();
        }
    }
}

