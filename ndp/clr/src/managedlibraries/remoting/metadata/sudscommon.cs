// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    SudsCommon.cs
//<EMAIL>
// Author:  Peter de Jong (Microsoft)
//</EMAIL>
// Purpose: Classes common to the SudsWriter and SudsParser
//
// Date:    November 18, 2000
//
//============================================================

namespace System.Runtime.Remoting.MetadataServices
{
    using System;
    using System.Collections;
    using System.Text;
    using System.Reflection;   
    using System.IO;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata.W3cXsd2001;
    using System.Globalization;

    [Serializable]
    public enum SdlType
    {
        Sdl = 0,
        Wsdl = 1
    }

    [Serializable]
    internal enum UrtType
    {
        None = 0,
        Interop = 1,
        UrtSystem = 2,
        UrtUser = 3,
        Xsd = 4,
    }

    [Serializable]
    internal enum SUDSType
    {
        None = 0,
        ClientProxy = 1,
        MarshalByRef = 2,
    }


    [Serializable]
    internal enum XsdVersion
    {
        V1999 = 0,
        V2000 = 1,
        V2001 = 2,
    }


    internal static class SudsConverter
    {

        internal static String Xsd1999 = "http://www.w3.org/1999/XMLSchema";
        internal static String Xsi1999 = "http://www.w3.org/1999/XMLSchema-instance";
        internal static String Xsd2000 = "http://www.w3.org/2000/10/XMLSchema";
        internal static String Xsi2000 = "http://www.w3.org/2000/10/XMLSchema-instance";
        internal static String Xsd2001 = "http://www.w3.org/2001/XMLSchema";
        internal static String Xsi2001 = "http://www.w3.org/2001/XMLSchema-instance";

        internal static Type typeofByte = typeof(Byte);
        internal static Type typeofSByte = typeof(SByte);
        internal static Type typeofBoolean = typeof(Boolean);
        internal static Type typeofChar = typeof(Char);
        internal static Type typeofDouble = typeof(Double);
        internal static Type typeofSingle = typeof(Single);
        internal static Type typeofDecimal = typeof(Decimal);
        internal static Type typeofInt16 = typeof(Int16);
        internal static Type typeofInt32 = typeof(Int32);
        internal static Type typeofInt64 = typeof(Int64);
        internal static Type typeofUInt16 = typeof(UInt16);
        internal static Type typeofUInt32 = typeof(UInt32);
        internal static Type typeofUInt64 = typeof(UInt64);
        internal static Type typeofSoapTime = typeof(SoapTime);
        internal static Type typeofSoapDate = typeof(SoapDate);
        internal static Type typeofSoapYearMonth = typeof(SoapYearMonth);
        internal static Type typeofSoapYear = typeof(SoapYear);
        internal static Type typeofSoapMonthDay = typeof(SoapMonthDay);
        internal static Type typeofSoapDay = typeof(SoapDay);
        internal static Type typeofSoapMonth = typeof(SoapMonth);
        internal static Type typeofSoapHexBinary = typeof(SoapHexBinary);
        internal static Type typeofSoapBase64Binary = typeof(SoapBase64Binary);
        internal static Type typeofSoapInteger = typeof(SoapInteger);
        internal static Type typeofSoapPositiveInteger = typeof(SoapPositiveInteger);
        internal static Type typeofSoapNonPositiveInteger = typeof(SoapNonPositiveInteger);
        internal static Type typeofSoapNonNegativeInteger = typeof(SoapNonNegativeInteger);
        internal static Type typeofSoapNegativeInteger = typeof(SoapNegativeInteger);
        internal static Type typeofSoapAnyUri = typeof(SoapAnyUri);
        internal static Type typeofSoapQName = typeof(SoapQName);
        internal static Type typeofSoapNotation = typeof(SoapNotation);
        internal static Type typeofSoapNormalizedString = typeof(SoapNormalizedString);
        internal static Type typeofSoapToken = typeof(SoapToken);
        internal static Type typeofSoapLanguage = typeof(SoapLanguage);
        internal static Type typeofSoapName = typeof(SoapName);
        internal static Type typeofSoapIdrefs = typeof(SoapIdrefs);
        internal static Type typeofSoapEntities = typeof(SoapEntities);
        internal static Type typeofSoapNmtoken = typeof(SoapNmtoken);
        internal static Type typeofSoapNmtokens = typeof(SoapNmtokens);
        internal static Type typeofSoapNcName = typeof(SoapNcName);
        internal static Type typeofSoapId = typeof(SoapId);
        internal static Type typeofSoapIdref = typeof(SoapIdref);
        internal static Type typeofSoapEntity = typeof(SoapEntity);
        internal static Type typeofString = typeof(String);
        internal static Type typeofObject = typeof(Object);
        internal static Type typeofVoid = typeof(void);
        internal static Type typeofDateTime = typeof(DateTime);
        internal static Type typeofTimeSpan = typeof(TimeSpan);
        internal static Type typeofISoapXsd = typeof(ISoapXsd);


        
        internal static String GetXsdVersion(XsdVersion xsdVersion)
        {
            String version = null;
            if (xsdVersion == XsdVersion.V1999)
                version = Xsd1999;
            else if (xsdVersion == XsdVersion.V2000)
                version = Xsd2000;
            else
                version = Xsd2001;
            return version;
        }

