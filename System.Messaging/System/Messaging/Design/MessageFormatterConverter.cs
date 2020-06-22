//------------------------------------------------------------------------------
// <copyright file="MessageFormatterConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Design
{
    using System.Messaging;
    using System.Threading;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Reflection;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Messaging.Interop;
    using System.Collections;
    using System.Runtime.Serialization.Formatters;
    using System.Globalization;

    /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter"]/*' />
    /// <internalonly/>
    internal class MessageFormatterConverter : ExpandableObjectConverter
    {

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            return (sourceType == typeof(string));
        }

        /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(InstanceDescriptor))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter.ConvertFrom"]/*' />
        /// <internalonly/>                                                  
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value != null && value is string)
            {
                if (((string)value) == typeof(ActiveXMessageFormatter).Name)
                    return new ActiveXMessageFormatter();
                if (((string)value) == typeof(BinaryMessageFormatter).Name)
                    return new BinaryMessageFormatter();
                if (((string)value) == typeof(XmlMessageFormatter).Name)
                    return new XmlMessageFormatter();
            }

            return null;
        }

        /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter.ConvertTo"]/*' />
        /// <internalonly/>                 
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType != null && destinationType == typeof(string))
            {
                if (value == null)
                    return Res.GetString(Res.toStringNone);

                return value.GetType().Name;
            }
            if (destinationType == typeof(InstanceDescriptor))
            {
                if (value is XmlMessageFormatter)
                {
                    XmlMessageFormatter f = (XmlMessageFormatter)value;
                    ConstructorInfo ctor = typeof(XmlMessageFormatter).GetConstructor(new Type[] { typeof(string[]) });
                    if (ctor != null)
                    {
                        return new InstanceDescriptor(ctor, new object[] { f.TargetTypeNames });
                    }
                }
                else if (value is ActiveXMessageFormatter)
                {
                    ConstructorInfo ctor = typeof(ActiveXMessageFormatter).GetConstructor(new Type[0]);
                    if (ctor != null)
                    {
                        return new InstanceDescriptor(ctor, new object[0]);
                    }
                }
                else if (value is BinaryMessageFormatter)
                {
                    BinaryMessageFormatter f = (BinaryMessageFormatter)value;
                    ConstructorInfo ctor = typeof(BinaryMessageFormatter).GetConstructor(new Type[] {
                        typeof(FormatterAssemblyStyle), typeof(FormatterTypeStyle) });

                    if (ctor != null)
                    {
                        return new InstanceDescriptor(ctor, new object[] { f.TopObjectFormat, f.TypeFormat });
                    }
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter.GetStandardValues"]/*' />
        /// <internalonly/>            
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            StandardValuesCollection values = new StandardValuesCollection(new object[] { new ActiveXMessageFormatter(), 
                                                                                                           new BinaryMessageFormatter(),                                                                                                            
                                                                                                           new XmlMessageFormatter(),
                                                                                                           null });

            return values;
        }

        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
        {
            return true;
        }

        /// <include file='doc\MessageFormatterConverter.uex' path='docs/doc[@for="MessageFormatterConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>                        
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
        {
            return true;
        }
    }
}
