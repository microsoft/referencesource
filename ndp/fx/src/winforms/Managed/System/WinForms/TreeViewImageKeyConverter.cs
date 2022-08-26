//------------------------------------------------------------------------------
// <copyright file="ImageIndexConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    using System.Collections.Specialized;

    /// <include file='doc\TreeViewImageKeyConverter.uex' path='docs/doc[@for="TreeViewImageKeyConverter"]/*' />
    /// <devdoc>
    /// ImageIndexConverter is a class that can be used to convert
    /// image index values one data type to another.
    /// </devdoc>
    public class TreeViewImageKeyConverter : ImageKeyConverter {

        /// <include file='doc\TreeViewImageKeyConverter.uex' path='docs/doc[@for="TreeViewImageKeyConverter.ConvertTo"]/*' />
        /// <devdoc>
        /// Converts the given object to another type.  The most common types to convert
        /// are to and from a string object.  The default implementation will make a call
        /// to ToString on the object if the object is valid and if the destination
        /// type is string.  If this cannot convert to the desitnation type, this will
        /// throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }
                
            if (destinationType == typeof(string) && (value == null)) {
                return SR.GetString(SR.toStringDefault);
            }
            else {
                string strValue = value as string;
                if (strValue != null && (strValue.Length == 0)) {
                    return SR.GetString(SR.toStringDefault);
                }
            }

	    return base.ConvertTo(context, culture, value, destinationType);
        }

    }
}