        internal static String GetXsiVersion(XsdVersion xsdVersion)
        {
            String version = null;
            if (xsdVersion == XsdVersion.V1999)
                version = Xsi1999;
            else if (xsdVersion == XsdVersion.V2000)
                version = Xsi2000;
            else
                version = Xsi2001;
            return version;
        }

        internal static String MapClrTypeToXsdType(Type clrType)
        {
            String typeName = null;

            if (clrType == typeofChar)
                return null;


            if (clrType.IsPrimitive)
            {
                if (clrType == typeofByte)
                    typeName = "xsd:unsignedByte";
                else if (clrType == typeofSByte)
                    typeName = "xsd:byte";
                else if (clrType == typeofBoolean)
                    typeName = "xsd:boolean";
                else if (clrType == typeofChar)
                    typeName = "xsd:char"; //Not an xsd type, but need a way to identify a char
                else if (clrType == typeofDouble)
                    typeName = "xsd:double";
                else if (clrType == typeofSingle)
                    typeName = "xsd:float";
                else if (clrType == typeofDecimal)
                    typeName = "xsd:decimal";
                else if (clrType == typeofDateTime)
                    typeName = "xsd:dateTime";
                else if (clrType == typeofInt16)
                    typeName = "xsd:short";
                else if (clrType == typeofInt32)
                    typeName = "xsd:int";
                else if (clrType == typeofInt64)
                    typeName = "xsd:long";
                else if (clrType == typeofUInt16)
                    typeName = "xsd:unsignedShort";
                else if (clrType == typeofUInt32)
                    typeName = "xsd:unsignedInt";
                else if (clrType == typeofUInt64)
                    typeName = "xsd:unsignedLong";
                else if (clrType == typeofTimeSpan)
                    typeName = "xsd:duration";

            }
            else if (typeofISoapXsd.IsAssignableFrom(clrType))
            {
                if (clrType == typeofSoapTime)
                    typeName = SoapTime.XsdType;
                else if (clrType == typeofSoapDate)
                    typeName = SoapDate.XsdType;
                else if (clrType == typeofSoapYearMonth)
                    typeName = SoapYearMonth.XsdType;
                else if (clrType == typeofSoapYear)
                    typeName = SoapYear.XsdType;
                else if (clrType == typeofSoapMonthDay)
                    typeName = SoapMonthDay.XsdType;
                else if (clrType == typeofSoapDay)
                    typeName = SoapDay.XsdType;
                else if (clrType == typeofSoapMonth)
                    typeName = SoapMonth.XsdType;
                else if (clrType == typeofSoapHexBinary)
                    typeName = SoapHexBinary.XsdType;
                else if (clrType == typeofSoapBase64Binary)
                    typeName = SoapBase64Binary.XsdType;
                else if (clrType == typeofSoapInteger)
                    typeName = SoapInteger.XsdType;
                else if (clrType == typeofSoapPositiveInteger)
                    typeName = SoapPositiveInteger.XsdType;
                else if (clrType == typeofSoapNonPositiveInteger)
                    typeName = SoapNonPositiveInteger.XsdType;
                else if (clrType == typeofSoapNonNegativeInteger)
                    typeName = SoapNonNegativeInteger.XsdType;
                else if (clrType == typeofSoapNegativeInteger)
                    typeName = SoapNegativeInteger.XsdType;
                else if (clrType == typeofSoapAnyUri)
                    typeName = SoapAnyUri.XsdType;
                else if (clrType == typeofSoapQName)
                    typeName = SoapQName.XsdType;
                else if (clrType == typeofSoapNotation)
                    typeName = SoapNotation.XsdType;
                else if (clrType == typeofSoapNormalizedString)
                    typeName = SoapNormalizedString.XsdType;
                else if (clrType == typeofSoapToken)
                    typeName = SoapToken.XsdType;
                else if (clrType == typeofSoapLanguage)
                    typeName = SoapLanguage.XsdType;
                else if (clrType == typeofSoapName)
                    typeName = SoapName.XsdType;
                else if (clrType == typeofSoapIdrefs)
                    typeName = SoapIdrefs.XsdType;
                else if (clrType == typeofSoapEntities)
                    typeName = SoapEntities.XsdType;
                else if (clrType == typeofSoapNmtoken)
                    typeName = SoapNmtoken.XsdType;
                else if (clrType == typeofSoapNmtokens)
                    typeName = SoapNmtokens.XsdType;
                else if (clrType == typeofSoapNcName)
                    typeName = SoapNcName.XsdType;
                else if (clrType == typeofSoapId)
                    typeName = SoapId.XsdType;
                else if (clrType == typeofSoapIdref)
                    typeName = SoapIdref.XsdType;
                else if (clrType == typeofSoapEntity)
                    typeName = SoapEntity.XsdType;
                typeName = "xsd:"+typeName;
            }
            else if (clrType == typeofString)
                typeName = "xsd:string";
            else if (clrType == typeofDecimal)
                typeName = "xsd:decimal";
            else if (clrType == typeofObject)
                typeName = "xsd:anyType";
            else if (clrType == typeofVoid)
                typeName = "void";
            else if (clrType == typeofDateTime)
                typeName = "xsd:dateTime";
            else if (clrType == typeofTimeSpan)
                typeName = "xsd:duration";

            return typeName;
        }

