//------------------------------------------------------------------------------
// <copyright file="ImageFormatConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;
    using System.Drawing.Imaging;

    /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter"]/*' />
    /// <devdoc>
    ///      ImageFormatConverter is a class that can be used to convert
    ///      colors from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class ImageFormatConverter : TypeConverter {
        private StandardValuesCollection values;
        
        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.ImageFormatConverter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ImageFormatConverter() {
        }
        
        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.CanConvertFrom"]/*' />
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

        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.CanConvertTo"]/*' />
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
        
        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {

            string strValue = value as string;

            if (strValue != null) {

                string text = strValue.Trim();
                PropertyInfo[] props = GetProperties();
                for (int i = 0; i < props.Length; i++) {
                    PropertyInfo prop = props[i];
                    if (string.Equals(prop.Name, text, StringComparison.OrdinalIgnoreCase))
                    {
                        object[] tempIndex = null;
                        return prop.GetValue(null, tempIndex);
                    }
                }
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (value is ImageFormat) {
                PropertyInfo targetProp = null;
                
                PropertyInfo[] props = GetProperties();
                foreach(PropertyInfo p in props) {
                    if (p.GetValue(null, null).Equals(value)) {
                        targetProp = p;
                        break;
                    }
                }
                
                if (targetProp != null) {
                    if (destinationType == typeof(string)) {
                        return targetProp.Name;
                    }
                    else if (destinationType == typeof(InstanceDescriptor)) {
                        return new InstanceDescriptor(targetProp, null);
                    }
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.GetProperties"]/*' />
        /// <devdoc>
        ///      Retrieves the properties for the available image formats.
        /// </devdoc>
        private PropertyInfo[] GetProperties() {
            return typeof(ImageFormat).GetProperties(BindingFlags.Static | BindingFlags.Public);
        }

        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///      Retrieves a collection containing a set of standard values
        ///      for the data type this validator is designed for.  This
        ///      will return null if the data type does not support a
        ///      standard set of values.
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {
                ArrayList list = new ArrayList();
                PropertyInfo[] props = GetProperties();
                for (int i = 0; i < props.Length; i++) {
                    PropertyInfo prop = props[i];
                    object[] tempIndex = null;
                    Debug.Assert(prop.GetValue(null, tempIndex) != null, "Property " + prop.Name + " returned NULL");
                    list.Add(prop.GetValue(null, tempIndex));
                }

                values = new StandardValuesCollection(list.ToArray());
            }

            return values;
        }

        /// <include file='doc\ImageFormatConverter.uex' path='docs/doc[@for="ImageFormatConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports a standard set of values
        ///      that can be picked from a list.
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}
