//---------------------------------------------------------------------
// <copyright file="ExpressionWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Serializes sub-expressions as query options in the URI.
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
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Special visitor to serialize supported expression as query parameters
    /// in the generated URI.
    /// </summary>
    internal class ExpressionWriter : DataServiceALinqExpressionVisitor
    {
        #region Private fields.

        /// <summary>Internal buffer.</summary>
        private readonly StringBuilder builder;

        /// <summary>Data context used to generate type names for types.</summary>
        private readonly DataServiceContext context;

        /// <summary>Stack of expressions being visited.</summary>
        private readonly Stack<Expression> expressionStack;

        /// <summary>set if can't translate expression</summary>
        private bool cantTranslateExpression;

        /// <summary>Parent expression of the current expression (expression.Peek()); possibly null.</summary>
        private Expression parent;

        #endregion Private fields.

        /// <summary>
        /// Creates an ExpressionWriter for the specified <paramref name="context"/>.
        /// </summary>
        /// <param name='context'>Data context used to generate type names for types.</param>
        private ExpressionWriter(DataServiceContext context)
        {
            Debug.Assert(context != null, "context != null");
            this.context = context;
            this.builder = new StringBuilder();
            this.expressionStack = new Stack<Expression>();
            this.expressionStack.Push(null);
        }

        /// <summary>
        /// Serializes an expression to a string
        /// </summary>
        /// <param name='context'>Data context used to generate type names for types.</param>
        /// <param name="e">Expression to serialize</param>
        /// <returns>serialized expression</returns>
        internal static string ExpressionToString(DataServiceContext context, Expression e)
        {
            ExpressionWriter ew = new ExpressionWriter(context);
            string serialized = ew.Translate(e);
            if (ew.cantTranslateExpression)
            {
                throw new NotSupportedException(Strings.ALinq_CantTranslateExpression(e.ToString()));
            }

            return serialized;
        }

        /// <summary>Main visit method.</summary>
        /// <param name="exp">Expression to visit</param>
        /// <returns>Visited expression</returns>
        internal override Expression Visit(Expression exp)
        {
            this.parent = this.expressionStack.Peek();
            this.expressionStack.Push(exp);
            Expression result = base.Visit(exp);
            this.expressionStack.Pop();
            return result;
        }

        /// <summary>
        /// ConditionalExpression visit method
        /// </summary>
        /// <param name="c">The ConditionalExpression expression to visit</param>
        /// <returns>The visited ConditionalExpression expression </returns>
        internal override Expression VisitConditional(ConditionalExpression c)
        {
            this.cantTranslateExpression = true;
            return c;
        }

        /// <summary>
        /// LambdaExpression visit method
        /// </summary>
        /// <param name="lambda">The LambdaExpression to visit</param>
        /// <returns>The visited LambdaExpression</returns>
        internal override Expression VisitLambda(LambdaExpression lambda)
        {
            this.cantTranslateExpression = true;
            return lambda;
        }

        /// <summary>
        /// NewExpression visit method
        /// </summary>
        /// <param name="nex">The NewExpression to visit</param>
        /// <returns>The visited NewExpression</returns>
        internal override NewExpression VisitNew(NewExpression nex)
        {
            this.cantTranslateExpression = true;
            return nex;
        }

        /// <summary>
        /// MemberInitExpression visit method
        /// </summary>
        /// <param name="init">The MemberInitExpression to visit</param>
        /// <returns>The visited MemberInitExpression</returns>
        internal override Expression VisitMemberInit(MemberInitExpression init)
        {
            this.cantTranslateExpression = true;
            return init;
        }

        /// <summary>
        /// ListInitExpression visit method
        /// </summary>
        /// <param name="init">The ListInitExpression to visit</param>
        /// <returns>The visited ListInitExpression</returns>
        internal override Expression VisitListInit(ListInitExpression init)
        {
            this.cantTranslateExpression = true;
            return init;
        }

        /// <summary>
        /// NewArrayExpression visit method
        /// </summary>
        /// <param name="na">The NewArrayExpression to visit</param>
        /// <returns>The visited NewArrayExpression</returns>
        internal override Expression VisitNewArray(NewArrayExpression na)
        {
            this.cantTranslateExpression = true;
            return na;
        }

        /// <summary>
        /// InvocationExpression visit method
        /// </summary>
        /// <param name="iv">The InvocationExpression to visit</param>
        /// <returns>The visited InvocationExpression</returns>
        internal override Expression VisitInvocation(InvocationExpression iv)
        {
            this.cantTranslateExpression = true;
            return iv;
        }

        /// <summary>
        /// Input resource set references are intentionally omitted from the URL string. 
        /// </summary>
        /// <param name="ire">The input reference</param>
        /// <returns>The same input reference expression</returns>
        internal override Expression VisitInputReferenceExpression(InputReferenceExpression ire)
        {
            // This method intentionally does not write anything to the URI.
            // This is how 'Where(<input>.Id == 5)' becomes '$filter=Id eq 5'.
            Debug.Assert(ire != null, "ire != null");
            if (this.parent == null || this.parent.NodeType != ExpressionType.MemberAccess)
            {
                // Ideally we refer to the parent expression as the un-translatable one,
                // because we cannot reference 'this' as a standalone expression; however
                // if the parent is null for any reasonn, we fall back to the expression itself.
                string expressionText = (this.parent != null) ? this.parent.ToString() : ire.ToString();
                throw new NotSupportedException(Strings.ALinq_CantTranslateExpression(expressionText));
            }

            return ire;
        }

        /// <summary>
        /// MethodCallExpression visit method
        /// </summary>
        /// <param name="m">The MethodCallExpression expression to visit</param>
        /// <returns>The visited MethodCallExpression expression </returns>
        internal override Expression VisitMethodCall(MethodCallExpression m)
        {
            string methodName;
            if (TypeSystem.TryGetQueryOptionMethod(m.Method, out methodName))
            {
                this.builder.Append(methodName);
                this.builder.Append(UriHelper.LEFTPAREN);

                // There is a single function, 'substringof', which reorders its argument with 
                // respect to the CLR method. Thus handling it as a special case rather than
                // using a more general argument reordering mechanism.
                if (methodName == "substringof")
                {
                    Debug.Assert(m.Method.Name == "Contains", "m.Method.Name == 'Contains'");
                    Debug.Assert(m.Object != null, "m.Object != null");
                    Debug.Assert(m.Arguments.Count == 1, "m.Arguments.Count == 1");
                    this.Visit(m.Arguments[0]);
                    this.builder.Append(UriHelper.COMMA);
                    this.Visit(m.Object);
                }
                else
                {
                    if (m.Object != null)
                    {
                        this.Visit(m.Object);
                    }

                    if (m.Arguments.Count > 0)
                    {
                        if (m.Object != null)
                        {
                            this.builder.Append(UriHelper.COMMA);
                        }

                        for (int ii = 0; ii < m.Arguments.Count; ii++)
                        {
                            this.Visit(m.Arguments[ii]);
                            if (ii < m.Arguments.Count - 1)
                            {
                                this.builder.Append(UriHelper.COMMA);
                            }
                        }
                    }
                }

                this.builder.Append(UriHelper.RIGHTPAREN);
            }
            else
            {
                this.cantTranslateExpression = true;
            }

            return m;
        }

        /// <summary>
        /// Serializes an MemberExpression to a string
        /// </summary>
        /// <param name="m">Expression to serialize</param>
        /// <returns>MemberExpression</returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            if (m.Member is FieldInfo)
            {
                throw new NotSupportedException(Strings.ALinq_CantReferToPublicField(m.Member.Name));
            }

            Expression e = this.Visit(m.Expression);

            // if this is a Nullable<T> instance, don't write out /Value since not supported by server
            if (m.Member.Name == "Value" && m.Member.DeclaringType.IsGenericType
                && m.Member.DeclaringType.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                return m;
            }

            if (!IsInputReference(e))
            {
                this.builder.Append(UriHelper.FORWARDSLASH);
            }

            this.builder.Append(m.Member.Name);

            return m;
        }

        /// <summary>
        /// ConstantExpression visit method
        /// </summary>
        /// <param name="c">The ConstantExpression expression to visit</param>
        /// <returns>The visited ConstantExpression expression </returns>
        internal override Expression VisitConstant(ConstantExpression c)
        {
            string result = null;
            if (c.Value == null)
            {
                this.builder.Append(UriHelper.NULL);
                return c;
            }
            else if (!ClientConvert.TryKeyPrimitiveToString(c.Value, out result))
            {
                throw new InvalidOperationException(Strings.ALinq_CouldNotConvert(c.Value));
            }

            Debug.Assert(result != null, "result != null");
            this.builder.Append(result);
            return c;
        }

        /// <summary>
        /// Serializes an UnaryExpression to a string
        /// </summary>
        /// <param name="u">Expression to serialize</param>
        /// <returns>UnaryExpression</returns>
        internal override Expression VisitUnary(UnaryExpression u)
        {
            switch (u.NodeType)
            {
                case ExpressionType.Not:
                    this.builder.Append(UriHelper.NOT);
                    this.builder.Append(UriHelper.SPACE);
                    this.VisitOperand(u.Operand);
                    break;
                case ExpressionType.Negate:
                case ExpressionType.NegateChecked:
                    this.builder.Append(UriHelper.SPACE);
                    this.builder.Append(UriHelper.NEGATE);
                    this.VisitOperand(u.Operand);
                    break;
                case ExpressionType.Convert:
                case ExpressionType.ConvertChecked:
                    if (u.Type != typeof(object))
                    {
                        this.builder.Append(UriHelper.CAST);
                        this.builder.Append(UriHelper.LEFTPAREN);
                        if (!IsInputReference(u.Operand))
                        {
                            this.Visit(u.Operand);
                            this.builder.Append(UriHelper.COMMA);
                        }

                        this.builder.Append(UriHelper.QUOTE);
                        this.builder.Append(this.TypeNameForUri(u.Type));
                        this.builder.Append(UriHelper.QUOTE);
                        this.builder.Append(UriHelper.RIGHTPAREN);
                    }
                    else
                    {
                        if (!IsInputReference(u.Operand))
                        {
                            this.Visit(u.Operand);
                        }
                    }

                    break;
                case ExpressionType.UnaryPlus:
                    // no-op always ignore.
                    break;
                default:
                    this.cantTranslateExpression = true;
                    break;
            }

            return u;
        }

        /// <summary>
        /// Serializes an BinaryExpression to a string
        /// </summary>
        /// <param name="b">BinaryExpression to serialize</param>
        /// <returns>serialized expression</returns>
        internal override Expression VisitBinary(BinaryExpression b)
        {
            this.VisitOperand(b.Left);
            this.builder.Append(UriHelper.SPACE);
            switch (b.NodeType)
            {
                case ExpressionType.AndAlso:
                case ExpressionType.And:
                    this.builder.Append(UriHelper.AND);
                    break;
                case ExpressionType.OrElse:
                case ExpressionType.Or:
                    this.builder.Append(UriHelper.OR);
                    break;
                case ExpressionType.Equal:
                    this.builder.Append(UriHelper.EQ);
                    break;
                case ExpressionType.NotEqual:
                    this.builder.Append(UriHelper.NE);
                    break;
                case ExpressionType.LessThan:
                    this.builder.Append(UriHelper.LT);
                    break;
                case ExpressionType.LessThanOrEqual:
                    this.builder.Append(UriHelper.LE);
                    break;
                case ExpressionType.GreaterThan:
                    this.builder.Append(UriHelper.GT);
                    break;
                case ExpressionType.GreaterThanOrEqual:
                    this.builder.Append(UriHelper.GE);
                    break;
                case ExpressionType.Add:
                case ExpressionType.AddChecked:
                    this.builder.Append(UriHelper.ADD);
                    break;
                case ExpressionType.Subtract:
                case ExpressionType.SubtractChecked:
                    this.builder.Append(UriHelper.SUB);
                    break;
                case ExpressionType.Multiply:
                case ExpressionType.MultiplyChecked:
                    this.builder.Append(UriHelper.MUL);
                    break;
                case ExpressionType.Divide:
                    this.builder.Append(UriHelper.DIV);
                    break;
                case ExpressionType.Modulo:
                    this.builder.Append(UriHelper.MOD);
                    break;
                case ExpressionType.ArrayIndex:
                case ExpressionType.Power:
                case ExpressionType.Coalesce:
                case ExpressionType.ExclusiveOr:
                case ExpressionType.LeftShift:
                case ExpressionType.RightShift:
                default:
                    this.cantTranslateExpression = true;
                    break;
            }

            this.builder.Append(UriHelper.SPACE);
            this.VisitOperand(b.Right);
            return b;
        }

        /// <summary>
        /// Serializes an TypeBinaryExpression to a string
        /// </summary>
        /// <param name="b">TypeBinaryExpression to serialize</param>
        /// <returns>serialized expression</returns>
        internal override Expression VisitTypeIs(TypeBinaryExpression b)
        {
            this.builder.Append(UriHelper.ISOF);
            this.builder.Append(UriHelper.LEFTPAREN);

            if (!IsInputReference(b.Expression))
            {
                this.Visit(b.Expression);
                this.builder.Append(UriHelper.COMMA);
                this.builder.Append(UriHelper.SPACE);
            }

            this.builder.Append(UriHelper.QUOTE);
            this.builder.Append(this.TypeNameForUri(b.TypeOperand));
            this.builder.Append(UriHelper.QUOTE);
            this.builder.Append(UriHelper.RIGHTPAREN);

            return b;
        }

        /// <summary>
        /// ParameterExpression visit method.
        /// </summary>
        /// <param name="p">The ParameterExpression expression to visit</param>
        /// <returns>The visited ParameterExpression expression </returns>
        internal override Expression VisitParameter(ParameterExpression p)
        {
            return p;
        }

        /// <summary>
        /// References to the current input - the resource set - do not appear in the URL.
        /// </summary>
        /// <param name="exp">The expression to test</param>
        /// <returns><c>true</c> if the expression represents a reference to the current (resource set) input; otherwise <c>false</c>.</returns>
        private static bool IsInputReference(Expression exp)
        {
            return (exp is InputReferenceExpression || exp is ParameterExpression);
        }

        /// <summary>Gets the type name to be used in the URI for the given <paramref name="type"/>.</summary>
        /// <param name="type">Type to get name for.</param>
        /// <returns>The name for the <paramref name="type"/>, suitable for including in a URI.</returns>
        private string TypeNameForUri(Type type)
        {
            Debug.Assert(type != null, "type != null");
            type = Nullable.GetUnderlyingType(type) ?? type;

            if (ClientConvert.IsKnownType(type))
            {
                if (ClientConvert.IsSupportedPrimitiveTypeForUri(type))
                {
                    return ClientConvert.ToTypeName(type);
                }

                // unsupported primitive type
                throw new NotSupportedException(Strings.ALinq_CantCastToUnsupportedPrimitive(type.Name));
            }
            else
            {
                return this.context.ResolveNameFromType(type) ?? type.FullName;
            }
        }

        /// <summary>
        /// Visits operands for Binary and Unary expressions.
        /// Will only output parens if operand is complex expression,
        /// this is so don't have unecessary parens in URI.
        /// </summary>
        /// <param name="e">The operand expression to visit</param>
        private void VisitOperand(Expression e)
        {
            if (e is BinaryExpression || e is UnaryExpression)
            {
                this.builder.Append(UriHelper.LEFTPAREN);
                this.Visit(e);
                this.builder.Append(UriHelper.RIGHTPAREN);
            }
            else
            {
                this.Visit(e);
            }
        }

        /// <summary>
        /// Serializes an expression to a string
        /// </summary>
        /// <param name="e">Expression to serialize</param>
        /// <returns>serialized expression</returns>
        private string Translate(Expression e)
        {
            this.Visit(e);
            return this.builder.ToString();
        }
    }
}
