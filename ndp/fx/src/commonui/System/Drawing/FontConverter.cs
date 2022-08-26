//------------------------------------------------------------------------------
// <copyright file="FontConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable", Scope="type", Target="System.Drawing.FontConverter")]

namespace System.Drawing {
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
    using System.Collections.Generic;
    using System.Runtime.Versioning;

    /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter"]/*' />
    /// <devdoc>
    ///      FontConverter is a class that can be used to convert
    ///      fonts from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class FontConverter : TypeConverter {

        private FontNameConverter fontNameConverter;

        private const string styleHdr = "style=";
        
        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.~FontConverter"]/*' />
        /// <devdoc>
        ///     Destructor
        /// </devdoc>
        ~FontConverter() {
            if (fontNameConverter != null) {
                ((IDisposable)fontNameConverter).Dispose();
            }
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.CanConvertFrom"]/*' />
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

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.CanConvertTo"]/*' />
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

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>        
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]
        [SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers")]        
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            string strValue = value as string;

            if (strValue != null) {

                string text = strValue.Trim();

                // Expected string format: "name[, size[, units[, style=style1[, style2[...]]]]]"
                // Example using 'vi-VN' culture: "Microsoft Sans Serif, 8,25pt, style=Italic, Bold"
                
                if (text.Length == 0) {
                    return null;
                }
                else {
                    if (culture == null) {
                        culture = CultureInfo.CurrentCulture;
                    } 
                                       
                    char sep = culture.TextInfo.ListSeparator[0];   // For vi-VN: ','
                    string name = text; // start with the assumption that only the font name was provided.
                    string styleStr = null;
                    string sizeStr  = null;
                    float size = 8.25f;
                    FontStyle style = FontStyle.Regular;
                    GraphicsUnit units =  GraphicsUnit.Point;

                    // Get the index of the first separator (would indicate the end of the name in the string).
                    int nameIndex = text.IndexOf(sep); 

                    if( nameIndex > 0 ){ // some parameters provided in addtion to name.
                        name = text.Substring(0, nameIndex);

                        if( nameIndex < text.Length - 1 ){ // other parameters provided.
                            // Get the style index (if any).  The size is a bit problematic because it can be formatted differently 
                            // depending on the culture, we'll parse it last.
                            int styleIndex = text.IndexOf(styleHdr);
                
                            if (styleIndex != -1) { // style found.
                                styleStr = text.Substring(styleIndex, text.Length - styleIndex);
                        
                                // Expected style format ~ "style=Italic, Bold"
                                if (!styleStr.StartsWith(styleHdr)){
                                    throw GetFormatException(text, sep);
                                }

                                // Get the mid-substring containing the size information.
                                sizeStr = text.Substring(nameIndex + 1, styleIndex - nameIndex - 1);
                            }
                            else { // no style.
                                sizeStr = text.Substring(nameIndex + 1, text.Length - nameIndex - 1);
                            }

                            // Parse size.
                            string[] unitTokens = ParseSizeTokens(sizeStr, sep);

                            if (unitTokens[0] != null) {
                                try{
                                    size = (float)TypeDescriptor.GetConverter(typeof(float)).ConvertFromString(context, culture, unitTokens[0]); 
                                }
                                catch{  // Exception from converter is too generic.
                                    throw GetFormatException(text, sep);
                                }
                            }

                            if (unitTokens[1] != null) {
                                // ParseGraphicsUnits throws an ArgumentException if format is invalid.
                                units = ParseGraphicsUnits(unitTokens[1]);
                            }

                            if (styleStr != null) {
                                int eqIndex = styleStr.IndexOf("=");
                                styleStr = styleStr.Substring(eqIndex + 1, styleStr.Length - styleHdr.Length);
                                string[] styleTokens = styleStr.Split(sep);

                                for (int tokenCount = 0; tokenCount < styleTokens.Length; tokenCount++) {
                                    string styleText = styleTokens[tokenCount];
                                    styleText = styleText.Trim();
                                    // Note: Enum.Parse will throw InvalidEnumArgumentException if the styleText is not valid but could
                                    // throw other exceptions depending on the string passed.
                                    try{
                                        style |= (FontStyle)Enum.Parse(typeof(FontStyle), styleText, true);
                                    }
                                    catch(Exception ex ){
                                        if( ex is InvalidEnumArgumentException ){
                                            throw;
                                        }
                                        throw GetFormatException(text, sep);
                                    }

                                    // Enum.IsDefined doesn't do what we want on flags enums...
                                    FontStyle validBits = FontStyle.Regular | FontStyle.Bold | FontStyle.Italic | FontStyle.Underline | FontStyle.Strikeout;
                                    if ((style | validBits) != validBits){
                                        throw new InvalidEnumArgumentException("style", (int)style, typeof(FontStyle));
                                    }
                                }
                            }
                        }
                    }

                    if (fontNameConverter == null) {
                        fontNameConverter = new FontNameConverter();
                    }

                    // should get cached version from TypeDescriptor                                                                                                
                    name = (string)(fontNameConverter.ConvertFrom(context, culture, name));
                    
                    // Name is the only parameter not validated, if it is invalid a default Font will be created.
                    return new Font(name, size, style, units);
                }
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.ConvertTo"]/*' />
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

            if (destinationType == typeof(string)) {
                // Convert to a string following: "name[, size[, units[, style=<style1[, style2[...]]>]]]"
                // Example using 'vi-VN' culture: "Microsoft Sans Serif, 8,25pt, style=Italic"

                Font font = value as Font;

                if (font == null) {
                    return SR.GetString(SR.toStringNone);
                }
                else {
                    if (culture == null) {
                        culture = CultureInfo.CurrentCulture;
                    }  
                    
                    string sep = culture.TextInfo.ListSeparator + " "; // For vi-VN: ','
                    
                    int argCount = 2;
                    if (font.Style != FontStyle.Regular){
                        argCount++;
                    }
                    string[] args = new string[argCount];
                    int nArg = 0;

                    // should go through type converters here -- we already need
                    // converts for Name, Size and Units.
                    // Add name.
                    args[nArg++] = font.Name;
                    
                    // Add size and units.
                    // Note: ConvertToString will raise exception if value cannot be converted.
                    args[nArg++] = TypeDescriptor.GetConverter(font.Size).ConvertToString(context, culture, font.Size) + GetGraphicsUnitText(font.Unit);

                    if (font.Style != FontStyle.Regular){
                        // Add style.
                        args[nArg++] = styleHdr + font.Style.ToString("G");
                    }

                    return string.Join(sep, args);
                }
            }
            if (destinationType == typeof(InstanceDescriptor) && value is Font) {
                
                Font font = (Font)value;
                
                // Custom font, not derived from any stock font
                //
                int argCount = 2;

                if (font.GdiVerticalFont) {
                    argCount = 6;
                }
                else if (font.GdiCharSet != SafeNativeMethods.DEFAULT_CHARSET) {
                    argCount = 5;
                }
                else if (font.Unit != GraphicsUnit.Point) {
                    argCount = 4;
                }
                else if (font.Style != FontStyle.Regular) {
                    argCount++;
                }

                object[] args = new object[argCount];
                Type[] types = new Type[argCount];

                // Always specifying the eight parameter constructor is nastily confusing.
                // Use as simple a constructor as possible.
                //
                args[0] = font.Name; types[0] = typeof(string);
                args[1] = font.Size; types[1] = typeof(float);
                
                if (argCount > 2) {
                    args[2] = font.Style; types[2] = typeof(FontStyle);
                }

                if (argCount > 3) {
                    args[3] = font.Unit; types[3] = typeof(GraphicsUnit);
                }
                
                if (argCount > 4) {
                    args[4] = font.GdiCharSet; types[4] = typeof(byte);
                }
                
                if (argCount > 5) {
                    args[5] = font.GdiVerticalFont; types[5] = typeof(bool);
                }
                
                MemberInfo ctor = typeof(Font).GetConstructor(types);
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, args);
                }
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.CreateInstance"]/*' />
        /// <devdoc>
        ///      Creates an instance of this type given a set of property values
        ///      for the object.  This is useful for objects that are immutable, but still
        ///      want to provide changable properties.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
            if( propertyValues == null ){
                throw new ArgumentNullException("propertyValues");
            }

            object name       = propertyValues["Name"];
            object size       = propertyValues["Size"];
            object units      = propertyValues["Unit"];
            object bold       = propertyValues["Bold"];
            object italic     = propertyValues["Italic"];
            object strikeout  = propertyValues["Strikeout"];
            object underline  = propertyValues["Underline"];
            object gdiCharSet = propertyValues["GdiCharSet"];
            object gdiVerticalFont = propertyValues["GdiVerticalFont"];

            // If any of these properties are null, it may indicate a change in font that
            // was not propgated to FontConverter.
            //
            Debug.Assert(name != null && size != null && units != null && 
                         bold != null && italic != null && strikeout != null && gdiCharSet != null && 
                         underline != null, "Missing font properties.  Did Font change without FontConverter getting updated?");

            // Check for null param values and in that case set param to default value.

            if (name == null){
                name = "Tahoma";
            }
            if (size == null){
                size = 8.0f;
            }
            if (units == null){
                units = GraphicsUnit.Point;
            }
            if( bold == null ) {
                bold = false;
            }
            if( italic == null ) {
                italic = false;
            }
            if( strikeout == null ) {
                strikeout = false;
            }
            if( underline == null ) {
                underline = false;
            }
            if (gdiCharSet == null) {
                gdiCharSet = (byte)0;
            } 
            if (gdiVerticalFont == null){
                gdiVerticalFont = false;
            }

            // now test param types.

            if( !(  name is string &&
                    size is float &&
                    gdiCharSet is byte &&
                    units is GraphicsUnit &&
                    bold is bool &&
                    italic is bool &&
                    strikeout is bool &&
                    underline is bool &&
                    gdiVerticalFont is bool )) {
                throw new ArgumentException( SR.GetString( SR.PropertyValueInvalidEntry ) );    
            }

            FontStyle style = 0;
            if( bold != null && ( (bool) bold ) ) {
                style |= FontStyle.Bold;
            }
            if( italic != null && ( (bool) italic ) ) {
                style |= FontStyle.Italic;
            }
            if( strikeout != null && ( (bool) strikeout ) ) {
                style |= FontStyle.Strikeout;
            }
            if( underline != null && ( (bool) underline ) ) {
                style |= FontStyle.Underline;
            }

            return new Font( (string) name,
                (float) size,
                style,
                (GraphicsUnit) units,
                (byte) gdiCharSet,
                (bool) gdiVerticalFont );
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.GetCreateInstanceSupported"]/*' />
        /// <devdoc>
        ///      Determines if changing a value on this object should require a call to
        ///      CreateInstance to create a new value.
        /// </devdoc>
        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
            return true;
        }