        internal static String MapXsdToClrTypes(String xsdType)
        {
            String lxsdType = xsdType.ToLower(CultureInfo.InvariantCulture);
            String clrType = null;

            if (xsdType == null || xsdType.Length == 0)
                return null;

            switch (lxsdType[0])
            {
                case 'a':
                    if (lxsdType == "anyuri")
                        clrType = "SoapAnyUri";
                    else if (lxsdType == "anytype" || lxsdType == "ur-type")
                        clrType = "Object";

                    break;
                case 'b':
                    if (lxsdType == "boolean")
                        clrType = "Boolean";
                    else if (lxsdType == "byte")
                        clrType = "SByte";
                    else if (lxsdType == "base64binary")
                        clrType = "SoapBase64Binary";

                    break;
                case 'c':
                    if (lxsdType == "char")
                        clrType = "Char";
                    break;
                case 'd':
                    if (lxsdType == "double")
                        clrType = "Double";
                    else if (lxsdType == "datetime")
                        clrType = "DateTime";
                    else if (lxsdType == "decimal")
                        clrType = "Decimal";
                    else if (lxsdType == "duration")
                        clrType = "TimeSpan";
                    else if (lxsdType == "date")
                        clrType = "SoapDate";

                    break;
                case 'e':
                    if (lxsdType == "entities")
                        clrType = "SoapEntities";
                    else if (lxsdType == "entity")
                        clrType = "SoapEntity";
                    break;
                case 'f':
                    if (lxsdType == "float")
                        clrType = "Single";
                    break;
                case 'g':
                    if (lxsdType == "gyearmonth")
                        clrType = "SoapYearMonth";
                    else if (lxsdType == "gyear")
                        clrType = "SoapYear";
                    else if (lxsdType == "gmonthday")
                        clrType = "SoapMonthDay";
                    else if (lxsdType == "gday")
                        clrType = "SoapDay";
                    else if (lxsdType == "gmonth")
                        clrType = "SoapMonth";
                    break;
                case 'h':
                    if (lxsdType == "hexbinary")
                        clrType = "SoapHexBinary";
                    break;
                case 'i':
                    if (lxsdType == "int")
                        clrType = "Int32";
                    else if (lxsdType == "integer")
                        clrType = "SoapInteger";
                    else if (lxsdType == "idrefs")
                        clrType = "SoapIdrefs";
                    else if (lxsdType == "id")
                        clrType = "SoapId";
                    else if (lxsdType == "idref")
                        clrType = "SoapIdref";
                    break;
                case 'l':
                    if (lxsdType == "long")
                        clrType = "Int64";
                    else if (lxsdType == "language")
                        clrType = "SoapLanguage";
                    break;
                case 'n':
                    if (lxsdType == "number")
                        clrType = "Decimal";
                    else if (lxsdType == "normalizedstring")
                        clrType = "SoapNormalizedString";
                    else if (lxsdType == "nonpositiveinteger")
                        clrType = "SoapNonPositiveInteger";
                    else if (lxsdType == "negativeinteger")
                        clrType = "SoapNegativeInteger";
                    else if (lxsdType == "nonnegativeinteger")
                        clrType = "SoapNonNegativeInteger";
                    else if (lxsdType == "notation")
                        clrType = "SoapNotation";
                    else if (lxsdType == "nmtoken")
                        clrType = "SoapNmtoken";
                    else if (lxsdType == "nmtokens")
                        clrType = "SoapNmtokens";
                    else if (lxsdType == "name")
                        clrType = "SoapName";
                    else if (lxsdType == "ncname")
                        clrType = "SoapNcName";
                    break;
                case 'p':
                    if (lxsdType == "positiveinteger")
                        clrType = "SoapPositiveInteger";
                    break;
                case 'q':
                    if (lxsdType == "qname")
                        clrType = "SoapQName";
                    break;
                case 's':
                    if (lxsdType == "string")
                        clrType = "String";
                    else if (lxsdType == "short")
                        clrType = "Int16";
                    break;
                case 't':
                    if (lxsdType == "time")
                        clrType = "SoapTime";
                    else if (lxsdType == "token")
                        clrType = "SoapToken";
                    break;
                case 'u':
                    if (lxsdType == "unsignedlong")
                        clrType = "UInt64";
                    else if (lxsdType == "unsignedint")
                        clrType = "UInt32";
                    else if (lxsdType == "unsignedshort")
                        clrType = "UInt16";
                    else if (lxsdType == "unsignedbyte")
                        clrType = "Byte";
                    break;

                default:
                    break;
            }
            return clrType;
        }
    }

