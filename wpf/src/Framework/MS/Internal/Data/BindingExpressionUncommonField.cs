//---------------------------------------------------------------------------
//
// <copyright file="BindingExpressionUncommonField.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Defines an UncommonField of type BindingExpression.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Data;

namespace MS.Internal.Data
{
    /// <summary>
    /// An UncommonField whose type is BindingExpression.
    /// </summary>
    internal class BindingExpressionUncommonField : UncommonField<BindingExpression>
    {
        internal new void SetValue(DependencyObject instance, BindingExpression bindingExpr)
        {
            base.SetValue(instance, bindingExpr);
            bindingExpr.Attach(instance);
        }

        internal new void ClearValue(DependencyObject instance)
        {
            BindingExpression bindingExpr = GetValue(instance);
            if (bindingExpr != null)
            {
                bindingExpr.Detach();
            }
            base.ClearValue(instance);
        }
    }
}