        private ArgumentException GetFormatException(string text, char separator){
            string fonstStringFormat = string.Format(CultureInfo.CurrentCulture, "name{0} size[units[{0} style=style1[{0} style2{0} ...]]]", separator);
            return new ArgumentException(SR.GetString(SR.TextParseFailedFormat, text, fonstStringFormat));
        }
        
        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.GetGraphicsUnitText"]/*' />
        /// <devdoc>
        ///     Returns a text description for the font units
        /// </devdoc>
        private string GetGraphicsUnitText(GraphicsUnit units) {
            string unitStr = "";

            for (int i = 0; i < UnitName.names.Length; i++) {
                if (UnitName.names[i].unit == units) {
                    unitStr = UnitName.names[i].name;
                    break;
                }
            }
            return unitStr;
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.GetProperties"]/*' />
        /// <devdoc>
        ///      Retrieves the set of properties for this type.  By default, a type has
        ///      does not return any properties.  An easy implementation of this method
        ///      can just call TypeDescriptor.GetProperties for the correct data type.
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(typeof(Font), attributes);
            return props.Sort(new string[] {"Name", "Size", "Unit", "Weight"});
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports properties.  By default, this
        ///      is false.
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.ParseSizeTokens"]/*' />
        /// <devdoc>
        ///      Takes a string of the format ####.##CC and parses it into two strings.
        /// </devdoc>
        private string[] ParseSizeTokens(string text, char separator) {
            string size = null;
            string units = null;

            text = text.Trim();
            
            int length = text.Length;
            int splitPoint;

            if( length > 0){
                // text is expected to have a format like " 8,25pt, ". Leading and trailing spaces (trimmed above), 
                // last comma, unit and decimal value may not appear.  We need to make it ####.##CC
                for (splitPoint = 0; splitPoint < length; splitPoint++) {
                    if (Char.IsLetter(text[splitPoint])) {
                        break;
                    }
                }

                char[] trimChars = new char[] { separator, ' ' };

                if (splitPoint > 0) {
                    size = text.Substring(0, splitPoint);
                    size = size.Trim(trimChars);
                }

                if (splitPoint < length) {
                    units = text.Substring(splitPoint);
                    units = units.TrimEnd(trimChars);
                }
            }

            return new string[] {size, units};
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.ParseGraphicsUnits"]/*' />
        /// <devdoc>
        ///     Parses the font units from the given text.
        /// </devdoc>
        private GraphicsUnit ParseGraphicsUnits(string units) {
            UnitName unitName = null;

            for (int i = 0; i < UnitName.names.Length; i++) {
                if (String.Equals(UnitName.names[i].name, units, StringComparison.OrdinalIgnoreCase)) {
                    unitName = UnitName.names[i];
                    break;
                }
            }

            if (unitName == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "units", units));
            }
            return unitName.unit;
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.UnitName"]/*' />
        /// <devdoc>
        ///     Simple private class to associate a font size unit with a text name.
        /// </devdoc>
        internal class UnitName {

            internal string name; 

            internal GraphicsUnit unit;

            internal static readonly UnitName[] names = new UnitName[] {
                    new UnitName("world", GraphicsUnit.World), // made up
                    new UnitName("display", GraphicsUnit.Display), // made up
                    new UnitName("px", GraphicsUnit.Pixel),
                    new UnitName("pt", GraphicsUnit.Point),
                    new UnitName("in", GraphicsUnit.Inch),
                    new UnitName("doc", GraphicsUnit.Document), // made up
                    new UnitName("mm", GraphicsUnit.Millimeter),
                };


            internal UnitName(string name, GraphicsUnit unit) {
                this.name = name;
                this.unit = unit;
            }
        }

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter"]/*' />
        /// <devdoc>
        ///      FontNameConverter is a type converter that is used to convert
        ///      a font name to and from various other representations.
        /// </devdoc>
        /// <internalonly/>
        public sealed class FontNameConverter : TypeConverter, IDisposable {

            private StandardValuesCollection values;

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.FontNameConverter"]/*' />
            /// <devdoc>
            ///      Creates a new font name converter.
            /// </devdoc>
            public FontNameConverter() {

                // Sink an event to let us know when the installed
                // set of fonts changes.
                //
                SystemEvents.InstalledFontsChanged += new EventHandler(this.OnInstalledFontsChanged);
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.CanConvertFrom"]/*' />
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

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.ConvertFrom"]/*' />
            /// <devdoc>
            ///      Converts the given object to the converter's native type.
            /// </devdoc>
            [SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
            public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
                if (value is string) {
                    return MatchFontName((string)value, context);
                }
                return base.ConvertFrom(context, culture, value);
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.IDisposable.Dispose"]/*' />
            /// <devdoc>
            ///     Disposes this converter.
            /// </devdoc>
            void IDisposable.Dispose() {
                SystemEvents.InstalledFontsChanged -= new EventHandler(this.OnInstalledFontsChanged);
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.GetStandardValues"]/*' />
            /// <devdoc>
            ///      Retrieves a collection containing a set of standard values
            ///      for the data type this validator is designed for.  This
            ///      will return null if the data type does not support a
            ///      standard set of values.
            /// </devdoc>
            public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
                if (values == null) {
                    FontFamily[] fonts =  FontFamily.Families;

                    Hashtable hash = new Hashtable();
                    for (int i = 0; i < fonts.Length; i++) {
                            string name = fonts[i].Name;
                            hash[name.ToLower(CultureInfo.InvariantCulture)] = name;
                    }

                    object[] array = new object[hash.Values.Count];
                    hash.Values.CopyTo(array, 0);

                    Array.Sort(array, Comparer.Default);
                    values = new StandardValuesCollection(array);
                }

                return values;
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.GetStandardValuesExclusive"]/*' />
            /// <devdoc>
            ///      Determines if the list of standard values returned from
            ///      GetStandardValues is an exclusive list.  If the list
            ///      is exclusive, then no other values are valid, such as
            ///      in an enum data type.  If the list is not exclusive,
            ///      then there are other valid values besides the list of
            ///      standard values GetStandardValues provides.
            /// </devdoc>
            public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
                return false;
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.GetStandardValuesSupported"]/*' />
            /// <devdoc>
            ///      Determines if this object supports a standard set of values
            ///      that can be picked from a list.
            /// </devdoc>
            public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
                return true;
            }
            
            private string MatchFontName(string name, ITypeDescriptorContext context) {
                Debug.Assert(name != null, "Expected an actual font name to match in FontNameConverter::MatchFontName.");
                
                // Try a partial match
                //
                string bestMatch = null;
                name = name.ToLower(CultureInfo.InvariantCulture);
                IEnumerator e = GetStandardValues(context).GetEnumerator();
                while (e.MoveNext()) {
                    string fontName = e.Current.ToString().ToLower(CultureInfo.InvariantCulture);
                    if (fontName.Equals(name)) {
                        // For an exact match, return immediately
                        //
                        return e.Current.ToString();
                    }
                    else if (fontName.StartsWith(name)) {
                        if (bestMatch == null || fontName.Length <= bestMatch.Length) {
                            bestMatch = e.Current.ToString();
                        }
                    }
                }
                
                if (bestMatch == null) {
                    // no match... fall back on whatever was provided
                    bestMatch = name;
                }
                return bestMatch;
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontNameConverter.OnInstalledFontsChanged"]/*' />
            /// <devdoc>
            ///      Called by system events when someone adds or removes a font.  Here
            ///      we invalidate our font name collection.
            /// </devdoc>
            private void OnInstalledFontsChanged(object sender, EventArgs e) {
                values = null;
            }
        }    

        /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontUnitConverter"]/*' />
        /// <devdoc>
        ///      FontUnitConverter strips out the members of GraphicsUnit that are invalid for fonts.
        /// </devdoc>
        /// <internalonly/>
        public class FontUnitConverter : EnumConverter {
            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontUnitConverter.FontUnitConverter"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public FontUnitConverter() : base(typeof(GraphicsUnit)) {
            }

            /// <include file='doc\FontConverter.uex' path='docs/doc[@for="FontConverter.FontUnitConverter.GetStandardValues"]/*' />
            /// <internalonly/>
            public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
                if (Values == null) {
                    base.GetStandardValues(context); // sets "values"
                    ArrayList filteredValues = new ArrayList(Values);
                    filteredValues.Remove(GraphicsUnit.Display);
                    Values = new StandardValuesCollection(filteredValues);
                }
                return Values;
            }
        }
    }
}