    internal static class Util
    {
        //internal static FileStream fout = null;
// disable csharp compiler warning #0414: field assigned unused value
#pragma warning disable 0414
        internal static StreamWriter writer = null;
#pragma warning restore 0414

        [System.Diagnostics.Conditional("_LOGGING")]
        internal static void Log(String message)
        {
            //InternalRM.InfoSoap(message); //uncomment for traces
            /*
            if (fout == null)
            {
                fout = new FileStream("suds.log", FileMode.Create, FileAccess.Write, FileShare.ReadWrite);
                writer = new StreamWriter(fout);
                writer.AutoFlush = true;
            }
            writer.WriteLine(message);
            */
            //System.Runtime.Serialization.Formatters.InternalST.InfoSoap(messages);

        }

        [System.Diagnostics.Conditional("_LOGGING")]
        internal static void LogInput(ref TextReader input)
        {
            //System.Runtime.Serialization.Formatters.InternalST.InfoSoap(messages);
            if (InternalRM.SoapCheckEnabled())
            {
                String strbuffer = input.ReadToEnd();
                InternalRM.InfoSoap("******************WSDL******************");
                InternalRM.InfoSoap(strbuffer);
                InternalRM.InfoSoap("******************End WSDL******************");
                input = (TextReader)new StringReader(strbuffer);
            }
        }

        [System.Diagnostics.Conditional("_LOGGING")]
        internal static void LogString(String strbuffer)
        {
            InternalRM.InfoSoap("******************WSDL******************");
            InternalRM.InfoSoap(strbuffer);
            InternalRM.InfoSoap("******************End WSDL******************");
        }
    }
}
    
