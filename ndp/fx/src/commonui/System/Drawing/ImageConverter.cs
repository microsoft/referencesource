//------------------------------------------------------------------------------
// <copyright file="ImageConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.IO;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;
    using System.Drawing.Imaging;
    using System.ComponentModel.Design.Serialization;
    using System.Runtime.Versioning;

    /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter"]/*' />
    /// <devdoc>
    ///      ImageConverter is a class that can be used to convert
    ///      Image from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class ImageConverter : TypeConverter {

        Type iconType = typeof(Icon);
        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.CanConvertFrom1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object in the given source type to the native type of the converter
        ///       using the context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == iconType) {
                return true;
            }

            if (sourceType == typeof(byte[])) {
                return true;
            }

            if(sourceType == typeof(InstanceDescriptor)){
                return false;
            }
            
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.CanConvertTo1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(byte[])) {
                return true;
            }

            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given object to the converter's native type.</para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is Icon) {
                Icon icon = (Icon) value;
                return icon.ToBitmap();
            }

            byte[] bytes = value as byte[];

            if (bytes != null) {

                Stream memStream = null;

                // this might be a magical OLE thing, try that first.
                //
                memStream = GetBitmapStream(bytes);

                if (memStream == null) {
                    // guess not.  Try plain memory.
                    //
                    memStream = new MemoryStream(bytes);
                }

                // hopefully GDI+ knows what to do with this!
                //
                return Image.FromStream(memStream);
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        [SuppressMessage("Microsoft.Reliability", "CA2000:DisposeObjectsBeforeLosingScope")]
        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value != null) {
                    // should do something besides ToString() the image...go get the filename
                    Image image = (Image)value;
                    return image.ToString();
                }
                else {
                    return SR.GetString(SR.toStringNone);
                }
            }
            else if (destinationType == typeof(byte[])) {
                if (value != null) {
                    bool createdNewImage = false;
                    MemoryStream ms = null;
                    Image image = null;
                    try {
                        ms = new MemoryStream();
                    
                        image = (Image) value;
                        
                        //We don't want to serialize an icon - since we're not really working with
                        //icons, these are "Images".  So, we'll force a new and valid bitmap to be
                        //created around our icon with the ideal size.
                        if (image.RawFormat.Equals(ImageFormat.Icon)) {
                            createdNewImage = true;
                            image = new Bitmap(image, image.Width, image.Height);
                        }
                        
                        image.Save(ms);
                    }
                    finally {
                        if (ms != null) {
                            ms.Close();
                        }
                        if (createdNewImage && image != null) {
                            image.Dispose();
                        }
                    }                  

                    if (ms != null) {
                        return ms.ToArray();
                    }
                    else {
                        return null;
                    }
                }
                else {
                    return new byte[0];
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <devdoc>
        /// Try to get a bitmap out of a byte array.  This is an ole format that Access uses.
        /// this fails very quickly so we can try this first without a big perf hit.
        /// </devdoc>        
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]
        private unsafe Stream GetBitmapStream(byte[] rawData) 
        {
            Debug.Assert( rawData != null, "rawData is null." );
            try 
            {
                fixed (byte* pByte = rawData) 
                {
                    IntPtr addr = (IntPtr)pByte;

                    if (addr == IntPtr.Zero)
                    {
                        return null;
                    }

                    // this will be pHeader.signature, but we try to
                    // do this first so we avoid building the structure when we shouldn't
                    //
                    if (rawData.Length <= sizeof(SafeNativeMethods.OBJECTHEADER) || Marshal.ReadInt16(addr) != 0x1c15) 
                    {
                        return null;
                    }

                    // the data is one of these OBJECTHEADER dudes.  It's an encoded format that Access uses to push images
                    // into the DB.  It's not particularly documented, but I found a KB:
                    //
                    // http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q175261
                    //
                    SafeNativeMethods.OBJECTHEADER pHeader = (SafeNativeMethods.OBJECTHEADER)Marshal.PtrToStructure(addr, typeof(SafeNativeMethods.OBJECTHEADER));

                    // "PBrush" should be the 6 chars after position 12 as well.
                    //
                    if (rawData.Length <= pHeader.headersize + 18)
                    {
                        return null;
                    }
                    string strPBrush = System.Text.Encoding.ASCII.GetString(rawData, pHeader.headersize + 12, 6);

                    if (strPBrush != "PBrush") 
                    {
                        return null;
                    }
                
                    // okay, now we can safely trust that we've got a bitmap.
                    byte[] searchArray = System.Text.Encoding.ASCII.GetBytes("BM");
                
                    // search for "BM" in the data which is the start of our bitmap data...
                    //
                    // 18 is from (12+6) above.
                    //
                    for (int i = pHeader.headersize + 18; i < pHeader.headersize + 510 && i+1 < rawData.Length; i++) 
                    {
                        if (searchArray[0] == pByte[i] && 
                            searchArray[1] == pByte[i+1]) 
                        {
                            // found the bitmap data.
                            //
                            return new MemoryStream(rawData, i, rawData.Length - i);
                        }
                    
                    }
                }
            }
            catch (OutOfMemoryException)            // this exception may be caused by creating a new MemoryStream
            {
            }
            catch (ArgumentException)               // may be caused by Marshal.PtrToStructure
            {
            }
            // nevermind...
            return null;
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.GetProperties"]/*' />
        /// <devdoc>
        ///      Retrieves the set of properties for this type.  By default, a type has
        ///      does not return any properties.  An easy implementation of this method
        ///      can just call TypeDescriptor.GetProperties for the correct data type.
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            return TypeDescriptor.GetProperties(typeof(Image), attributes);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports properties.  By default, this
        ///      is false.
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

