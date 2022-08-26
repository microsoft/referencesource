//---------------------------------------------------------------------
// <copyright file="PropertyAccessVisitor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Visitor to detect and replace access to properties
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Internal
{
    using System.Linq.Expressions;
    using System.Diagnostics;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Data.Services.Providers;

    /// <summary>Expression visitor class which provides recognission of property access</summary>
    /// <remarks>This class understands all the different ways WCF Data Services can use
    /// to access a property value in the generated query.
    /// The class is meant to be inherited for expression tree processing where
    /// property accesses are interesting.
    /// Note that it assumes that the expression tree looks the way WCF Data Services generate it.
    /// Noticable that all the GetValue method calls get the property parameter as a 
    /// <see cref="ConstantExpression"/> node. This may mean that if there were some rewrites applied
    /// to the tree, the class may not recognize the property accesses anymore.</remarks>
    internal abstract class PropertyAccessVisitor : ALinqExpressionVisitor
    {
        /// <summary>MethodCallExpression visit method</summary>
        /// <param name="m">The MethodCallExpression expression to visit</param>
        /// <returns>The visited MethodCallExpression expression </returns>
        internal override Expression VisitMethodCall(MethodCallExpression m)
        {
            string propertyName = null;
            ResourceProperty resourceProperty = null;

            if ((m.Method.IsGenericMethod &&
                m.Method.GetGenericMethodDefinition() == DataServiceProviderMethods.GetSequenceValueMethodInfo) ||
                (m.Method == DataServiceProviderMethods.GetValueMethodInfo))
            {
                // DataServiceProviderMethods.GetSequenceValue(object value, ResourceProperty property)
                // or
                // DataServiceProviderMethods.GetValue(object value, ResourceProperty property)
                ConstantExpression propertyExpression = m.Arguments[1] as ConstantExpression;
                Debug.Assert(
                    propertyExpression != null && propertyExpression.Value is ResourceProperty, 
                    "We should have only calls with constant ResourceProperty values.");
                resourceProperty = propertyExpression.Value as ResourceProperty;
                propertyName = resourceProperty.Name;
            }
            else if (m.Method == OpenTypeMethods.GetValueOpenPropertyMethodInfo)
            {
                // OpenTypeMethods.GetValue(object value, string propertyName)
                ConstantExpression propertyExpression = m.Arguments[1] as ConstantExpression;
                Debug.Assert(
                    propertyExpression != null && propertyExpression.Value is string, 
                    "We should have only calls with constant string values.");
                propertyName = propertyExpression.Value as string;
            }

            if (propertyName != null)
            {
                Expression operand = m.Arguments[0];
                Expression result = m;
                if (this.ProcessPropertyAccess(propertyName, ref operand, ref result))
                {
                    if (result == null)
                    {
                        Debug.Assert(
                            m.Arguments.Count == 2, 
                            "Right now all methods have 2 arguments. If it ever changes this code needs to be modified.");
                        return Expression.Call(m.Object, m.Method, operand, m.Arguments[1]);
                    }
                    else
                    {
                        return result;
                    }
                }
            }

            return base.VisitMethodCall(m);
        }

        /// <summary>
        /// MemberExpression visit method
        /// </summary>
        /// <param name="m">The MemberExpression expression to visit</param>
        /// <returns>The visited MemberExpression expression </returns>
        internal override Expression VisitMemberAccess(MemberExpression m)
        {
            if (m.Member.MemberType == MemberTypes.Property)
            {
                Expression operand = m.Expression;
                Expression result = m;
                if (this.ProcessPropertyAccess(m.Member.Name, ref operand, ref result))
                {
                    if (result == null)
                    {
                        return Expression.MakeMemberAccess(operand, m.Member);
                    }
                    else
                    {
                        return result;
                    }
                }
            }

            return base.VisitMemberAccess(m);
        }

        /// <summary>Dervied class will override them method to process any property accesses found in the tree.</summary>
        /// <param name="propertyName">The name of the property being accessed.</param>
        /// <param name="operandExpression">The expression on which the property is being accessed.
        /// The implementation may choose to return a different expression through this ref parameter.
        /// If the method returns true, the <paramref name="accessExpression"/> is null and the method
        /// changed this parameter, the caller will replace the operand in the original property
        /// access with the new expression provided in this parameter. The way the property is accessed
        /// and its name remains the same.</param>
        /// <param name="accessExpression">The entire expression of the property access.
        /// The implementation may choose to return a different expression through this ref parameter.
        /// If the method returns true and this parameter is not null the caller will replace the entire
        /// property access expression with the new one passed in this parameter.</param>
        /// <returns>If the method returns false it means that it is not interested in this property access,
        /// and the processing of the tree will continue by examining the children of the property access expression.
        /// If the method returns true the caller looks at the returned value of <paramref name="accessExpression"/>.
        /// If it is not-null it will replace the entire property access expression with it.
        /// If it's null it will just replace the operand of the property access with the <paramref name="operandExpression"/>.
        /// If the implementation wants to skip this property access without modification it should return true
        /// and not modify the ref parameters.</returns>
        /// <remarks>If the method returns true the caller will not continue walking the children of the property
        /// access expression. It's the responsibility of the implementation to do so if it requires such
        /// functionality.</remarks>
        protected abstract bool ProcessPropertyAccess(
            string propertyName,
            ref Expression operandExpression,
            ref Expression accessExpression);
    }
}
