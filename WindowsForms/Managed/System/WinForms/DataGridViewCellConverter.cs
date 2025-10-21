
//------------------------------------------------------------------------------
// <copyright file="DataGridViewCell.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Windows.Forms;
    using System.Globalization;
    using System.ComponentModel;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel.Design.Serialization;

    // used by the designer to serialize the DataGridViewCell class
    internal class DataGridViewCellConverter : ExpandableObjectConverter {
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }

            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            DataGridViewCell cell = value as DataGridViewCell;
            if (destinationType == typeof(InstanceDescriptor) && cell != null) {                
                ConstructorInfo ctor = cell.GetType().GetConstructor(new Type[0]);
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[0], false);
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
