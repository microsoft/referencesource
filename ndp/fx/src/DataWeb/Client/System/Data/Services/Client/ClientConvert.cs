//---------------------------------------------------------------------
// <copyright file="ClientConvert.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static utility functions for conversion
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// static utility functions for conversions
    /// </summary>
    internal static class ClientConvert
    {
#if !ASTORIA_LIGHT // System.Data.Linq not available
        /// <summary>fullname for assembly</summary>
        private const string SystemDataLinq = "System.Data.Linq, Version=" + FXAssembly.Version + ", Culture=neutral, PublicKeyToken=" + AssemblyRef.EcmaPublicKeyToken;
#endif

        /// <summary>list of known value (and reference) types</summary>
        /// <remarks>
        /// examples of unsupported value types
        /// IntPtr, UIntPtr, byte*, char*
        /// </remarks>
        private static readonly Type[] knownTypes = CreateKnownPrimitives();

        /// <summary>mapping of type names to type</summary>
        private static readonly Dictionary<string, Type> namedTypesMap = CreateKnownNamesMap();

#if !ASTORIA_LIGHT // System.Data.Linq not available
        /// <summary>has System.Data.Link.Binary not been delay loaded yet</summary>
        private static bool needSystemDataLinqBinary = true;
#endif

        /// <summary>StorageType enum</summary>
        internal enum StorageType
        {
            /// <summary>Boolean</summary>
            Boolean,

            /// <summary>Byte</summary>
            Byte,

            /// <summary>ByteArray</summary>
            ByteArray,

            /// <summary>Char</summary>
            Char,

            /// <summary>CharArray</summary>
            CharArray,

            /// <summary>DateTime</summary>
            DateTime,

            /// <summary>DateTimeOffset</summary>
            DateTimeOffset,

            /// <summary>Decimal</summary>
            Decimal,

            /// <summary>Double</summary>
            Double,

            /// <summary>Guid</summary>
            Guid,

            /// <summary>Int16</summary>
            Int16,

            /// <summary>Int32</summary>
            Int32,

            /// <summary>Int64</summary>
            Int64,

            /// <summary>Single</summary>
            Single,

            /// <summary>String</summary>
            String,

            /// <summary>SByte</summary>
            SByte,

            /// <summary>TimeSpan</summary>
            TimeSpan,

            /// <summary>Type</summary>
            Type,

            /// <summary>UInt16</summary>
            UInt16,

            /// <summary>UInt32</summary>
            UInt32,

            /// <summary>UInt64</summary>
            UInt64,

            /// <summary>Uri</summary>
            Uri,

            /// <summary>System.Xml.Linq.XDocument</summary>
            XDocument,

            /// <summary>System.Xml.Linq.XElement</summary>
            XElement,

#if !ASTORIA_LIGHT // System.Data.Linq not available
            /// <summary>System.Data.Linq.Binary</summary>
            Binary,
#endif
        }

        /// <summary>
        /// convert from string to the appropriate type
        /// </summary>
        /// <param name="propertyValue">incoming string value</param>
        /// <param name="propertyType">type to convert to</param>
        /// <returns>converted value</returns>
        internal static object ChangeType(string propertyValue, Type propertyType)
        {
            Debug.Assert(null != propertyValue, "should never be passed null");
            try
            {
                switch ((StorageType)IndexOfStorage(propertyType))
                {
                    case StorageType.Boolean:
                        return XmlConvert.ToBoolean(propertyValue);
                    case StorageType.Byte:
                        return XmlConvert.ToByte(propertyValue);
                    case StorageType.ByteArray:
                        return Convert.FromBase64String(propertyValue);
                    case StorageType.Char:
                        return XmlConvert.ToChar(propertyValue);
                    case StorageType.CharArray:
                        return propertyValue.ToCharArray();
                    case StorageType.DateTime:
                        return XmlConvert.ToDateTime(propertyValue, XmlDateTimeSerializationMode.RoundtripKind);
                    case StorageType.DateTimeOffset:
                        return XmlConvert.ToDateTimeOffset(propertyValue);
                    case StorageType.Decimal:
                        return XmlConvert.ToDecimal(propertyValue);
                    case StorageType.Double:
                        return XmlConvert.ToDouble(propertyValue);
                    case StorageType.Guid:
                        return new Guid(propertyValue);
                    case StorageType.Int16:
                        return XmlConvert.ToInt16(propertyValue);
                    case StorageType.Int32:
                        return XmlConvert.ToInt32(propertyValue);
                    case StorageType.Int64:
                        return XmlConvert.ToInt64(propertyValue);
                    case StorageType.Single:
                        return XmlConvert.ToSingle(propertyValue);
                    case StorageType.String:
                        return propertyValue;
                    case StorageType.SByte:
                        return XmlConvert.ToSByte(propertyValue);
                    case StorageType.TimeSpan:
                        return XmlConvert.ToTimeSpan(propertyValue);
                    case StorageType.Type:
                        return Type.GetType(propertyValue, true);
                    case StorageType.UInt16:
                        return XmlConvert.ToUInt16(propertyValue);
                    case StorageType.UInt32:
                        return XmlConvert.ToUInt32(propertyValue);
                    case StorageType.UInt64:
                        return XmlConvert.ToUInt64(propertyValue);
                    case StorageType.Uri:
                        return Util.CreateUri(propertyValue, UriKind.RelativeOrAbsolute);
                    case StorageType.XDocument:
                        return (0 < propertyValue.Length ? System.Xml.Linq.XDocument.Parse(propertyValue) : new System.Xml.Linq.XDocument());
                    case StorageType.XElement:
                        return System.Xml.Linq.XElement.Parse(propertyValue);
#if !ASTORIA_LIGHT // System.Data.Linq not available
                    case StorageType.Binary:
                        Debug.Assert(null != knownTypes[(int)StorageType.Binary], "null typeof(System.Data.Linq.Binary)");
                        return Activator.CreateInstance(knownTypes[(int)StorageType.Binary], Convert.FromBase64String(propertyValue));
#endif
                    default:
                        Debug.Assert(false, "new StorageType without update to knownTypes");
                        return propertyValue;
                }
            }
            catch (FormatException ex)
            {
                propertyValue = (0 == propertyValue.Length ? "String.Empty" : "String");
                throw Error.InvalidOperation(Strings.Deserialize_Current(propertyType.ToString(), propertyValue), ex);
            }
            catch (OverflowException ex)
            {
                propertyValue = (0 == propertyValue.Length ? "String.Empty" : "String");
                throw Error.InvalidOperation(Strings.Deserialize_Current(propertyType.ToString(), propertyValue), ex);
            }
        }

#if !ASTORIA_LIGHT
        /// <summary>Determines whether the specified value is a System.Data.Linq.Binary value.</summary>
        /// <param name="value">Value to check.</param>
        /// <returns>true if the value is a System.Data.Linq.Binary value; false otherwise.</returns>
        internal static bool IsBinaryValue(object value)
        {
            Debug.Assert(value != null, "value != null");
            return StorageType.Binary == (StorageType)IndexOfStorage(value.GetType());
        }

        /// <summary>Converts the specified System.Data.Linq.Binary to a serializable string for URI key.</summary>
        /// <param name="binaryValue">Non-null value to convert.</param>
        /// <param name="result">out parameter for value converted to a serializable string for URI key.</param>
        /// <returns>true/ false indicating success</returns>
        internal static bool TryKeyBinaryToString(object binaryValue, out string result)
        {
            Debug.Assert(binaryValue != null, "binaryValue != null");
            Debug.Assert(IsBinaryValue(binaryValue), "IsBinaryValue(binaryValue) - otherwise TryKeyBinaryToString shouldn't have been called.");
            const System.Reflection.BindingFlags Flags = System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.InvokeMethod;
            byte[] bytes = (byte[])binaryValue.GetType().InvokeMember("ToArray", Flags, null, binaryValue, null, System.Globalization.CultureInfo.InvariantCulture);
            return System.Data.Services.Parsing.WebConvert.TryKeyPrimitiveToString(bytes, out result);
        }
#endif

        /// <summary>Converts the specified value to a serializable string for URI key.</summary>
        /// <param name="value">Non-null value to convert.</param>
        /// <param name="result">out parameter for value converted to a serializable string for URI key.</param>
        /// <returns>true/ false indicating success</returns>
        /// <remarks>
        /// This is a version of WebConvert.TryKeyPrimitiveToString to be used on the client, to avoid
        /// referencing System.Data.Linq.Binary unless required.
        /// </remarks>
        internal static bool TryKeyPrimitiveToString(object value, out string result)
        {
            Debug.Assert(value != null, "value != null");
#if !ASTORIA_LIGHT
            if (IsBinaryValue(value))
            {
                return TryKeyBinaryToString(value, out result);
            }
#endif
            return System.Data.Services.Parsing.WebConvert.TryKeyPrimitiveToString(value, out result);
        }

        /// <summary>
        /// change primtive typeName into non-nullable type
        /// </summary>
        /// <param name="typeName">like Edm.String or Edm.Binary</param>
        /// <param name="type">the mapped output type</param>
        /// <returns>true if named</returns>
        internal static bool ToNamedType(string typeName, out Type type)
        {
            type = typeof(string);
            return String.IsNullOrEmpty(typeName) || ClientConvert.namedTypesMap.TryGetValue(typeName, out type);
        }

        /// <summary>Change primitive type into a well-known type name.</summary>
        /// <param name="type">Primitive type to get well-known name for.</param>
        /// <returns>The well-known name for the specified <paramref name="type"/>.</returns>
        internal static string ToTypeName(Type type)
        {
            Debug.Assert(type != null, "type != null");
            foreach (var pair in ClientConvert.namedTypesMap)
            {
                if (pair.Value == type)
                {
                    return pair.Key;
                }
            }

            // this should never happen because a prior call to IsSupportedPrimitiveTypeForUri 
            return type.FullName; 
        }

        /// <summary>
        /// Convert from primitive value to an xml payload string. 
        /// NOTE: We need to pay special attention to DateTimes - if the converted value is going to be used as a content of 
        /// atom:updated or atom:published element we have to ensure it contains information about time zone. At the same time we 
        /// must not touch datetime values that in content or are mapped to custom elements. 
        /// </summary>
        /// <param name="propertyValue">incoming object value</param>
        /// <param name="atomDateConstruct">whether the converted value is going to be used as a content of an atom 
        /// element of atomDateConstruct type (e.g. atom:updated or atom:published)</param>
        /// <returns>converted value</returns>
        internal static string ToString(object propertyValue, bool atomDateConstruct)
        {
            Debug.Assert(null != propertyValue, "null should be handled by caller");
            switch ((StorageType)IndexOfStorage(propertyValue.GetType()))
            {
                case StorageType.Boolean:
                    return XmlConvert.ToString((bool)propertyValue);
                case StorageType.Byte:
                    return XmlConvert.ToString((byte)propertyValue);
                case StorageType.ByteArray:
                    return Convert.ToBase64String((byte[])propertyValue);
                case StorageType.Char:
                    return XmlConvert.ToString((char)propertyValue);
                case StorageType.CharArray:
                    return new String((char[])propertyValue);
                case StorageType.DateTime:                  
                    DateTime dt = (DateTime)propertyValue;
                    /* If the dt.Kind == DateTimeKind.Unspecified XmlConvert will not add information about timezone to the resulting datetime 
                       string. For atom:published and atom:updated elements information about timezone is required otherwise the message is not 
                       a valid atom message. To get the information about timezone we need to make datetimes of Unspecified kind become UniversalTime.
                       We do it the same way as we do it in JsonWriter class. 
                       WE MUST NOT TOUCH THE DATE IF IT IS NOT GOING TO BE USED FOR atom:updated OR atom:published ELEMENTS (i.e. isForAtomElement == false). */
                    return XmlConvert.ToString(dt.Kind == DateTimeKind.Unspecified && atomDateConstruct ? new DateTime(dt.Ticks, DateTimeKind.Utc) : dt, XmlDateTimeSerializationMode.RoundtripKind); 
                case StorageType.DateTimeOffset:
                    return XmlConvert.ToString((DateTimeOffset)propertyValue);
                case StorageType.Decimal:
                    return XmlConvert.ToString((Decimal)propertyValue);
                case StorageType.Double:
                    return XmlConvert.ToString((Double)propertyValue);
                case StorageType.Guid:
                    return ((Guid)propertyValue).ToString();
                case StorageType.Int16:
                    return XmlConvert.ToString((Int16)propertyValue);
                case StorageType.Int32:
                    return XmlConvert.ToString((Int32)propertyValue);
                case StorageType.Int64:
                    return XmlConvert.ToString((Int64)propertyValue);
                case StorageType.Single:
                    return XmlConvert.ToString((Single)propertyValue);
                case StorageType.String:
                    return (String)propertyValue;
                case StorageType.SByte:
                    return XmlConvert.ToString((SByte)propertyValue);
                case StorageType.TimeSpan:
                    return XmlConvert.ToString((TimeSpan)propertyValue);
                case StorageType.Type:
                    return ((Type)propertyValue).AssemblyQualifiedName;
                case StorageType.UInt16:
                    return XmlConvert.ToString((UInt16)propertyValue);
                case StorageType.UInt32:
                    return XmlConvert.ToString((UInt32)propertyValue);
                case StorageType.UInt64:
                    return XmlConvert.ToString((UInt64)propertyValue);
                case StorageType.Uri:
                    return ((Uri)propertyValue).ToString();
                case StorageType.XDocument:
                    return ((System.Xml.Linq.XDocument)propertyValue).ToString();
                case StorageType.XElement:
                    return ((System.Xml.Linq.XElement)propertyValue).ToString();
#if !ASTORIA_LIGHT // System.Data.Linq not available
                case StorageType.Binary:
                    Debug.Assert(null != knownTypes[(int)StorageType.Binary], "null typeof(System.Data.Linq.Binary)");
                    Debug.Assert(knownTypes[(int)StorageType.Binary].IsInstanceOfType(propertyValue), "not IsInstanceOfType System.Data.Linq.Binary");
                    return propertyValue.ToString();
#endif
                default:
                    Debug.Assert(false, "new StorageType without update to knownTypes");
                    return propertyValue.ToString();
            }
        }

        /// <summary>
        /// Is this a known primitive type (including string,byte[],uri)
        /// </summary>
        /// <param name="type">type to analyze</param>
        /// <returns>true if known primitive type</returns>
        internal static bool IsKnownType(Type type)
        {
            return (0 <= IndexOfStorage(type));
        }

        /// <summary>
        /// Is this a known primitive type or a nullable based on a primitive type (including string,byte[],uri)
        /// </summary>
        /// <param name="type">type to analyze, possibly nullable</param>
        /// <returns>true if known primitive type or a nullable based on a primitive type</returns>
        internal static bool IsKnownNullableType(Type type)
        {
            return IsKnownType(Nullable.GetUnderlyingType(type) ?? type);
        }

        /// <summary>
        /// Is this a primitive type that can go in the URI
        /// </summary>
        /// <param name="type">type to analyze</param>
        /// <returns>true if known primitive type</returns>
        internal static bool IsSupportedPrimitiveTypeForUri(Type type)
        {
            // 
            return Util.ContainsReference(namedTypesMap.Values.ToArray(), type);
        }

        /// <summary>type edm type string for content</summary>
        /// <param name="propertyType">type to analyze</param>
        /// <returns>edm type string for payload, null for string and unknown</returns>
        internal static string GetEdmType(Type propertyType)
        {
            switch ((StorageType)IndexOfStorage(propertyType))
            {
                case StorageType.Boolean:
                    return XmlConstants.EdmBooleanTypeName;
                case StorageType.Byte:
                    return XmlConstants.EdmByteTypeName;
#if !ASTORIA_LIGHT // System.Data.Linq not available
                case StorageType.Binary:
#endif
                case StorageType.ByteArray:
                    return XmlConstants.EdmBinaryTypeName;
                case StorageType.DateTime:
                    return XmlConstants.EdmDateTimeTypeName;
                case StorageType.Decimal:
                    return XmlConstants.EdmDecimalTypeName;
                case StorageType.Double:
                    return XmlConstants.EdmDoubleTypeName;
                case StorageType.Guid:
                    return XmlConstants.EdmGuidTypeName;
                case StorageType.Int16:
                    return XmlConstants.EdmInt16TypeName;
                case StorageType.Int32:
                    return XmlConstants.EdmInt32TypeName;
                case StorageType.Int64:
                    return XmlConstants.EdmInt64TypeName;
                case StorageType.Single:
                    return XmlConstants.EdmSingleTypeName;
                case StorageType.SByte:
                    return XmlConstants.EdmSByteTypeName;
                case StorageType.DateTimeOffset:
                case StorageType.TimeSpan:
                case StorageType.UInt16:
                case StorageType.UInt32:
                case StorageType.UInt64:
                    // don't support reverse mappings for these types in this version
                    // allows us to add real server support in the future without a
                    // "breaking change" in the future client
                    throw new NotSupportedException(Strings.ALinq_CantCastToUnsupportedPrimitive(propertyType.Name));
                case StorageType.Char:
                case StorageType.CharArray:
                case StorageType.String:
                case StorageType.Type:
                case StorageType.Uri:
                case StorageType.XDocument:
                case StorageType.XElement:
                    return null; // returning null which implies typeof(string)
                default:
                    Debug.Assert(false, "knowntype without reverse mapping");
                    return null;
            }
        }

        /// <summary>create list of known value (and reference) types</summary>
        /// <returns>static list of known types matching StorageType enum</returns>
        private static Type[] CreateKnownPrimitives()
        {
#if !ASTORIA_LIGHT // System.Data.Linq not available
            Type[] types = new Type[1 + (int)StorageType.Binary];
#else
            Type[] types = new Type[1 + (int)StorageType.XElement];
#endif
            types[(int)StorageType.Boolean] = typeof(Boolean);
            types[(int)StorageType.Byte] = typeof(Byte);
            types[(int)StorageType.ByteArray] = typeof(Byte[]);
            types[(int)StorageType.Char] = typeof(Char);
            types[(int)StorageType.CharArray] = typeof(Char[]);
            types[(int)StorageType.DateTime] = typeof(DateTime);
            types[(int)StorageType.DateTimeOffset] = typeof(DateTimeOffset);
            types[(int)StorageType.Decimal] = typeof(Decimal);
            types[(int)StorageType.Double] = typeof(Double);
            types[(int)StorageType.Guid] = typeof(Guid);
            types[(int)StorageType.Int16] = typeof(Int16);
            types[(int)StorageType.Int32] = typeof(Int32);
            types[(int)StorageType.Int64] = typeof(Int64);
            types[(int)StorageType.Single] = typeof(Single);
            types[(int)StorageType.String] = typeof(String);
            types[(int)StorageType.SByte] = typeof(SByte);
            types[(int)StorageType.TimeSpan] = typeof(TimeSpan);
            types[(int)StorageType.Type] = typeof(Type);
            types[(int)StorageType.UInt16] = typeof(UInt16);
            types[(int)StorageType.UInt32] = typeof(UInt32);
            types[(int)StorageType.UInt64] = typeof(UInt64);
            types[(int)StorageType.Uri] = typeof(Uri);
            types[(int)StorageType.XDocument] = typeof(System.Xml.Linq.XDocument);
            types[(int)StorageType.XElement] = typeof(System.Xml.Linq.XElement);
#if !ASTORIA_LIGHT // System.Data.Linq not available
            types[(int)StorageType.Binary] = null; // delay populated
#endif
            return types;
        }

        /// <summary>generate mapping of primitive type names to type</summary>
        /// <returns>mapping of primitive type names to type</returns>
        private static Dictionary<string, Type> CreateKnownNamesMap()
        {
            Dictionary<string, Type> named = new Dictionary<string, Type>(EqualityComparer<String>.Default);

            named.Add(XmlConstants.EdmStringTypeName, typeof(string));
            named.Add(XmlConstants.EdmBooleanTypeName, typeof(Boolean));
            named.Add(XmlConstants.EdmByteTypeName, typeof(Byte));
            named.Add(XmlConstants.EdmDateTimeTypeName, typeof(DateTime));
            named.Add(XmlConstants.EdmDecimalTypeName, typeof(Decimal));
            named.Add(XmlConstants.EdmDoubleTypeName, typeof(Double));
            named.Add(XmlConstants.EdmGuidTypeName, typeof(Guid));
            named.Add(XmlConstants.EdmInt16TypeName, typeof(Int16));
            named.Add(XmlConstants.EdmInt32TypeName, typeof(Int32));
            named.Add(XmlConstants.EdmInt64TypeName, typeof(Int64));
            named.Add(XmlConstants.EdmSByteTypeName, typeof(SByte));
            named.Add(XmlConstants.EdmSingleTypeName, typeof(Single));
            named.Add(XmlConstants.EdmBinaryTypeName, typeof(byte[]));
            return named;
        }

        /// <summary>get the StorageType for known types</summary>
        /// <param name="type">type being tested for known type</param>
        /// <returns>-1 or (int)StorageType</returns>
        /// <remarks>will validate System.Data.Linq.Binary on demand</remarks>
        private static int IndexOfStorage(Type type)
        {
            int index = Util.IndexOfReference(ClientConvert.knownTypes, type);
#if !ASTORIA_LIGHT // System.Data.Linq not available
            if ((index < 0) && needSystemDataLinqBinary && (type.Name == "Binary"))
            {
                return LoadSystemDataLinqBinary(type);
            }
#endif
            return index;
        }

#if !ASTORIA_LIGHT // System.Data.Linq not available
        /// <summary>validating type is System.Data.Linq.Binary</summary>
        /// <param name="type">type to verify its System.Data.Linq.Binary</param>
        /// <returns>-1 or (int)StorageType</returns>
        private static int LoadSystemDataLinqBinary(Type type)
        {
            if ((type.Namespace == "System.Data.Linq") &&
                (System.Reflection.AssemblyName.ReferenceMatchesDefinition(
                    type.Assembly.GetName(), new System.Reflection.AssemblyName(SystemDataLinq))))
            {
                ClientConvert.knownTypes[(int)StorageType.Binary] = type;
                needSystemDataLinqBinary = false;
                return (int)StorageType.Binary;
            }

            return -1;
        }
#endif
    }
}
