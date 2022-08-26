//------------------------------------------------------------------------------
// <copyright file="MarginsConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;

    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a
    ///       type converter to convert <see cref='System.Drawing.Printing.Margins'/> to and from various other representations, such as a string.</para>
    /// </devdoc>
    public class MarginsConverter : ExpandableObjectConverter {

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Determines if a converter can convert an object of the given source
        ///       type to the native type of the converter.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>        
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {

            string strValue = value as string;

            if (strValue != null) {

                string text = strValue.Trim();

                if (text.Length == 0) {
                    return null;
                }
                else {

                    // Parse 4 integer values.
                    //
                    if (culture == null) {
                        culture = CultureInfo.CurrentCulture;
                    }
                    char sep = culture.TextInfo.ListSeparator[0];
                    string[] tokens = text.Split(new char[] {sep});
                    int[] values = new int[tokens.Length];
                    TypeConverter intConverter = TypeDescriptor.GetConverter(typeof(int));
                    for (int i = 0; i < values.Length; i++) {
                        // Note: ConvertFromString will raise exception if value cannot be converted.
                        values[i] = (int)intConverter.ConvertFromString(context, culture, tokens[i]);
                    }

                    if (values.Length != 4) {
                        throw new ArgumentException(SR.GetString(SR.TextParseFailedFormat,
                                                                  text,
                                                                  "left, right, top, bottom"));
                    }
                    return new Margins(values[0], values[1], values[2], values[3]);
                }
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        [SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if( value is Margins ){
                if (destinationType == typeof(string)) {
                    Margins margins = (Margins)value;

                    if (culture == null) {
                        culture = CultureInfo.CurrentCulture;
                    }
                    string sep = culture.TextInfo.ListSeparator + " ";
                    TypeConverter intConverter = TypeDescriptor.GetConverter(typeof(int));
                    string[] args = new string[4];
                    int nArg = 0;

                    // Note: ConvertToString will raise exception if value cannot be converted.
                    args[nArg++] = intConverter.ConvertToString(context, culture, margins.Left);
                    args[nArg++] = intConverter.ConvertToString(context, culture, margins.Right);
                    args[nArg++] = intConverter.ConvertToString(context, culture, margins.Top);
                    args[nArg++] = intConverter.ConvertToString(context, culture, margins.Bottom);

                    return string.Join(sep, args);
                }
                if (destinationType == typeof(InstanceDescriptor)) {
                    Margins margins = (Margins)value;
                    
                    ConstructorInfo ctor = typeof(Margins).GetConstructor(new Type[] {
                        typeof(int), typeof(int), typeof(int), typeof(int)});
                        
                    if (ctor != null) {
                        return new InstanceDescriptor(ctor, new object[] {
                            margins.Left, margins.Right, margins.Top, margins.Bottom});
                    }
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.CreateInstance"]/*' />
        /// <devdoc>
        ///      Creates an instance of this type given a set of property values
        ///      for the object.  This is useful for objects that are immutable, but still
        ///      want to provide changable properties.
        /// </devdoc>        
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]
        [SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers")]        
        public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
            if( propertyValues == null ){
                throw new ArgumentNullException("propertyValues");
            }

            object left = propertyValues["Left"];
            object right = propertyValues["Right"];
            object top = propertyValues["Top"];
            object bottom = propertyValues["Bottom"];            

            if(left == null || right == null || bottom == null || top == null ||
                !(left is int) || !(right is int) || !(bottom is int) || !(top is int)) {
                    throw new ArgumentException(SR.GetString(SR.PropertyValueInvalidEntry));
            }

            return new Margins((int)left,
                                (int)right,
                                (int)top,
                                (int)bottom);
        }

        /// <include file='doc\MarginsConverter.uex' path='docs/doc[@for="MarginsConverter.GetCreateInstanceSupported"]/*' />
        /// <devdoc>
        ///      Determines if changing a value on this object should require a call to
        ///      CreateInstance to create a new value.
        /// </devdoc>
        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

