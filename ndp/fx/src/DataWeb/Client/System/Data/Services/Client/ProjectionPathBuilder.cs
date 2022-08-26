//---------------------------------------------------------------------
// <copyright file="ProjectionPathBuilder.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can materialize ATOM entries into typed
// objects, while maintaining a log of changes done.
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

    #endregion Namespaces.

    /// <summary>
    /// Use this class to help keep track of projection paths built
    /// while compiling a projection-based materialization plan.
    /// </summary>
    internal class ProjectionPathBuilder
    {
        #region Private fields.

        /// <summary>Stack of whether entities are in scope.</summary>
        private readonly Stack<bool> entityInScope;

        /// <summary>Registers rewrites for member initialization blocks.</summary>
        private readonly List<MemberInitRewrite> rewrites;

        /// <summary>Stack of lambda expressions in scope.</summary>
        private readonly Stack<ParameterExpression> parameterExpressions;

        /// <summary>
        /// Stack of expected type expression for <fieldref name="parameterExpressions"/>.
        /// </summary>
        private readonly Stack<Expression> parameterExpressionTypes;

        /// <summary>Stack of 'entry' parameter expressions.</summary>
        private readonly Stack<Expression> parameterEntries;

        /// <summary>Stack of projection (target-tree) types for parameters.</summary>
        private readonly Stack<Type> parameterProjectionTypes;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ProjectionPathBuilder"/> instance.</summary>
        internal ProjectionPathBuilder()
        {
            this.entityInScope = new Stack<bool>();
            this.rewrites = new List<MemberInitRewrite>();
            this.parameterExpressions = new Stack<ParameterExpression>();
            this.parameterExpressionTypes = new Stack<Expression>();
            this.parameterEntries = new Stack<Expression>();
            this.parameterProjectionTypes = new Stack<Type>();
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Whether the current scope is acting on an entity.</summary>
        internal bool CurrentIsEntity
        {
            get { return this.entityInScope.Peek(); }
        }

        /// <summary>Expression for the expected type parameter.</summary>
        internal Expression ExpectedParamTypeInScope
        {
            get
            {
                Debug.Assert(this.parameterExpressionTypes.Count > 0, "this.parameterExpressionTypes.Count > 0");
                return this.parameterExpressionTypes.Peek();
            }
        }

        /// <summary>Whether any rewrites have been registered.</summary>
        internal bool HasRewrites
        {
            get { return this.rewrites.Count > 0; }
        }

        /// <summary>Expression for the entity parameter in the source tree lambda.</summary>
        internal Expression LambdaParameterInScope
        {
            get
            {
                return this.parameterExpressions.Peek();
            }
        }

        /// <summary>Expression for the entry parameter in the target tree.</summary>
        internal Expression ParameterEntryInScope
        {
            get
            {
                return this.parameterEntries.Peek();
            }
        }

        #endregion Internal properties.

        #region Methods.

        /// <summary>Provides a string representation of this object.</summary>
        /// <returns>String representation of this object.</returns>
        public override string ToString()
        {
            string result = "ProjectionPathBuilder: ";
            if (this.parameterExpressions.Count == 0)
            {
                result += "(empty)";
            }
            else
            {
                result +=
                    "entity:" + this.CurrentIsEntity +
                    " param:" + this.ParameterEntryInScope;
            }

            return result;
        }

        /// <summary>Records that a lambda scope has been entered when visiting a projection.</summary>
        /// <param name="lambda">Lambda being visited.</param>
        /// <param name="entry">Expression to the entry parameter from the target tree.</param>
        /// <param name="expectedType">Expression to the entry-expected-type from the target tree.</param>
        internal void EnterLambdaScope(LambdaExpression lambda, Expression entry, Expression expectedType)
        {
            Debug.Assert(lambda != null, "lambda != null");
            Debug.Assert(lambda.Parameters.Count == 1, "lambda.Parameters.Count == 1");

            ParameterExpression param = lambda.Parameters[0];
            Type projectionType = lambda.Body.Type;
            bool isEntityType = ClientType.CheckElementTypeIsEntity(projectionType);

            this.entityInScope.Push(isEntityType);
            this.parameterExpressions.Push(param);
            this.parameterExpressionTypes.Push(expectedType);
            this.parameterEntries.Push(entry);
            this.parameterProjectionTypes.Push(projectionType);
        }

        /// <summary>
        /// Records that a member initialization expression has been entered 
        /// when visting a projection.
        /// </summary>
        /// <param name="init">Expression for initialization.</param>
        internal void EnterMemberInit(MemberInitExpression init)
        {
            bool isEntityType = ClientType.CheckElementTypeIsEntity(init.Type);
            this.entityInScope.Push(isEntityType);
        }

        /// <summary>Gets a rewrite for the specified expression; null if none is found.</summary>
        /// <param name="expression">Expression to match.</param>
        /// <returns>A rewrite for the expression; possibly null.</returns>
        internal Expression GetRewrite(Expression expression)
        {
            Debug.Assert(expression != null, "expression != null");

            List<string> names = new List<string>();
            while (expression.NodeType == ExpressionType.MemberAccess)
            {
                MemberExpression member = (MemberExpression)expression;
                names.Add(member.Member.Name);
                expression = member.Expression;
            }

            Expression result = null;
            foreach (var rewrite in this.rewrites)
            {
                if (rewrite.Root != expression)
                {
                    continue;
                }

                if (names.Count != rewrite.MemberNames.Length)
                {
                    continue;
                }

                bool match = true;
                for (int i = 0; i < names.Count && i < rewrite.MemberNames.Length; i++)
                {
                    if (names[names.Count - i - 1] != rewrite.MemberNames[i])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    result = rewrite.RewriteExpression;
                    break;
                }
            }

            return result;
        }

        /// <summary>Records that a lambda scope has been left when visting a projection.</summary>
        internal void LeaveLambdaScope()
        {
            this.entityInScope.Pop(); 
            this.parameterExpressions.Pop();
            this.parameterExpressionTypes.Pop();
            this.parameterEntries.Pop();
            this.parameterProjectionTypes.Pop();
        }

        /// <summary>
        /// Records that a member initialization expression has been left when 
        /// visting a projection.
        /// </summary>
        internal void LeaveMemberInit()
        {
            this.entityInScope.Pop();
        }

        /// <summary>Registers a member initialization rewrite.</summary>
        /// <param name="root">Root of member access path, typically a source tree parameter of entity type.</param>
        /// <param name="names">Sequence of names to match.</param>
        /// <param name="rewriteExpression">Rewrite expression for names.</param>
        internal void RegisterRewrite(Expression root, string[] names, Expression rewriteExpression)
        {
            Debug.Assert(root != null, "root != null");
            Debug.Assert(names != null, "names != null");
            Debug.Assert(rewriteExpression != null, "rewriteExpression != null");

            this.rewrites.Add(new MemberInitRewrite() { Root = root, MemberNames = names, RewriteExpression = rewriteExpression });
            this.parameterEntries.Push(rewriteExpression);
        }

        /// <summary>Revokes the latest rewrite registered on the specified <paramref name="root"/>.</summary>
        /// <param name="root">Root of rewrites to revoke.</param>
        /// <param name="names">Names to revoke.</param>
        internal void RevokeRewrite(Expression root, string[] names)
        {
            Debug.Assert(root != null, "root != null");

            for (int i = 0; i < this.rewrites.Count; i++)
            {
                if (this.rewrites[i].Root != root)
                {
                    continue;
                }

                if (names.Length != this.rewrites[i].MemberNames.Length)
                {
                    continue;
                }

                bool match = true;
                for (int j = 0; j < names.Length; j++)
                {
                    if (names[j] != this.rewrites[i].MemberNames[j])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    this.rewrites.RemoveAt(i);
                    this.parameterEntries.Pop();
                    return;
                }
            }
        }

        #endregion Methods.

        #region Inner types.

        /// <summary>Use this class to record how rewrites should occur under nested member initializations.</summary>
        internal class MemberInitRewrite
        {
            /// <summary>Sequence of member names to match.</summary>
            internal string[] MemberNames 
            { 
                get; 
                set; 
            }

            /// <summary>Root of member access path, typically a source tree parameter of entity type.</summary>
            internal Expression Root 
            { 
                get; 
                set; 
            }
         
            /// <summary>Rewrite expressions for the last member path.</summary>
            internal Expression RewriteExpression
            { 
                get; 
                set; 
            }
        }

        #endregion Inner types.
    }
}
