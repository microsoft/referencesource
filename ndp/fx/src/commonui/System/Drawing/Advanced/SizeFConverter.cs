//------------------------------------------------------------------------------
// <copyright file="SizeFConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;
    using System.Diagnostics.CodeAnalysis;

    /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter"]/*' />
    /// <devdoc>
    ///      SizeFConverter is a class that can be used to convert
    ///      SizeF from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class SizeFConverter : TypeConverter {
    
        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///      Determines if this converter can convert an object in the given source
        ///      type to the native type of the converter.
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.CanConvertTo"]/*' />
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

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.ConvertFrom"]/*' />
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
                
                    // Parse 2 integer values.
                    //
                    if (culture == null) {
                        culture = CultureInfo.CurrentCulture;
                    }
                    char sep = culture.TextInfo.ListSeparator[0];
                    string[] tokens = text.Split(new char[] {sep});
                    float[] values = new float[tokens.Length];
                    TypeConverter floatConverter = TypeDescriptor.GetConverter(typeof(float));
                    for (int i = 0; i < values.Length; i++) {
                        values[i] = (float)floatConverter.ConvertFromString(context, culture, tokens[i]);
                    }
                    
                    if (values.Length == 2) {
                        return new SizeF(values[0], values[1]);
                    }
                    else {
                        throw new ArgumentException(SR.GetString(SR.TextParseFailedFormat,
                                                                  text,
                                                                  "Width,Height"));
                    }
                }
            }
            
            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.ConvertTo"]/*' />
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

            if (destinationType == typeof(string) && value is SizeF) {
                SizeF size = (SizeF)value;

                if (culture == null) {
                    culture = CultureInfo.CurrentCulture;
                }
                string sep = culture.TextInfo.ListSeparator + " ";
                TypeConverter floatConverter = TypeDescriptor.GetConverter(typeof(float));
                string[] args = new string[2];
                int nArg = 0;
                
                args[nArg++] = floatConverter.ConvertToString(context, culture, size.Width);
                args[nArg++] = floatConverter.ConvertToString(context, culture, size.Height);
                
                return string.Join(sep, args);
            }
            if (destinationType == typeof(InstanceDescriptor) && value is SizeF) {
                SizeF size = (SizeF)value;

                ConstructorInfo ctor = typeof(SizeF).GetConstructor(new Type[] {typeof(float), typeof(float)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {size.Width, size.Height});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.CreateInstance"]/*' />
        /// <devdoc>
        ///      Creates an instance of this type given a set of property values
        ///      for the object.  This is useful for objects that are immutable, but still
        ///      want to provide changable properties.
        /// </devdoc>
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]        
        public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
            return new SizeF((float)propertyValues["Width"],
                            (float)propertyValues["Height"]);
        }

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.GetCreateInstanceSupported"]/*' />
        /// <devdoc>
        ///      Determines if changing a value on this object should require a call to
        ///      CreateInstance to create a new value.
        /// </devdoc>
        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
            return true;
        }

        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.GetProperties"]/*' />
        /// <devdoc>
        ///      Retrieves the set of properties for this type.  By default, a type has
        ///      does not return any properties.  An easy implementation of this method
        ///      can just call TypeDescriptor.GetProperties for the correct data type.
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(typeof(SizeF), attributes);
            return props.Sort(new string[] {"Width", "Height"});
        }

       
        /// <include file='doc\SizeFConverter.uex' path='docs/doc[@for="SizeFConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports properties.  By default, this
        ///      is false.
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

