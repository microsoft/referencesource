//------------------------------------------------------------------------------
// <copyright file="SizeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Design
{
    using System.Messaging;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design.Serialization;
    using System.Reflection;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\SizeConverter.uex' path='docs/doc[@for="SizeConverter"]/*' />
    internal class SizeConverter : TypeConverter
    {

        /// <include file='doc\SizeConverter.uex' path='docs/doc[@for="SizeConverter.SizeConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///      Determines if this converter can convert an object in the given source
        ///      type to the native type of the converter.
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\SizeConverter.uex' path='docs/doc[@for="SizeConverter.SizeConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {

            if (value is string)
            {

                string text = ((string)value).Trim();

                if (text.Length == 0 || string.Compare(text, Res.GetString(Res.InfiniteValue), true, CultureInfo.CurrentCulture) == 0)
                    return (long)uint.MaxValue;
                else
                {
                    long size = Convert.ToInt64(text, culture);
                    return size;
                }

            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\SizeConverter.uex' path='docs/doc[@for="SizeConverter.SizeConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == null)
            {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string))
            {
                if (value != null)
                {
                    if ((long)value == uint.MaxValue)
                        return Res.GetString(Res.InfiniteValue);
                    else
                        return value.ToString();
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

