/***************************************************************************\
*
* File: TemplateBindingExpressionConverter.cs
*
*  Class for serializing a TemplateBindingExpression.
*
* Copyright (C) 2005 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Data;
using System.Windows.Markup;

namespace System.Windows
{


    /// <summary>
    /// Converts a template binding expression into a MarkupExtension.  This is used
    /// during serialization (the serializer native knows how to serialize an ME).
    /// </summary>
    public class TemplateBindingExpressionConverter: TypeConverter
    {
        /// <summary>
        /// Returns true for MarkupExtension
        /// </summary>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(MarkupExtension))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <summary>
        /// Converts to a MarkupExtension
        /// </summary>
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(MarkupExtension))
            {
                TemplateBindingExpression templateBindingExpression = value as TemplateBindingExpression;
                if (templateBindingExpression == null)
                    throw new ArgumentException(SR.Get(SRID.MustBeOfType, "value", "TemplateBindingExpression"));
                return templateBindingExpression.TemplateBindingExtension;
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }


}


