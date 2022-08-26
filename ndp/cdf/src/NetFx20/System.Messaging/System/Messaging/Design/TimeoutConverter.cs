//------------------------------------------------------------------------------
// <copyright file="TimeoutConverter.cs" company="Microsoft">
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

    /// <include file='doc\TimeoutConverter.uex' path='docs/doc[@for="TimeoutConverter"]/*' />
    /// <devdoc>
    ///      TimeoutConverter is a class that can be used to convert
    ///      Timeout from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    internal class TimeoutConverter : TypeConverter
    {

        /// <include file='doc\TimeoutConverter.uex' path='docs/doc[@for="TimeoutConverter.CanConvertFrom"]/*' />
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

        /// <include file='doc\TimeoutConverter.uex' path='docs/doc[@for="TimeoutConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {

            if (value is string)
            {
                string text = ((string)value).Trim();
                if (text.Length == 0 || (string.Compare(text, Res.GetString(Res.InfiniteValue), true, CultureInfo.CurrentCulture) == 0))
                    return TimeSpan.FromSeconds((double)uint.MaxValue);
                else
                {
                    double totalSeconds = Convert.ToDouble(text, culture);
                    if (totalSeconds > (double)uint.MaxValue)
                        totalSeconds = (double)uint.MaxValue;

                    return TimeSpan.FromSeconds(totalSeconds);
                }
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\TimeoutConverter.uex' path='docs/doc[@for="TimeoutConverter.ConvertTo"]/*' />
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
                    double totalSeconds = ((TimeSpan)value).TotalSeconds;
                    if (totalSeconds >= uint.MaxValue)
                        return Res.GetString(Res.InfiniteValue);
                    else
                        return ((uint)totalSeconds).ToString(culture);
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
