// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    WsdlParser.cs
//<EMAIL>
// Author:  Peter de Jong (Microsoft)
//</EMAIL>
// Purpose: Defines WsdlParser that parses a given WSDL document
//          and generates types defined in it.
//
// Date:    November 15, 2000
//
//============================================================
namespace System.Runtime.Remoting.MetadataServices
{
    using System;
    using System.Threading;
    using System.Collections;
    using System.Reflection;
    using System.Xml;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Net;
    using System.Runtime.Remoting.Channels; // This is so we can get the resource strings.
    using System.Runtime.Remoting;
    using System.Globalization;

    // This class parses SUDS documents
    internal class WsdlParser
    {

        // Main parser
        internal WsdlParser(TextReader input, String outputDir, ArrayList outCodeStreamList, String locationURL, bool bWrappedProxy, String proxyNamespace)
        {
            Util.Log("WsdlParser.WsdlParser outputDir "+outputDir+" locationURL "+locationURL+" bWrappedProxy "+bWrappedProxy+" proxyNamespace "+proxyNamespace);
            // Initialize member variables
            _XMLReader = null;
            _readerStreamsWsdl = new ReaderStream(locationURL);
            _readerStreamsWsdl.InputStream = input;
            _writerStreams = null;
            _outputDir = outputDir;
            _outCodeStreamList = outCodeStreamList;
            _bWrappedProxy = bWrappedProxy;
            if (proxyNamespace == null || proxyNamespace.Length == 0)
                _proxyNamespace = "InteropNS";
            else
                _proxyNamespace = proxyNamespace;
            if (outputDir == null)
                outputDir = ".";

            int length = outputDir.Length;
            if (length > 0)
            {
                char endChar = outputDir[length-1];
                if (endChar != '\\' && endChar != '/')
                    _outputDir = _outputDir + '\\';
            }
            //_namespaceStack = null;
            _URTNamespaces = new ArrayList();
            _blockDefault = SchemaBlockType.ALL;
            _primedNametable = CreatePrimedNametable();
            
        }

        internal String SchemaNamespaceString
        {
            get
            { 
                String schemaStr = null;
                switch (_xsdVersion)
                {
                case XsdVersion.V1999:
                    schemaStr = s_schemaNamespaceString1999;
                    break;
                case XsdVersion.V2000:
                    schemaStr = s_schemaNamespaceString2000;
                    break;
                case XsdVersion.V2001:
                    schemaStr = s_schemaNamespaceString;
                    break;
                }
                return schemaStr;
            }
        }

        internal String ProxyNamespace
        {
            get {return _proxyNamespace;}
        }

        internal int ProxyNamespaceCount
        {
            get {return _proxyNamespaceCount;}
            set {_proxyNamespaceCount = value;}
        }

        internal XmlTextReader XMLReader
        {
            get {return _XMLReader;}
        }

        // Skips past endtags and non content tags
        private bool SkipXmlElement()
        {
            Util.Log("WsdlParser.SkipXmlElement");          
            //PrintNode(Console.Out);
            _XMLReader.Skip();
            XmlNodeType nodeType = _XMLReader.MoveToContent();
            while (nodeType == XmlNodeType.EndElement)
            {
                _XMLReader.Read();
                nodeType = _XMLReader.MoveToContent();
                if (nodeType == XmlNodeType.None)
                    break;
            }

            return(nodeType != XmlNodeType.None);
        }

        // Reads past endtags and non content tags
        private bool ReadNextXmlElement()
        {
            Util.Log("WsdlParser.ReadNextXmlElement");                      
            _XMLReader.Read();
            XmlNodeType nodeType = _XMLReader.MoveToContent();
            while (nodeType == XmlNodeType.EndElement)
            {
                _XMLReader.Read();
                nodeType = _XMLReader.MoveToContent();
                if (nodeType == XmlNodeType.None)
                    break;
            }

            //PrintNode(Console.Out);
            return(nodeType != XmlNodeType.None);
        }

        // Parses complex types
        private URTComplexType ParseComplexType(URTNamespace parsingNamespace, String typeName)
        {
            Util.Log("WsdlParser.ParseComplexType NS "+parsingNamespace.Name+" typeName "+typeName);                                 
            // Lookup the name of the type and the base type from which it derives
            if (typeName == null)
                typeName = LookupAttribute(s_nameString, null, true);

            URTNamespace xns = null;
            String typeNS = ParseQName(ref typeName, parsingNamespace, out xns);            

            Util.Log("WsdlParser.ParseComplexType actualNS 1 "+xns);
            Util.Log("WsdlParser.ParseComplexType actualNS 2 "+xns.Name);
            URTComplexType parsingComplexType = xns.LookupComplexType(typeName);
            if (parsingComplexType == null)
            {
                parsingComplexType = new URTComplexType(typeName, xns.Name, xns.Namespace,
                                                        xns.EncodedNS, _blockDefault,
                                                        false, typeName != null, this, xns);
                xns.AddComplexType(parsingComplexType);
            }

            String baseType = LookupAttribute(s_baseString, null, false);

            if (!MatchingStrings(baseType, s_emptyString))
            {
                String baseNS = ParseQName(ref baseType, parsingNamespace);
                parsingComplexType.Extends(baseType, baseNS);
            }

            if (parsingComplexType.Fields.Count > 0)
            {
                SkipXmlElement();
            }
            else
            {
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                int fieldNum = 0;
                String elementName;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if (MatchingStrings(elementName, s_elementString))
                    {
                        ParseElementField(xns, parsingComplexType, fieldNum);
                        ++fieldNum;
                        continue;
                    }
                    else if (MatchingStrings(elementName, s_attributeString))
                    {
                        ParseAttributeField(xns, parsingComplexType);
                        continue;
                    }
                    else if (MatchingStrings(elementName, s_allString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.ALL;
                    }
                    else if (MatchingStrings(elementName, s_sequenceString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.SEQUENCE;
                    }
                    else if (MatchingStrings(elementName, s_choiceString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.CHOICE;
                    }
                    else if (MatchingStrings(elementName, s_complexContentString))
                    {
                        parsingComplexType.BlockType = SchemaBlockType.ComplexContent; 
                    }
                    else if (MatchingStrings(elementName, s_restrictionString))
                    {
                        ParseRestrictionField(xns, parsingComplexType);
                        //++fieldNum;
                        continue;
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        SkipXmlElement();
                        continue;
                    }

                    // Read next element
                    ReadNextXmlElement();
                }
            }

            return(parsingComplexType);
        }

        // Parses simple types
        private URTSimpleType ParseSimpleType(URTNamespace parsingNamespace, String typeName)
        {
            Util.Log("WsdlParser.ParseSimpleType NS "+parsingNamespace+" typeName "+typeName);                                              
            // Lookup the name of the type and the base type from which it derives
            if (typeName == null)
                typeName = LookupAttribute(s_nameString, null, true);
            String enumType = LookupAttribute(s_enumTypeString, s_wsdlSudsNamespaceString, false);
            URTSimpleType parsingSimpleType = parsingNamespace.LookupSimpleType(typeName);
            if (parsingSimpleType == null)
            {
                parsingSimpleType = new URTSimpleType(typeName, parsingNamespace.Name, parsingNamespace.Namespace,
                                                      parsingNamespace.EncodedNS, typeName != null, this);
                String baseType = LookupAttribute(s_baseString, null, false);
                if (!MatchingStrings(baseType, s_emptyString))
                {
                    String baseNS = ParseQName(ref baseType, parsingNamespace);
                    parsingSimpleType.Extends(baseType, baseNS);
                }
                parsingNamespace.AddSimpleType(parsingSimpleType);

                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                //int enumFacetNum = 0;
                string elementName;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;

                    if (MatchingStrings(elementName, s_restrictionString))
                    {
                        ParseRestrictionField(parsingNamespace, parsingSimpleType);
                    }
                    /*
                    else if (MatchingStrings(elementName, s_encodingString))
                    {
                        ParseEncoding(parsingSimpleType);
                    }
                    */
                    else
                    {
                        SkipXmlElement();
                    }
                }
            }
            else
            {
                SkipXmlElement();
            }
            if (enumType != null)
                parsingSimpleType.EnumType = enumType;

            return(parsingSimpleType);
        }

        /*
        // Parses encoding
        private void ParseEncoding(URTSimpleType parsingSimpleType)
        {
            Util.Log("WsdlParser.ParseEncoding URTSimpleType "+parsingSimpleType);                                              
            if (_XMLReader.IsEmptyElement == true)
            {
                // Get the encoding value
                String valueString = LookupAttribute(s_valueString, null, true);
                parsingSimpleType.Encoding = valueString;
            }
            else
            {
                throw new SUDSParserException(
                                             CoreChannel.GetResourceString("Remoting_Suds_EncodingMustBeEmpty"));
            }

            ReadNextXmlElement();
            return;
        }
        */

        // Parses enumeration
        private void ParseEnumeration(URTSimpleType parsingSimpleType, int enumFacetNum)
        {
            Util.Log("WsdlParser.ParseEnumeration facitNum "+enumFacetNum);
            if (_XMLReader.IsEmptyElement == true)
            {
                // Get the enum value
                String valueString = LookupAttribute(s_valueString, null, true);
                parsingSimpleType.IsEnum = true;
                parsingSimpleType.AddFacet(new EnumFacet(valueString, enumFacetNum));
            }
            else
            {
                throw new SUDSParserException(
                                             CoreChannel.GetResourceString("Remoting_Suds_EnumMustBeEmpty"));
            }
            return;
        }


        // Parses element fields
        private void ParseElementField(URTNamespace parsingNamespace,
                                       URTComplexType parsingComplexType,
                                       int fieldNum)
        {
            Util.Log("WsdlParser.ParseElementField NS "+parsingNamespace+" fieldNum "+fieldNum);            
            // Determine the field name
            String fieldTypeName, fieldTypeXmlNS;
            String fieldName = LookupAttribute(s_nameString, null, true);

            // Look for array bounds
            String minOccurs = LookupAttribute(s_minOccursString, null, false);
            String maxOccurs = LookupAttribute(s_maxOccursString, null, false);

            // Check if the field is optional
            bool bOptional = false;
            if (MatchingStrings(minOccurs, s_zeroString))
                bOptional = true;

            // Check if the field is an inline array
            bool bArray = false;
            String arraySize = null;
            if (!MatchingStrings(maxOccurs, s_emptyString) &&
                !MatchingStrings(maxOccurs, s_oneString))
            {
                if (MatchingStrings(maxOccurs, s_unboundedString))
                    arraySize = String.Empty;
                else
                    arraySize = maxOccurs;
                bArray = true;
            }

            // Handle anonymous types
            bool bEmbedded, bPrimitive;
            if (_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case
                fieldTypeName = LookupAttribute(s_typeString, null, false);

                // Handle the absense of type attribute (Object case)
                ResolveTypeAttribute(ref fieldTypeName, out fieldTypeXmlNS,
                                     out bEmbedded, out bPrimitive);

                // Read next element
                ReadNextXmlElement();
            }
            else
            {
                // Anonymous type case
                fieldTypeXmlNS = parsingNamespace.Namespace;
                fieldTypeName = parsingNamespace.GetNextAnonymousName();
                bPrimitive = false;
                bEmbedded = true;
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                // Parse the type
                String elementName;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if (MatchingStrings(elementName, s_complexTypeString))
                    {
                        URTComplexType complexType = ParseComplexType(parsingNamespace, fieldTypeName);
                        if (complexType.IsEmittableFieldType)
                        {
                            fieldTypeXmlNS = complexType.FieldNamespace;
                            fieldTypeName = complexType.FieldName;
                            bPrimitive = complexType.PrimitiveField;
                            parsingNamespace.RemoveComplexType(complexType);
                        }
                    }
                    else if (MatchingStrings(elementName, s_simpleTypeString))
                    {
                        URTSimpleType simpleType = ParseSimpleType(parsingNamespace, fieldTypeName);
                        if (simpleType.IsEmittableFieldType)
                        {
                            fieldTypeXmlNS = simpleType.FieldNamespace;
                            fieldTypeName = simpleType.FieldName;
                            bPrimitive = simpleType.PrimitiveField;
                            parsingNamespace.RemoveSimpleType(simpleType);
                        }
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        SkipXmlElement();
                    }
                }
            }

            // Add field to the type being parsed
            parsingComplexType.AddField(new URTField(fieldName, fieldTypeName, fieldTypeXmlNS,
                                                     this, bPrimitive, bEmbedded, false,
                                                     bOptional, bArray, arraySize, parsingNamespace));
            return;
        }

        // Parses attribute fields
        private void ParseAttributeField(URTNamespace parsingNamespace,
                                         URTComplexType parsingComplexType)
        {
            Util.Log("WsdlParser.ParseAttributeField NS "+parsingNamespace);
            // Lookup field name
            String attrTypeName, attrTypeNS;
            String attrName = LookupAttribute(s_nameString, null, true);

            // Check if the field is optional
            bool bOptional = false;
            String minOccurs = LookupAttribute(s_minOccursString, null, false);
            if (MatchingStrings(minOccurs, s_zeroString))
                bOptional = true;

            // Handle anonymous types
            bool bEmbedded, bPrimitive;
            if (_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case and type has to present
                attrTypeName = LookupAttribute(s_typeString, null, true);
                ResolveTypeAttribute(ref attrTypeName, out attrTypeNS,
                                     out bEmbedded, out bPrimitive);

                // Read next element
                ReadNextXmlElement();

                // Check for xsd:ID type
                if (MatchingStrings(attrTypeName, s_idString) &&
                    MatchingSchemaStrings(attrTypeNS))
                {
                    parsingComplexType.IsStruct = false;
                    return;
                }
            }
            else
            {
                // Anonymous type case
                attrTypeNS = parsingNamespace.Namespace;
                attrTypeName = parsingNamespace.GetNextAnonymousName();
                bPrimitive = false;
                bEmbedded = true;
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                // Parse the type
                String elementName;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if (MatchingStrings(elementName, s_simpleTypeString))
                    {
                        URTSimpleType simpleType = ParseSimpleType(parsingNamespace, attrTypeName);
                        if (simpleType.IsEmittableFieldType)
                        {
                            attrTypeNS = simpleType.FieldNamespace;
                            attrTypeName = simpleType.FieldName;
                            bPrimitive = simpleType.PrimitiveField;
                            parsingNamespace.RemoveSimpleType(simpleType);
                        }
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        SkipXmlElement();
                    }
                }
            }

            // Add field to the type being parsed
            parsingComplexType.AddField(new URTField(attrName, attrTypeName, attrTypeNS,
                                                     this, bPrimitive, bEmbedded, true,
                                                     bOptional, false, null, parsingNamespace));
            return;
        }


        // Parses RestrictionField fields
        // Now only set up to recognize arrays
        private void ParseRestrictionField(URTNamespace parsingNamespace,
                                           BaseType parsingType)
        {
            Util.Log("WsdlParser.ParseRestrictionField Enter NS "+parsingNamespace+" type "+parsingType);
            // Lookup field name

            String attrName = LookupAttribute(s_baseString, null, true);
            String baseNS = ParseQName(ref attrName, parsingNamespace);
            //if (MatchingStrings(baseNS, s_soapEncodingString) && MatchingStrings(attrName, s_arrayString))
            {
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();

                // Parse the type
                String elementName;
                String arrayNS;
                String arrayType;
                int enumFacetNum = 0;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    Util.Log("WsdlParser.ParseRestrictionField elementName "+elementName);
                    if (MatchingStrings(elementName, s_attributeString))
                    {
                        String refValue = LookupAttribute(s_refString, null, true);
                        String refNS = ParseQName(ref refValue, parsingNamespace);
                        if (MatchingStrings(refNS, s_soapEncodingString) && MatchingStrings(refValue, s_arrayTypeString))
                        {
                            URTComplexType parsingComplexType = (URTComplexType)parsingType;
                            arrayType = LookupAttribute(s_arrayTypeString, s_wsdlNamespaceString, true);
                            Util.Log("WsdlParser.ParseRestrictionField arrayType "+arrayType);
                            URTNamespace arrayNamespace = null;
                            arrayNS = ParseQName(ref arrayType, null, out arrayNamespace);

                            parsingComplexType.AddArray(arrayType, arrayNamespace);
                            //Add array to the array namespace
                            arrayNamespace.AddComplexType(parsingComplexType); 
                            parsingComplexType.IsPrint = false;
                        }
                    }
                    else if (MatchingStrings(elementName, s_enumerationString))
                    {
                        URTSimpleType parsingSimpleType = (URTSimpleType)parsingType;
                        ParseEnumeration(parsingSimpleType, enumFacetNum);
                        ++enumFacetNum;
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        SkipXmlElement();
                    }
                    ReadNextXmlElement();
                }
            }
            // else
            // SkipXmlElement();

            Util.Log("WsdlParser.ParseRestrictionField Exit NS "+parsingNamespace+" type "+parsingType);
            return;
        }


        // Parses a global element declaration
        private void ParseElementDecl(URTNamespace parsingNamespace)
        {
            Util.Log("WsdlParser.ParseElementDecl");            
            // Obtain element name and its type
            String elmName = LookupAttribute(s_nameString, null, true);
            String elmNS = parsingNamespace.Name;
            String typeName = LookupAttribute(s_typeString, null, false);

            // Handle the anonymous types
            String typeNS;
            bool bEmbedded, bPrimitive;
            if (_XMLReader.IsEmptyElement == true)
            {
                // Non-anonymous type case
                // We cannot assert that the type attribute must have been present
                // due to the Object/ur-type case
                ResolveTypeAttribute(ref typeName, out typeNS, out bEmbedded, out bPrimitive);

                // Position to the next element
                ReadNextXmlElement();
            }
            else
            {
                // Anonymous type case
                typeNS = parsingNamespace.Name;
                typeName = parsingNamespace.GetNextAnonymousName();
                bEmbedded = true;
                bPrimitive = false;

                // Parse the type
                int curDepth = _XMLReader.Depth;
                ReadNextXmlElement();
                String elementName;
                while (_XMLReader.Depth > curDepth)
                {
                    elementName = _XMLReader.LocalName;
                    if (MatchingStrings(elementName, s_complexTypeString))
                    {
                        ParseComplexType(parsingNamespace, typeName);
                    }
                    else if (MatchingStrings(elementName, s_simpleTypeString))
                    {
                        ParseSimpleType(parsingNamespace, typeName);
                    }
                    else
                    {
                        // Ignore others elements such as annotations
                        SkipXmlElement();
                    }
                }
            }

            // Create a new global element under the current namespace
            parsingNamespace.AddElementDecl(new ElementDecl(elmName, elmNS, typeName, typeNS,
                                                            bPrimitive));

            return;
        }

        // Checks for reference and array types and resolves to
        // actual types. It returns true if the type needs [Embedded] attribute
        private void ResolveTypeNames(ref String typeNS, ref String typeName,
                                      out bool bEmbedded, out bool bPrimitive)
        {
            Util.Log("WsdlParser.ResolveTypeNames typeNS "+typeNS+" typeName "+typeName);           
            // Check for reference and array types
            bEmbedded = true;
            bool bArrayType = false;
            if (MatchingStrings(typeNS, s_wsdlSoapNamespaceString))
            {
                if (MatchingStrings(typeName, s_referenceString))
                    bEmbedded = false;
                else if (MatchingStrings(typeName, s_arrayString))
                    bArrayType = true;
            }

            Util.Log("WsdlParser.ResolveTypeNames typeNS 1 bEmbedded "+bEmbedded+" bArrayType "+bArrayType);
            // Resolve to the actual type in the case of reference and array types
            if ((bEmbedded == false) || (bArrayType == true))
            {
                typeName = LookupAttribute(s_refTypeString, s_wsdlSudsNamespaceString, true);
                typeNS = ParseQName(ref typeName);
            }

            // Primitive types do not need the [Embedded] attribute;
            bPrimitive = IsPrimitiveType(typeNS, typeName);
            if (bPrimitive)
            {
                typeName = MapSchemaTypesToCSharpTypes(typeName);
                bEmbedded = false;
            }
            else if (MatchingStrings(typeName, s_urTypeString) &&
                     MatchingSchemaStrings(typeNS))
            {
                typeName = s_objectString;
            }

            return;
        }

        // Parses namespace declaration elements
        private URTNamespace ParseNamespace()
        {
            Util.Log("WsdlParser.ParseNamespace");          
            // Determine the new namespace encountered
            String name = (String) LookupAttribute(s_targetNamespaceString, null, false);
            bool bUnique = false;
            if (MatchingStrings(name, s_emptyString) &&
                MatchingStrings(_XMLReader.LocalName, s_sudsString) &&
                _parsingInput.UniqueNS == null)
            {
                name = _parsingInput.TargetNS;
                bUnique = true;
            }

            // Add the namespace being parsed to the list if neccessary
            URTNamespace parsingNamespace = LookupNamespace(name);
            if (parsingNamespace == null)
            {
                parsingNamespace = new URTNamespace(name, this);
            }
            if (bUnique)
                _parsingInput.UniqueNS = parsingNamespace;
            //_namespaceStack = NamespaceStack.Push(_namespaceStack, _parsingNamespace, _XMLReader.Depth);

            // Parse schema defaults
            //if(MatchingStrings(_XMLReader.LocalName, s_sudsString))
            //{

            //}

            // Read the next record
            ReadNextXmlElement();

            return(parsingNamespace);
        }

#if false
        private void AddToNamespace(String name)
        {
        URTNamespace parsingNamespace = LookupNamespace(name);
        if(parsingNamespace == null)
        {
        parsingNamespace = new URTNamespace(name, this);
        _URTNamespaces.Add(parsingNamespace);
        }
        }
#endif

        private void ParseReaderStreamLocation(ReaderStream reader, ReaderStream currentReaderStream)
        {
            Util.Log("WsdlParser.ParseReaderStreamLocation location "+reader.Location+" current location "+currentReaderStream.Location);           
            String location = reader.Location;
            int index = location.IndexOf(':');
            if (index == -1)
            {
                // relative path
                if (currentReaderStream == null || currentReaderStream.Location == null)
                    throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_Import"), reader.Location));

                if (currentReaderStream.Uri == null)
                    currentReaderStream.Uri= new Uri(currentReaderStream.Location); // If relative path, will already be changed to absolute path by next statement in previous invocation.
                Uri uri = new Uri(currentReaderStream.Uri, location);
                reader.Uri= uri;
                location = uri.ToString();
                index = location.IndexOf(':');
                if (index == -1)
                    return;
                reader.Location= location;
            }
            
            String protocol = location.Substring(0, index).ToLower(CultureInfo.InvariantCulture);
            String value = location.Substring(index+1);
            if (protocol == "file")
            {
                //Console.WriteLine("Loading file:" + value);
                reader.InputStream = new StreamReader(value);
            }
            else if (protocol.StartsWith("http", StringComparison.Ordinal))
            {
                Util.Log("WsdlParser.ParseReaderStreamLocation http "+location);            
                WebRequest request = WebRequest.Create(location);
                WebResponse response = request.GetResponse();
                Stream responseStream = response.GetResponseStream();
                reader.InputStream = new StreamReader(responseStream);
            }
        }
            
        private void ParseImport()
        {
            Util.Log("WsdlParser.ParseImport");         
            String ns = LookupAttribute(s_namespaceString, null, true);
            String location = null;
            location = LookupAttribute(s_locationString, null, false); //wsdl

            Util.Log("WsdlParser.ParseImport "+ns+" location "+location);         
            if (location != null && location.Length > 0)
            {
                ReaderStream reader = new ReaderStream(location);
                ParseReaderStreamLocation(reader, (ReaderStream)_currentReaderStack.Peek());
                ReaderStream.GetReaderStream(_readerStreamsWsdl, reader);
            }
            ReadNextXmlElement();
            return;
        }

        internal void Parse()
        {
            Util.Log("WsdlParser.Parse");                       
            //XmlNameTable primedNametable = CreatePrimedNametable();
            ReaderStream input = _readerStreamsWsdl;
            do
            {
                // Initialize the parser
                _XMLReader = new XmlTextReader(input.InputStream, _primedNametable) { DtdProcessing = DtdProcessing.Ignore };
                _XMLReader.WhitespaceHandling = WhitespaceHandling.None;
                _XMLReader.XmlResolver = null;
                ParseInput(input);
                input = ReaderStream.GetNextReaderStream(input);
            } while (input != null);

            StartWsdlResolution();

            if (null != _writerStreams)
            {
                WriterStream.Close(_writerStreams);
            }

            return;
        }

        // Starts the parser
        private void ParseInput(ReaderStream input)
        {
            Util.Log("WsdlParser.ParseInput" + input.Location);                                  
            _parsingInput = input;
            try
            {
                ReadNextXmlElement();
                String elementName = _XMLReader.LocalName;
                if (MatchingNamespace(s_wsdlNamespaceString) && MatchingStrings(elementName, s_definitionsString))
                {
                    Util.Log("WsdlParser.ParseInput before ParseWsdl "+input.Location);
                    _currentReaderStack.Push(input); // need this to get the base url for relative import elements.
                    ParseWsdl();
                    _currentReaderStack.Pop(); // need this to get the base url for relative import elements.
                }
                else if (MatchingNamespace(s_wsdlNamespaceString) && MatchingStrings(elementName, s_typesString))
                {
                    Util.Log("WsdlParser.ParseInput before ParseWsdlTypes "+input.Location);
                    _currentReaderStack.Push(input); // need this to get the base url for relative import elements.
                    ParseWsdlTypes();
                    _currentReaderStack.Pop(); // need this to get the base url for relative import elements.
                }

                else if (MatchingSchemaNamespace() && MatchingStrings(elementName, s_schemaString))
                {
                    Util.Log("WsdlParser.ParseInput before ParseWsdl "+input.Location);
                    _currentReaderStack.Push(input); // need this to get the base url for relative import elements.
                    ParseSchema();
                    _currentReaderStack.Pop(); // need this to get the base url for relative import elements.
                }
                else
                    throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_UnknownElementAtRootLevel"), elementName));
            }
            finally
            {
                WriterStream.Flush(_writerStreams);
            }
        }

        private void ParseWsdl()
        {
            Util.Log("WsdlParser.ParseWsdl elementName "+_XMLReader.LocalName+" namespace "+ _XMLReader.NamespaceURI);
            int curDepth = _XMLReader.Depth;            
            _parsingInput.Name = LookupAttribute(s_nameString, null, false);
            _parsingInput.TargetNS = LookupAttribute(s_targetNamespaceString, null, false);

            URTNamespace parsingNamespace = ParseNamespace();           

            //ReadNextXmlElement();
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                Util.Log("WsdlParser.ParseWsdl Loop "+_XMLReader.LocalName);
                if (MatchingNamespace(s_wsdlNamespaceString))
                {
                    if (MatchingStrings(elementName,s_typesString))
                    {
                        ParseWsdlTypes();
                        continue;
                    }
                    else if (MatchingStrings(elementName,s_messageString))
                    {
                        ParseWsdlMessage();
                        continue;
                    }
                    else if (MatchingStrings(elementName,s_portTypeString))
                    {
                        ParseWsdlPortType();
                        continue;
                    }
                    else if (MatchingStrings(elementName,s_bindingString))
                    {
                        ParseWsdlBinding(parsingNamespace);
                        continue;
                    }
                    else if (MatchingStrings(elementName,s_serviceString))
                    {
                        ParseWsdlService();
                        continue;
                    }
                    else if (MatchingStrings(elementName, s_importString))
                    {
                        ParseImport();
                        continue;
                    }
                }
                // Ignore others elements such as annotations
                SkipXmlElement();
            }

            
        }

        private void StartWsdlResolution()
        {
            Util.Log("WsdlParser.StartWsdlResolution ");                                  
            DumpWsdl();
            ResolveWsdl();
            Resolve();
            PruneNamespaces();
            Util.Log("WsdlParser.ParseWsdl Invoke PrintCSC");                       
            PrintCSC();
        }
        
        // Since the Wsdl can contains binding sections which are not Rcp,Soap, Encoded. Prune the namespaces from messages etc
        // which are not directly referenced by the Soap binding sections.
        private void PruneNamespaces()
        {
            Util.Log("WsdlParser.PruneNamespaces");       
            ArrayList tempList = new ArrayList(10);
            for (int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if (urtNS.bReferenced)
                    tempList.Add(urtNS);
            }

            _URTNamespaces = tempList;
        }

        [System.Diagnostics.Conditional("_LOGGING")]        
        private void DumpWsdl()
        {
            foreach (DictionaryEntry d in wsdlMessages)
            ((IDump)d.Value).Dump();
            foreach (DictionaryEntry d in wsdlPortTypes)
            ((IDump)d.Value).Dump();                    
            foreach (WsdlBinding item in wsdlBindings)
            item.Dump();
            foreach (WsdlService item in wsdlServices)
            item.Dump();            
        }

        private void ParseWsdlTypes()
        {
            Util.Log("WsdlParser.ParseWsdlTypes");                                              
            int curDepth = _XMLReader.Depth;
            ReadNextXmlElement();               
            
            // Initialize mechanism to support xsd import statements;
            _currentSchemaReaderStack.Push(_currentReaderStack.Peek()); // put current wsdl location as xsd location
            
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if (MatchingSchemaNamespace())
                {
                    if (MatchingStrings(elementName, s_schemaString))
                    {
                        ParseSchema();
                        if (_readerStreamsXsd != null)
                        {
                            // import element appeared in schema
                            ParseImportedSchemaController();
                        }

                        continue;
                    }
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
            _currentSchemaReaderStack.Pop();
        }


        private void ParseSchemaIncludeElement()
        {
            ParseSchemaImportElement(false);
        }

        private void ParseSchemaImportElement()
        {
            ParseSchemaImportElement(true);
        }


        // Processes an import statement in an xsd schema.
        private void ParseSchemaImportElement(bool bImport)
        { 
            Util.Log("WsdlParser.ParseSchemaImportElement IsImport "+bImport);         
            String ns = null;
            if (bImport)
                ns = LookupAttribute(s_namespaceString, null, true);
            String location = null;
            location = LookupAttribute(s_schemaLocationString, null, false);

            Util.Log("WsdlParser.ParseSchemaImportElement "+ns+" location "+location);         
            if (location != null && location.Length > 0)
            {
                if (_readerStreamsXsd == null)
                {
                _readerStreamsXsd = new ReaderStream(location);
                ParseReaderStreamLocation(_readerStreamsXsd, (ReaderStream)_currentSchemaReaderStack.Peek()); 
                }
                else
                {
                    ReaderStream reader = new ReaderStream(location);
                    ParseReaderStreamLocation(reader, (ReaderStream)_currentSchemaReaderStack.Peek());
                    ReaderStream.GetReaderStream(_readerStreamsWsdl, reader);
                }
            }
            ReadNextXmlElement();
            return;
        }

        // Controls the processing of the imported xsd schema.
        internal void ParseImportedSchemaController()
        {
            Util.Log("WsdlParser.ParseImportedSchemaController");  
            XmlNameTable primedNametable = CreatePrimedNametable();
            ReaderStream input = _readerStreamsXsd;
            XmlTextReader _XMLReaderWsdl = _XMLReader;
            ReaderStream _parsingInputWsdl = _parsingInput;
            do
            {
                // Initialize the parser
                _XMLReader = new XmlTextReader(input.InputStream, _primedNametable) { DtdProcessing = DtdProcessing.Ignore };
                _XMLReader.WhitespaceHandling = WhitespaceHandling.None;
                _XMLReader.XmlResolver = null;
                _parsingInput = input;
                ParseImportedSchema(input);
                input = ReaderStream.GetNextReaderStream(input);
            } while (input != null);
            _readerStreamsXsd = null;
            _XMLReader = _XMLReaderWsdl;
            _parsingInput = _parsingInputWsdl;
            return;
        }

        // Process the xsd schema imported from an xsd import statement
        private void ParseImportedSchema(ReaderStream input)
        {
            Util.Log("WsdlParser.ParseImportedSchema "+input.Location);                                  
            try
            {
                String elementName = _XMLReader.LocalName;
                _currentSchemaReaderStack.Push(input); // need this to get the base url for relative import elements.
                ReadNextXmlElement();               
                ParseSchema();
                _currentSchemaReaderStack.Pop(); // need this to get the base url for relative import elements.
            }
            finally
            {
                WriterStream.Flush(_writerStreams);
            }
        }

        private void ParseWsdlMessage()
        {
            Util.Log("WsdlParser.ParseWsdlMessage");
            WsdlMessage message = new WsdlMessage();
            message.name = LookupAttribute(s_nameString, null, true);
            message.nameNs = _parsingInput.TargetNS;
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingStrings(elementName, s_partString))
                {
                    WsdlMessagePart part = new WsdlMessagePart();
                    part.name = LookupAttribute(s_nameString, null, true); 
                    part.nameNs = _parsingInput.TargetNS;
                    //AddToNamespace(part.nameNs);//temp
                    part.element = LookupAttribute(s_elementString, null, false);
                    part.typeName = LookupAttribute(s_typeString, null, false);
                    if (part.element != null)
                    {
                        part.elementNs = ParseQName(ref part.element);
                    }
                    if (part.typeName != null)
                    {
                        part.typeNameNs = ParseQName(ref part.typeName); 
                    }

                    message.parts.Add(part);
                    ReadNextXmlElement();
                    continue;
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
            wsdlMessages[message.name] = message;
        }

        private void ParseWsdlPortType()
        {
            Util.Log("WsdlParser.ParseWsdlPortType");
            WsdlPortType portType = new WsdlPortType();
            portType.name = LookupAttribute(s_nameString, null, true);
            //portType.nameNs = ParseQName(ref portType.name);
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingStrings(elementName, s_operationString))
                {
                    WsdlPortTypeOperation portTypeOperation = new WsdlPortTypeOperation();
                    portTypeOperation.name = LookupAttribute(s_nameString, null, true);
                    portTypeOperation.nameNs = ParseQName(ref portTypeOperation.nameNs);
                    portTypeOperation.parameterOrder = LookupAttribute(s_parameterOrderString, null, false);
                    ParseWsdlPortTypeOperationContent(portType, portTypeOperation);
                    portType.operations.Add(portTypeOperation);
                    continue;
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
            wsdlPortTypes[portType.name] = portType;
        }


        private void ParseWsdlPortTypeOperationContent(WsdlPortType portType, WsdlPortTypeOperation portTypeOperation)
        { 
            Util.Log("WsdlParser.ParseWsdlPortTypeOperationContent type "+portType.name+" operationName "+portTypeOperation.name);
            int curDepth = _XMLReader.Depth; 

            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingStrings(elementName, s_inputString))
                {
                    WsdlPortTypeOperationContent portContent = new WsdlPortTypeOperationContent();
                    portContent.element = Atomize("input");
                    portContent.name = LookupAttribute(s_nameString, null, false);

                    if (MatchingStrings(portContent.name, s_emptyString))
                    {
                        portContent.name = Atomize(portTypeOperation.name+"Request");
                        if (portType.sections.ContainsKey(portContent.name))
                            throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_DuplicatePortTypesOperationName"), portTypeOperation.name));

                        portType.sections[portContent.name] = portTypeOperation; //for request response
                        portType.sections[portTypeOperation.name] = portTypeOperation; // for one way don't know yet if one way or response
                    }
                    else
                    {
                        if (portType.sections.ContainsKey(portContent.name))
                            throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_DuplicatePortSectionName"), portContent.name));


                        portType.sections[portContent.name] = portTypeOperation;
                    }

                    portContent.message = LookupAttribute(s_messageString, null, true);
                    portContent.messageNs = ParseQName(ref portContent.message);
                    portTypeOperation.contents.Add(portContent);
                    ReadNextXmlElement();
                    continue;                   
                }
                else if (MatchingStrings(elementName, s_outputString))
                {
                    WsdlPortTypeOperationContent portContent = new WsdlPortTypeOperationContent();                  
                    portContent.element = Atomize("output");
                    portContent.name = LookupAttribute(s_nameString, null, false);
                    portContent.nameNs = ParseQName(ref portContent.name);
                    if (MatchingStrings(portContent.name, s_emptyString))
                        portContent.name = Atomize(portTypeOperation.name+"Response");
                    if (!portType.sections.ContainsKey(portContent.name))
                        portType.sections[portContent.name] = portTypeOperation;
                    portContent.message = LookupAttribute(s_messageString, null, true);
                    portContent.messageNs = ParseQName(ref portContent.message);
                    portTypeOperation.contents.Add(portContent);                    
                    ReadNextXmlElement();
                    continue;                   
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
        }


        private void ParseWsdlBinding(URTNamespace inparsingNamespace)
        {
            Util.Log("WsdlParser.ParseWsdlBinding");
            WsdlBinding binding = new WsdlBinding();
            binding.name = LookupAttribute(s_nameString, null, true);
            //binding.nameNs = ParseQName(ref binding.name);
            binding.type = LookupAttribute(s_typeString, null, true);
            binding.typeNs = ParseQName(ref binding.type);
            URTNamespace parsingNamespace = LookupNamespace(binding.typeNs);
            if (parsingNamespace == null)
            {
                parsingNamespace = new URTNamespace(binding.typeNs, this); 
            }
            binding.parsingNamespace = parsingNamespace;
            bool bSoapBinding = false;
            bool bRpcBinding = false;
            bool bSoapEncoded = false;
            bool bSoapSuds = false;
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if (MatchingNamespace(s_wsdlSoapNamespaceString) &&
                    MatchingStrings(elementName, s_bindingString))
                {
                    bSoapBinding = true;
                    WsdlBindingSoapBinding sb = new WsdlBindingSoapBinding();
                    sb.style = LookupAttribute(s_styleString, null, true);
                    if (sb.style == "rpc")
                        bRpcBinding = true;

                    /*
                    if (sb.style == "document")
                    {
                        throw new SUDSParserException(
                            String.Format(CoreChannel.GetResourceString("Remoting_Suds_SoapStyleNotSupported"),
                                          sb.style));
                    }
                    */

                    sb.transport = LookupAttribute(s_transportString, null, true);
                    binding.soapBinding = sb;
                    ReadNextXmlElement();
                    continue;                   
                }
                else if (MatchingNamespace(s_wsdlSudsNamespaceString))
                {
                    bSoapSuds = true;
                    if (MatchingStrings(elementName, s_classString) || MatchingStrings(elementName, s_structString))
                    {
                        WsdlBindingSuds suds = new WsdlBindingSuds();
                        suds.elementName = elementName;
                        suds.typeName = LookupAttribute(s_typeString, null, true);
                        suds.ns = ParseQName(ref suds.typeName);
                        suds.extendsTypeName = LookupAttribute(s_extendsString, null, false);
                        String use = LookupAttribute(s_rootTypeString, null, false);
                        suds.sudsUse = ProcessSudsUse(use, elementName);
                        if (!MatchingStrings(suds.extendsTypeName, s_emptyString))
                            suds.extendsNs = ParseQName(ref suds.extendsTypeName);
                        ParseWsdlBindingSuds(suds);
                        binding.suds.Add(suds);
                        continue;                       
                    }
                    else if (MatchingStrings(elementName, s_interfaceString))
                    {
                        WsdlBindingSuds suds = new WsdlBindingSuds();                       
                        suds.elementName = elementName; //Atomize("interface");
                        suds.typeName = LookupAttribute(s_typeString, null, true);
                        suds.ns = ParseQName(ref suds.typeName);        
                        String use = LookupAttribute(s_rootTypeString, null, false);
                        suds.sudsUse = ProcessSudsUse(use, elementName);
                        ParseWsdlBindingSuds(suds);
                        binding.suds.Add(suds);
                        continue;                       
                    }
                }
                else if (MatchingNamespace(s_wsdlNamespaceString) &&
                         MatchingStrings(elementName, s_operationString))
                {
                    WsdlBindingOperation op = new WsdlBindingOperation();
                    op.name = LookupAttribute(s_nameString, null, true); 
                    op.nameNs = _parsingInput.TargetNS;
                    ParseWsdlBindingOperation(op, ref bRpcBinding, ref bSoapEncoded);
                    binding.operations.Add(op);
                    continue;                   
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
            if (bSoapBinding && bRpcBinding && bSoapEncoded || bSoapSuds)
                wsdlBindings.Add(binding);
        }

        private void ParseWsdlBindingSuds(WsdlBindingSuds suds)
        {
            Util.Log("WsdlParser.ParseWsdlBindingSuds");            
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingStrings(elementName, s_implementsString) ||
                    MatchingStrings(elementName, s_extendsString))
                {
                    WsdlBindingSudsImplements impl = new WsdlBindingSudsImplements();
                    impl.typeName = LookupAttribute(s_typeString, null, true);
                    impl.ns = ParseQName(ref impl.typeName);
                    suds.implements.Add(impl);
                    ReadNextXmlElement();
                    continue;                   
                }
                else if (MatchingStrings(elementName, s_nestedTypeString))
                {
                    WsdlBindingSudsNestedType nestedType = new WsdlBindingSudsNestedType();
                    nestedType.name = LookupAttribute(s_nameString, null, true);
                    nestedType.typeName = LookupAttribute(s_typeString, null, true);
                    nestedType.ns = ParseQName(ref nestedType.typeName);
                    suds.nestedTypes.Add(nestedType);
                    ReadNextXmlElement();
                    continue;                   
                }


                // Ignore others elements such as annotations
                SkipXmlElement();
            }
        }

        private SudsUse ProcessSudsUse(String use, String elementName)
        {
            Util.Log("WsdlParser.ProcessSudsUse use enter "+use+" elementName "+elementName);            
            SudsUse sudsUse = SudsUse.Class;

            if (use == null || use.Length == 0)
                use = elementName;

            if (MatchingStrings(use, s_interfaceString))
                sudsUse = SudsUse.Interface;
            else if (MatchingStrings(use, s_classString))
                sudsUse = SudsUse.Class;
            else if (MatchingStrings(use, s_structString))
                sudsUse = SudsUse.Struct;
            else if (MatchingStrings(use, s_ISerializableString))
                sudsUse = SudsUse.ISerializable;
            else if (MatchingStrings(use, s_marshalByRefString))
                sudsUse = SudsUse.MarshalByRef;
            else if (MatchingStrings(use, s_delegateString))
                sudsUse = SudsUse.Delegate;
            else if (MatchingStrings(use, s_servicedComponentString))
                sudsUse = SudsUse.ServicedComponent;

            Util.Log("WsdlParser.ProcessSudsUse use exit "+((Enum)sudsUse).ToString());
            return sudsUse;
        }

        private void ParseWsdlBindingOperation(WsdlBindingOperation op, ref bool bRpcBinding, ref bool bSoapEncoded)
        {
            Util.Log("WsdlParser.ParseWsdlBindingOperation");                                                                                   
            int curDepth = _XMLReader.Depth; 
            bool binput = false;
            bool boutput = false;
            WsdlBindingOperationSection opSec = null;

            ReadNextXmlElement();               

            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingNamespace(s_wsdlSudsNamespaceString) && MatchingStrings(elementName, s_methodString))
                {
                    op.methodAttributes = LookupAttribute(s_attributesString, null, true);
                    ReadNextXmlElement();                                   
                    continue;                                       
                }
                else if (MatchingNamespace(s_wsdlSoapNamespaceString) &&
                         MatchingStrings(elementName, s_operationString))
                {
                    WsdlBindingSoapOperation soapOp = new WsdlBindingSoapOperation();
                    soapOp.soapAction = LookupAttribute(s_soapActionString, null, false);
                    soapOp.style = LookupAttribute(s_styleString, null, false);
                    if (soapOp.style == "rpc")
                        bRpcBinding = true;
                    {
                    }
                    op.soapOperation = soapOp;
                    ReadNextXmlElement();                                   
                    continue;                   
                }
                else if (MatchingNamespace(s_wsdlNamespaceString))
                {
                    if (MatchingStrings(elementName, s_inputString))
                    {
                        binput = true;
                        opSec = ParseWsdlBindingOperationSection(op, elementName, ref bSoapEncoded);
                        continue;                       
                    }
                    else if (MatchingStrings(elementName, s_outputString))
                    {
                        boutput = true;
                        ParseWsdlBindingOperationSection(op, elementName, ref bSoapEncoded);
                        continue;                       
                    }
                    else if (MatchingStrings(elementName, s_faultString))
                    {
                        ParseWsdlBindingOperationSection(op, elementName, ref bSoapEncoded);
                        continue;                       
                    }
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }

            // if no output section then the default name is just the op name.
            if (opSec != null && binput && !boutput)
                opSec.name = op.name;


        }

        private WsdlBindingOperationSection ParseWsdlBindingOperationSection(WsdlBindingOperation op, String inputElementName, ref bool bSoapEncoded)
        {
            Util.Log("WsdlParser.ParseWsdlBindingOperationSections");
            bool breturn = false;
            WsdlBindingOperationSection opSec = new WsdlBindingOperationSection();
            op.sections.Add(opSec);
            opSec.name = LookupAttribute(s_nameString, null, false);
            if (MatchingStrings(opSec.name, s_emptyString))
            {
                if (MatchingStrings(inputElementName, s_inputString))
                {
                    breturn = true;
                    opSec.name = Atomize(op.name+"Request");
                }
                else if (MatchingStrings(inputElementName, s_outputString))
                    opSec.name = Atomize(op.name+"Response");
            }
            opSec.elementName = inputElementName;
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if (MatchingNamespace(s_wsdlSoapNamespaceString))
                {
                    if (MatchingStrings(elementName, s_bodyString))
                    {
                        WsdlBindingSoapBody soapBody = new WsdlBindingSoapBody();
                        opSec.extensions.Add(soapBody);
                        soapBody.parts = LookupAttribute(s_partsString, null, false);
                        soapBody.use = LookupAttribute(s_useString, null, true); 
                        if (soapBody.use == "encoded")
                            bSoapEncoded = true;
                        soapBody.encodingStyle = LookupAttribute(s_encodingStyleString, null, false);
                        soapBody.namespaceUri = LookupAttribute(s_namespaceString, null, false);
                        ReadNextXmlElement();
                        continue;                       
                    }
                    else if (MatchingStrings(elementName, s_headerString))
                    {
                        WsdlBindingSoapHeader soapHeader = new WsdlBindingSoapHeader();
                        opSec.extensions.Add(soapHeader);
                        soapHeader.message = LookupAttribute(s_messageString, null, true);  
                        soapHeader.messageNs = ParseQName(ref soapHeader.message);
                        soapHeader.part = LookupAttribute(s_partString, null, true);
                        soapHeader.use = LookupAttribute(s_useString, null, true);
                        soapHeader.encodingStyle = LookupAttribute(s_encodingStyleString, null, false);
                        soapHeader.namespaceUri = LookupAttribute(s_namespaceString, null, false);
                        ReadNextXmlElement();
                        continue;                       
                    }
                    else if (MatchingStrings(elementName, s_faultString))
                    {
                        WsdlBindingSoapFault soapFault = new WsdlBindingSoapFault();
                        opSec.extensions.Add(soapFault);
                        soapFault.name = LookupAttribute(s_nameString, null, true);  
                        soapFault.use = LookupAttribute(s_useString, null, true);
                        soapFault.encodingStyle = LookupAttribute(s_encodingStyleString, null, false);
                        soapFault.namespaceUri = LookupAttribute(s_namespaceString, null, false);
                        ReadNextXmlElement();
                        continue;                       
                    }

                }
                // Ignore others elements such as annotations
                // headerFault currently ignored
                SkipXmlElement();
            }

            // returning opSec only if a fixup might be necessary, this is the case of an input section with an empty input name
            // it will be fixed up later if there is no output section
            if (breturn)
                return opSec;
            else
                return null;
        }

        private void ParseWsdlService()
        {
            Util.Log("WsdlParser.ParseWsdlService");
            WsdlService service = new WsdlService();
            service.name = LookupAttribute(s_nameString, null, true);
            //service.nameNs = ParseQName(ref service.name);
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;

                if (MatchingNamespace(s_wsdlNamespaceString) &&
                    MatchingStrings(elementName, s_portString))
                {
                    WsdlServicePort port = new WsdlServicePort();
                    port.name = LookupAttribute(s_nameString, null, true);
                    port.nameNs = ParseQName(ref port.nameNs);
                    port.binding = LookupAttribute(s_bindingString, null, true);
                    port.bindingNs = ParseQName(ref port.binding);
                    ParseWsdlServicePort(port);
                    service.ports[port.binding] = port;
                    continue;                   
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
            wsdlServices.Add(service);
        }

        private void ParseWsdlServicePort(WsdlServicePort port)
        {
            Util.Log("WsdlParser.ParseWsdlServicePort");
            int curDepth = _XMLReader.Depth; 
            ReadNextXmlElement();               
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                if (MatchingNamespace(s_wsdlSoapNamespaceString) &&
                    MatchingStrings(elementName, s_addressString))
                {
                    if (port.locations == null)
                        port.locations = new ArrayList(10);
                    port.locations.Add(LookupAttribute(s_locationString, null, true));
                    ReadNextXmlElement();
                    continue;                   
                }

                // Ignore others elements such as annotations
                SkipXmlElement();
            }
        }

        private void ResolveWsdl()
        {
            Util.Log("WsdlParser.ResolveWsdl ");

            if (wsdlBindings.Count == 0)
                throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_RpcBindingsMissing")));

            foreach (WsdlBinding binding in wsdlBindings)
            {
                // Only creating classes for soap
                if (binding.soapBinding != null)
                {
                    if (binding.suds != null && binding.suds.Count > 0)
                    {
                        bool bFirstSuds = true;
                        foreach (WsdlBindingSuds suds in binding.suds)
                        {
                            if (MatchingStrings(suds.elementName, s_classString) || MatchingStrings(suds.elementName, s_structString))
                            {
                                ResolveWsdlClass(binding, suds, bFirstSuds);
                                bFirstSuds = false;
                            }
                            else if (MatchingStrings(suds.elementName, s_interfaceString))
                                ResolveWsdlInterface(binding, suds);
                            else
                            {
                                throw new SUDSParserException(
                                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveElementInNS"),
                                                                           suds.elementName, s_wsdlSudsNamespaceString));
                            }
                        }
                    }
                    else
                        ResolveWsdlClass(binding, null, true); // No suds, create a default class
                }
            }
        }

        private void ResolveWsdlClass(WsdlBinding binding, WsdlBindingSuds suds, bool bFirstSuds)
        {
            // bFirstSuds is true then the suds class is the class for the binding. 
            // The other suds are additional class without bindings in the same namespace.
            Util.Log("WsdlParser.ResolveWsdlClass suds "+suds);
            URTComplexType parsingComplexType;
            //URTNamespace parsingNamespace = binding.parsingNamespace;
            URTNamespace parsingNamespace;
            //URTNamespace parsingNamespace = binding.typeNs;

            if (suds != null)
            {
                Util.Log("WsdlParser.ResolveWsdlClass suds not null "+suds.elementName+" "+suds.typeName);
                // Suds
                parsingNamespace = AddNewNamespace(suds.ns);
                parsingComplexType = parsingNamespace.LookupComplexType(suds.typeName);
                if (parsingComplexType == null)
                {
                    parsingComplexType = new URTComplexType(suds.typeName, parsingNamespace.Name, parsingNamespace.Namespace, parsingNamespace.EncodedNS, _blockDefault ,false, false, this, parsingNamespace);
                    parsingNamespace.AddComplexType(parsingComplexType);
                }

                if (MatchingStrings(suds.elementName, s_structString))
                    parsingComplexType.IsValueType = true;

                parsingComplexType.SudsUse = suds.sudsUse;

                if (suds.sudsUse == SudsUse.MarshalByRef || 
                    suds.sudsUse == SudsUse.ServicedComponent
                   )
                {
                    Util.Log("WsdlParser.ResolveWsdlClass MarshalByRef IsSudsType true 1 "+suds.elementName+" "+suds.typeName);
                    parsingComplexType.IsSUDSType = true; 

                    if (_bWrappedProxy)
                        parsingComplexType.SUDSType = SUDSType.ClientProxy;
                    else
                        parsingComplexType.SUDSType = SUDSType.MarshalByRef;


                    if ((suds.extendsTypeName != null) && (suds.extendsTypeName.Length > 0))
                    {
                        URTNamespace extendsNamespace = AddNewNamespace(suds.extendsNs);                    
                        /*
                        if (extendsNamespace == null)
                            extendsNamespace = new URTNamespace(suds.extendsTypeName, this);
                            */

                        URTComplexType extendsComplexType = extendsNamespace.LookupComplexType(suds.extendsTypeName);                   
                        if (extendsComplexType == null)
                        {
                            extendsComplexType = new URTComplexType(suds.extendsTypeName, extendsNamespace.Name, extendsNamespace.Namespace, extendsNamespace.EncodedNS, _blockDefault ,true, false, this, extendsNamespace);
                            extendsNamespace.AddComplexType(extendsComplexType);
                        }
                        else
                        {
                            Util.Log("WsdlParser.ResolveWsdlClass IsSudsType true 2 "+suds.elementName+" "+suds.typeName);
                            extendsComplexType.IsSUDSType = true;
                        }

                        if (_bWrappedProxy)
                            extendsComplexType.SUDSType = SUDSType.ClientProxy;
                        else
                            extendsComplexType.SUDSType = SUDSType.MarshalByRef;

                        extendsComplexType.SudsUse = suds.sudsUse;

                        // Only top of inheritance hierarchy is marked
                        //parsingComplexType.SUDSType = SUDSType.None; 
                    }
                }

                foreach (WsdlBindingSudsNestedType nestedType in suds.nestedTypes)
                {
                    ResolveWsdlNestedType(binding, suds, nestedType);
                }
            }
            else
            {
                // No suds
                Util.Log("WsdlParser.ResolveWsdlClass no suds ");
                parsingNamespace = AddNewNamespace(binding.typeNs);
                String name = binding.name;
                int index = binding.name.IndexOf("Binding");
                if (index > 0)
                {
                    //name = Atomize(binding.name.Substring(0,index));
                    name = binding.name.Substring(0,index);
                }

                parsingComplexType = parsingNamespace.LookupComplexTypeEqual(name);
                if (parsingComplexType == null)
                {
                    parsingComplexType = new URTComplexType(name, parsingNamespace.Name, parsingNamespace.Namespace, parsingNamespace.EncodedNS, _blockDefault ,true, false, this, parsingNamespace);
                    parsingNamespace.AddComplexType(parsingComplexType);                    
                }
                else
                {
                    Util.Log("WsdlParser.ResolveWsdlClass IsSudsType true 3 "+name);
                    parsingComplexType.IsSUDSType = true;
                }
                if (_bWrappedProxy)
                    parsingComplexType.SUDSType = SUDSType.ClientProxy;
                else
                    parsingComplexType.SUDSType = SUDSType.MarshalByRef;

                parsingComplexType.SudsUse = SudsUse.MarshalByRef;
            }

            // Resolve address
            parsingComplexType.ConnectURLs = ResolveWsdlAddress(binding);

            // Resolve extends and implements
            if (suds != null)
            {
                if (!MatchingStrings(suds.extendsTypeName, s_emptyString))
                {
                    parsingComplexType.Extends(suds.extendsTypeName, suds.extendsNs);
                }

                foreach (WsdlBindingSudsImplements impl in suds.implements)
                parsingComplexType.Implements(impl.typeName, impl.ns, this);  
            }




            if (bFirstSuds && 
                (parsingComplexType.SudsUse == SudsUse.MarshalByRef || 
                 parsingComplexType.SudsUse == SudsUse.ServicedComponent || 
                 parsingComplexType.SudsUse == SudsUse.Delegate || 
                 parsingComplexType.SudsUse == SudsUse.Interface))
            {
                // Resolve methods

                ArrayList methodInfos = ResolveWsdlMethodInfo(binding);

                foreach (WsdlMethodInfo methodInfo in methodInfos)
                {
                    if ((methodInfo.inputMethodName != null) && (methodInfo.outputMethodName != null))
                    {
                        RRMethod parsingRRMethod = new RRMethod(methodInfo, parsingComplexType);
                        parsingRRMethod.AddRequest(methodInfo.methodName, methodInfo.methodNameNs);
                        parsingRRMethod.AddResponse(methodInfo.methodName, methodInfo.methodNameNs);
                        parsingComplexType.AddMethod(parsingRRMethod);
                    }
                    else if (methodInfo.inputMethodName != null)
                    {
                        OnewayMethod parsingOWMethod = new OnewayMethod(methodInfo, parsingComplexType);
                        parsingComplexType.AddMethod(parsingOWMethod);
                        parsingOWMethod.AddMessage(methodInfo.methodName, methodInfo.methodNameNs);
                    }
                    else
                    {
                        throw new SUDSParserException(
                                                     String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidMessage"),
                                                                   methodInfo.methodName));
                    }
                }
            }
        }

        private void ResolveWsdlInterface(WsdlBinding binding, WsdlBindingSuds suds)
        {
            Util.Log("WsdlParser.ResolveWsdlInterface "+binding.name+" ns "+binding.parsingNamespace.Namespace+" suds "+suds.typeName);

            URTNamespace parsingNamespace = binding.parsingNamespace;

            URTNamespace sudsNamespace = AddNewNamespace(suds.ns);
            /*
            if (sudsNamespace == null)
            {
                sudsNamespace = new URTNamespace(suds.ns, this);
            }
            */

            URTInterface parsingInterface = sudsNamespace.LookupInterface(suds.typeName);           
            if (parsingInterface == null)
            {
                parsingInterface = new URTInterface(suds.typeName, sudsNamespace.Name, sudsNamespace.Namespace, sudsNamespace.EncodedNS, this);
                sudsNamespace.AddInterface(parsingInterface);
            }

            if (suds.extendsTypeName != null)
            {
                parsingInterface.Extends(suds.extendsTypeName, suds.extendsNs, this);
            }
            foreach (WsdlBindingSudsImplements impl in suds.implements)
            {
                parsingInterface.Extends(impl.typeName, impl.ns, this);             
            }

            ArrayList methodInfos = ResolveWsdlMethodInfo(binding);

            foreach (WsdlMethodInfo methodInfo in methodInfos)          
            {
                if ((methodInfo.inputMethodName != null) && (methodInfo.outputMethodName != null))
                {
                    RRMethod parsingRRMethod = new RRMethod(methodInfo, null);
                    parsingRRMethod.AddRequest(methodInfo.methodName, methodInfo.methodNameNs);
                    parsingRRMethod.AddResponse(methodInfo.methodName, methodInfo.methodNameNs);
                    parsingInterface.AddMethod(parsingRRMethod);
                }
                else if (methodInfo.inputMethodName != null)
                {
                    OnewayMethod parsingOWMethod = new OnewayMethod(methodInfo.methodName, methodInfo.soapAction, null);
                    parsingOWMethod.AddMessage(methodInfo.methodName, methodInfo.methodNameNs);                 
                    parsingInterface.AddMethod(parsingOWMethod);
                }
                else
                {
                    throw new SUDSParserException(
                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidMessage"),
                                                               methodInfo.methodName));

                }
            }
        }

        private void ResolveWsdlNestedType(WsdlBinding binding, WsdlBindingSuds suds, WsdlBindingSudsNestedType nested)
        {
            Util.Log("WsdlParser.ResolveWsdlNestedType "+binding.name+" ns "+binding.parsingNamespace.Namespace+" suds "+suds.typeName+" nestedName "+nested.name+" nestedTypeName "+nested.typeName);
            String className = suds.typeName;
            String ns = nested.ns;
            String nestedName = nested.name;
            String nestedTypeName = nested.typeName;
            if (suds.ns != ns)
            {
                throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveNestedTypeNS"),
                                                           suds.typeName, suds.ns));
            }

            URTNamespace sudsNamespace = AddNewNamespace(suds.ns);

            URTComplexType outerType = sudsNamespace.LookupComplexType(suds.typeName);
            if (outerType == null)
            {
                throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveNestedType"),
                                                           suds.typeName, suds.ns));
            }

            BaseType innerType = sudsNamespace.LookupType(nested.typeName);
            if (innerType == null)
            {
                // Can be URTSimpleType for Enum
                Util.Log("WsdlParser.ResolveWsdlNestedType cann't find inner type "+nested.typeName+" className "+className+" ns "+ns);

                innerType = sudsNamespace.LookupComplexType(nested.typeName);
                if (innerType == null)
                {
                    innerType = new URTComplexType(nested.typeName, sudsNamespace.Name, sudsNamespace.Namespace, sudsNamespace.EncodedNS, _blockDefault ,false, false, this, sudsNamespace);
                    sudsNamespace.AddComplexType((URTComplexType)innerType);
                }
            }


            innerType.bNestedType = true;
            innerType.NestedTypeName = nested.name;
            innerType.FullNestedTypeName = nested.typeName;
            innerType.OuterTypeName = suds.typeName;

            outerType.AddNestedType(innerType);
        }

        private ArrayList ResolveWsdlAddress(WsdlBinding binding)
        {
            Util.Log("WsdlParser.ResolveWsdlAddress "+binding.name);            
            ArrayList serviceEndpoints = null;
            if (_bWrappedProxy)
            {
                foreach (WsdlService service in wsdlServices)
                {
                    WsdlServicePort port = (WsdlServicePort)service.ports[binding.name];
                    if (port != null)
                    {
                        serviceEndpoints = port.locations;
                        break;
                    }
                    if (serviceEndpoints != null)
                        break;
                }
            }
            return serviceEndpoints;
        }

        private ArrayList ResolveWsdlMethodInfo(WsdlBinding binding)
        {
            Util.Log("WsdlParser.ResolveWsdlMethodInfo "+binding.name);                     
            ArrayList methodInfos = new ArrayList(10);
            Hashtable propertyMethod = new Hashtable(3);
            for (int i=0; i<binding.operations.Count; i++)
            {
                bool bGet = false;
                bool bSet = false;
                WsdlBindingOperation op = (WsdlBindingOperation)binding.operations[i];
                if (op.soapOperation != null)
                {
                    WsdlMethodInfo methodInfo = new WsdlMethodInfo();
                    methodInfo.methodName = op.name;
                    methodInfo.methodNameNs = op.nameNs;
                    methodInfo.methodAttributes = op.methodAttributes;
                    AddNewNamespace(op.nameNs);
                    WsdlBindingSoapOperation opSoap = (WsdlBindingSoapOperation)op.soapOperation;

                    if ((methodInfo.methodName.StartsWith("get_", StringComparison.Ordinal) && methodInfo.methodName.Length > 4))
                        bGet = true;
                    else if ((methodInfo.methodName.StartsWith("set_", StringComparison.Ordinal) && methodInfo.methodName.Length > 4))
                        bSet = true;
                    if (bGet || bSet)
                    {
                        bool bNew = false;
                        String propertyName = methodInfo.methodName.Substring(4);
                        WsdlMethodInfo propertyMethodInfo = (WsdlMethodInfo)propertyMethod[propertyName];
                        if (propertyMethodInfo == null)
                        {
                            propertyMethod[propertyName] = methodInfo;
                            methodInfos.Add(methodInfo);
                            propertyMethodInfo = methodInfo;
                            methodInfo.propertyName = propertyName;
                            methodInfo.bProperty = true;
                            bNew = true;
                        }

                        if (bGet)
                        {
                            propertyMethodInfo.bGet = true;
                            propertyMethodInfo.soapActionGet = opSoap.soapAction;
                        }
                        else
                        {
                            propertyMethodInfo.bSet = true;
                            propertyMethodInfo.soapActionSet = opSoap.soapAction;
                            //propertyMethodInfo.Dump();
                        }

                        if (!bNew)
                            continue; //already processed this property
                    }
                    else
                        methodInfos.Add(methodInfo);
                    methodInfo.soapAction = opSoap.soapAction;

                    WsdlPortType portType = (WsdlPortType)wsdlPortTypes[binding.type];

                    if ((portType == null) || (portType.operations.Count != binding.operations.Count))
                    {
                        throw new SUDSParserException(
                                                     String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidPortType"),
                                                                   binding.type));
                    }

                    // PortType operations are obtained by the <binding><operation><input name = porttype operation>

                    WsdlPortTypeOperation portTypeOp = null;
                    foreach (WsdlBindingOperationSection opSec in op.sections)
                    {
                        if (MatchingStrings(opSec.elementName, s_inputString))
                        {

                            portTypeOp = (WsdlPortTypeOperation)portType.sections[opSec.name];

                            Util.Log("WsdlParser.ResolveWsdlMethodInfo find portTypeOp 1 "+opSec.name+" portTypeOp "+portTypeOp);

                            if (portTypeOp == null)
                            {
                                //this is for interop testing because other implementations are using the opSec.name wrong.
                                // a "Request" is not being added to the end of the name.
                                int index  = opSec.name.LastIndexOf("Request");
                                if (index > 0)
                                {
                                    String newOpName = opSec.name.Substring(0, index);
                                    portTypeOp = (WsdlPortTypeOperation)portType.sections[newOpName];
                                    Util.Log("WsdlParser.ResolveWsdlMethodInfo find portTypeOp 2 "+newOpName+" portTypeOp "+portTypeOp);
                                }
                            }

                            if (portTypeOp != null && portTypeOp.parameterOrder != null && portTypeOp.parameterOrder.Length > 0)
                            {
                                methodInfo.paramNamesOrder = portTypeOp.parameterOrder.Split(' ');
                            }

                            foreach (WsdlBindingSoapBody body in opSec.extensions)
                            {
                                if (body.namespaceUri != null || body.namespaceUri.Length > 0)
                                    methodInfo.inputMethodNameNs = body.namespaceUri;
                            }
                        }
                        else if (MatchingStrings(opSec.elementName, s_outputString))
                        {
                            foreach (WsdlBindingSoapBody body in opSec.extensions)
                            {
                                if (body.namespaceUri != null || body.namespaceUri.Length > 0)
                                    methodInfo.outputMethodNameNs = body.namespaceUri;
                            }
                        }
                    }

                    /*
                    if (portTypeOp == null)
                    {
                    throw new SUDSParserException(
                    String.Format(CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidPortType"),
                    binding.type));
                    }
                    */

                    if (portTypeOp != null)
                    {

                        foreach (WsdlPortTypeOperationContent content in portTypeOp.contents)
                        {
                            if (MatchingStrings(content.element, s_inputString))
                            {
                                methodInfo.inputMethodName = content.message;
                                if (methodInfo.inputMethodNameNs == null)
                                    methodInfo.inputMethodNameNs = content.messageNs;
                                WsdlMessage message = (WsdlMessage)wsdlMessages[content.message];
                                if (message == null)
                                {
                                    throw new SUDSParserException(
                                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlMissingMessage"),
                                                                               content.message));
                                }
                                else
                                {
                                    if (message.parts != null)
                                    {
                                        methodInfo.inputNames = new String[message.parts.Count];
                                        methodInfo.inputNamesNs = new String[message.parts.Count];
                                        methodInfo.inputElements = new String[message.parts.Count];
                                        methodInfo.inputElementsNs = new String[message.parts.Count];
                                        methodInfo.inputTypes = new String[message.parts.Count];
                                        methodInfo.inputTypesNs = new String[message.parts.Count];
                                        for (int ip=0; ip<message.parts.Count; ip++)
                                        {
                                            methodInfo.inputNames[ip] = ((WsdlMessagePart)message.parts[ip]).name;
                                            methodInfo.inputNamesNs[ip] = ((WsdlMessagePart)message.parts[ip]).nameNs;
                                            AddNewNamespace(methodInfo.inputNamesNs[ip]);
                                            methodInfo.inputElements[ip] = ((WsdlMessagePart)message.parts[ip]).element;
                                            methodInfo.inputElementsNs[ip] = ((WsdlMessagePart)message.parts[ip]).elementNs;
                                            AddNewNamespace(methodInfo.inputElementsNs[ip]);
                                            methodInfo.inputTypes[ip] = ((WsdlMessagePart)message.parts[ip]).typeName;
                                            methodInfo.inputTypesNs[ip] = ((WsdlMessagePart)message.parts[ip]).typeNameNs;
                                            AddNewNamespace(methodInfo.inputTypesNs[ip]);
                                            if (methodInfo.bProperty && methodInfo.inputTypes[ip] != null && methodInfo.propertyType == null)
                                            {
                                                methodInfo.propertyType = methodInfo.inputTypes[ip];
                                                methodInfo.propertyNs = methodInfo.inputTypesNs[ip];
                                                AddNewNamespace(methodInfo.propertyNs);
                                            }

                                        }
                                    }
                                }
                            }
                            else if (MatchingStrings(content.element, s_outputString))
                            {
                                methodInfo.outputMethodName = content.message;
                                if (methodInfo.outputMethodNameNs == null)
                                    methodInfo.outputMethodNameNs = content.messageNs;
                                WsdlMessage message = (WsdlMessage)wsdlMessages[content.message];
                                if (message == null)
                                {
                                    throw new SUDSParserException(
                                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlMissingMessage"),
                                                                               content.message));
                                }
                                else
                                {
                                    if (message.parts != null)
                                    {
                                        methodInfo.outputNames = new String[message.parts.Count];
                                        methodInfo.outputNamesNs = new String[message.parts.Count];
                                        methodInfo.outputElements = new String[message.parts.Count];
                                        methodInfo.outputElementsNs = new String[message.parts.Count];
                                        methodInfo.outputTypes = new String[message.parts.Count];
                                        methodInfo.outputTypesNs = new String[message.parts.Count];
                                        for (int ip=0; ip<message.parts.Count; ip++)
                                        {
                                            methodInfo.outputNames[ip] = ((WsdlMessagePart)message.parts[ip]).name;
                                            methodInfo.outputNamesNs[ip] = ((WsdlMessagePart)message.parts[ip]).nameNs;
                                            AddNewNamespace(methodInfo.outputNamesNs[ip]);
                                            methodInfo.outputElements[ip] = ((WsdlMessagePart)message.parts[ip]).element;
                                            methodInfo.outputElementsNs[ip] = ((WsdlMessagePart)message.parts[ip]).elementNs;
                                            AddNewNamespace(methodInfo.outputElementsNs[ip]);
                                            methodInfo.outputTypes[ip] = ((WsdlMessagePart)message.parts[ip]).typeName;
                                            methodInfo.outputTypesNs[ip] = ((WsdlMessagePart)message.parts[ip]).typeNameNs;
                                            AddNewNamespace(methodInfo.outputTypesNs[ip]);
                                            if (methodInfo.bProperty && methodInfo.outputTypes[ip] != null && methodInfo.propertyType == null)
                                            {
                                                methodInfo.propertyType = methodInfo.outputTypes[ip];
                                                methodInfo.propertyNs = methodInfo.outputTypesNs[ip];
                                                AddNewNamespace(methodInfo.outputTypesNs[ip]);
                                            }
                                        }
                                    }
                                }
                            }
                            else
                                throw new SUDSParserException(
                                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidPortType"),
                                                                           content.element));
                        }
                        //methodInfo.Dump();
                    } //no porttype
                }
            }
            return methodInfos;
        }

        // Parse Schema
        private void ParseSchema()
        {
            Util.Log("WsdlParser.ParseSchema");                                 
            // Remember the current depth
            int curDepth = _XMLReader.Depth;

            // Parse the target namespace first
            URTNamespace parsingNamespace = ParseNamespace();

            // Parse schema elements
            while (_XMLReader.Depth > curDepth)
            {
                String elementName = _XMLReader.LocalName;
                //if (MatchingSchemaNamespace())
                {
                    if (MatchingStrings(elementName, s_complexTypeString))
                        ParseComplexType(parsingNamespace, null);
                    else if (MatchingStrings(elementName, s_simpleTypeString))
                        ParseSimpleType(parsingNamespace, null);
                    else if (MatchingStrings(elementName, s_schemaString))
                        ParseSchema();
                    else if (MatchingStrings(elementName, s_elementString))
                        ParseElementDecl(parsingNamespace);
                    else if (MatchingStrings(elementName, s_importString))
                        ParseSchemaImportElement();
                    else if (MatchingStrings(elementName, s_includeString))
                        ParseSchemaIncludeElement();
                    else
                        goto SkipXMLNode;

                    continue;
                }

                SkipXMLNode:
                // Ignore others elements such as annotations
                SkipXmlElement();
            }

            return;
        }


        // Resolves internal references
        private void Resolve()
        {
            Util.Log("WsdlParser.Resolve");                                 
            for (int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveElements(this);

            for (int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveTypes(this);

            for (int i=0;i<_URTNamespaces.Count;i++)
                ((URTNamespace)_URTNamespaces[i]).ResolveMethods();
        }

        // Lookup a given attribute position.
        // Note that the supplied strings must have been atomized
        private String LookupAttribute(String attrName, String attrNS, bool throwExp)
        {
            Util.Log("WsdlParser.LookupAttribute Enter "+attrName+", NS "+attrNS+", Exp "+throwExp);                                    
            String value = s_emptyString;
            bool bPresent;
            if (attrNS != null)
                bPresent = _XMLReader.MoveToAttribute(attrName, attrNS);
            else
                bPresent = _XMLReader.MoveToAttribute(attrName);

            if (bPresent)
                value = Atomize(_XMLReader.Value.Trim());
            _XMLReader.MoveToElement();

            if ((bPresent == false) && (throwExp == true))
            {
                throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_AttributeNotFound"),
                                                           attrName, XMLReader.LineNumber, XMLReader.LinePosition, XMLReader.Name));
            }
            Util.Log("WsdlParser.LookupAttribute exit "+attrName+"="+value+", NS "+attrNS+", Exp "+throwExp);                                    
            return(value);
        }

        // Resolves type attribute into its constituent parts
        private void ResolveTypeAttribute(ref String typeName, out String typeNS,
                                          out bool bEmbedded, out bool bPrimitive)
        {
            Util.Log("WsdlParser.ResolveTypeAttribute typeName "+typeName);                                 
            if (MatchingStrings(typeName, s_emptyString))
            {
                typeName = s_objectString;
                typeNS = SchemaNamespaceString;
                bEmbedded = true;
                bPrimitive = false;
            }
            else
            {
                // The type field is a QName
                typeNS = ParseQName(ref typeName);

                // Check for reference and array types
                ResolveTypeNames(ref typeNS, ref typeName, out bEmbedded, out bPrimitive);
            }

            return;
        }

        // Parses a qname

        private String ParseQName(ref String qname)
        {
            return ParseQName(ref qname, null);
        }
        private String ParseQName(ref String qname, URTNamespace defaultNS)
        {
            URTNamespace returnNS = null;
            return ParseQName( ref qname, defaultNS, out returnNS);
        }

        private String ParseQName(ref String qname, URTNamespace defaultNS, out URTNamespace returnNS)
        {
            Util.Log("WsdlParser.ParseQName Enter qname "+qname+" default xmlns NS "+_XMLReader.LookupNamespace(""));

            String ns = null;
            returnNS = null;
            if ((qname == null) || (qname.Length == 0))
            {
                Util.Log("WsdlParser.ParseQName Exit null");
                return null;
            }

            int colonIndex = qname.IndexOf(":");
            if (colonIndex == -1)
            {
                // The default namespace is the s_empty string (this will need to change)
                //textWriter.WriteLine("DefaultNS: " + _XMLReader.LookupNamespace(s_emptyString) + '\n' +
                //                     "ElementNS: " + _XMLReader.Namespace);
                // Should this be element namespace or default namespace
                // For attributes names, element namespace makes more sense
                // For QName values, default namespace makes more sense
                // I am currently returning default namespace

                returnNS = defaultNS;
                if (defaultNS == null)
                {
                    Util.Log("WsdlParser.ParseQName Exit defaultNS qname "+qname);          
                    ns = _XMLReader.LookupNamespace("");
                }
                else
                {
                    Util.Log("WsdlParser.ParseQName Exit defaultNS qname "+qname+" default "+defaultNS.Name);          
                    ns = defaultNS.Name;
                }

            }
            else
            {
                // Get the suffix and atmoize it
                String prefix = qname.Substring(0, colonIndex);
                qname = Atomize(qname.Substring(colonIndex+1));
                ns = _XMLReader.LookupNamespace(prefix);
            }

            ns = Atomize(ns);

            URTNamespace xns = LookupNamespace(ns);

            if (xns == null)
            {
                xns = new URTNamespace(ns, this);
            }


            returnNS = xns;

            Util.Log("WsdlParser.ParseQName Exit qname "+qname+" typeString "+ns+" returnNS "+(returnNS == null?"null":returnNS.Name));          
            return(ns);
        }

        // Returns true if the type needs to be qualified with namespace
        private bool Qualify(String typeNS, String curNS)
        {
            Util.Log("WsdlParser.Qualify typeNS "+typeNS+" curNS "+curNS);                                  
            if (MatchingSchemaStrings(typeNS) ||
                MatchingStrings(typeNS, s_soapNamespaceString) ||
                MatchingStrings(typeNS, s_wsdlSoapNamespaceString) ||               
                MatchingStrings(typeNS, "System") ||
                MatchingStrings(typeNS, curNS))
                return(false);

            return(true);
        }

        // Returns true if the current element node namespace has matching namespace
        private bool MatchingNamespace(String elmNS)
        {
            Util.Log("WsdlParser.MatchingNamespace "+elmNS+" ***** "+_XMLReader.NamespaceURI);                                              
            if (MatchingStrings(_XMLReader.NamespaceURI, elmNS))
                // ||
                //   MatchingStrings(_XMLReader.Prefix, s_emptyString))
                return(true);

            return(false);
        }

        private bool MatchingSchemaNamespace()
        {
            Util.Log("WsdlParser.MatchingSchemaNamespace ");
            if (MatchingNamespace(s_schemaNamespaceString))
                return true;
            else if (MatchingNamespace(s_schemaNamespaceString1999))
            {
                _xsdVersion = XsdVersion.V1999;
                return true;
            }
            else if (MatchingNamespace(s_schemaNamespaceString2000))
            {
                _xsdVersion = XsdVersion.V2000;
                return true;
            }
            else if (MatchingNamespace(s_schemaNamespaceString))
            {
                _xsdVersion = XsdVersion.V2001;
                return true;
            }
            else
            {
                return false;
            }
        }

        private static string TransliterateString(string str)
        {
            if (string.IsNullOrEmpty(str))
            {
                return "\"\"";
            }

            //UnicodeCategory: (Lu)UppercaseLetter, (Ll)LowercaseLetter, (Lt)TitlecaseLetter, (Lm)ModifierLetter, (Lo)OtherLetter, or (Nd)DecimalDigitNumber.

            StringBuilder sb = new StringBuilder("\"");

            foreach (char c in str)
            {
                if (char.IsControl(c))
                {
                    continue;
                }

                if (char.IsLetterOrDigit(c))
                {
                    sb.Append(c);
                }
                else
                {
                    sb.Append("\\u");
                    sb.Append(Convert.ToInt32(c).ToString("X4"));
                }
            }

            sb.Append("\"");
            return sb.ToString();
        }

        static StringBuilder vsb = new StringBuilder();
        internal static string IsValidUrl(string value)
        {
            if (!System.Runtime.Remoting.Configuration.AppSettings.AllowUnsanitizedWSDLUrls)
            {
                return WsdlParser.TransliterateString(value);
            }

            if (value == null)
            {
                return "\"\"";
            }

            vsb.Length= 0;
            vsb.Append("@\"");

            for (int i=0; i<value.Length; i++) 
            {
                if (value[i] == '\"')
                    vsb.Append("\"\"");
                else
                    vsb.Append(value[i]);
            }

            vsb.Append("\"");
            return vsb.ToString();
        }


        static Hashtable cSharpKeywords;
         static bool IsCSharpKeyword(string value) 
         {
            if (cSharpKeywords == null) InitKeywords();
            return cSharpKeywords.ContainsKey(value);
         }

        static void InitKeywords() 
        {
            // build the cSharpKeywords hashtable
            Hashtable cSharpKeywordstemp = new Hashtable(75); // about 75 cSharpKeywords to be added
            Object obj = new Object();
            cSharpKeywordstemp["abstract"] = obj;
            cSharpKeywordstemp["base"] = obj;
            cSharpKeywordstemp["bool"] = obj;
            cSharpKeywordstemp["break"] = obj;
            cSharpKeywordstemp["byte"] = obj;
            cSharpKeywordstemp["case"] = obj;
            cSharpKeywordstemp["catch"] = obj;
            cSharpKeywordstemp["char"] = obj;
            cSharpKeywordstemp["checked"] = obj;
            cSharpKeywordstemp["class"] = obj;
            cSharpKeywordstemp["const"] = obj;
            cSharpKeywordstemp["continue"] = obj;
            cSharpKeywordstemp["decimal"] = obj;
            cSharpKeywordstemp["default"] = obj;
            cSharpKeywordstemp["delegate"] = obj;
            cSharpKeywordstemp["do"] = obj;
            cSharpKeywordstemp["double"] = obj;
            cSharpKeywordstemp["else"] = obj;
            cSharpKeywordstemp["enum"] = obj;
            cSharpKeywordstemp["event"] = obj;
            cSharpKeywordstemp["exdouble"] = obj;
            cSharpKeywordstemp["exfloat"] = obj;
            cSharpKeywordstemp["explicit"] = obj;
            cSharpKeywordstemp["extern"] = obj;
            cSharpKeywordstemp["false"] = obj;
            cSharpKeywordstemp["finally"] = obj;
            cSharpKeywordstemp["fixed"] = obj;
            cSharpKeywordstemp["float"] = obj;
            cSharpKeywordstemp["for"] = obj;
            cSharpKeywordstemp["foreach"] = obj;
            cSharpKeywordstemp["goto"] = obj;
            cSharpKeywordstemp["if"] = obj;
            cSharpKeywordstemp["implicit"] = obj;
            cSharpKeywordstemp["in"] = obj;
            cSharpKeywordstemp["int"] = obj;
            cSharpKeywordstemp["interface"] = obj;
            cSharpKeywordstemp["internal"] = obj;
            cSharpKeywordstemp["is"] = obj;
            cSharpKeywordstemp["lock"] = obj;
            cSharpKeywordstemp["long"] = obj;
            cSharpKeywordstemp["namespace"] = obj;
            cSharpKeywordstemp["new"] = obj;
            cSharpKeywordstemp["null"] = obj;
            cSharpKeywordstemp["object"] = obj;
            cSharpKeywordstemp["operator"] = obj;
            cSharpKeywordstemp["out"] = obj;
            cSharpKeywordstemp["override"] = obj;
            cSharpKeywordstemp["private"] = obj;
            cSharpKeywordstemp["protected"] = obj;
            cSharpKeywordstemp["public"] = obj;
            cSharpKeywordstemp["readonly"] = obj;
            cSharpKeywordstemp["ref"] = obj;
            cSharpKeywordstemp["return"] = obj;
            cSharpKeywordstemp["sbyte"] = obj;
            cSharpKeywordstemp["sealed"] = obj;
            cSharpKeywordstemp["short"] = obj;
            cSharpKeywordstemp["sizeof"] = obj;
            cSharpKeywordstemp["static"] = obj;
            cSharpKeywordstemp["string"] = obj;
            cSharpKeywordstemp["struct"] = obj;
            cSharpKeywordstemp["switch"] = obj;
            cSharpKeywordstemp["this"] = obj;
            cSharpKeywordstemp["throw"] = obj;
            cSharpKeywordstemp["true"] = obj;
            cSharpKeywordstemp["try"] = obj;
            cSharpKeywordstemp["typeof"] = obj;
            cSharpKeywordstemp["uint"] = obj;
            cSharpKeywordstemp["ulong"] = obj;
            cSharpKeywordstemp["unchecked"] = obj;
            cSharpKeywordstemp["unsafe"] = obj;
            cSharpKeywordstemp["ushort"] = obj;
            cSharpKeywordstemp["using"] = obj;
            cSharpKeywordstemp["virtual"] = obj;
            cSharpKeywordstemp["void"] = obj;
            cSharpKeywordstemp["while"] = obj;
            cSharpKeywords = cSharpKeywordstemp;
        }



        static bool IsValidLanguageIndependentIdentifier(string ident)
         {
             for (int i=0; i<ident.Length; i++)
             {
                 char c = ident[i];
                 UnicodeCategory uc = Char.GetUnicodeCategory(c);
                 // each char must be Lu, Ll, Lt, Lm, Lo, Nd, Mn, Mc, Pc
                 // 
                 switch (uc) 
                 {
                 case UnicodeCategory.UppercaseLetter:        // Lu
                 case UnicodeCategory.LowercaseLetter:        // Ll
                 case UnicodeCategory.TitlecaseLetter:        // Lt
                 case UnicodeCategory.ModifierLetter:         // Lm
                 case UnicodeCategory.OtherLetter:            // Lo
                 case UnicodeCategory.DecimalDigitNumber:     // Nd
                 case UnicodeCategory.NonSpacingMark:         // Mn
                 case UnicodeCategory.SpacingCombiningMark:   // Mc
                 case UnicodeCategory.ConnectorPunctuation:   // Pc
                 break;
                 case UnicodeCategory.LetterNumber:
                 case UnicodeCategory.OtherNumber:
                 case UnicodeCategory.EnclosingMark:
                 case UnicodeCategory.SpaceSeparator:
                 case UnicodeCategory.LineSeparator:
                 case UnicodeCategory.ParagraphSeparator:
                 case UnicodeCategory.Control:
                 case UnicodeCategory.Format:
                 case UnicodeCategory.Surrogate:
                 case UnicodeCategory.PrivateUse:
                 case UnicodeCategory.DashPunctuation:
                 case UnicodeCategory.OpenPunctuation:
                 case UnicodeCategory.ClosePunctuation:
                 case UnicodeCategory.InitialQuotePunctuation:
                 case UnicodeCategory.FinalQuotePunctuation:
                 case UnicodeCategory.OtherPunctuation:
                 case UnicodeCategory.MathSymbol:
                 case UnicodeCategory.CurrencySymbol:
                 case UnicodeCategory.ModifierSymbol:
                 case UnicodeCategory.OtherSymbol:
                 case UnicodeCategory.OtherNotAssigned:
                 return false;
                 default:
                        return false;
                 }
             }
             return true;
        }


        internal static void CheckValidIdentifier(string ident)
        {
            if (!IsValidLanguageIndependentIdentifier(ident))
                throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidStringSyntax"),
                                                           ident));
        }

        // Used when identifier appears in a custom attribute. Need to strip out leading @ for keyword
        internal static string IsValidCSAttr(string identifier)
        {
            string returnstr = IsValidCS(identifier);
            if (returnstr.Length > 0 && returnstr[0] == '@')
                return returnstr.Substring(1);
            else return returnstr;
        }

        internal static string IsValidCS(string identifier) {
            if (identifier == null || identifier.Length == 0 || identifier == " ") return identifier;
            string originalIdentifier = identifier;
            int index = identifier.IndexOf('[');
            string arraybrackets = null;
            if (index > -1)
            {
                arraybrackets = identifier.Substring(index);
                identifier = identifier.Substring(0,index);
                // Check arraybrackets
                for (int i=0; i<arraybrackets.Length; i++)
                {
                    switch(arraybrackets[i])
                    {
                    case '[':
                    case ']':
                    case ',':
                    case ' ':
                        break;
                    default:
                        throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidStringSyntax"),
                                                           identifier));
                    }
                }
            }

            string[] names = identifier.Split(new char[] {'.'});
            bool newIdent = false;
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < names.Length; i++) 
            {
                if (i > 0) sb.Append(".");
                if (IsCSharpKeyword(names[i])) 
                {
                    sb.Append("@");
                    newIdent = true;
                }
                CheckValidIdentifier(names[i]);
                sb.Append(names[i]);
            }

            if (newIdent)
            {
                if (arraybrackets != null)
                    sb.Append(arraybrackets);
                return sb.ToString();
            }
            return originalIdentifier;
        }


        // Returns true if the atomized strings match
        private static bool MatchingStrings(String left, String right)
        {
            //Util.Log("WsdlParser.MatchingStrings left "+left+" right "+right);
            return((Object) left == (Object) right);
        }

        private bool MatchingSchemaStrings(String left)
        {
            Util.Log("WsdlParser.MatchingSchemaStrings left "+left+" _xsdVersion "+((Enum)_xsdVersion).ToString());

            if (MatchingStrings(left, s_schemaNamespaceString1999))
            {
                _xsdVersion = XsdVersion.V1999;
                return true;
            }
            else if (MatchingStrings(left, s_schemaNamespaceString2000))
            {
                _xsdVersion = XsdVersion.V2000;
                return true;
            }
            else if (MatchingStrings(left, s_schemaNamespaceString))
            {
                _xsdVersion = XsdVersion.V2001;
                return true;
            }
            else
                return false;
        }

        // Atmozie the given string
        internal String Atomize(String str)
        {
            // Always atmoize using the table defined on the
            // current XML parser
            return(_XMLReader.NameTable.Add(str));
        }

        // Maps URT types to CSharp types
        private String MapSchemaTypesToCSharpTypes(String xsdType)
        {
            Util.Log("SudsConverter.MapSchemaTypesToCSharpTypes Enter  "+xsdType);
            String stype = xsdType; 
            int index = xsdType.IndexOf('[');
            if (index != -1)
                stype = xsdType.Substring(0, index);

            String clrType = SudsConverter.MapXsdToClrTypes(stype);

            if (clrType == null)
                throw new SUDSParserException(
                                             String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                           xsdType, s_schemaNamespaceString));

            // Handle array types
            if (index != -1)
                clrType = clrType + xsdType.Substring(index);
            Util.Log("SudsConverter.MapSchemaTypesToCSharpTypes Exit Type "+clrType);           

            return clrType;
        }

        // Return true if the given type is a primitive type
        private bool IsPrimitiveType(String typeNS, String typeName)
        {
            Util.Log("WsdlParser.IsPrimitiveType typeNS "+typeNS+" typeName "+typeName);            
            bool fReturn = false;
            if (MatchingSchemaStrings(typeNS))
            {
                if (!MatchingStrings(typeName, s_urTypeString))
                    fReturn = true;
            }

            return(fReturn);
        }

        // Looksup a matching namespace
        private URTNamespace LookupNamespace(String name)
        {
            Util.Log("WsdlParser.lookupNamespace name "+name+" number of NS "+_URTNamespaces.Count);            
            for (int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if (WsdlParser.MatchingStrings(urtNS.Name, name))
                //Util.Log("WsdlParser.lookupNamespace search ns "+urtNS.Name+" input "+name);            
                //if (urtNS.Name == name)
                {
                    Util.Log("WsdlParser.lookupNamespace search ns found "+urtNS.GetHashCode()+" "+urtNS.Name+" input "+name);            
                    return(urtNS);
                }
            }

            Util.Log("WsdlParser.lookupNamespace search ns Not found "+name);            
            return(null);
        }
        
        // This routine is used when finding and creating namespaces during the resolve phase.
        // These references are from binding section which support encoded rpc. Any namespace
        // not referenced during this phase will be pruned before creating proxy. This will
        // prevent Types not used from being placed in the proxy.
        
        internal URTNamespace AddNewNamespace(String ns)
        {
            Util.Log("WsdlParser.AddNewNamespace name "+ns); 
            if (ns == null)
            return null;

            URTNamespace xns = LookupNamespace(ns);
            if (xns == null)
            {
            xns = new URTNamespace(ns, this);
            Util.Log("WsdlParser.AddNewNamespace new namespace "+ns);            
            }
            else
            {
            Util.Log("WsdlParser.AddNewNamespace existing namespace "+ns);            
            }
            if (!xns.IsSystem)
                xns.bReferenced= true;
            return xns;
        }

        internal void AddNamespace(URTNamespace xns)
        {
            //Util.Log("WsdlParser.AddNamespace "+xns.GetHashCode()+" "+xns.Namespace);
            _URTNamespaces.Add(xns);
        }       

        // Prints the parsed entities
        private void PrintCSC()
        {
            Util.Log("WsdlParser.PrintCSC ");
            int interopCount = 0;

            // For interop, recalculate the interop names
            for (int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if (!urtNS.IsEmpty && urtNS.UrtType == UrtType.Interop)
                {
                    if (interopCount == 0)
                        urtNS.EncodedNS = _proxyNamespace;
                    else
                        urtNS.EncodedNS = _proxyNamespace+interopCount;
                    interopCount++;
                    Util.Log("WsdlParser.PrintCSC Interop "+urtNS.EncodedNS+" "+urtNS.Namespace+" urtType "+((Enum)urtNS.UrtType).ToString());                 
                }
            }

            for (int i=0;i<_URTNamespaces.Count;i++)
            {
                URTNamespace urtNS = (URTNamespace) _URTNamespaces[i];
                if (!urtNS.IsEmpty && !(urtNS.UrtType == UrtType.UrtSystem || urtNS.UrtType == UrtType.Xsd || urtNS.UrtType == UrtType.None))
                {
                    Util.Log("WsdlParser.PrintCSC fileName urtNS "+urtNS.Namespace+" urtType "+((Enum)urtNS.UrtType).ToString());                 
                    String fileName = urtNS.IsURTNamespace ? urtNS.AssemName : urtNS.EncodedNS;
                    int index = fileName.IndexOf(','); // reduce to simple assembly name
                    if (index > -1)
                    {
                        fileName = fileName.Substring(0,index);
                    }
                    Util.Log("WsdlParser.PrintCSC fileName "+fileName+" "+urtNS.Namespace);                 
                    String completeFileName = "";

                    WriterStream output = WriterStream.GetWriterStream(ref _writerStreams, _outputDir, fileName, ref completeFileName);
                    if (completeFileName.Length > 0)
                        _outCodeStreamList.Add(completeFileName);
                    urtNS.PrintCSC(output);
                }
            }
            return;
        }

        internal UrtType IsURTExportedType(String name, out String ns, out String assemName)
        {
            Util.Log("WsdlParser.IsURTExportedType Enter "+name);           
            //Console.WriteLine("Parsing " + name);
            UrtType urtType = UrtType.None;
            ns = null;
            assemName = null;

            if (MatchingSchemaStrings(name))
            {
                Util.Log("WsdlParser.IsURTExportedType trace 1 "+name);           
                urtType = UrtType.Xsd;
            }
            else
            {
                Util.Log("WsdlParser.IsURTExportedType trace 2 "+name);           
                if (SoapServices.IsClrTypeNamespace(name))
                {
                    SoapServices.DecodeXmlNamespaceForClrTypeNamespace(name, out ns, out assemName);

                    if (assemName == null)
                    {
                        assemName = typeof(String).Assembly.GetName().Name;
                        urtType = UrtType.UrtSystem;
                    }
                    else
                        urtType = UrtType.UrtUser;
                }

                if (urtType == UrtType.None)
                {
                    ns = name;
                    assemName = ns;
                    urtType = UrtType.Interop;
                }

                ns = Atomize(ns);
                assemName = Atomize(assemName);
            }

            //Console.WriteLine("NS: " + ns + " Assembly: " + assemName);
            Util.Log("WsdlParser.IsURTExportedType Exit "+((Enum)urtType).ToString());
            return(urtType);
        }

        internal String GetTypeString(String curNS, bool bNS, URTNamespace urtNS, String typeName, String typeNS) 
        {
            Util.Log("WsdlParser.GetTypeString Entry curNS "+curNS+" bNS "+bNS+" URTNamespace "+urtNS.Name+" typeName "+typeName+" typeNS "+typeNS);
            String type;

            URTComplexType ct = urtNS.LookupComplexType(typeName);
            if (ct != null && ct.IsArray())
            {
                if (ct.GetArray() == null)
                    ct.ResolveArray();
                String arrayName = ct.GetArray();
                URTNamespace arrayNS = ct.GetArrayNS();
                StringBuilder sb = new StringBuilder(50);
                if (arrayNS.EncodedNS != null && Qualify(urtNS.EncodedNS, arrayNS.EncodedNS))
                {
                    sb.Append(WsdlParser.IsValidCSAttr(arrayNS.EncodedNS));
                    sb.Append('.');
                }
                sb.Append(WsdlParser.IsValidCSAttr(arrayName));
                type = sb.ToString();
            }
            else
            {
                String encodedNS = null;
                if (urtNS.UrtType == UrtType.Interop)
                    encodedNS = urtNS.EncodedNS;
                else
                    encodedNS = typeNS;

                if (bNS && Qualify(encodedNS, curNS))
                {
                    StringBuilder sb = new StringBuilder(50);
                    if (encodedNS != null)
                    {
                        sb.Append(WsdlParser.IsValidCSAttr(encodedNS));
                        sb.Append('.');
                    }
                    sb.Append(WsdlParser.IsValidCSAttr(typeName));
                    type = sb.ToString();
                }
                else
                {
                    type = typeName;
                }

            }

            int index = type.IndexOf('+');
            if (index > 0)
            {
                // nested type, replace + with . Should be done earlier when forming names
                if (bNS)
                    type = type.Replace('+', '.');
                else
                    type = type.Substring(0,index);
            }


            Util.Log("WsdlParser.GetTypeString Exit type "+type);              
            return(type);
        }


        // Creates and initializes the name table if neccessary
        static private XmlNameTable CreatePrimedNametable()
        {
            Util.Log("WsdlParser.CreatePrimedNametable");           

            //Interlocked.Increment(ref s_counter);
            /*if(s_atomizedTable == null)
            {
            // Create a new nametable
            //MTNameTable newTable = new MTNameTable(true);
            }*/
            NameTable newTable = new NameTable();

            // Atomically update static location to point to the current table
            /*Object oldTable = Interlocked.CompareExchange(ref s_atomizedTable, newTable, null);
            if(oldTable != null)
            newTable = (MTNameTable) oldTable; */

            // Atomize frequently used strings for perf
            // The following ops are not done inside a lock as they are idempotent
            s_emptyString = newTable.Add(String.Empty);
            s_complexTypeString = newTable.Add("complexType");
            s_simpleTypeString = newTable.Add("simpleType");
            s_elementString = newTable.Add("element");
            s_enumerationString = newTable.Add("enumeration");
            s_encodingString = newTable.Add("encoding");
            s_attributeString = newTable.Add("attribute");
            s_attributesString = newTable.Add("attributes");
            s_allString = newTable.Add("all");
            s_sequenceString = newTable.Add("sequence");
            s_choiceString = newTable.Add("choice");
            s_minOccursString = newTable.Add("minOccurs");
            s_maxOccursString = newTable.Add("maxOccurs");
            s_unboundedString = newTable.Add("unbounded");
            s_oneString = newTable.Add("1");
            s_zeroString = newTable.Add("0");
            s_nameString = newTable.Add("name");
            s_typeString = newTable.Add("type");
            s_baseString = newTable.Add("base");
            s_valueString = newTable.Add("value");
            s_interfaceString = newTable.Add("interface");
            s_serviceString = newTable.Add("service");
            s_extendsString = newTable.Add("extends");
            s_addressesString = newTable.Add("addresses");
            s_addressString = newTable.Add("address");
            s_uriString = newTable.Add("uri");
            s_implementsString = newTable.Add("implements");
            s_nestedTypeString = newTable.Add("nestedType");
            s_requestString = newTable.Add("request");
            s_responseString = newTable.Add("response");
            s_requestResponseString = newTable.Add("requestResponse");
            s_messageString = newTable.Add("message");
            s_locationString = newTable.Add("location");
            s_schemaLocationString = newTable.Add("schemaLocation");
            s_importString = newTable.Add("import");
            s_onewayString = newTable.Add("oneway");
            s_includeString = newTable.Add("include");
            s_refString = newTable.Add("ref");
            s_refTypeString = newTable.Add("refType");
            s_referenceString = newTable.Add("Reference");
            s_objectString = newTable.Add("Object");
            s_urTypeString = newTable.Add("anyType");
            s_arrayString = newTable.Add("Array");
            s_sudsString = newTable.Add("suds");
            s_methodString = newTable.Add("method");
            s_useString = newTable.Add("use");
            s_rootTypeString = newTable.Add("rootType");
            s_soapString = newTable.Add("soap");
            s_serviceDescString = newTable.Add("serviceDescription");
            s_schemaString = newTable.Add("schema");
            s_targetNamespaceString = newTable.Add("targetNamespace");
            s_namespaceString = newTable.Add("namespace");
            s_idString = newTable.Add("ID");
            s_soapActionString = newTable.Add("soapAction");
            s_schemaNamespaceString1999 = newTable.Add(SudsConverter.Xsd1999);
            s_instanceNamespaceString1999 = newTable.Add(SudsConverter.Xsi1999);
            s_schemaNamespaceString2000 = newTable.Add(SudsConverter.Xsd2000);
            s_instanceNamespaceString2000 = newTable.Add(SudsConverter.Xsi2000);
            s_schemaNamespaceString = newTable.Add(SudsConverter.Xsd2001);
            s_instanceNamespaceString = newTable.Add(SudsConverter.Xsi2001);
            s_soapNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:soap.v1");
            //s_sudsNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:suds.v1");
            s_sudsNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:soap-sdl-2000-01-25");
            //s_URTNamespaceString = newTable.Add("urn:schamas-xmlsoap-org:urt.v1");
            //s_serviceNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:servicedesc.v1");
            s_serviceNamespaceString = newTable.Add("urn:schemas-xmlsoap-org:sdl.2000-01-25");

            // Wsdl strings
            s_definitionsString = newTable.Add("definitions");
            s_wsdlNamespaceString = newTable.Add("http://schemas.xmlsoap.org/wsdl/");
            s_wsdlSoapNamespaceString = newTable.Add("http://schemas.xmlsoap.org/wsdl/soap/");
            s_wsdlSudsNamespaceString = newTable.Add("http://www.w3.org/2000/wsdl/suds");
            s_enumTypeString = newTable.Add("enumType");
            s_typesString = newTable.Add("types");
            s_partString = newTable.Add("part");
            s_portTypeString = newTable.Add("portType");
            s_operationString = newTable.Add("operation");
            s_inputString = newTable.Add("input");
            s_outputString = newTable.Add("output");
            s_bindingString = newTable.Add("binding");
            s_classString = newTable.Add("class");
            s_structString = newTable.Add("struct");
            s_ISerializableString = newTable.Add("ISerializable");
            s_marshalByRefString = newTable.Add("MarshalByRefObject");
            s_delegateString = newTable.Add("Delegate");
            s_servicedComponentString = newTable.Add("ServicedComponent");
            s_comObjectString = newTable.Add("__ComObject");
            s_portString = newTable.Add("port");
            s_styleString = newTable.Add("style");
            s_transportString = newTable.Add("transport");
            s_encodedString = newTable.Add("encoded");
            s_faultString = newTable.Add("fault");
            s_bodyString = newTable.Add("body");
            s_partsString = newTable.Add("parts");
            s_headerString = newTable.Add("header");
            s_encodingStyleString = newTable.Add("encodingStyle");                                  
            s_restrictionString = newTable.Add("restriction");                                  
            s_complexContentString = newTable.Add("complexContent");
            s_soapEncodingString = newTable.Add("http://schemas.xmlsoap.org/soap/encoding/");
            s_arrayTypeString = newTable.Add("arrayType");
            s_parameterOrderString = newTable.Add("parameterOrder");


            return((XmlNameTable) newTable);

            // Enqueue a timer if it has not already been done
            /*Timer timer = new Timer(new TimerCallback(Cleanup), null, 1000, 1000);
            Object existingTimer = Interlocked.CompareExchange(ref s_enqueuedTimer, timer, null);
            if(existingTimer != null)
            timer.Dispose(); */
            //}

            //return((XmlNameTable) s_atomizedTable);
        }

        // Private fields
        private XmlTextReader _XMLReader;
        private ArrayList _URTNamespaces;       
        private ReaderStream _parsingInput;
        internal bool _bWrappedProxy;
        private String _proxyNamespace;
        private int _proxyNamespaceCount = 0;
        private ReaderStream _readerStreamsWsdl;
        private ReaderStream _readerStreamsXsd;
        private String _outputDir;
        private ArrayList _outCodeStreamList;
        private WriterStream _writerStreams;
        private SchemaBlockType _blockDefault;
        private XsdVersion _xsdVersion;
        private Hashtable wsdlMessages = new Hashtable(10);
        private Hashtable wsdlPortTypes = new Hashtable(10);
        private ArrayList wsdlBindings = new ArrayList(10);
        private ArrayList wsdlServices = new ArrayList(10);
        private Stack _currentReaderStack = new Stack(5);
        private Stack _currentSchemaReaderStack = new Stack(5);
        private XmlNameTable _primedNametable = null;


        //static private Object s_atomizedTable = null;
        //static private int s_counter = 0;
        //static private Object s_enqueuedTimer = null;
        static private String s_emptyString;
        static private String s_complexTypeString;
        static private String s_simpleTypeString;
        static private String s_elementString;
        static private String s_enumerationString;
        static private String s_encodingString;
        static private String s_attributeString;
        static private String s_attributesString;
        static private String s_allString;
        static private String s_sequenceString;
        static private String s_choiceString;
        static private String s_minOccursString;
        static private String s_maxOccursString;
        static private String s_unboundedString;
        static private String s_oneString;
        static private String s_zeroString;
        static private String s_nameString;
        static private String s_enumTypeString;
        static private String s_typeString;
        static private String s_baseString;
        static private String s_valueString;
        static private String s_interfaceString;
        static private String s_serviceString;
        static private String s_extendsString;
        static private String s_addressesString;
        static private String s_addressString;
        static private String s_uriString;
        static private String s_implementsString;
        static private String s_nestedTypeString;
        static private String s_requestString;
        static private String s_responseString;
        static private String s_requestResponseString;
        static private String s_messageString;
        static private String s_locationString;
        static private String s_schemaLocationString;
        static private String s_importString;
        static private String s_includeString;      
        static private String s_onewayString;
        static private String s_refString;
        static private String s_refTypeString;
        static private String s_referenceString;
        static private String s_arrayString;
        static private String s_objectString;
        static private String s_urTypeString;
        static private String s_methodString;
        static private String s_sudsString;
        static private String s_useString;
        static private String s_rootTypeString;
        static private String s_soapString;
        static private String s_serviceDescString;
        static private String s_schemaString;
        static private String s_targetNamespaceString;
        static private String s_namespaceString;
        static private String s_idString;
        static private String s_soapActionString;
        static private String s_instanceNamespaceString;
        static private String s_schemaNamespaceString;
        static private String s_instanceNamespaceString1999;
        static private String s_schemaNamespaceString1999;
        static private String s_instanceNamespaceString2000;
        static private String s_schemaNamespaceString2000;
        static private String s_soapNamespaceString;
        static private String s_sudsNamespaceString;
        static private String s_serviceNamespaceString;

        //Wsdl
        static private String s_definitionsString;
        static private String s_wsdlNamespaceString;
        static private String s_wsdlSoapNamespaceString;
        static private String s_wsdlSudsNamespaceString;
        static private String s_typesString;
        static private String s_partString;
        static private String s_portTypeString;
        static private String s_operationString;
        static private String s_inputString;
        static private String s_outputString;
        static private String s_bindingString;
        static private String s_classString;
        static private String s_structString;
        static private String s_ISerializableString;
        static private String s_marshalByRefString;
        static private String s_delegateString;
        static private String s_servicedComponentString;
        static private String s_comObjectString;
        static private String s_portString;
        static private String s_styleString;
        static private String s_transportString;
        static private String s_encodedString;
        static private String s_faultString;
        static private String s_bodyString;
        static private String s_partsString;
        static private String s_headerString;
        static private String s_encodingStyleString;        
        static private String s_restrictionString;        
        static private String s_complexContentString;        
        static private String s_soapEncodingString;        
        static private String s_arrayTypeString;        
        static private String s_parameterOrderString;



        /***************************************************************
         **
         ** Private classes used by SUDS Parser
         **
         ***************************************************************/
        // Input streams
        internal class ReaderStream
        {
            internal ReaderStream(String location)
            {
                Util.Log("ReaderStream.ReaderStream "+location);
                _location = location;
                _name = String.Empty;
                _targetNS = String.Empty;
                _uniqueNS = null;
                _reader = null;
                _next = null;
            }
            internal String Location
            {
                get { return(_location);}
                set { _location = value;}                
            }
            internal String Name
            {
                set { _name = value;}
            }
            internal String TargetNS
            {
                get { return(_targetNS);}
                set { _targetNS = value;}
            }
            internal URTNamespace UniqueNS
            {
                get { return(_uniqueNS);}
                set { _uniqueNS = value;}
            }
            internal TextReader InputStream
            {
                get { return(_reader);}
                set { _reader = value;}
            }
            internal Uri Uri
            {
                get { return(_uri);}
                set { _uri = value;}
            }

            internal static void GetReaderStream(ReaderStream inputStreams, ReaderStream newStream)
            {
                Util.Log("ReaderStream.GetReaderStream "+newStream.Location);             
                ReaderStream input = inputStreams;
                ReaderStream last;
                do
                {
                    Util.Log("ReaderStream.GetReaderStream location match input.location "+input._location+" location "+newStream.Location);             
                    if (input._location == newStream.Location)
                        return;
                    last = input;
                    input = input._next;
                } while (input != null);

                input = newStream;
                last._next = input;

                return;
            }
            internal static ReaderStream GetNextReaderStream(ReaderStream input)
            {
                Util.Log("ReaderStream.GetNextReaderStream "+input._next);
                return input._next;
            }

            private String _location;
            private String _name;
            private String _targetNS;
            private URTNamespace _uniqueNS;
            private TextReader _reader;
            private ReaderStream _next;
            private Uri _uri;            
        }

        internal class WriterStream
        {

            private WriterStream(String fileName, TextWriter writer)
            {
                Util.Log("WriterStream.WriterStream "+fileName);                
                _fileName = fileName;
                _writer = writer;
            }
            internal TextWriter OutputStream
            {
                get { return(_writer);}
            }
                        
            internal bool GetWrittenTo()
            {
                return _bWrittenTo;

            }

            internal void SetWrittenTo()
            {
                _bWrittenTo = true;
            }

            internal static void Flush(WriterStream writerStream)
            {
                while (writerStream != null)
                {
                    writerStream._writer.Flush();
                    writerStream = writerStream._next;
                }

                return;
            }
            
            internal static WriterStream GetWriterStream(ref WriterStream outputStreams, String outputDir, String fileName, ref String completeFileName)
            {
                Util.Log("WriterStream.GetWriterStream "+fileName);                             
                WriterStream output = outputStreams;
                while (output != null)
                {
                    if (output._fileName == fileName)
                        return(output);
                    output = output._next;
                }

                String diskFileName = fileName;
                if (diskFileName.EndsWith(".exe", StringComparison.Ordinal) || diskFileName.EndsWith(".dll", StringComparison.Ordinal))
                    diskFileName = diskFileName.Substring(0, diskFileName.Length - 4);
                String _completeFileName = outputDir + diskFileName + ".cs";
                completeFileName = _completeFileName;
                //TextWriter textWriter = new StreamWriter(outputDir + fileName + ".cs", false);
                TextWriter textWriter = new StreamWriter(_completeFileName, false, new UTF8Encoding(false));
                output = new WriterStream(fileName, textWriter);
                output._next = outputStreams;
                outputStreams = output;
                Util.Log("WriterStream.GetWriterStream in fileName "+fileName+" completeFileName "+_completeFileName);                              
                return(output);
            }

            internal static void Close(WriterStream outputStreams)
            {
                WriterStream output = outputStreams;
                while (output != null)
                {
                    output._writer.Close();
                    output = output._next;
                }
            }

            private String _fileName;
            private TextWriter _writer;
            private WriterStream _next;
            private bool _bWrittenTo = false;            
        }

        // Represents a parameter of a method
        [Serializable]
        internal enum URTParamType
        {
            IN, OUT, REF
        }
        internal class URTParam
        {
            internal URTParam(String name, String typeName, String typeNS, String encodedNS,
                              URTParamType pType, bool bEmbedded, WsdlParser parser, URTNamespace urtNamespace)
            {
                Util.Log("URTParam.URTParam name "+name+" typeName "+typeName+" typeNS "+typeNS+" ecodedNS "+encodedNS+" pType "+pType+" bEmbedded "+bEmbedded);
                _name = name;
                _typeName = typeName;
                _typeNS = typeNS;
                _encodedNS = encodedNS;
                _pType = pType;
                _embeddedParam = bEmbedded;
                _parser = parser;
                _urtNamespace = urtNamespace;

            }
            public override bool Equals(Object obj)
            {
                //Util.Log("URTParam.Equals ");
                URTParam suppliedParam = (URTParam) obj;
                if (_pType == suppliedParam._pType &&
                    WsdlParser.MatchingStrings(_typeName, suppliedParam._typeName) &&
                    WsdlParser.MatchingStrings(_typeNS, suppliedParam._typeNS))
                    return(true);

                return(false);
            }


            public override int GetHashCode()
            {
                return base.GetHashCode();
            }
            internal URTParamType ParamType
            {
                get { return(_pType);}
                set { _pType = value;}
            }
            internal String Name
            {
                get { return(_name);}
            }
            internal String TypeName
            {
                get { return(_typeName);}
            }
            internal String TypeNS
            {
                get { return(_typeNS);}
            }

            internal String GetTypeString(String curNS, bool bNS)
            {
                return _parser.GetTypeString(curNS, bNS, _urtNamespace, _typeName, _encodedNS);
            }

            internal void PrintCSC( StringBuilder sb, String curNS)
            {
                Util.Log("URTParam.PrintCSC curNS "+curNS+" name "+_name);                                              
                //if(_embeddedParam)
                //    sb.Append("[Embedded] ");
                sb.Append(PTypeString[(int) _pType]);
                sb.Append(GetTypeString(curNS, true));
                sb.Append(' ');
                sb.Append(WsdlParser.IsValidCS(_name));
                return;
            }
            internal void PrintCSC(StringBuilder sb)
            {
                Util.Log("URTParam.PrintCSC name "+_name);                                              
                sb.Append(PTypeString[(int) _pType]);
                sb.Append(WsdlParser.IsValidCS(_name));
                return;
            }

            static private String[] PTypeString = { "", "out ", "ref "};
            private String _name;
            private String _typeName;
            private String _typeNS;
            private String _encodedNS;
            private URTParamType _pType;
            private bool _embeddedParam;
            private URTNamespace _urtNamespace;
            private WsdlParser _parser;
        }

        [Flags]
        internal enum MethodPrintEnum
        {
            PrintBody = 0x1,
            InterfaceMethods = 0x2,
            InterfaceInClass = 0x4,
        }

        [Flags]
        internal enum MethodFlags
        {
            None = 0x0,
            Public = 0x1,
            Protected = 0x2,
            Override = 0x4,
            New = 0x8,
            Virtual = 0x10,
            Internal = 0x20,
        }


        // Represents a method
        internal abstract class URTMethod
        {

            internal static bool FlagTest(MethodPrintEnum flag, MethodPrintEnum target)
            {
                if ((flag & target) == target)
                    return true;
                else
                    return false;
            }

            internal static bool MethodFlagsTest(MethodFlags flag, MethodFlags target)
            {
                if ((flag & target) == target)
                    return true;
                else
                    return false;
            }

            internal URTMethod(String name, String soapAction, String methodAttributes, URTComplexType complexType)
            {
                Util.Log("URTMethod.URTMethod name "+name+" soapAction "+soapAction+" attributes "+methodAttributes+" complexType "+complexType);
                _methodName = name;
                _soapAction = soapAction;
                _methodType = null;
                _complexType = complexType;
                int index = name.IndexOf('.');
                _methodFlags = MethodFlags.None;

                // set method flags
                if (methodAttributes != null && methodAttributes.Length > 0)
                {
                    String[] attributes = methodAttributes.Split(' ');
                    foreach (String attribute in attributes)
                    {
                        if (attribute == "virtual")
                            _methodFlags |= MethodFlags.Virtual;
                        if (attribute == "new")
                            _methodFlags |= MethodFlags.New;
                        if (attribute == "override")
                            _methodFlags |= MethodFlags.Override;
                        if (attribute == "public")
                            _methodFlags |= MethodFlags.Public;
                        if (attribute == "protected")
                            _methodFlags |= MethodFlags.Protected;
                        if (attribute == "internal")
                            _methodFlags |= MethodFlags.Internal;
                    }
                }

                Util.Log("URTMethod.URTMethod methodFlags "+((Enum)_methodFlags).ToString());
            }


            internal String Name
            {
                get { return(_methodName);}
            }

            internal String SoapAction
            {
                get { return(_soapAction);}
            }
            internal MethodFlags MethodFlags
            {
                get { return(_methodFlags);}
                set { _methodFlags = value;}
            }

            internal String GetTypeString(String curNS, bool bNS)
            {
                Util.Log("URTMethod.GetTypeString curNS "+curNS+" bNS "+bNS);
                return((_methodType != null) ? _methodType.GetTypeString(curNS, bNS) : "void");
            }
            protected URTParam MethodType
            {
                get { return(_methodType);}
            }
            public override int GetHashCode()
            {
                return base.GetHashCode();
            }           
            public override bool Equals(Object obj)
            {
                URTMethod suppliedMethod = (URTMethod) obj;
                if (WsdlParser.MatchingStrings(_methodName, suppliedMethod._methodName) &&
                    _params.Count == suppliedMethod._params.Count)
                {
                    for (int i=0;i<_params.Count;i++)
                    {
                        if (_params[i].Equals(suppliedMethod._params[i]) == false)
                            return(false);
                    }

                    return(true);
                }

                return(false);
            }
            internal MethodFlags GetMethodFlags(MethodInfo method)
            {
                return(_methodFlags);
            }

            internal void AddParam(URTParam newParam)
            {
                Util.Log("URTMethod.AddParam "+newParam.Name);
                for (int i=0;i<_params.Count;i++)
                {
                    URTParam extParam = (URTParam) _params[i];
                    if (WsdlParser.MatchingStrings(extParam.Name, newParam.Name))
                    {
                        if (extParam.ParamType == URTParamType.IN &&
                            newParam.ParamType == URTParamType.OUT &&
                            WsdlParser.MatchingStrings(extParam.TypeName, newParam.TypeName) &&
                            WsdlParser.MatchingStrings(extParam.TypeNS, newParam.TypeNS))
                        {
                            extParam.ParamType = URTParamType.REF;
                            return;
                        }

                        throw new SUDSParserException(CoreChannel.GetResourceString("Remoting_Suds_DuplicateParameter"));
                    }
                }

                // Find parameter position
                int paramPosition = -1;
                if (_paramNamesOrder != null)
                {
                    // Parameter order find the parameter position or determine if the parameter is a return value
                    for (int i=0; i<_paramNamesOrder.Length; i++)
                    {
                        if (_paramNamesOrder[i] == newParam.Name)
                        {
                            paramPosition = i;
                            break;
                        }
                    }

                    if (paramPosition == -1)
                    {
                        // return value
                        _methodType = newParam;
                    }
                    else
                    {
                        // Parameter, add the correct position
                        //_params[paramPosition] = newParam; // _params array list is the size of the parameters.
                        _params.Add(newParam);
                        _paramPosition.Add(paramPosition);
                    }
                }
                else
                {
                    // no parameterOrder in wsdl, do the best we can to determine if there is a return value
                    if ((_methodType == null) && (newParam.ParamType == URTParamType.OUT))
                    {
                        // return value
                        _methodType = newParam;
                    }
                    else
                    {
                        // parameter put as the next parameter
                        _params.Add(newParam);
                    }
                }
            }

            internal void ResolveMethodAttributes()
            {
                Util.Log("URTMethod.ResolveMethodAttributes "+Name+" MethodFlags "+((Enum)_methodFlags).ToString());
                if (!(MethodFlagsTest(_methodFlags,MethodFlags.Override) || MethodFlagsTest(_methodFlags,MethodFlags.New)))
                    FindMethodAttributes();
            }

            private void FindMethodAttributes()
            {
                Util.Log("URTMethod.FindMethodAttributes "+Name+" complexType "+_complexType);
                if (_complexType == null)
                    return;

                Util.Log("URTMethod.FindMethodAttributes "+_complexType.Name);

                ArrayList inherit = _complexType.Inherit;
                Type baseType = null;
                if (inherit == null)
                {
                    inherit = new ArrayList();
                    if (_complexType.SUDSType == SUDSType.ClientProxy)
                        baseType = typeof(System.Runtime.Remoting.Services.RemotingClientProxy);
                    else if (_complexType.SudsUse == SudsUse.MarshalByRef)
                        baseType = typeof(MarshalByRefObject);
                    else if (_complexType.SudsUse == SudsUse.ServicedComponent)
                        baseType = typeof(MarshalByRefObject);

                    if (baseType == null)
                        return;

                    while (baseType != null)
                    {
                        Util.Log("URTMethod.FindMethodAttributes baseType Enter "+baseType);
                        inherit.Add(baseType);
                        baseType = baseType.BaseType;
                    }

                    _complexType.Inherit = inherit;
                }

                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;

                bool bVirtual = MethodFlagsTest(_methodFlags, MethodFlags.Virtual);
                Util.Log("URTMethod.FindMethodAttributes "+Name+" bVirtual "+bVirtual);

                // See if method hides inherited methods
                bool bfound = false;
                for (int j=0; j<inherit.Count; j++)
                {
                    baseType = (Type)inherit[j];
                    MethodInfo[] baseInfos = null;
                    try
                    {
                        MethodInfo methodInfo = baseType.GetMethod(Name, bFlags);
                        if (methodInfo != null)
                        {
                            baseInfos = new MethodInfo[1];
                            baseInfos[0] = methodInfo;
                        }
                    }
                    catch {
                        // Ambiguous match, overloaded methos
                        baseInfos = baseType.GetMethods(bFlags);
                    }

                    if (baseInfos != null)
                    {
                        foreach (MethodBase baseInfo in baseInfos)
                        {
                            if (baseInfo != null && 
                                baseInfo.Name == Name && 
                                (baseInfo.IsFamily || baseInfo.IsPublic || baseInfo.IsAssembly) 
                                && IsSignature(baseInfo))
                            {
                                bfound = true;

                                // Change attribute to match new hiearchy
                                if (!baseInfo.IsPublic)
                                {
                                    if (baseInfo.IsAssembly)
                                    {
                                        _methodFlags &= ~(MethodFlags.Public);
                                        _methodFlags |= MethodFlags.Internal;
                                    }
                                    else if (baseInfo.IsFamily)
                                    {
                                        _methodFlags &= ~(MethodFlags.Public);
                                        _methodFlags |= MethodFlags.Protected;
                                    }
                                }

                                // Hides
                                if (baseInfo.IsFinal)
                                    _methodFlags |= MethodFlags.New;
                                else if (baseInfo.IsVirtual && bVirtual)
                                    _methodFlags |= MethodFlags.Override;
                                else
                                    _methodFlags |= MethodFlags.New;
                                break;
                            }
                        }
                    }
                    if (bfound)
                        break;
                }

                Util.Log("URTMethod.FindMethodAttributes Exit "+Name+" "+((Enum)_methodFlags).ToString());
            }

            private bool IsSignature(MethodBase baseInfo)
            {
                ParameterInfo[] paramInfos = baseInfo.GetParameters();
                Util.Log("URTMethod.IsSignature  param length "+paramInfos.Length+" URTParams length "+_params.Count);

                if (_params.Count != paramInfos.Length)
                    return false;

                bool bsig = true;

                for (int i=0; i<paramInfos.Length; i++)
                {
                    URTParam param = (URTParam)_params[i];
                    if (param.GetTypeString(null, true) != paramInfos[i].ParameterType.FullName)
                    {
                        bsig = false;
                        break;

                    }
                }

                return bsig;
            }

            internal void PrintSignature(StringBuilder sb, String curNS)
            {
                Util.Log("URTMethod.PrintSignature curNS "+curNS);              
                for (int i=0;i<_params.Count;i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    Util.Log("URTMethod.PrintSignature Invoke _params PrintCSC");                               
                    ((URTParam) _params[i]).PrintCSC(sb, curNS);
                }

                return;
            }
            /*
            internal abstract void PrintCSC(TextWriter textWriter, String indentation,
                                            String namePrefix, String curNS, MethodPrintEnum methodPrintEnum,
                                            bool bURTType, String bodyPrefix, StringBuilder sb);
                                            */

            internal virtual void PrintCSC(TextWriter textWriter, String indentation,
                                           String namePrefix, String curNS, MethodPrintEnum methodPrintEnum, bool bURTType,
                                           String bodyPrefix, StringBuilder sb)
            {
                Util.Log("URTMethod.PrintCSC name "+_methodName+" namePrefix "+namePrefix+" curNS "+curNS+" MethodPrintEnum "+((Enum)methodPrintEnum).ToString());

                // Check for class methods
                sb.Length = 0;
                sb.Append(indentation);

                if (Name == "Finalize")
                    return;

                if (FlagTest(methodPrintEnum, MethodPrintEnum.InterfaceInClass))
                    sb.Append("public ");
                else if (MethodFlagsTest(_methodFlags, MethodFlags.Public))
                    sb.Append("public ");
                else if (MethodFlagsTest(_methodFlags, MethodFlags.Protected))
                    sb.Append("protected ");
                else if (MethodFlagsTest(_methodFlags, MethodFlags.Internal))
                    sb.Append("internal ");

                if (MethodFlagsTest(_methodFlags, MethodFlags.Override))
                    sb.Append("override ");

                else if (MethodFlagsTest(_methodFlags, MethodFlags.Virtual))
                    sb.Append("virtual ");

                if (MethodFlagsTest(_methodFlags, MethodFlags.New))
                    sb.Append("new ");

                sb.Append(WsdlParser.IsValidCSAttr(GetTypeString(curNS, true)));

                if (FlagTest(methodPrintEnum, MethodPrintEnum.InterfaceInClass))
                    sb.Append(" ");
                else
                    sb.Append(WsdlParser.IsValidCSAttr(namePrefix));
                if (_wsdlMethodInfo.bProperty)
                    sb.Append(WsdlParser.IsValidCS(_wsdlMethodInfo.propertyName));
                else
                {
                    sb.Append(WsdlParser.IsValidCS(_methodName));

                    sb.Append('(');
                    if (_params.Count > 0)
                    {
                        Util.Log("URTMethod.PrintCSC Invoke _params[0] 1 PrintCSC");
                        ((URTParam) _params[0]).PrintCSC(sb, curNS);
                        for (int i=1;i<_params.Count;i++)
                        {
                            sb.Append(", ");
                            Util.Log("URTMethod.PrintCSC Invoke _params 2 PrintCSC "+i);                       
                            ((URTParam) _params[i]).PrintCSC(sb, curNS);
                        }
                    }
                    sb.Append(')');
                }

                if (_wsdlMethodInfo.bProperty && FlagTest(methodPrintEnum, MethodPrintEnum.InterfaceMethods))
                {
                    sb.Append("{");
                    if (_wsdlMethodInfo.bGet)
                        sb.Append(" get; ");
                    if (_wsdlMethodInfo.bSet)
                        sb.Append(" set; ");
                    sb.Append("}");
                }
                else if (!FlagTest(methodPrintEnum, MethodPrintEnum.PrintBody))
                    sb.Append(';');

                textWriter.WriteLine(sb); 

                if (_wsdlMethodInfo.bProperty && FlagTest(methodPrintEnum, MethodPrintEnum.PrintBody))
                    PrintPropertyBody(textWriter,indentation, sb, bodyPrefix);

                else if (FlagTest(methodPrintEnum, MethodPrintEnum.PrintBody))
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append('{');
                    textWriter.WriteLine(sb);

                    String newIndentation = indentation + "    ";
                    if (bodyPrefix == null)
                    {
                        for (int i=0;i<_params.Count;i++)
                        {
                            URTParam param = (URTParam) _params[i];
                            if (param.ParamType == URTParamType.OUT)
                            {
                                sb.Length = 0;
                                sb.Append(newIndentation);
                                sb.Append(WsdlParser.IsValidCS(param.Name));
                                sb.Append(" = ");
                                sb.Append(ValueString(param.GetTypeString(curNS, true)));
                                sb.Append(';');
                                textWriter.WriteLine(sb);
                            }
                        }
                        Util.Log("URTMethod.PrintCSC return print");
                        sb.Length = 0;
                        sb.Append(newIndentation);
                        sb.Append("return");
                        String returnString = ValueString(GetTypeString(curNS, true));
                        if (returnString != null)
                        {
                            sb.Append('(');
                            sb.Append(returnString);
                            sb.Append(')');
                        }
                        sb.Append(';');
                    }
                    else
                    {
                        sb.Length = 0;
                        sb.Append(newIndentation);
                        if (ValueString(GetTypeString(curNS, true)) != null)
                            sb.Append("return ");
                        PrintMethodName(sb, bodyPrefix, _methodName);
                        //sb.Append(bodyPrefix);
                        //sb.Append(_methodName);
                        sb.Append('(');
                        if (_params.Count > 0)
                        {
                            Util.Log("URTMethod.PrintCSC Invoke _params[0] 3 PrintCSC");                                                    
                            ((URTParam) _params[0]).PrintCSC(sb);
                            for (int i=1;i<_params.Count;i++)
                            {
                                sb.Append(", ");
                                Util.Log("URTMethod.PrintCSC Invoke _params 4 PrintCSC");                                                       
                                ((URTParam) _params[i]).PrintCSC(sb);
                            }
                        }
                        sb.Append(");");
                    }
                    textWriter.WriteLine(sb);

                    textWriter.Write(indentation);
                    textWriter.WriteLine('}');
                }
            }

            private void PrintSoapAction(String action, StringBuilder sb)
            {
                sb.Append("[SoapMethod(SoapAction=");
                sb.Append(WsdlParser.IsValidUrl(action));
                sb.Append(")]");
            }

            private void PrintPropertyBody(TextWriter textWriter, String indentation, StringBuilder sb, String bodyPrefix)
            {
#if false
                [SoapMethod(SoapAction="http://schemas.microsoft.com/clr/nsassem/Testing.TestSimpleObject/test12#get_Value")]
                get{return ((TestSimpleObject) _tp).Value;}

                [SoapMethod(SoapAction="http://schemas.microsoft.com/clr/nsassem/Testing.TestSimpleObject/test12#set_Value")]
                set{((TestSimpleObject) _tp).Value = value;}
#endif

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append('{');
                textWriter.WriteLine(sb);

                String newIndentation = indentation + "    ";
                sb.Length = 0;
                sb.Append(newIndentation);
                if (_wsdlMethodInfo.bGet)
                {
                    sb.Length = 0;
                    sb.Append(newIndentation);
                    PrintSoapAction(_wsdlMethodInfo.soapActionGet, sb);
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(newIndentation);
                    sb.Append("get{return ");
                    PrintMethodName(sb, bodyPrefix, _wsdlMethodInfo.propertyName);
                    //sb.Append(bodyPrefix);
                    //sb.Append(_wsdlMethodInfo.propertyName);
                    sb.Append(";}");
                    textWriter.WriteLine(sb);
                }

                if (_wsdlMethodInfo.bSet)
                {
                    if (_wsdlMethodInfo.bGet)
                        textWriter.WriteLine();

                    sb.Length = 0;
                    sb.Append(newIndentation);
                    PrintSoapAction(_wsdlMethodInfo.soapActionSet, sb);
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(newIndentation);
                    sb.Append("set{");
                    PrintMethodName(sb, bodyPrefix, _wsdlMethodInfo.propertyName);
                    //sb.Append(bodyPrefix);
                    //sb.Append(_wsdlMethodInfo.propertyName);
                    sb.Append("= value;}");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append('}');
                textWriter.WriteLine(sb);
            }

            private void PrintMethodName(StringBuilder sb, String bodyPrefix, String name)
            {
                int index = name.LastIndexOf('.');
                if (index < 0)
                {
                    sb.Append(bodyPrefix);
                    sb.Append(WsdlParser.IsValidCS(name));
                }
                else
                {
                    // interface name, need to reconstruct bodyPrefix and strip name qualifier
                    String iface = name.Substring(0,index);
                    String methodName = name.Substring(index+1);
                    if (bodyPrefix == null)
                    {
                        // Non-wrapped proxy
                        sb.Append("(");
                        sb.Append(WsdlParser.IsValidCS(iface));
                        sb.Append(")");
                        sb.Append(WsdlParser.IsValidCS(methodName));
                    }
                    else
                    {
                        // wrapped proxy
                        sb.Append("((");
                        sb.Append(WsdlParser.IsValidCS(iface));
                        sb.Append(") _tp).");
                        sb.Append(WsdlParser.IsValidCS(methodName));
                    }
                }
            }

            // Returns string that is appropriate for the return type
            internal static String ValueString(String paramType)
            {
                String valueString;
                if (paramType == "void")
                    valueString = null;
                else if (paramType == "bool")
                    valueString = "false";
                else if (paramType == "string")
                    valueString = "null";
                else if (paramType == "sbyte" ||
                         paramType == "byte" ||
                         paramType == "short" ||
                         paramType == "ushort" ||
                         paramType == "int" ||
                         paramType == "uint" ||
                         paramType == "long" ||
                         paramType == "ulong")
                    valueString = "1";
                else if (paramType == "float" ||
                         paramType == "exfloat")
                    valueString = "(float)1.0";
                else if (paramType == "double" ||
                         paramType == "exdouble")
                    valueString = "1.0";
                else
                {
                    StringBuilder sb = new StringBuilder(50);
                    sb.Append('(');
                    sb.Append(WsdlParser.IsValidCS(paramType));
                    sb.Append(") (Object) null");
                    valueString = sb.ToString();
                }
                Util.Log("URTMethod.ValueString paramType "+paramType+" valueString "+valueString);                             
                return(valueString);
            }

            // This method is called when the parsing is complete
            // and is useful for derived types
            internal abstract void ResolveTypes(WsdlParser parser);


            // Helper method used by Resolve
            protected void ResolveWsdlParams(WsdlParser parser, String targetNS, String targetName,
                                             bool bRequest, WsdlMethodInfo wsdlMethodInfo)
            {
                Util.Log("URTMethod.ResolveWsdlParams targetName "+targetName+" targetNS "+targetNS+" bRequest "+bRequest+" wsdlMethodInfo "+wsdlMethodInfo);                               
                _wsdlMethodInfo = wsdlMethodInfo;
                _paramNamesOrder = _wsdlMethodInfo.paramNamesOrder;

                int length;
                if (_wsdlMethodInfo.bProperty)
                    length = 1;
                else if (bRequest)
                    length = wsdlMethodInfo.inputNames.Length;
                else
                    length = wsdlMethodInfo.outputNames.Length;

                for (int i=0; i<length; i++)
                {
                    String element = null;
                    String elementNs = null;
                    String name = null;
                    String nameNs = null;
                    String typeName = null;;
                    String typeNameNs = null;;
                    URTParamType pType;
                    if (_wsdlMethodInfo.bProperty)
                    {
                        typeName = wsdlMethodInfo.propertyType;
                        typeNameNs = wsdlMethodInfo.propertyNs;
                        pType = URTParamType.OUT;
                    }
                    else if (bRequest && !_wsdlMethodInfo.bProperty)
                    {
                        element = wsdlMethodInfo.inputElements[i];
                        elementNs = wsdlMethodInfo.inputElementsNs[i];
                        name = wsdlMethodInfo.inputNames[i];
                        nameNs = wsdlMethodInfo.inputNamesNs[i];
                        typeName = wsdlMethodInfo.inputTypes[i];
                        typeNameNs = wsdlMethodInfo.inputTypesNs[i];

                        pType = URTParamType.IN;
                    }
                    else
                    {
                        element = wsdlMethodInfo.outputElements[i];
                        elementNs = wsdlMethodInfo.outputElementsNs[i];
                        name = wsdlMethodInfo.outputNames[i];
                        nameNs = wsdlMethodInfo.outputNamesNs[i];
                        typeName = wsdlMethodInfo.outputTypes[i];
                        typeNameNs = wsdlMethodInfo.outputTypesNs[i];
                        pType = URTParamType.OUT;
                    }

                    String actualType;
                    String actualTypeNs;
                    if ((element == null) || element.Length == 0)
                    {
                        actualType = typeName;
                        actualTypeNs = typeNameNs;
                    }
                    else
                    {
                        actualType = element;
                        actualTypeNs = elementNs;
                    }

                    Util.Log("URTMethod.ResolveWsdlParams actualType "+actualType+" actualTypeNs "+actualTypeNs);
                    URTNamespace ns = parser.LookupNamespace(actualTypeNs);
                    if (ns == null)
                    {
                        throw new SUDSParserException(
                                                     String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                                                   actualTypeNs, actualType));
                    }

                    URTComplexType ct = ns.LookupComplexType(actualType);

                    if (ct != null && ct.IsArray())
                    {
                        if (ct.GetArray() == null)
                            ct.ResolveArray();
                        String arrayName = ct.GetArray();
                        URTNamespace arrayNS = ct.GetArrayNS();
                        AddParam(new URTParam(name, arrayName, arrayNS.Name, arrayNS.EncodedNS, pType, true, parser, arrayNS));
                    }
                    else
                    {
                        Util.Log("URTMethod.ResolveWsdlParams actualType 2 UrtType "+((Enum)ns.UrtType).ToString());
                        if (ns.UrtType == UrtType.Xsd)
                        {
                            String clrtypeName = parser.MapSchemaTypesToCSharpTypes(actualType);
                            AddParam(new URTParam(name, clrtypeName, ns.Namespace, ns.EncodedNS, pType, true, parser, ns));
                        }
                        else
                        {
                            String foundTypeName = null;
                            if (ct != null)
                            {
                                foundTypeName = ct.Name;
                            }
                            else
                            {
                                URTSimpleType stype = ns.LookupSimpleType(actualType);
                                if (stype != null)
                                {
                                    foundTypeName = stype.Name;
                                }
                                else
                                {
                                    foundTypeName = actualType; 

                                    /*
                                    throw new SUDSParserException(
                                        String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                      actualType, ns.Name));
                                                      */
                                }
                            }
                            //typeNS.RemoveComplexType(type);
                            AddParam(new URTParam(name, foundTypeName, ns.Namespace, ns.EncodedNS, pType, true, parser, ns));
                        }
                    }
                }
            }


            // Fields
            private String _methodName;
            private String _soapAction;
            private URTParam _methodType;
            internal URTComplexType _complexType;
            protected String[] _paramNamesOrder; // parameterOrder names from wsdl 
            protected ArrayList _params = new ArrayList();
            protected ArrayList _paramPosition = new ArrayList();
            private MethodFlags _methodFlags;
            private WsdlMethodInfo _wsdlMethodInfo;
        }

        // Repesents a request response method
        internal class RRMethod : URTMethod
        {
            // Constructor

            internal RRMethod(WsdlMethodInfo wsdlMethodInfo, URTComplexType complexType) 
            : base (wsdlMethodInfo.methodName, wsdlMethodInfo.soapAction, wsdlMethodInfo.methodAttributes, complexType)
            {
                Util.Log("RRMethod.RRMethod WsdlMethodInfo name "+wsdlMethodInfo.methodName+" soapAction "+wsdlMethodInfo.soapAction);
                _wsdlMethodInfo = wsdlMethodInfo;
                _requestElementName = null;
                _requestElementNS = null;
                _responseElementName = null;
                _responseElementNS = null;
            }

            // Adds the request element
            internal void AddRequest(String name, String ns)
            {
                Util.Log("RRMethod.AddRequest name "+name+" ns "+ns);               
                _requestElementName = name;
                _requestElementNS = ns;
            }

            // Adds the response element
            internal void AddResponse(String name, String ns)
            {
                Util.Log("RRMethod.AddResponse name "+name+" ns "+ns);                              
                _responseElementName = name;
                _responseElementNS = ns;
            }

            // Resolves the method
            internal override void ResolveTypes(WsdlParser parser)
            {
                Util.Log("RRMethod.ResolveTypes "+_requestElementName+" "+_responseElementName);
                ResolveWsdlParams(parser, _requestElementNS, _requestElementName, true, _wsdlMethodInfo);
                ResolveWsdlParams(parser, _responseElementNS, _responseElementName, false, _wsdlMethodInfo);

                if (_paramNamesOrder != null)
                {
                    // reorder parameters if there is a parameter order
                    Object[] paramsInOrder = new Object[_params.Count];
                    for (int i=0; i<_params.Count; i++)
                    {
                        paramsInOrder[(int)_paramPosition[i]] = _params[i];
                    }
                    _params =  new ArrayList(paramsInOrder);
                }

                ResolveMethodAttributes(); //Needs to be after param order

                return;
            }

            internal override void PrintCSC(TextWriter textWriter, String indentation,
                                            String namePrefix, String curNS, MethodPrintEnum methodPrintEnum,
                                            bool bURTType, String bodyPrefix, StringBuilder sb)
            {
                Util.Log("RRMethod.PrintCSC name "+_requestElementName+" namePrefix "+namePrefix+" curNS "+curNS+" methodPrintEnum "+((Enum)methodPrintEnum).ToString());
                //if(bURTType == false)

                // Don't want to generate a Finalize or destructor method
                if (Name == "Finalize")
                    return;

                bool bSoapAction = false;
                if (SoapAction != null)
                    bSoapAction = true;

                if ((bSoapAction || !bURTType) && !_wsdlMethodInfo.bProperty)
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[SoapMethod(");

                    if (bSoapAction)
                    {
                        sb.Append("SoapAction=");
                        sb.Append(WsdlParser.IsValidUrl(SoapAction));
                    }
                    if (!bURTType)
                    {
                        if (bSoapAction)
                            sb.Append(",");

                        sb.Append("ResponseXmlElementName=");
                        sb.Append(WsdlParser.IsValidUrl(_responseElementName));
                        if (MethodType != null)
                        {
                            sb.Append(", ReturnXmlElementName=");
                            sb.Append(WsdlParser.IsValidUrl(MethodType.Name));
                        }
                        sb.Append(", XmlNamespace=");
                        sb.Append(WsdlParser.IsValidUrl(_wsdlMethodInfo.inputMethodNameNs)); 
                        sb.Append(", ResponseXmlNamespace=");
                        sb.Append(WsdlParser.IsValidUrl(_wsdlMethodInfo.outputMethodNameNs)); 
                    }
                    sb.Append(")]");
                    textWriter.WriteLine(sb);                   
                }

                Util.Log("RRMethod.PrintCSC Invoke base PrintCSC");                                     
                base.PrintCSC(textWriter, indentation, namePrefix, curNS, methodPrintEnum, bURTType,
                              bodyPrefix, sb);
                return;
            }

            // Fields
            private String _requestElementName;
            private String _requestElementNS;
            //private String _requestTypeName;
            //private String _requestTypeNS;
            private String _responseElementName;
            private String _responseElementNS;
            private WsdlMethodInfo _wsdlMethodInfo;
            //private String _responseTypeName;
            //private String _responseTypeNS;
        }

        // Represents a oneway method
        internal class OnewayMethod : URTMethod
        {
            // Constructor
            internal OnewayMethod(String name, String soapAction, URTComplexType complexType)
            : base(name, soapAction, null, complexType)
            {
                Util.Log("OnewayMethod.OnewayMethod name "+name+" soapAction "+soapAction);
                _messageElementName = null;
                _messageElementNS = null;
                //_messageTypeName = null;
                //_messageTypeNS = null;
            }

            internal OnewayMethod(WsdlMethodInfo wsdlMethodInfo, URTComplexType complexType) 
            : base (wsdlMethodInfo.methodName, wsdlMethodInfo.soapAction, wsdlMethodInfo.methodAttributes, complexType)
            {
                Util.Log("OnewayMethod.OnewayMethod WsdlMethodInfo name "+wsdlMethodInfo.methodName+" soapAction "+wsdlMethodInfo.soapAction);
                _wsdlMethodInfo = wsdlMethodInfo;
                _messageElementName = null;
                _messageElementNS = null;
            }


            // Adds the request element
            internal void AddMessage(String name, String ns)
            {
                Util.Log("OnewayMethod.AddMessage name "+name+" ns "+ns);               
                _messageElementName = name;
                _messageElementNS = ns;
            }

            // Resolves the method
            internal override void ResolveTypes(WsdlParser parser)
            {
                Util.Log("OnewayMethod.ResolveTypes name "+ _messageElementName);
                ResolveWsdlParams(parser, _messageElementNS, _messageElementName, true, _wsdlMethodInfo);
                if (_paramNamesOrder != null)
                {
                    // reorder parameters if there is a parameter order
                    Object[] paramsInOrder = new Object[_params.Count];
                    for (int i=0; i<_params.Count; i++)
                    {
                        paramsInOrder[(int)_paramPosition[i]] = _params[i];
                    }
                    _params =  new ArrayList(paramsInOrder);
                }

                ResolveMethodAttributes(); //Needs to be after param order

                return;
            }



            // Writes the oneway attribute and delegates to the base implementation
            internal override void PrintCSC(TextWriter textWriter, String indentation,
                                            String namePrefix, String curNS, MethodPrintEnum methodPrintEnum,
                                            bool bURTType, String bodyPrefix, StringBuilder sb)
            {
                Util.Log("OnewayMethod.PrintCSC name "+_messageElementName+" namePrefix "+namePrefix+" curNS "+curNS+" methodPrintEnum "+((Enum)methodPrintEnum).ToString());

                if (Name == "Finalize")
                    return;

                bool bSoapAction = false;
                if (SoapAction != null)
                    bSoapAction = true;

                if (!(bSoapAction || !bURTType))
                {
                    textWriter.Write(indentation);
                    textWriter.WriteLine("[OneWay]");
                }
                else
                {
                    sb.Length = 0;
                    sb.Append(indentation);
                    sb.Append("[OneWay, SoapMethod(");

                    if (bSoapAction)
                    {
                        sb.Append("SoapAction=");
                        sb.Append(WsdlParser.IsValidUrl(SoapAction));
                    }
                    if (!bURTType)
                    {
                        if (bSoapAction)
                            sb.Append(",");

                        sb.Append("XmlNamespace=");
                        sb.Append(WsdlParser.IsValidUrl(_wsdlMethodInfo.inputMethodNameNs)); 
                        //sb.Append(_messageElementNS);
                    }
                    sb.Append(")]");
                    textWriter.WriteLine(sb);                   
                }


                Util.Log("OnewayMethod.PrintCSC Invoke base PrintCSC");                                                     
                base.PrintCSC(textWriter, indentation, namePrefix, curNS, methodPrintEnum, bURTType,
                              bodyPrefix, sb);

                return;
            }

            // Fields
            private String _messageElementName;
            private String _messageElementNS;
            private WsdlMethodInfo _wsdlMethodInfo;
            //private String _messageTypeName;
            //private String _messageTypeNS;
        }

        // Base class for interfaces
        internal abstract class BaseInterface
        {
            internal BaseInterface(String name, String urlNS, String ns, String encodedNS, WsdlParser parser)
            {
                Util.Log("BaseInterface.BaseInterface");
                _name = name;
                _urlNS = urlNS;
                _namespace = ns;
                _encodedNS = encodedNS;
                _parser = parser;
            }
            internal String Name
            {
                get { return(_name);}
            }
            internal String UrlNS
            {
                get { return(_urlNS);}
            }
            internal String Namespace
            {
                get { return(_namespace);}
            }

            internal bool IsURTInterface
            {
                get { return((Object) _namespace == (Object) _encodedNS);}
            }
            internal String GetName(String curNS)
            {
                String name;
                if (_parser.Qualify(_namespace, curNS))
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    sb.Append(WsdlParser.IsValidCS(_name));
                    name = sb.ToString();
                }
                else
                    name = _name;

                Util.Log("BaseInterface.GetName curNS "+curNS);
                return(name);
            }
            internal abstract void PrintClassMethods(TextWriter textWriter,
                                                     String indentation,
                                                     String curNS,
                                                     ArrayList printedIFaces,
                                                     bool bProxy, StringBuilder sb);
            private String _name;
            private String _urlNS;
            private String _namespace;
            private String _encodedNS;
            private WsdlParser _parser;
        }

        // Represents a system interface
        internal class SystemInterface : BaseInterface
        {
            internal SystemInterface(String name, String urlNS, String ns, WsdlParser parser, String assemName)
            : base(name, urlNS, ns, ns, parser)
            {
                Util.Log("SystemInterface.SystemInterface");                
                Debug.Assert(ns.StartsWith("System", StringComparison.Ordinal), "Invalid System type");
                String fullName = ns + '.' + name;

                Assembly assem = null;
                if (assemName == null)
                    assem = typeof(string).Assembly;
                else
#pragma warning disable 618
                    assem = Assembly.LoadWithPartialName(assemName, null);
#pragma warning restore 618

                if (assem == null)
                    throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_AssemblyNotFound"), assemName));

                _type = assem.GetType(fullName, true);
            }
            internal override void PrintClassMethods(TextWriter textWriter,
                                                     String indentation,
                                                     String curNS,
                                                     ArrayList printedIFaces,
                                                     bool bProxy,
                                                     StringBuilder sb)
            {
                Util.Log("SystemInterface.PrintClassMethods "+curNS+" bProxy "+bProxy);                             
                // Return if the interfaces have already been printed
                int i;
                for (i=0;i<printedIFaces.Count;i++)
                {
                    if (printedIFaces[i] is SystemInterface)
                    {
                        SystemInterface iface = (SystemInterface) printedIFaces[i];
                        if (iface._type == _type)
                            return;
                    }
                }
                printedIFaces.Add(this);

                // Count of implemented methods
                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
                                      BindingFlags.Public;// | BindingFlags.NonPublic;
                ArrayList types = new ArrayList();
                sb.Length = 0;
                types.Add(_type);
                i=0;
                int j=1;
                while (i<j)
                {
                    Type type = (Type) types[i];
                    MethodInfo[] methods = type.GetMethods(bFlags);
                    Type[] iFaces = type.GetInterfaces();
                    for (int k=0;k<iFaces.Length;k++)
                    {
                        for (int l=0;l<j;l++)
                        {
                            if (type == iFaces[k])
                                goto Loopback;
                        }
                        types.Add(iFaces[k]);
                        j++;
                        Loopback:
                        continue;
                    }

                    for (int k=0;k<methods.Length;k++)
                    {
                        MethodInfo method = methods[k];
                        sb.Length = 0;
                        sb.Append(indentation);
                        sb.Append(CSharpTypeString(method.ReturnType.FullName));
                        sb.Append(' ');
                        sb.Append(WsdlParser.IsValidCS(type.FullName));
                        sb.Append('.');
                        sb.Append(WsdlParser.IsValidCS(method.Name));
                        sb.Append('(');
                        ParameterInfo[] parameters = method.GetParameters();
                        for (int l=0;l<parameters.Length;l++)
                        {
                            if (l != 0)
                                sb.Append(", ");
                            ParameterInfo param = parameters[l];
                            Type parameterType = param.ParameterType;
                            if (param.IsIn)
                                sb.Append("in ");
                            else if (param.IsOut)
                                sb.Append("out ");
                            else if (parameterType.IsByRef)
                            {
                                sb.Append("ref ");
                                parameterType = parameterType.GetElementType();
                            }
                            sb.Append(CSharpTypeString(parameterType.FullName));
                            sb.Append(' ');
                            sb.Append(WsdlParser.IsValidCS(param.Name));
                        }
                        sb.Append(')');
                        textWriter.WriteLine(sb);

                        textWriter.Write(indentation);
                        textWriter.WriteLine('{');

                        String newIndentation = indentation + "    ";
                        if (bProxy == false)
                        {
                            for (int l=0;l<parameters.Length;l++)
                            {
                                ParameterInfo param = parameters[l];
                                Type parameterType = param.ParameterType;
                                if (param.IsOut)
                                {
                                    sb.Length = 0;
                                    sb.Append(newIndentation);
                                    sb.Append(WsdlParser.IsValidCS(param.Name));
                                    sb.Append(URTMethod.ValueString(CSharpTypeString(param.ParameterType.FullName)));
                                    sb.Append(';');
                                    textWriter.WriteLine(sb);
                                }
                            }

                            Util.Log("SystemInterface.PrintClassMethods return 1 print");                           
                            sb.Length = 0;
                            sb.Append(newIndentation);
                            sb.Append("return");
                            String valueString = URTMethod.ValueString(CSharpTypeString(method.ReturnType.FullName));
                            if (valueString != null)
                            {
                                sb.Append('(');
                                sb.Append(valueString);
                                sb.Append(')');
                            }
                            sb.Append(';');
                        }
                        else
                        {
                            Util.Log("SystemInterface.PrintClassMethods return 2 print");                                                       
                            sb.Length = 0;
                            sb.Append(newIndentation);
                            sb.Append("return((");
                            sb.Append(WsdlParser.IsValidCS(type.FullName));
                            sb.Append(") _tp).");
                            sb.Append(WsdlParser.IsValidCS(method.Name));
                            sb.Append('(');
                            if (parameters.Length > 0)
                            {
                                int lastParameter = parameters.Length-1;
                                for (int l=0;l<parameters.Length;l++)
                                {
                                    ParameterInfo param = parameters[0];
                                    Type parameterType = param.ParameterType;
                                    if (param.IsIn)
                                        sb.Append("in ");
                                    else if (param.IsOut)
                                        sb.Append("out ");
                                    else if (parameterType.IsByRef)
                                        sb.Append("ref ");
                                    sb.Append(WsdlParser.IsValidCS(param.Name));
                                    if (l < lastParameter)
                                        sb.Append(", ");
                                }
                            }
                            sb.Append(");");
                        }
                        textWriter.WriteLine(sb);

                        textWriter.Write(indentation);
                        textWriter.WriteLine('}');
                    }

                    ++i;
                }

                return;
            }
            private static String CSharpTypeString(String typeName)
            {
                Util.Log("SystemInterface.CSharpTypeString typeName "+typeName);                                                
                String CSCTypeName = typeName;
                if (typeName == "System.SByte")
                    CSCTypeName = "sbyte";
                else if (typeName == "System.byte")
                    CSCTypeName = "byte";
                else if (typeName == "System.Int16")
                    CSCTypeName = "short";
                else if (typeName == "System.UInt16")
                    CSCTypeName = "ushort";
                else if (typeName == "System.Int32")
                    CSCTypeName = "int";
                else if (typeName == "System.UInt32")
                    CSCTypeName = "uint";
                else if (typeName == "System.Int64")
                    CSCTypeName = "long";
                else if (typeName == "System.UInt64")
                    CSCTypeName = "ulong";
                else if (typeName == "System.Char")
                    CSCTypeName = "char";
                else if (typeName == "System.Single")
                    CSCTypeName = "float";
                else if (typeName == "System.Double")
                    CSCTypeName = "double";
                else if (typeName == "System.Boolean")
                    CSCTypeName = "boolean";
                else if (typeName == "System.Void")
                    CSCTypeName = "void";
                else if (typeName == "System.String")
                    CSCTypeName = "String";

                return(WsdlParser.IsValidCSAttr(CSCTypeName));
            }

            Type _type;
        }

        // Represents an interface

        internal class URTInterface : BaseInterface
        {
            internal URTInterface(String name, String urlNS, String ns, String encodedNS, WsdlParser parser)
            : base(name, urlNS, ns, encodedNS, parser)
            {
                Util.Log("URTInterface.URTInterface name "+name+" ns "+ns+" encodedNS "+encodedNS);                                             
                _baseIFaces = new ArrayList();
                _baseIFaceNames = new ArrayList();
                _extendsInterface = new ArrayList();
                _methods = new ArrayList();
                _parser = parser;
            }
            internal void Extends(String baseName, String baseNS, WsdlParser parser)
            {
                Util.Log("URTInterface.Extends baseName "+baseName+" baseNSf "+baseNS);
                _baseIFaceNames.Add(baseName);
                _baseIFaceNames.Add(baseNS);
                // Urt namespace will not have schema, they need to be recorded.
                URTNamespace parsingNamespace = parser.AddNewNamespace(baseNS);
                /*
                if (parsingNamespace == null)
                {
                    parsingNamespace = new URTNamespace(baseNS, parser);
                }
                */

                URTInterface parsingInterface = parsingNamespace.LookupInterface(baseName);         
                if (parsingInterface == null)
                {
                    parsingInterface = new URTInterface(baseName, parsingNamespace.Name, parsingNamespace.Namespace, parsingNamespace.EncodedNS, parser);                  
                    parsingNamespace.AddInterface(parsingInterface);
                }
                _extendsInterface.Add(parsingInterface);
            }

            internal void AddMethod(URTMethod method)
            {
                Util.Log("URTInterface.AddMethod method "+method.Name);
                _methods.Add(method);
                method.MethodFlags = MethodFlags.None; // method names don't have public modifiers
            }

            // Check if interface method occurs up the inheritance hierarchy
            internal void NewNeeded(URTMethod method)
            {
                Util.Log("URTInterface.NewNeeded Enter interface "+Name+" method "+method.Name);
                foreach (URTInterface urtInterface in _extendsInterface)
                {
                    urtInterface.CheckIfNewNeeded(method);
                    if (URTMethod.MethodFlagsTest(method.MethodFlags, MethodFlags.New))
                        break;

                }
                Util.Log("URTInterface.NewNeeded Exit interface "+Name+" method "+method.Name+" "+((Enum)method.MethodFlags).ToString());
            }

            // Check this interface for method
            private void CheckIfNewNeeded(URTMethod method)
            {
                foreach (URTMethod urtMethod in _methods)
                {
                    if (urtMethod.Name == method.Name)
                    {
                        method.MethodFlags |= MethodFlags.New;
                        break;
                    }
                }

                if (URTMethod.MethodFlagsTest(method.MethodFlags, MethodFlags.New))
                    NewNeeded(method);
            }

            internal void ResolveTypes(WsdlParser parser)
            {
                Util.Log("URTInterface.ResolveTypes "+Name);                
                for (int i=0;i<_baseIFaceNames.Count;i=i+2)
                {
                    String baseIFaceName = (String) _baseIFaceNames[i];
                    String baseIFaceXmlNS = (String) _baseIFaceNames[i+1];
                    String baseIFaceNS, baseIFaceAssemName;
                    BaseInterface iFace;
                    UrtType iType = parser.IsURTExportedType(baseIFaceXmlNS, out baseIFaceNS,
                                                             out baseIFaceAssemName);

                    Util.Log("URTInterface.ResolveTypes Is System "+Name+" iType "+((Enum)iType).ToString()+" baseIFaceNS "+baseIFaceNS);                                   
                    if ((iType != UrtType.Interop) && baseIFaceNS.StartsWith("System", StringComparison.Ordinal))
                    {
                        iFace = new SystemInterface(baseIFaceName, baseIFaceXmlNS, baseIFaceNS, _parser, baseIFaceAssemName);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(baseIFaceXmlNS);
                        if (ns == null)
                        {
                            throw new SUDSParserException(
                                                         String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                                                       baseIFaceXmlNS, baseIFaceName));
                        }
                        iFace = ns.LookupInterface(baseIFaceName);
                        if (iFace == null)
                        {
                            throw new SUDSParserException(
                                                         String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                                       baseIFaceName, baseIFaceXmlNS));
                        }
                    }
                    _baseIFaces.Add(iFace);
                }
                for (int i=0;i<_methods.Count;i++)
                    ((URTMethod) _methods[i]).ResolveTypes(parser);
            }
            internal void PrintCSC(TextWriter textWriter, String indentation,
                                   String curNS, StringBuilder sb)
            {
                Util.Log("URTInterface.PrintCSC name "+Name+" curNS "+curNS);               
                bool bURTType = IsURTInterface;

                sb.Length = 0;
                sb.Append("\n"); 
                sb.Append(indentation);
                sb.Append("[SoapType(");

                if (_parser._xsdVersion == XsdVersion.V1999)
                {
                    sb.Append("SoapOptions=SoapOption.Option1|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }
                else if (_parser._xsdVersion == XsdVersion.V2000)
                {
                    sb.Append("SoapOptions=SoapOption.Option2|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }

                if (!bURTType)
                {
                    sb.Append("XmlElementName=");
                    sb.Append(WsdlParser.IsValidUrl(Name));
                    sb.Append(", XmlNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(Namespace));
                    sb.Append(", XmlTypeName=");
                    sb.Append(WsdlParser.IsValidUrl(Name));
                    sb.Append(", XmlTypeNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(Namespace));
                }
                else
                {
                    // Need namespace for clr type because proxy dll might have a different name than server dll
                    sb.Append("XmlNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(UrlNS));
                    sb.Append(", XmlTypeNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(UrlNS));
                }

                sb.Append(")]");
                sb.Append("[ComVisible(true)]");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("public interface ");
                sb.Append(WsdlParser.IsValidCS(Name));

                if (_baseIFaces.Count > 0)
                    sb.Append(" : ");

                if (_baseIFaces.Count > 0)
                {
                    sb.Append(WsdlParser.IsValidCSAttr(((BaseInterface) _baseIFaces[0]).GetName(curNS)));
                    for (int i=1;i<_baseIFaces.Count;i++)
                    {
                        sb.Append(", ");
                        sb.Append(WsdlParser.IsValidCSAttr(((BaseInterface) _baseIFaces[i]).GetName(curNS)));
                    }
                }

                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                String namePrefix = " ";
                Util.Log("URTInterface.PrintCSC method count "+_methods.Count);

                for (int i=0;i<_methods.Count;i++)
                {
                    Util.Log("URTInterface.PrintCSC Invoke methods in Interface PrintCSC "+Name+" methodName "+((URTMethod) _methods[i]).Name);
                    NewNeeded((URTMethod)_methods[i]);
                    ((URTMethod) _methods[i]).PrintCSC(textWriter, newIndentation,
                                                       namePrefix, curNS, MethodPrintEnum.InterfaceMethods, bURTType,
                                                       null, sb);
                }
                textWriter.Write(indentation);
                textWriter.WriteLine('}');
            }

            // No longer used
            internal override void PrintClassMethods(TextWriter textWriter,
                                                     String indentation,
                                                     String curNS,
                                                     ArrayList printedIFaces,
                                                     bool bProxy,
                                                     StringBuilder sb)
            {
                Util.Log("URTInterface.PrintClassMethods method "+Name+" curNS "+curNS+" bProxy "+bProxy);              
                // Return if the interface has already been printed
                for (int i=0;i<printedIFaces.Count;i++)
                {
                    if (printedIFaces[i] == this)
                    {
                        Util.Log("URTInterface.PrintClassMethods printedIFaces return "+Name);
                        return;
                    }
                }
                Util.Log("URTInterface.PrintClassMethods method 2 "+Name+" _methods.Count "+_methods.Count);                
                printedIFaces.Add(this);
                sb.Length = 0;
                sb.Append(indentation);
                if (_methods.Count > 0)
                {
                    sb.Append("// ");
                    sb.Append(WsdlParser.IsValidCS(Name));
                    sb.Append(" interface Methods");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(' ');
                    String ifaceName = GetName(curNS);
                    sb.Append(WsdlParser.IsValidCS(ifaceName));
                    sb.Append('.');
                    String namePrefix = sb.ToString();

                    String bodyPrefix = null;
                    if (bProxy)
                    {
                        sb.Length = 0;
                        sb.Append("((");
                        sb.Append(WsdlParser.IsValidCS(ifaceName));
                        sb.Append(") _tp).");
                        bodyPrefix = sb.ToString();
                    }

                    MethodPrintEnum methodPrintEnum = MethodPrintEnum.PrintBody | MethodPrintEnum.InterfaceInClass;
                    for (int i=0;i<_methods.Count;i++)
                    {
                        Util.Log("URTInterface.PrintClassMethods URTMethod invoke interface methods in class PrintCSC "+Name+" methodName "+((URTMethod) _methods[i]));

                        ((URTMethod) _methods[i]).PrintCSC(textWriter, indentation,
                                                           namePrefix, curNS, methodPrintEnum,
                                                           true, bodyPrefix, sb); 
                    }
                }

                for (int i=0;i<_baseIFaces.Count;i++)
                {
                    Util.Log("URTInterface.PrintClassMethods BaseIFaces "+Name);
                    ((BaseInterface) _baseIFaces[i]).PrintClassMethods(textWriter,
                                                                       indentation,
                                                                       curNS,
                                                                       printedIFaces,
                                                                       bProxy, sb);
                }
            }

            private WsdlParser _parser;
            private ArrayList _baseIFaces;
            private ArrayList _baseIFaceNames;
            private ArrayList _methods;
            private ArrayList _extendsInterface;
        }

        // Represents a field of a type
        internal class URTField
        {
            internal URTField(String name, String typeName, String xmlNS, WsdlParser parser,
                              bool bPrimitive, bool bEmbedded, bool bAttribute, bool bOptional,
                              bool bArray, String arraySize, URTNamespace urtNamespace)
            {
                Util.Log("URTField.URTField "+name+" typeName "+typeName+" xmlNS "+xmlNS+" bPrimitive "+bPrimitive+" bEmbedded "+bEmbedded+" bAttribute "+bAttribute);
                _name = name;
                _typeName = typeName;
                _parser = parser;
                String typeAssemName;

                UrtType urtType = parser.IsURTExportedType(xmlNS, out _typeNS, out typeAssemName);
                if (urtType == UrtType.Interop)
                    _encodedNS = urtNamespace.EncodedNS;
                else
                    _encodedNS = _typeNS;
                _primitiveField = bPrimitive;
                _embeddedField = bEmbedded;
                _attributeField = bAttribute;
                _optionalField = bOptional;
                _arrayField = bArray;
                _arraySize = arraySize;
                _urtNamespace = urtNamespace;
            }
            internal String TypeName
            {
                get
                {
                    if (_arrayField)
                        return(_typeName + "[]");
                    return(_typeName);
                }
            }
            internal String TypeNS
            {
                get { return(_typeNS);}
            }
            internal bool IsPrimitive
            {
                get { return(_primitiveField);}
            }
            internal bool IsArray
            {
                get { return(_arrayField);}
            }
            internal String GetTypeString(String curNS, bool bNS)
            {
                return _parser.GetTypeString (curNS, bNS, _urtNamespace, TypeName, _typeNS);
            }

            internal void PrintCSC(TextWriter textWriter, String indentation,
                                   String curNS, StringBuilder sb)
            {
                Util.Log("URTField.PrintCSC name "+_name+" curNS"+curNS);               
                if (_embeddedField)
                {
                    textWriter.Write(indentation);
                    textWriter.WriteLine("[SoapField(Embedded=true)]");
                }

                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("public ");
                sb.Append(WsdlParser.IsValidCSAttr(GetTypeString(curNS, true))); 
                sb.Append(' ');
                sb.Append(WsdlParser.IsValidCS(_name));
                sb.Append(';');
                textWriter.WriteLine(sb);
            }

            private String _name;
            private String _typeName;
            private String _typeNS;
            private String _encodedNS;
            private bool _primitiveField;
            private bool _embeddedField;
            private bool _attributeField;
            private bool _optionalField;
            private bool _arrayField;
            private String _arraySize;
            private WsdlParser _parser;
            private URTNamespace _urtNamespace;
        }

        internal abstract class SchemaFacet
        {
            protected SchemaFacet()
            {
            }
            internal virtual void ResolveTypes(WsdlParser parser)
            {
            }
            internal abstract void PrintCSC(TextWriter textWriter, String newIndentation,
                                            String curNS, StringBuilder sb);
        }

        internal class EnumFacet : SchemaFacet
        {
            internal EnumFacet(String valueString, int value)
            : base()
            {
                Util.Log("EnumFacet.EnumFacet valueString "+valueString+" value "+value);               
                _valueString = valueString;
                _value = value;
            }
            internal override void PrintCSC(TextWriter textWriter, String newIndentation,
                                            String curNS, StringBuilder sb)
            {
                Util.Log("EnumFacet.PrintCSC _valueString "+_valueString+" value "+_value+" curNS "+curNS);
                sb.Length = 0;
                sb.Append(newIndentation);
                sb.Append(WsdlParser.IsValidCS(_valueString));
                sb.Append(" = ");
                sb.Append(_value);
                sb.Append(',');
                textWriter.WriteLine(sb);
                return;
            }

            private String _valueString;
            private int _value;
        }

        // Represents a Base type
        internal abstract class BaseType
        {
            internal BaseType(String name, String urlNS, String ns, String encodedNS)
            {
                _searchName = name;
                _name = name;
                _urlNS = urlNS;
                _namespace = ns;
                _elementName = _name;
                _elementNS = ns;
                _encodedNS = encodedNS;
                Util.Log("BaseType.BaseType in name "+name+" storedname "+_name+" nested "+_bNestedType);
            }
            internal String Name
            {
                get { return(_name);}
                set { _name = value;}
            }

            internal String SearchName
            {
                get { return(_searchName);}
                set { _searchName = value;}
            }

            internal String OuterTypeName
            {
                set { _outerTypeName = value;}


            }

            internal String NestedTypeName
            {
                get { return(_nestedTypeName);}
                set { _nestedTypeName = value;}
            }

            internal String FullNestedTypeName
            {
                set { _fullNestedTypeName = value;}
            }

            internal bool bNestedType
            {
                get { return(_bNestedType);}
                set { _bNestedType = value;}
            }

            internal bool bNestedTypePrint
            {
                get { return(_bNestedTypePrint);}
                set { _bNestedTypePrint = value;}
            }

            internal String UrlNS
            {
                get { return(_urlNS);}
            }

            internal String Namespace
            {
                get { return(_namespace);}
            }

            internal String ElementName
            {
                set { _elementName = value;}
            }
            internal String ElementNS
            {
                set { _elementNS = value;}
            }

            internal bool IsURTType
            {
                get {
                    Util.Log("BaseType.IsURTType _namespace "+_namespace+" _encodedNS "+_encodedNS+" "+((Object) _namespace == (Object) _encodedNS));
                    return((Object) _namespace == (Object) _encodedNS);}
            }

            internal virtual String GetName(String curNS)
            {
                String name;
                if (MatchingStrings(_namespace, curNS))
                    name = _name;
                else
                {
                    StringBuilder sb = new StringBuilder(_encodedNS, 50);
                    sb.Append('.');
                    sb.Append(WsdlParser.IsValidCS(_name));
                    name = sb.ToString();
                }

                return(name);
            }
            internal abstract MethodFlags GetMethodFlags(URTMethod method);
            internal abstract bool IsEmittableFieldType
            {
                get;
            }
            internal abstract String FieldName
            {
                get;
            }

            internal abstract String FieldNamespace
            {
                get;
            }

            internal abstract bool PrimitiveField
            {
                get;
            }

            private String _name;
            private String _searchName;
            private String _urlNS;
            private String _namespace;
            private String _elementName;
            private String _elementNS;
            private String _encodedNS;
            internal ArrayList _nestedTypes; // nested types within this type
            internal String _nestedTypeName;  //If this type is nested, name of nested type (without + mangle)
            internal String _fullNestedTypeName; //If this type is nested, name of nested type (without + mangle) with outer class qualifier
            internal String _outerTypeName;
            internal bool _bNestedType = false; //Contains nested types
            internal bool _bNestedTypePrint = false;
        }

        // Representa a system type
        internal class SystemType : BaseType
        {
            internal SystemType(String name, String urlNS, String ns, String assemName)
            : base(name, urlNS, ns, ns)
            {
                Util.Log("SystemType.SystemType name "+name+" ns "+ns+" assemName "+assemName);             
                Debug.Assert(ns.StartsWith("System", StringComparison.Ordinal), "Invalid System type");

                String fullName = ns + '.' + name;

                Assembly assem = null;
                if (assemName == null)
                    assem = typeof(string).Assembly;
                else
#pragma warning disable 618
                    assem = Assembly.LoadWithPartialName(assemName, null);
#pragma warning restore 618

                if (assem == null)
                    throw new SUDSParserException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_AssemblyNotFound"), assemName));

                _type = assem.GetType(fullName, true);
            }
            internal override MethodFlags GetMethodFlags(URTMethod method)
            {
                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
                                      BindingFlags.Public | BindingFlags.NonPublic;
                Type type = _type;
                while (type != null)
                {
                    MethodInfo[] methods = type.GetMethods(bFlags);
                    for (int i=0;i<methods.Length;i++)
                    {
                        MethodFlags methodFlags = method.GetMethodFlags(methods[i]);
                        if (methodFlags != 0)
                            return(methodFlags);
                    }
                    type = type.BaseType;
                }

                return(0);
            }
            internal override bool IsEmittableFieldType
            {
                get { return(true);}
            }

            internal override String FieldName
            {
                get { return(null);}
            }
            internal override String FieldNamespace
            {
                get { return(null);}
            }
            internal override bool PrimitiveField
            {
                get { return(false);}
            }

            private Type _type;
        }

        // Represents a simple type
        internal class URTSimpleType : BaseType
        {
            internal URTSimpleType(String name, String urlNS, String ns, String encodedNS, bool bAnonymous, WsdlParser parser)
            : base(name, urlNS, ns, encodedNS)
            {
                Util.Log("URTSimpleType.URTSimpleType name "+name+" ns "+ns+" encodedNS "+encodedNS+" bAnonymous "+bAnonymous);
                _baseTypeName = null;
                _baseTypeXmlNS = null;
                _baseType = null;
                _fieldString = null;
                _facets = new ArrayList();
                _bEnum = false;
                _bAnonymous = bAnonymous;
                _encoding = null;
                _parser = parser;
            }

            internal void Extends(String baseTypeName, String baseTypeNS)
            {
                Util.Log("URTSimpleType.Extends baseTypeName "+baseTypeName+" baseTypeNS "+baseTypeNS);
                _baseTypeName = baseTypeName;
                _baseTypeXmlNS = baseTypeNS;

            }

            internal bool IsEnum
            {
                get { return(_bEnum);}
                set { _bEnum = value;}
            }

            internal String EnumType
            {
                set {
                    String typeName = value;
                    String typeNS = _parser.ParseQName(ref typeName);
                    if (typeName != null && typeName.Length > 0)
                        _enumType = MapToEnumType(_parser.MapSchemaTypesToCSharpTypes(typeName));

                }
            }

            private String MapToEnumType(String type)
            {
                String etype = null;
                if (type == "Byte")
                    etype = "byte";
                else if (type == "SByte")
                    etype = "sbyte";
                else if (type == "Int16")
                    etype = "short";
                else if (type == "UInt16")
                    etype = "ushort";
                else if (type == "Int32")
                    etype = "int";
                else if (type == "UInt32")
                    etype = "uint";
                else if (type == "Int64")
                    etype = "long";
                else if (type == "UInt64")
                    etype = "ulong";
                else
                    throw new SUDSParserException(
                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_InvalidEnumType"), type));

                return etype;
            }

            internal void AddFacet(SchemaFacet facet)
            {
                Util.Log("URTSimpleType.AddFacet");
                _facets.Add(facet);
            }

            internal override bool IsEmittableFieldType
            {
                get
                {
                    if (_fieldString == null)
                    {
                        if ((_bAnonymous == true) &&
                            (_facets.Count == 0) &&
                            (_encoding != null) &&
                            (_baseTypeName == "binary") &&
                            (_parser.MatchingSchemaStrings(_baseTypeXmlNS)))
                            _fieldString = "byte[]";
                        else
                            _fieldString = String.Empty;
                    }

                    return(_fieldString != String.Empty);
                }
            }
            internal override String FieldName
            {
                get { return(_fieldString);}
            }

            internal override String FieldNamespace
            {
                get { 
                    String schemaStr = null;
                    if (_parser._xsdVersion == XsdVersion.V1999)
                        schemaStr = s_schemaNamespaceString1999;
                    else if (_parser._xsdVersion == XsdVersion.V2000)
                        schemaStr = s_schemaNamespaceString2000;
                    else if (_parser._xsdVersion == XsdVersion.V2001)
                        schemaStr = s_schemaNamespaceString;
                    return schemaStr;
                }
            }

            internal override bool PrimitiveField
            {
                get { return(true);}
            }

            internal override String GetName(String curNS)
            {
                if ((_fieldString != null) && (_fieldString != String.Empty))
                    return(_fieldString);

                Util.Log("URTSimpleType.GetName curNS "+curNS+" return "+base.GetName(curNS));              
                return(base.GetName(curNS));
            }

            internal void PrintCSC(TextWriter textWriter, String indentation,
                                   String curNS, StringBuilder sb)
            {
                Util.Log("URTSimpleType.PrintCSC name "+Name+" curNS "+curNS);                              

                // Print only if the type is not an emittable field type
                if (IsEmittableFieldType == true)
                    return;

                if (bNestedType && !bNestedTypePrint)
                    return;

                // Handle encoding
                if (_encoding != null)
                {
                    // sb.Length = 0;
                    // sb.Append(indentation);
                }

                sb.Length = 0;
                sb.Append("\n"); 
                sb.Append(indentation);
                sb.Append("[");
                sb.Append("Serializable, ");
                sb.Append("SoapType(");

                if (_parser._xsdVersion == XsdVersion.V1999)
                {
                    sb.Append("SoapOptions=SoapOption.Option1|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }
                else if (_parser._xsdVersion == XsdVersion.V2000)
                {
                    sb.Append("SoapOptions=SoapOption.Option2|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }

                // Need namespace for clr type because proxy dll might have a different name than server dll
                sb.Append("XmlNamespace=");
                sb.Append(WsdlParser.IsValidUrl(UrlNS));
                sb.Append(", XmlTypeNamespace=");
                sb.Append(WsdlParser.IsValidUrl(UrlNS)); 

                sb.Append(")]");
                textWriter.WriteLine(sb);


                // Print type
                sb.Length = 0;
                sb.Append(indentation);

                // Handle Enum case
                if (IsEnum)
                    sb.Append("public enum ");
                else
                    sb.Append("public class ");

                if (_bNestedType)
                    sb.Append(WsdlParser.IsValidCS(NestedTypeName));
                else
                    sb.Append(WsdlParser.IsValidCS(Name));
                if (_baseType != null)
                {
                    sb.Append(" : ");
                    sb.Append(WsdlParser.IsValidCSAttr(_baseType.GetName(curNS)));
                }
                else if (IsEnum && _enumType != null && _enumType.Length > 0)
                {
                    sb.Append(" : ");
                    sb.Append(WsdlParser.IsValidCSAttr(_enumType));
                }

                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                for (int i=0;i<_facets.Count;i++)
                {
                    Util.Log("URTSimpleType.PrintCSC Invoke _facets PrintCSC ");                                                                                                        
                    ((SchemaFacet) _facets[i]).PrintCSC(textWriter, newIndentation, curNS, sb);
                }

                textWriter.Write(indentation);
                textWriter.WriteLine('}');
                return;
            }

            internal override MethodFlags GetMethodFlags(URTMethod method)
            {
                Debug.Assert(false, "GetMethodFlags called on a SimpleSchemaType");
                return(0);
            }

            private String _baseTypeName;
            private String _baseTypeXmlNS;
            private BaseType _baseType;
            private String _fieldString;
            private bool  _bEnum;
            private bool _bAnonymous;
            private String _encoding;
            private ArrayList _facets;
            private String _enumType;
            private WsdlParser _parser;
        }

        // Represents a complex type
        internal class URTComplexType : BaseType
        {
            internal URTComplexType(String name, String urlNS, String ns, String encodedNS,
                                    SchemaBlockType blockDefault, bool bSUDSType, bool bAnonymous, WsdlParser parser, URTNamespace xns)
            : base(name, urlNS, ns, encodedNS)
            {
                Util.Log("URTComplexType.URTComplexType name "+this.GetHashCode()+" "+name+" urlNS "+urlNS+" ns "+ns+" encodedNS "+encodedNS+" bSUDStype "+bSUDSType+" bAnonymous "+bAnonymous);
                _baseTypeName = null;
                _baseTypeXmlNS = null;
                _baseType = null;
                _connectURLs = null;
                _bStruct = !bSUDSType;
                _blockType = blockDefault;
                _bSUDSType = bSUDSType;
                _bAnonymous = bAnonymous;
                Debug.Assert(bAnonymous == false || _bSUDSType == false);
                _fieldString = null;
                _fields = new ArrayList();
                _methods = new ArrayList();
                _implIFaces = new ArrayList();
                _implIFaceNames = new ArrayList();
                _sudsType = SUDSType.None;              
                _parser = parser;

                int index = name.IndexOf('+');
                if (index > 0)
                {
                    // Nested type see if outer type has been added to namespace
                    String outerType = parser.Atomize(name.Substring(0,index));
                    URTComplexType cs = xns.LookupComplexType(outerType);
                    if (cs == null)
                    {
                        URTComplexType newCs = new URTComplexType(outerType, urlNS, ns, encodedNS, blockDefault, bSUDSType, bAnonymous, parser, xns);
                        Util.Log("URTComplexType.URTComplexType add outerType to namespace "+outerType+" nestedname "+name);
                        xns.AddComplexType(newCs);
                    }
                }


                if (xns.UrtType == UrtType.Interop)
                {
                    // Interop class names can have '.', replace these with '_', and set wire name to original type name.
                    index = name.LastIndexOf('.');
                    if (index > -1)
                    {
                        // class names can't have '.' so replace with '$'. Use xmlType attribute to send original name on wire.
                        _wireType = name;
                        Name = name.Replace(".", "_");
                        SearchName = name;
                    }
                }

            }

            internal void AddNestedType(BaseType ct)
            {
                if (_nestedTypes == null)
                    _nestedTypes = new ArrayList(10);

                _nestedTypes.Add(ct);
            }

            internal void Extends(String baseTypeName, String baseTypeNS)
            {
                Util.Log("URTComplexType.Extends baseTypeName "+baseTypeName+" baseTypeNS "+baseTypeNS);
                _baseTypeName = baseTypeName;
                _baseTypeXmlNS = baseTypeNS;
            }
            internal void Implements(String iFaceName, String iFaceNS, WsdlParser parser)
            {
                Util.Log("URTComplexType.Implements IFaceName "+iFaceName+" iFaceNS "+iFaceNS);
                _implIFaceNames.Add(iFaceName);
                _implIFaceNames.Add(iFaceNS);
                // Urt namespace will not have schema, they need to be recorded.
                URTNamespace parsingNamespace = parser.AddNewNamespace(iFaceNS);
                /*
                if (parsingNamespace == null)
                {
                    parsingNamespace = new URTNamespace(iFaceNS, parser);
                }
                */

                URTInterface parsingInterface = parsingNamespace.LookupInterface(iFaceName);            
                if (parsingInterface == null)
                {
                    parsingInterface = new URTInterface(iFaceName, parsingNamespace.Name, parsingNamespace.Namespace, parsingNamespace.EncodedNS, _parser);                    
                    parsingNamespace.AddInterface(parsingInterface);
                }
            }

            internal ArrayList ConnectURLs
            {
                set {
                    _connectURLs = value;
                }
            }
            internal bool IsStruct
            {
                set { _bStruct = value;}
            }
            internal bool IsSUDSType
            {
                get { return(_bSUDSType);}
                set {_bSUDSType = value; _bStruct = !value;}
            }
            internal SUDSType SUDSType
            {
                get { return(_sudsType);}
                set { _sudsType = value;}
            }
            internal SudsUse SudsUse
            {
                get { return(_sudsUse);}
                set { _sudsUse = value;}
            }

            internal bool IsValueType
            {
                set {_bValueType = value;}
            }

            internal SchemaBlockType BlockType
            {
                set { _blockType = value;}
            }

            internal String WireType
            {
                get { return(_wireType);}
            }

            internal ArrayList Inherit
            {
                get { return(_inherit);}
                set { _inherit = value;}
            }

            internal bool IsArray()
            {
                Util.Log("URTComplexType.IsArray "+this.GetHashCode()+" "+Name+" IsArray "+_arrayType);            
                if (_arrayType != null)
                    return true;
                else
                    return false;
            }
            internal String GetArray()
            {
                return _clrarray;
            }
            internal URTNamespace GetArrayNS()
            {
                return _arrayNS;
            }

            internal String GetClassName()
            {
                String cname = null;
                if (_bNameMethodConflict)
                    cname = "C"+Name; // Class name generated from a non-Suds wsdl and a method name and portType conflicted.
                else
                    cname = Name;
                return cname;
            }

            internal bool IsPrint
            {
                get {return _bprint;}
                set {_bprint = value;}
            }

            internal override bool IsEmittableFieldType
            {
                get
                {
                    Util.Log("URTComplexType.IsEmittableFieldType _fieldString "+_fieldString+" _bAnonymous "+_bAnonymous+" _fields.Count "+_fields.Count);
                    if (_fieldString == null)
                    {
                        if ((_bAnonymous == true) &&
                            (_fields.Count == 1))
                        {
                            URTField field = (URTField) _fields[0];
                            if (field.IsArray)
                            {
                                _fieldString = field.TypeName;
                                return(true);
                            }
                        }
                        _fieldString = String.Empty;
                    }

                    return(_fieldString != String.Empty);
                }
            }
            internal override String FieldName
            {
                get { return(_fieldString);}
            }
            internal override String FieldNamespace
            {
                get { return(((URTField) _fields[0]).TypeNS);}
            }
            internal override bool PrimitiveField
            {
                get { return((((URTField) _fields[0]).IsPrimitive));}
            }
            internal override String GetName(String curNS)
            {
                if ((_fieldString != null) && (_fieldString != String.Empty))
                    return(_fieldString);

                return(base.GetName(curNS));
            }
            internal ArrayList Fields
            {
                get { return _fields;}
            }

            internal void AddField(URTField field)
            {
                Util.Log("URTComplexType.AddField");
                _fields.Add(field);
            }
            internal void AddMethod(URTMethod method)
            {
                Util.Log("URTComplexType.AddMethod "+method);               
                if (method.Name == Name)
                {
                    // Type generated from an non-suds wsdl. Append the class name with a C when printing.
                    _bNameMethodConflict = true;
                }
                _methods.Add(method);
                int index = method.Name.IndexOf('.');
                if (index > 0)
                    method.MethodFlags = MethodFlags.None; //interface qualfied method names have no method modifier
                else
                    method.MethodFlags = method.MethodFlags |= MethodFlags.Public; // method names are public for this version of wsdl

            }
            private URTMethod GetMethod(String name)
            {
                Util.Log("URTComplexType.GetMethod "+name+" count "+_methods.Count+" Name "+Name+" base ns "+_baseTypeXmlNS+" base name "+_baseTypeName);                             
                for (int i=0;i<_methods.Count;i++)
                {
                    URTMethod method = (URTMethod) _methods[i];
                    Util.Log("URTComplexType.GetMethod interate "+method.Name);                             
                    if (method.Name == name)
                        return(method);
                }

                return(null);
            }
            internal void ResolveTypes(WsdlParser parser)
            {
                Util.Log("URTComplexType.ResolveTypes "+Name+" _baseTypeName "+_baseTypeName+" IsSUDSType "+IsSUDSType);
                String baseTypeNS = null;
                String baseTypeAssemName = null;
                if (IsArray())
                {
                    ResolveArray();
                    return;
                }

                if (IsSUDSType)
                {
                    // BaseType == null;
                    if (_sudsType == SUDSType.None)
                    {
                        if (_parser._bWrappedProxy)
                            _sudsType = SUDSType.ClientProxy;
                        else
                            _sudsType = SUDSType.MarshalByRef;
                    }
                }

                if (_baseTypeName != null)
                {
                    Util.Log("URTComplexType.ResolveTypes 1 ");
                    UrtType urtType = parser.IsURTExportedType(_baseTypeXmlNS, out baseTypeNS, out baseTypeAssemName);
                    if (urtType == UrtType.UrtSystem || baseTypeNS.StartsWith("System", StringComparison.Ordinal))
                    {
                        _baseType = new SystemType(_baseTypeName, _baseTypeXmlNS, baseTypeNS, baseTypeAssemName);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(_baseTypeXmlNS);
                        if (ns == null)
                        {
                            throw new SUDSParserException(
                                                         String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                                                       _baseTypeXmlNS, _baseTypeName));
                        }
                        _baseType = ns.LookupComplexType(_baseTypeName);
                        if (_baseType == null)
                        {
                            _baseType = new SystemType(_baseTypeName, _baseTypeXmlNS, baseTypeNS, baseTypeAssemName);

                            /*
                            throw new SUDSParserException(
                                                         String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                                       _baseTypeName, _baseTypeXmlNS));
                                                                       */
                        }
                    }
                }
                // Top of inheritance hiearchy for a Wrapped proxy is RemotingClientProxy
                if (IsSUDSType)
                {
                    if (_parser._bWrappedProxy)
                    {

                        if (_baseTypeName == null || _baseType is SystemType)
                        {
                            _baseTypeName = "RemotingClientProxy";
                            //<
                            _baseTypeXmlNS = SoapServices.CodeXmlNamespaceForClrTypeNamespace("System.Runtime.Remoting","System.Runtime.Remoting");
                            baseTypeNS = "System.Runtime.Remoting.Services";
                            baseTypeAssemName = "System.Runtime.Remoting";
                            _baseType = new SystemType(_baseTypeName, _baseTypeXmlNS, baseTypeNS, baseTypeAssemName);
                        }
                    }
                    else if (_baseTypeName == null)
                    {                       
                        _baseTypeName = "MarshalByRefObject";
                        //<
                        _baseTypeXmlNS = SoapServices.CodeXmlNamespaceForClrTypeNamespace("System", null);
                        baseTypeNS = "System";
                        baseTypeAssemName = null;
                        _baseType = new SystemType(_baseTypeName, _baseTypeXmlNS, baseTypeNS, baseTypeAssemName);                        
                    }                   
                }
                else if (_baseType == null)
                {
                    Util.Log("URTComplexType.ResolveTypes 5 ");                                         
                    _baseType = new SystemType("Object", SoapServices.CodeXmlNamespaceForClrTypeNamespace("System", null), "System", null);
                }
                for (int i=0;i<_implIFaceNames.Count;i=i+2)
                {
                    String implIFaceName = (String) _implIFaceNames[i];
                    String implIFaceXmlNS = (String) _implIFaceNames[i+1];
                    String implIFaceNS, implIFaceAssemName;
                    BaseInterface iFace;


                    UrtType iType = parser.IsURTExportedType(implIFaceXmlNS, out implIFaceNS,
                                                             out implIFaceAssemName);

                    if (iType == UrtType.UrtSystem)// && implIFaceNS.StartsWith("System", StringComparison.Ordinal))
                    {
                        iFace = new SystemInterface(implIFaceName, implIFaceXmlNS, implIFaceNS, parser, implIFaceAssemName);
                    }
                    else
                    {
                        URTNamespace ns = parser.LookupNamespace(implIFaceXmlNS);
                        if (ns == null)
                        {
                            throw new SUDSParserException(
                                                         String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                                                       implIFaceXmlNS, implIFaceName));
                        }
                        iFace = ns.LookupInterface(implIFaceName);
                        if (iFace == null)
                        {
                            throw new SUDSParserException(
                                                         String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                                       implIFaceName, implIFaceXmlNS));
                        }
                    }
                    _implIFaces.Add(iFace);
                }
                for (int i=0;i<_methods.Count;i++)
                    ((URTMethod) _methods[i]).ResolveTypes(parser);
            }
            internal void ResolveMethods()
            {
                Util.Log("URTComplexType.ResolveMethods "+Name);                
                for (int i=0;i<_methods.Count;i++)
                {
                    URTMethod method = (URTMethod) _methods[i];
                    /*
                    if (method.MethodFlags == 0)
                        method.MethodFlags = _baseType.GetMethodFlags(method);
                        */
                }

                return;
            }
            internal override MethodFlags GetMethodFlags(URTMethod method)
            {
                /*
                Debug.Assert(method.MethodFlags == 0, "Method has already been considered");

                MethodFlags methodFlags = _baseType.GetMethodFlags(method);
                for (int i=0;i<_methods.Count;i++)
                {
                    URTMethod thisMethod = (URTMethod) _methods[i];
                    if (thisMethod.Equals(method))
                        thisMethod.MethodFlags = method.MethodFlags;
                }
                */

                return(method.MethodFlags);
            }

            internal void PrintCSC(TextWriter textWriter, String indentation, String curNS, StringBuilder sb)
            {
                Util.Log("URTComplexType.PrintCSC enter name "+Name+" curNS "+curNS+" sudsUse "+((Enum)_sudsUse).ToString()+" bNestedType "+bNestedType+" bNestedTypePrint "+bNestedTypePrint);
                // Print only if the type is not an emittable field type
                if (IsEmittableFieldType == true)
                    return;

                if (bNestedType && !bNestedTypePrint)
                    return;             

                // Handle delegate case
                sb.Length = 0;
                sb.Append(indentation);
                if (_baseTypeName != null)
                {
                    String baseName = _baseType.GetName(curNS);
                    if ((baseName == "System.Delegate") || (baseName == "System.MulticastDelegate"))
                    {
                        sb.Append("public delegate ");
                        URTMethod invokeMethod = GetMethod("Invoke");
                        if (invokeMethod == null)
                        {
                            throw new SUDSParserException(
                                                         CoreChannel.GetResourceString("Remoting_Suds_DelegateWithoutInvoke"));
                        }
                        String typeString = invokeMethod.GetTypeString(curNS, true);
                        sb.Append(WsdlParser.IsValidCSAttr(typeString));
                        sb.Append(' ');

                        String printName = Name;
                        int index = printName.IndexOf('.');
                        if (index > 0)
                        {
                            printName = printName.Substring(index+1);
                        }
                        sb.Append(WsdlParser.IsValidCS(printName));
                        sb.Append('(');
                        invokeMethod.PrintSignature(sb, curNS);
                        sb.Append(");");
                        textWriter.WriteLine(sb);
                        return;
                    }
                }

                bool bURTType = IsURTType;


                sb.Length = 0;
                sb.Append("\n"); 
                sb.Append(indentation);
                sb.Append("[");
                if (_sudsType != SUDSType.ClientProxy)
                    sb.Append("Serializable, ");
                sb.Append("SoapType(");

                if (_parser._xsdVersion == XsdVersion.V1999)
                {
                    sb.Append("SoapOptions=SoapOption.Option1|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }
                else if (_parser._xsdVersion == XsdVersion.V2000)
                {
                    sb.Append("SoapOptions=SoapOption.Option2|SoapOption.AlwaysIncludeTypes|SoapOption.XsdString|SoapOption.EmbedAll,");
                }

                if (!bURTType)
                {
                    sb.Append("XmlElementName=");
                    sb.Append(WsdlParser.IsValidUrl(GetClassName()));
                    sb.Append(", XmlNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(Namespace));
                    sb.Append(", XmlTypeName=");
                    if (WireType != null)
                        sb.Append(WsdlParser.IsValidUrl(WireType));
                    else
                        sb.Append(WsdlParser.IsValidUrl(GetClassName()));
                    sb.Append(", XmlTypeNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(Namespace));
                }
                else
                {
                    // Need namespace for clr type because proxy dll might have a different name than server dll
                    sb.Append("XmlNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(UrlNS));
                    sb.Append(", XmlTypeNamespace=");
                    sb.Append(WsdlParser.IsValidUrl(UrlNS)); 
                    if (WireType != null)
                    {
                        sb.Append(", XmlTypeName=");
                        sb.Append(WsdlParser.IsValidUrl(WireType));
                    }
                }

                sb.Append(")]");
                sb.Append("[ComVisible(true)]");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indentation);

                if (_sudsUse == SudsUse.Struct)
                    sb.Append("public struct ");
                else
                    sb.Append("public class ");
                if (_bNestedType)
                    sb.Append(WsdlParser.IsValidCS(NestedTypeName));
                else
                    sb.Append(WsdlParser.IsValidCS(GetClassName()));

                if (_baseTypeName != null || _sudsUse == SudsUse.ISerializable || _implIFaces.Count > 0)
                    sb.Append(" : ");
                bool bBaseIsURTType = true;
                String baseString = null;

                bool binherit = false;
                bool fClientProxy;
                if (_baseTypeName == "RemotingClientProxy")
                    fClientProxy = true;
                else
                    fClientProxy = false;

                if (fClientProxy)
                {
                    sb.Append("System.Runtime.Remoting.Services.RemotingClientProxy");
                    binherit = true;
                }
                else if (_baseTypeName != null)
                {
                    bBaseIsURTType = _baseType.IsURTType;
                    baseString = _baseType.GetName(curNS);
                    if (baseString == "System.__ComObject")
                    {
                        /*textWriter.Write(indentation);
                        textWriter.WriteLine("[guid(\"cc3bf020-1881-4e44-88d8-39b1052b1b11\")]");
                        textWriter.Write(indentation);
                        textWriter.WriteLine("[comimport]"); */
                        sb.Append("System.MarshalByRefObject");
                        binherit = true;
                    }
                    else
                    {
                        sb.Append(WsdlParser.IsValidCSAttr(baseString));
                        binherit = true;
                    }
                }
                else
                {
                    // no base name
                    if (_sudsUse == SudsUse.ISerializable)
                    {
                        sb.Append("System.Runtime.Serialization.ISerializable");
                        binherit = true;
                    }
                }

                if (_implIFaces.Count > 0)
                {
                    for (int i=0;i<_implIFaces.Count;i++)
                    {
                        if (binherit)
                            sb.Append(", ");
                        sb.Append(WsdlParser.IsValidCS(((BaseInterface) _implIFaces[i]).GetName(curNS)));
                        binherit = true;
                    }
                }


                textWriter.WriteLine(sb);

                textWriter.Write(indentation);
                textWriter.WriteLine('{');

                String newIndentation = indentation + "    ";
                int newIndentationLength = newIndentation.Length;
                //bool fClientProxy = _connectURL != null;

                Util.Log("URTComplexType.PrintCSC _sudsType "+((Enum)_sudsType).ToString());

                if (fClientProxy)
                {
                    PrintClientProxy(textWriter, indentation, curNS, sb);
                }

                if (_methods.Count > 0)
                {
                    //textWriter.Write(newIndentation);
                    //textWriter.WriteLine("// Class Methods");
                    String bodyPrefix = null;

                    if (_parser._bWrappedProxy)
                    {
                        sb.Length = 0;
                        sb.Append("((");
                        sb.Append(WsdlParser.IsValidCS(GetClassName()));
                        sb.Append(") _tp).");
                        bodyPrefix = sb.ToString();
                    }

                    for (int i=0;i<_methods.Count;i++)
                    {
                        Util.Log("URTComplexType.PrintCSC Invoke methods class methods PrintCSC class "+Name+" methodName "+((URTMethod) _methods[i]).Name);
                        ((URTMethod) _methods[i]).PrintCSC(textWriter, newIndentation,
                                                           " ", curNS, MethodPrintEnum.PrintBody, bURTType,
                                                           bodyPrefix, sb); 
                    }
                    textWriter.WriteLine();
                }


                /*
                if (_implIFaces.Count > 0)
                {
                    ArrayList printedIFaces = new ArrayList(_implIFaces.Count);
                    for (int i=0;i<_implIFaces.Count;i++)
                        ((BaseInterface) _implIFaces[i]).PrintClassMethods(textWriter, newIndentation, curNS, printedIFaces, fClientProxy, sb);
                    textWriter.WriteLine();
                }
                */

                // Standard class
                if (_fields.Count > 0)
                {
                    textWriter.Write(newIndentation);
                    textWriter.WriteLine("// Class Fields");
                    for (int i=0;i<_fields.Count;i++)
                    {
                        Util.Log("URTComplexType.PrintCS Invoke _fields PrintCSC");                                                                                                         
                        ((URTField) _fields[i]).PrintCSC(textWriter, newIndentation, curNS, sb);
                    }
                }

                // print nested types
                if (_nestedTypes != null && _nestedTypes.Count > 0)
                {
                    foreach (BaseType ctype in _nestedTypes)
                    {
                        ctype.bNestedTypePrint = true;
                        if (ctype is URTSimpleType)
                            ((URTSimpleType)ctype).PrintCSC(textWriter, newIndentation, curNS, sb);
                        else
                            ((URTComplexType)ctype).PrintCSC(textWriter, newIndentation, curNS, sb);

                        ctype.bNestedTypePrint = false;
                    }
                }

                if (_sudsUse == SudsUse.ISerializable)
                    PrintISerializable(textWriter, indentation, curNS, sb, baseString);

                // Close class
                sb.Length = 0;
                sb.Append(indentation);
                sb.Append("}");
                textWriter.WriteLine(sb);
                Util.Log("URTComplexType.PrintCSC Exit name "+Name+" curNS "+curNS);
                return;
            }

            private void PrintClientProxy(TextWriter textWriter, String indentation, String curNS, StringBuilder sb)
            {
                Util.Log("URTComplexType.PrintCSC PrintClientProxy ");
                String indent1 = indentation + "    ";
                String indent2 = indent1 + "    ";
                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("// Constructor");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("public ");
                sb.Append(WsdlParser.IsValidCS(GetClassName()));
                sb.Append("()");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append('{');
                textWriter.WriteLine(sb);

                if (_connectURLs != null)
                {
                    for (int i=0; i<_connectURLs.Count; i++)
                    {
                        sb.Length = 0;
                        sb.Append(indent2);
                        if (i == 0)
                        {
                            sb.Append("base.ConfigureProxy(this.GetType(), ");
                            sb.Append(WsdlParser.IsValidUrl((string)_connectURLs[i]));
                            sb.Append(");");
                        }
                        else
                        {
                            // Only the first location is used, the rest are commented out in the proxy
                            sb.Append("//base.ConfigureProxy(this.GetType(), ");
                            sb.Append(WsdlParser.IsValidUrl((string)_connectURLs[i]));
                            sb.Append(");");
                        }
                        textWriter.WriteLine(sb);
                    }
                }



                //Preload classes
                foreach (URTNamespace ns in _parser._URTNamespaces)
                {
                    foreach (URTComplexType cs in ns._URTComplexTypes)
                    {
                        if ((cs._sudsType != SUDSType.ClientProxy) && !cs.IsArray())
                        {
                            sb.Length = 0;
                            sb.Append(indent2);
                            sb.Append("System.Runtime.Remoting.SoapServices.PreLoad(typeof(");
                            sb.Append(WsdlParser.IsValidCS(ns.EncodedNS));
                            if (ns.EncodedNS != null && ns.EncodedNS.Length > 0)
                                sb.Append(".");
                            sb.Append(WsdlParser.IsValidCS(cs.Name));
                            sb.Append("));");
                            textWriter.WriteLine(sb);
                        }
                    }
                }

                foreach (URTNamespace ns in _parser._URTNamespaces)
                {
                    foreach (URTSimpleType ss in ns._URTSimpleTypes)
                    {
                        if (ss.IsEnum)
                        {
                            sb.Length = 0;
                            sb.Append(indent2);
                            sb.Append("System.Runtime.Remoting.SoapServices.PreLoad(typeof(");
                            sb.Append(WsdlParser.IsValidCS(ns.EncodedNS));
                            if (ns.EncodedNS != null && ns.EncodedNS.Length > 0)
                                sb.Append(".");
                            sb.Append(WsdlParser.IsValidCS(ss.Name));
                            sb.Append("));");
                            textWriter.WriteLine(sb);
                        }
                    }
                }

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append('}');
                textWriter.WriteLine(sb);
                
                // Base class for Client Proxy
                // Write Property to retrieve Transparent Proxy

                textWriter.WriteLine();

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("public Object RemotingReference");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("{");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent2);
                sb.Append("get{return(_tp);}");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("}");
                textWriter.WriteLine(sb);               

                textWriter.WriteLine();
            }

            private void PrintISerializable(TextWriter textWriter, String indentation, String curNS, StringBuilder sb, String baseString)
            {
                Util.Log("URTComplexType.PrintCSC PrintISerializable ");
                String indent1 = indentation + "    ";
                String indent2 = indent1 + "    ";

                if (baseString == null || baseString.StartsWith("System.", StringComparison.Ordinal))
                {
                    // Don't generate if base class already contains field
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("public System.Runtime.Serialization.SerializationInfo info;");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("public System.Runtime.Serialization.StreamingContext context; \n");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indent1);

                if (_baseTypeName == null)
                    sb.Append("public ");
                else
                    sb.Append("protected ");

                if (_bNestedType)
                    sb.Append(WsdlParser.IsValidCS(NestedTypeName));
                else
                    sb.Append(WsdlParser.IsValidCS(GetClassName()));

                sb.Append("(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)");

                if (_baseTypeName != null)
                    sb.Append(" : base(info, context)");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("{");
                textWriter.WriteLine(sb);

                if (baseString == null || baseString.StartsWith("System.", StringComparison.Ordinal))
                {
                    // Don't generate if base class already contains field
                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("this.info = info;");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("this.context = context;");
                    textWriter.WriteLine(sb);
                }


                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("}");
                textWriter.WriteLine(sb);

                if (_baseTypeName == null)
                {
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("public void GetObjectData(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("{");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("}");
                    textWriter.WriteLine(sb);
                }


            }

            internal void AddArray(String arrayType, URTNamespace arrayNS)
            {
                Util.Log("URTComplexType.ResolveArray Entry "+this.GetHashCode()+" "+_arrayType+" ns "+_arrayNS);
                _arrayType = arrayType;

                /*
                int index = arrayType.IndexOf('+');
                if (index > 0)
                {
                    // Nested Type
                    String outerTypeName = arrayType.Substring(0, index);
                    String nestedTypeName = arrayType.Substring(index+1);
                    _arrayType = outerTypeName+"."+nestedTypeName;
                }
                */
                _arrayNS = arrayNS;
                Util.Log("URTComplexType.AddArray "+arrayType+" ns "+(arrayNS == null?"null":arrayNS.Namespace)+" finalName "+_arrayType);
            }

            internal void ResolveArray()
            {
                Util.Log("URTComplexType.ResolveArray Entry "+this.GetHashCode()+" "+_arrayType+" ns "+_arrayNS);

                if (_clrarray != null)
                    return;

                String actualElementType = null;
                String wireElementType = _arrayType;
                int index = _arrayType.IndexOf("[");
                if (index < 0)
                {
                    throw new SUDSParserException(
                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_WsdlInvalidArraySyntax"),
                                                               _arrayType));
                }

                wireElementType = _arrayType.Substring(0, index);

                switch (_arrayNS.UrtType)
                {
                case UrtType.Xsd:
                    actualElementType = _parser.MapSchemaTypesToCSharpTypes(wireElementType);
                    break;
                case UrtType.UrtSystem:
                case UrtType.UrtUser:
                    actualElementType = wireElementType;
                    break;
                case UrtType.Interop:
                    actualElementType = wireElementType;
                    break;
                default:
                    Debug.Assert(false, "!UrtType.None");
                    break;
                }

                _clrarray = actualElementType+FilterDimensions(_arrayType.Substring(index));
                Util.Log("URTComplexType.ResolveArray Exit "+_clrarray+" "+_arrayNS+" elementname "+actualElementType);
            }

            // Need to remove dimensions in an array (some wsdl's generate fixed arrays with dimensions)
            private String FilterDimensions(String value)
            {
                Char[] outChar = new Char[value.Length];
                for (int i=0; i<value.Length; i++)
                {
                    if (Char.IsDigit(value[i]))
                        outChar[i] = ' ';
                    else
                        outChar[i] = value[i];
                }
                return new String(outChar);
            }


            private String _baseTypeName;
            private String _baseTypeXmlNS;
            private BaseType _baseType;
            private ArrayList _connectURLs;
            private bool _bStruct;
            private SchemaBlockType _blockType;
            private bool _bSUDSType;
            private bool _bAnonymous;
            private String _wireType;
            private ArrayList _inherit; // proxy hierarchy 
            private String _fieldString;
            private ArrayList _implIFaceNames;
            private ArrayList _implIFaces;
            private ArrayList _fields;
            private ArrayList _methods;
            private SUDSType _sudsType;
            private SudsUse _sudsUse;
            private bool _bValueType;
            private WsdlParser _parser;
            private String _arrayType;
            private URTNamespace _arrayNS;
            private String _clrarray;
            private bool _bprint = true;
            private bool _bNameMethodConflict = false;

        }

        // Represents an XML element declaration
        internal class ElementDecl
        {
            // Constructor
            internal ElementDecl(String elmName, String elmNS, String typeName, String typeNS,
                                 bool bPrimitive){
                Util.Log("ElementDecl.ElementDecl elmName "+elmName+" elmNS "+elmNS+" typeName "+typeName+" typeNS "+typeNS+" bPrimitive "+bPrimitive);
                _elmName = elmName;
                _elmNS = elmNS;
                _typeName = typeName;
                _typeNS = typeNS;
                _bPrimitive = bPrimitive;
            }

            // Field accessors
            internal String Name{
                get{ return(_elmName);}
            }
            internal String Namespace{
                get{ return(_elmNS);}
            }
            internal String TypeName{
                get{ return(_typeName);}
            }
            internal String TypeNS{
                get{ return(_typeNS);}
            }

            internal bool Resolve(WsdlParser parser){
                Util.Log("ElementDecl.Resolve "+TypeName+" "+TypeNS);
                // Return immediately for element declaration of primitive types
                if (_bPrimitive)
                    return true;

                // Lookup the type from the element declaration
                URTNamespace typeNS = parser.LookupNamespace(TypeNS);
                if (typeNS == null)
                {
                    throw new SUDSParserException(
                                                 String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Suds_CantResolveSchemaNS"),
                                                               TypeNS, TypeName));
                }
                BaseType type = typeNS.LookupType(TypeName);
                if (type == null)
                {
                    // Because there might be multiple bindings and some not soap, there can be failures in the non soap bindings.
                    return false; 
                    /*
                    throw new SUDSParserException(
                                                 String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveTypeInNS"),
                                                               TypeName, TypeNS));
                                                               */
                }

                type.ElementName = Name;
                type.ElementNS = Namespace;

                return true;
            }

            // Fields
            private String _elmName;
            private String _elmNS;
            private String _typeName;
            private String _typeNS;
            private bool _bPrimitive;
        }

        // Represents a namespace
        internal class URTNamespace
        {
            // Constructor
            internal URTNamespace(String name, WsdlParser parser)
            {
                Util.Log("URTNamespace.URTNamespace Enter name "+name);
                _name = name;
                _parser = parser;
                _nsType = parser.IsURTExportedType(name, out _namespace, out _assemName);
                if (_nsType == UrtType.Interop)
                {
                    _encodedNS = EncodeInterop(_namespace, parser);
                }
                else
                    _encodedNS = _namespace;
                _elmDecls = new ArrayList();
                _URTComplexTypes = new ArrayList();
                _numURTComplexTypes = 0;
                _URTSimpleTypes = new ArrayList();
                _numURTSimpleTypes = 0;
                _URTInterfaces = new ArrayList();
                _anonymousSeqNum = 0;
                parser.AddNamespace(this);
                Util.Log("URTNamespace.URTNamespace Exit name "+this.GetHashCode()+" "+name+" _namespace "+_namespace+" _assemName "+_assemName+" _encodedNS "+_encodedNS);
            }

            internal static String EncodeInterop(String name, WsdlParser parser)
            {
                String _encodedNS = name;
                if ((parser.ProxyNamespace != null) && parser.ProxyNamespace.Length > 0)
                {
                    String countStr = "";
                    if (parser.ProxyNamespaceCount > 0)
                        countStr = (parser.ProxyNamespaceCount).ToString(CultureInfo.InvariantCulture);
                    parser.ProxyNamespaceCount++;
                    _encodedNS = parser.ProxyNamespace+countStr;
                }
                else
                {
                    int index = name.IndexOf(":");
                    if (index > 0)
                        _encodedNS = _encodedNS.Substring(index+1);
                    if (_encodedNS.StartsWith("//", StringComparison.Ordinal))
                        _encodedNS = _encodedNS.Substring(2);
                    _encodedNS = _encodedNS.Replace('/', '_');
                }
                Util.Log("URTNamespace.EncodeInterop  encoded "+_encodedNS);
                return _encodedNS;
            }


            // Get the next anonymous type name
            internal String GetNextAnonymousName(){
                ++_anonymousSeqNum;
                Util.Log("URTNamespace.GetNextAnonymousName AnonymousType"+_anonymousSeqNum+" ComplexType "+_name);
                return("AnonymousType" + _anonymousSeqNum);
            }

            // Add a new element declaration to the namespace
            internal void AddElementDecl(ElementDecl elmDecl){
                Util.Log("URTNamespace.AddElementDecl ");
                _elmDecls.Add(elmDecl);
            }

            // Add a new type into the namespace
            internal void AddComplexType(URTComplexType type){
                Util.Log("URTNamespace.AddComplexType "+this.GetHashCode()+" "+type.Name+" "+type.GetHashCode()+" ns "+_name);
                // Assert that simple and complex types share the same namespace
                Debug.Assert(LookupSimpleType(type.Name) == null,
                             "Complex type has the same name as a simple type");
                _URTComplexTypes.Add(type);
                ++_numURTComplexTypes;
            }

            // Add a new type into the namespace
            internal void AddSimpleType(URTSimpleType type){
                Util.Log("URTNamespace.AddSimpleType "+type.Name);              
                // Assert that simple and complex types share the same namespace
                Debug.Assert(LookupComplexType(type.Name) == null,
                             "Simple type has the same name as a complex type");
                _URTSimpleTypes.Add(type);
                ++_numURTSimpleTypes;
            }

            // Adds a new interface into the namespace
            internal void AddInterface(URTInterface iface){
                Util.Log("URTNamespace.AddInterface "+iface.Name);              
                _URTInterfaces.Add(iface);
            }

            // Returns the namespace
            internal String Namespace{
                get{ return(_namespace);}
            }
            
            internal bool IsSystem
            {
                get
                { 
                    if (_namespace != null && _namespace.StartsWith("System", StringComparison.Ordinal))
                        return true;
                    else
                        return false;
                }
            }
            
            // Returns Encoded namespace
            internal String EncodedNS{
                get{ return(_encodedNS);}
                set{ _encodedNS = value;}
            }

            internal bool bReferenced{
                get{ return(_bReferenced);}
                set{ _bReferenced = value;}
            }
            
            // Returns the full name
            internal String Name{
                get{ return(_name);}
            }

            // Returns Assembly name
            internal String AssemName{
                get{ return(_assemName);}
            }

            internal UrtType UrtType{
                get{ return _nsType;}
            }

            // Returns true if this represents a URTNamespace
            internal bool IsURTNamespace{
                get{ return((Object) _namespace == (Object) _encodedNS);}
            }

            // Returns true if the namespace has no types defined
            internal bool IsEmpty{
                get{
                    bool isEmpty = true;
                    if (ComplexTypeOnlyArrayorEmpty() &&
                        (_URTInterfaces.Count == 0) &&
                        (_numURTSimpleTypes == 0))
                        isEmpty = true;
                    else
                        isEmpty = false;
                    return isEmpty;
                }
            }

            internal bool ComplexTypeOnlyArrayorEmpty()
            {
                bool bempty = true;
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                    if (type != null && !type.IsArray())
                    {
                        bempty = false;
                        break;
                    }
                }
                return bempty;
            }

            // Looks up a complex type
            internal URTComplexType LookupComplexType(String typeName){
                URTComplexType ctype = null;
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                    //if (type != null && type.SearchName == typeName)
                    //Util.Log("URTNamespace.LookupComplexType "+this.GetHashCode()+" "+typeName+" "+type.SearchName);
                    if ((type != null) && WsdlParser.MatchingStrings(type.SearchName, typeName))
                    {
                        //Util.Log("URTNamespace.LookupComplexType found");                    
                        ctype = type;
                        break;
                    }
                }

                Util.Log("URTNamespace.LookupComplexType "+typeName+" ns "+this.GetHashCode()+" "+_name+" return "+((ctype != null) ? ctype.Name:"null"));               
                return(ctype);
            }

            // Looks up a complex type
            internal URTComplexType LookupComplexTypeEqual(String typeName){
                URTComplexType ctype = null;
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                    if (type != null && type.SearchName == typeName)
                    {
                        ctype = type;
                        break;
                    }
                }

                Util.Log("URTNamespace.LookupComplexTypeEqual "+typeName+" ns "+this.GetHashCode()+" "+_name+" return "+((ctype != null) ? ctype.Name:"null"));               
                return(ctype);
            }


            // Looks up a simple type
            internal URTSimpleType LookupSimpleType(String typeName){
                Util.Log("URTNamespace.LookupSimpleType "+typeName+" ns "+_name);                                
                for (int i=0;i<_URTSimpleTypes.Count;i++)
                {
                    URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                    if ((type != null) && WsdlParser.MatchingStrings(type.Name, typeName))
                        return(type);
                }

                return(null);
            }

            // Looks up a complex or simple type
            internal BaseType LookupType(String typeName){
                BaseType type = LookupComplexType(typeName);
                if (type == null)
                    type = LookupSimpleType(typeName);

                return(type);
            }

            // Removes the given type from the namespace
            internal void RemoveComplexType(URTComplexType type){
                Util.Log("URTNamespace.RemoveComplexType "+type.Name+" complex Type "+_name);
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    Util.Log("URTNamespace.RemoveComplexType 1 "+type.Name+" complexTypes "+((_URTComplexTypes[i] != null) ? ((URTComplexType)_URTComplexTypes[i]).Name : "Null"));                 
                    if (_URTComplexTypes[i] == type)
                    {
                        Util.Log("URTNamespace.RemoveComplexType 2 match "+type.Name+" complexTypes "+((_URTComplexTypes[i] != null) ? ((URTComplexType)_URTComplexTypes[i]).Name : "Null"));                                       
                        _URTComplexTypes[i] = null;
                        --_numURTComplexTypes;
                        return;
                    }
                }

                throw new SUDSParserException(
                                             CoreChannel.GetResourceString("Remoting_Suds_TriedToRemoveNonexistentType"));
            }

            // Removes the given type from the namespace
            internal void RemoveSimpleType(URTSimpleType type){
                Util.Log("URTNamespace.RemoveSimpleType "+type.Name+" SimpleType "+_name);              
                for (int i=0;i<_URTSimpleTypes.Count;i++)
                {
                    if (_URTSimpleTypes[i] == type)
                    {
                        _URTSimpleTypes[i] = null;
                        --_numURTSimpleTypes;
                        return;
                    }
                }

                throw new SUDSParserException(
                                             CoreChannel.GetResourceString("Remoting_Suds_TriedToRemoveNonexistentType"));
            }

            // Looks up an interface
            internal URTInterface LookupInterface(String iFaceName){
                Util.Log("URTNamespace.LookupInterface "+iFaceName);
                for (int i=0;i<_URTInterfaces.Count;i++)
                {
                    URTInterface iFace = (URTInterface) _URTInterfaces[i];
                    if (WsdlParser.MatchingStrings(iFace.Name, iFaceName))
                        return(iFace);
                }

                return(null);
            }

            // Resolve element references
            internal void ResolveElements(WsdlParser parser){
                Util.Log("URTNamespace.ResolveElements "+Name);

                for (int i=0;i<_elmDecls.Count;i++)
                {
                    ((ElementDecl) _elmDecls[i]).Resolve(parser);
                }
            }

            // Resolves internal references
            internal void ResolveTypes(WsdlParser parser){
                Util.Log("URTNamespace.ResolveTypes "+Name); 

                /*
                // Process nested types 
                Hashtable typeTable = new Hashtable(10);
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    // fill up hashtable with types
                    URTComplexType ctype = (URTComplexType)_URTComplexTypes[i];

                    if (ctype.bNestedType)
                    {
                        Util.Log("URTNamespace.ResolveTypes nested type outer type "+ctype.OuterTypeName+" nested type "+ctype.NestedTypeName+" fullNestedTypeName "+ctype.Name);
                        // type is a nested class 
                        URTComplexType outerType = (URTComplexType)typeTable[ctype.OuterTypeName];
                        if (outerType == null)
                        {
                            // place all the URTComplex types in the table, the nested type appears before the outer type in Wsdl
                            for (int j=0;j<_URTComplexTypes.Count;j++)
                            {
                                URTComplexType ctype2 = (URTComplexType)_URTComplexTypes[j];
                                Util.Log("URTNamespace.ResolveTypes miss find all types "+ctype2.Name);
                                typeTable[ctype2.Name] = ctype2;
                            }
                        }
                        outerType = (URTComplexType)typeTable[ctype.OuterTypeName];
                        if (outerType == null)
                        {
                            throw new SUDSParserException(
                                                 String.Format(CoreChannel.GetResourceString("Remoting_Suds_CantResolveNestedType"),
                                                               ctype.Name, ctype.UrlNS));
                        }
                        outerType._nestedTypes.Add(ctype);

                    }
                    else
                    {

                        Util.Log("URTNamespace.ResolveTypes nested place in table "+ctype.Name);
                        typeTable[ctype.Name] = ctype;
                    }
                }
                */


                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    if (_URTComplexTypes[i] != null)
                        ((URTComplexType)_URTComplexTypes[i]).ResolveTypes(parser);
                }

                for (int i=0;i<_URTInterfaces.Count;i++)
                    ((URTInterface)_URTInterfaces[i]).ResolveTypes(parser);
            }

            // Resolves method types
            internal void ResolveMethods(){
                Util.Log("URTNamespace.ResolveMethods "+Name);              
                for (int i=0;i<_URTComplexTypes.Count;i++)
                {
                    if (_URTComplexTypes[i] != null)
                        ((URTComplexType)_URTComplexTypes[i]).ResolveMethods();
                }
            }

            // Prints all the types in the namespace
            internal void PrintCSC(WriterStream writerStream)
            {
                Util.Log("URTNamespace.PrintCSC Entry "+Name);               
                Debug.Assert(!IsEmpty, "Empty namespace " + Name + " being printed");
                TextWriter textWriter = writerStream.OutputStream;
                // Check to see if there is anything to print for this namespace
                bool bprint = false;
                if (_numURTComplexTypes > 0)
                {
                    for (int i=0;i<_URTComplexTypes.Count;i++)
                    {
                        URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                        if (type != null && type.IsPrint)
                            bprint = true;
                    }
                }

                if (_numURTSimpleTypes > 0)
                {
                    for (int i=0;i<_URTSimpleTypes.Count;i++)
                    {
                        URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                        if (type != null)
                            bprint = true;
                    }
                }

                if (_URTInterfaces.Count > 0)
                    bprint = true;


                if (!bprint)
                    return;
                // End of check to see if there is anything to print


                String indentation = String.Empty;

                Stream stream = ((StreamWriter)textWriter).BaseStream;
                if (!writerStream.GetWrittenTo())
                {
                    // Only print when new output file is used
                    textWriter.WriteLine("using System;");
                    textWriter.WriteLine("using System.Runtime.Remoting.Messaging;");
                    textWriter.WriteLine("using System.Runtime.Remoting.Metadata;");
                    textWriter.WriteLine("using System.Runtime.Remoting.Metadata.W3cXsd2001;");
                    textWriter.WriteLine("using System.Runtime.InteropServices;");
                    writerStream.SetWrittenTo();
                }

                if ((Namespace != null) &&
                    (Namespace.Length != 0))
                {
                    textWriter.Write("namespace ");
                    textWriter.Write(WsdlParser.IsValidCS(EncodedNS));
                    textWriter.WriteLine(" {");
                    indentation = "    ";
                }


                StringBuilder sb = new StringBuilder(256);
                if (_numURTComplexTypes > 0)
                {
                    for (int i=0;i<_URTComplexTypes.Count;i++)
                    {
                        URTComplexType type = (URTComplexType) _URTComplexTypes[i];
                        if (type != null && type.IsPrint)
                        {
                            Util.Log("URTNamespace.PrintCSC Invoke Complex type PrintCSC");                                                                                                             
                            type.PrintCSC(textWriter, indentation, _encodedNS, sb);
                        }
                    }
                }

                if (_numURTSimpleTypes > 0)
                {
                    for (int i=0;i<_URTSimpleTypes.Count;i++)
                    {
                        URTSimpleType type = (URTSimpleType) _URTSimpleTypes[i];
                        if (type != null)
                        {
                            Util.Log("URTNamespace.PrintCSC Invoke Simple type PrintCSC");                                                                                                                                          
                            type.PrintCSC(textWriter, indentation, _encodedNS, sb);
                        }
                    }
                }

                for (int i=0;i<_URTInterfaces.Count;i++)
                {
                    Util.Log("URTNamespace.PrintCSC Invoke Interfaces PrintCSC");                                                                                                                                   
                    ((URTInterface)_URTInterfaces[i]).PrintCSC(textWriter, indentation, _encodedNS, sb);
                }

                if ((Namespace != null) &&
                    (Namespace.Length != 0))
                    textWriter.WriteLine('}');

                return;
            }

            // Fields
            private String _name;
            private UrtType _nsType;
            private WsdlParser _parser;
            private String _namespace;
            private String _encodedNS;
            private String _assemName;
            private int _anonymousSeqNum;
            private ArrayList _elmDecls;
            internal ArrayList _URTComplexTypes;
            private int _numURTComplexTypes;
            internal ArrayList _URTSimpleTypes;
            private int _numURTSimpleTypes;
            private ArrayList _URTInterfaces;
            private bool _bReferenced = false;            
        }

        internal interface IDump{
            void Dump();
        }

        internal interface INamespaces
        {
            void UsedNamespace(Hashtable namespaces);
        }

        internal class WsdlMessage : IDump, INamespaces
        {
            internal String name;
            internal String nameNs;
            internal ArrayList parts = new ArrayList(10);

            public void UsedNamespace(Hashtable namespaces)
            {
                Util.Log("WsdlMessage.UsedNamespace "+name+" "+nameNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                    */
                for (int i=0; i<parts.Count; i++)
                    ((INamespaces)parts[i]).UsedNamespace(namespaces);
            }

            public void Dump(){
                Util.Log("WsdlMessage.Dump");
                Util.Log("   name "+name);
                Util.Log("   ns "+nameNs);
                for (int i=0; i<parts.Count; i++)
                    ((IDump)parts[i]).Dump();
            }


        }

        internal class WsdlMessagePart : IDump, INamespaces
        {
            internal String name;
            internal String nameNs;
            internal String element;
            internal String elementNs;
            internal String typeName;
            internal String typeNameNs;


            public void UsedNamespace(Hashtable namespaces)
            {
                Util.Log("WsdlMessagePart.UsedNamespace "+name+" "+nameNs);
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                if (elementNs != null)
                    namespaces[elementNs] = 1;

            }

            public void Dump(){
                Util.Log("WsdlMessagePart.Dump");
                Util.Log("   name "+name);
                Util.Log("   element "+element);
                Util.Log("   elementNs "+elementNs);
                Util.Log("   typeName "+typeName);
                Util.Log("   typeNameNs "+typeNameNs);                              
            }
        }

        internal class WsdlPortType : IDump, INamespaces
        {
            internal String name;
            //internal String nameNs = null;
            internal ArrayList operations = new ArrayList(10);
            internal Hashtable sections = new Hashtable(10);

            public void UsedNamespace(Hashtable namespaces)
            {
                //Util.Log("WsdlPortType.UsedNamespace "+name+" "+nameNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                    */
                foreach (INamespaces item in operations)
                item.UsedNamespace(namespaces);

            }
            public void Dump()          {
                Util.Log("WsdlPortType.Dump");
                Util.Log("   name "+name);
                foreach (DictionaryEntry d in sections)
                Util.Log("   sections key "+d.Key+" value "+((WsdlPortTypeOperation)d.Value).name);
                foreach (IDump item in operations)
                item.Dump();
            }
        }

        internal class WsdlPortTypeOperation : IDump, INamespaces
        {
            internal String name;
            internal String nameNs;
            internal String parameterOrder;
            internal ArrayList contents = new ArrayList(3);

            public void UsedNamespace(Hashtable namespaces) 
            {
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                    */

                foreach (INamespaces item in contents)
                item.UsedNamespace(namespaces);
            }

            public void Dump()          {
                Util.Log("WsdlPortTypeOperation.Dump");
                Util.Log("   name "+name+" parameterOrder "+parameterOrder);
                foreach (IDump item in contents)
                item.Dump();
            }
        }


        internal class WsdlPortTypeOperationContent : IDump, INamespaces
        {
            internal String element;
            internal String name;
            internal String nameNs;
            internal String message;
            internal String messageNs;
            public void UsedNamespace(Hashtable namespaces)
            {
                Util.Log("WsdlPortTypeOperationContent.UsedNamespace "+name+" "+nameNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                if (messageNs != null)
                    namespaces[messageNs] = 1;
                    */
            }

            public void Dump()          {
                Util.Log("WsdlPortTypeOperationContent.Dump");
                Util.Log("   element "+element);                
                Util.Log("   name "+name);
                Util.Log("   message "+message);                
                Util.Log("   messageNs "+messageNs);                
            }
        }

        internal class WsdlBinding : IDump, INamespaces
        {
            internal URTNamespace parsingNamespace;
            internal String name;
            //internal String nameNs = null;
            internal String type;
            internal String typeNs;
            internal ArrayList suds = new ArrayList(10);
            internal WsdlBindingSoapBinding soapBinding;
            internal ArrayList operations = new ArrayList(10);

            public void UsedNamespace(Hashtable namespaces)
            {
                //Util.Log("WsdlBinding.UsedNamespace "+name+" "+nameNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                if (typeNs != null)
                    namespaces[typeNs] = 1;
                    */

                if (soapBinding != null)
                    soapBinding.UsedNamespace(namespaces);

                foreach (INamespaces item in suds)
                item.UsedNamespace(namespaces);

                foreach (INamespaces item in operations)
                item.UsedNamespace(namespaces);
            }

            public void Dump(){
                Util.Log("WsdlBinding.Dump");
                Util.Log("   name "+name);
                Util.Log("   type "+type);
                Util.Log("   typeNs "+typeNs);
                Util.Log("   parsingNamespace.ns "+parsingNamespace.Namespace);
                Util.Log("   parsingNamespace.EncodedNS "+parsingNamespace.EncodedNS);              

                if (soapBinding != null)
                    soapBinding.Dump();

                foreach(IDump item in suds)
                item.Dump();

                foreach (IDump item in operations)
                item.Dump();
            }
        }

        internal class WsdlBindingOperation : IDump, INamespaces
        {
            internal String name;
            internal String nameNs;
            internal String methodAttributes;
            internal WsdlBindingSoapOperation soapOperation;
            internal ArrayList sections = new ArrayList(10);

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBIndingOperation.UsedNamespace "+name+" "+nameNs);

                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1;
                    */

                soapOperation.UsedNamespace(namespaces);
                foreach (INamespaces item in sections)
                item.UsedNamespace(namespaces);
            }

            public void Dump()          {
                Util.Log("WsdlBindingOperation.Dump");
                Util.Log("   name "+name);
                Util.Log("   methodAttributes "+methodAttributes);
                soapOperation.Dump();
                foreach (IDump item in sections)
                item.Dump();
            }
        }

        internal class WsdlBindingOperationSection : IDump, INamespaces
        {
            internal String name;
            internal String elementName;
            internal ArrayList extensions = new ArrayList(10);

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBIndingOperationSection.UsedNamespace "+name);
                foreach (INamespaces item in extensions)
                item.UsedNamespace(namespaces);
            }

            public void Dump()          {
                Util.Log("WsdlBindingOperationSection.Dump");
                Util.Log("   name "+name);
                Util.Log("   elementName "+elementName);                
                foreach (IDump item in extensions)
                item.Dump();
            }
        }


        internal class WsdlBindingSoapBinding : IDump, INamespaces
        {
            internal String style;
            internal String transport;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSoapBinding.UsedNamespace ");
            }

            public void Dump()          {
                Util.Log("WsdlBindingSoapBinding.Dump");
                Util.Log("   style "+style);
                Util.Log("   transport "+transport);
            }


        }

        internal class WsdlBindingSoapBody : IDump, INamespaces
        {
            internal String parts;
            internal String use;
            internal String encodingStyle;
            internal String namespaceUri;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBIndingSoapBody.UsedNamespace "+parts);
            }

            public void Dump()          {
                Util.Log("WsdlBindingSoapBody.Dump");
                Util.Log("   parts "+parts);
                Util.Log("   use "+use);
                Util.Log("   encodingStyle "+encodingStyle);
                Util.Log("   namespaceUri "+namespaceUri);              
            }
        }

        internal class WsdlBindingSoapHeader : IDump, INamespaces
        {
            internal String message;
            internal String messageNs;
            internal String part;
            internal String use;
            internal String encodingStyle;
            internal String namespaceUri;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSoapHeader.UsedNamespace "+message+" "+messageNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1; 
                    */
            }

            public void Dump()          {
                Util.Log("WsdlBindingSoapHeader.Dump");
                Util.Log("   message "+message);
                Util.Log("   part "+part);
                Util.Log("   use "+use);
                Util.Log("   encodingStyle "+encodingStyle);
                Util.Log("   namespaceUri "+namespaceUri);              
            }
        }

        internal class WsdlBindingSoapOperation : IDump, INamespaces
        {
            internal String soapAction;
            internal String style;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSoapOperation.UsedNamespace ");
            }

            public void Dump(){
                Util.Log("WsdlBindingSoapOperation.Dump");
                Util.Log("   soapAction "+soapAction);
                Util.Log("   style "+style);
            }
        }

        internal class WsdlBindingSoapFault : IDump, INamespaces
        {
            internal String name;
            internal String use;
            internal String encodingStyle;
            internal String namespaceUri;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSoapFault.UsedNamespace "+name+" "+namespaceUri);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1; 
                    */
            }

            public void Dump()          {
                Util.Log("WsdlBindingSoapFault.Dump");
                Util.Log("   name "+name);
                Util.Log("   use "+use);
                Util.Log("   encodingStyle "+encodingStyle);
                Util.Log("   namespaceUri "+namespaceUri);              
            }
        }

        internal enum SudsUse
        {
            Class = 0,
            ISerializable = 1,
            Struct = 2,
            Interface = 3,
            MarshalByRef = 4,
            Delegate = 5,
            ServicedComponent = 6,
        }

        internal class WsdlBindingSuds : IDump, INamespaces
        {
            internal String elementName;
            internal String typeName;
            internal String ns;
            internal String extendsTypeName;
            internal String extendsNs;          
            internal SudsUse sudsUse;
            internal ArrayList implements = new ArrayList(10);
            internal ArrayList nestedTypes = new ArrayList(10);

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSuds.UsedNamespace elementName "+elementName+" typeName "+typeName+" ns "+ns+" extendsTypeName "+extendsTypeName+" extendsNs "+extendsNs+" sudsUse "+((Enum)sudsUse).ToString());
                if (ns != null)
                    namespaces[ns] = 1;

                if (extendsNs != null)
                    namespaces[extendsNs] = 1;

                foreach (INamespaces item in implements)
                item.UsedNamespace(namespaces);                    
            }


            public void Dump()          {
                Util.Log("WsdlBindingSuds.Dump");
                Util.Log("   elementName "+elementName);
                Util.Log("   typeName "+typeName);
                Util.Log("   ns "+ns);
                Util.Log("   extendsTypeName "+extendsTypeName);
                Util.Log("   extendsNs "+extendsNs);                
                Util.Log("   sudsUse "+((Enum)sudsUse).ToString());
                foreach (IDump item in implements)
                item.Dump();                    
                foreach (IDump item in nestedTypes)
                item.Dump();                    
            }
        }

        internal class WsdlBindingSudsImplements : IDump, INamespaces
        {
            internal String typeName;
            internal String ns;

            public void UsedNamespace(Hashtable namespaces)          
            {
                Util.Log("WsdlBindingSudsImplements.UsedNamespace typeName "+typeName+" ns "+ns);
                if (ns != null)
                    namespaces[ns] = 1;
            }

            public void Dump()  
            {
                Util.Log("WsdlBindingSudsImplements.Dump");             
                Util.Log("   typeName "+typeName);
                Util.Log("   ns "+ns);
            }
        }

        internal class WsdlBindingSudsNestedType : IDump
        {
            internal String name;
            internal String typeName;
            internal String ns;

            public void Dump()   
            {
                Util.Log("WsdlBindingSudsNestedType.Dump");             
                Util.Log("   name "+name);
                Util.Log("   typeName "+typeName);
                Util.Log("   ns "+ns);
            }
        }

        internal class WsdlService : IDump, INamespaces
        {
            internal String name;
            //internal String nameNs = null;
            internal Hashtable ports = new Hashtable(10);

            public void UsedNamespace(Hashtable namespaces)          
            {
                //Util.Log("WsdlService.UsedNamespace "+name+" "+nameNs);
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1; 
                    */
                foreach (DictionaryEntry  d in ports)
                ((INamespaces)d.Value).UsedNamespace(namespaces);
            }

            public void Dump()          {
                Util.Log("WsdlService.Dump");
                Util.Log("   name "+name);
                foreach (DictionaryEntry  d in ports)
                ((IDump)d.Value).Dump();
            }
        }

        internal class WsdlServicePort : IDump, INamespaces
        {
            internal String name;
            internal String nameNs;
            internal String binding;
            internal String bindingNs;
            internal ArrayList locations;

            public void UsedNamespace(Hashtable namespaces)          
            {
                /*
                if (nameNs != null)
                    namespaces[nameNs] = 1; 

                if (bindingNs != null)
                    namespaces[bindingNs] = 1; 
                    */
            }

            public void Dump()          {
                Util.Log("WsdlServicePort.Dump");
                Util.Log("   name "+name);
                Util.Log("   nameNs "+nameNs);
                Util.Log("   binding "+binding);
                Util.Log("   bindingNs"+bindingNs);
                if (locations != null)
                {
                    foreach (String item in locations)
                    Util.Log("   location "+item);
                }
            }
        }



        internal class WsdlMethodInfo : IDump
        {
            internal String soapAction;
            internal String methodName;
            internal String methodNameNs;
            internal String methodAttributes;
            internal String[] paramNamesOrder;
            internal String inputMethodName;
            internal String inputMethodNameNs;
            internal String outputMethodName;
            internal String outputMethodNameNs;
            internal String[] inputNames;
            internal String[] inputNamesNs;
            internal String[] inputElements;
            internal String[] inputElementsNs;
            internal String[] inputTypes;
            internal String[] inputTypesNs;
            internal String[] outputNames;
            internal String[] outputNamesNs;
            internal String[] outputElements;
            internal String[] outputElementsNs;
            internal String[] outputTypes;
            internal String[] outputTypesNs;
            internal String propertyName;
            internal bool bProperty = false;
            internal bool bGet = false;
            internal bool bSet = false;
            internal String propertyType;
            internal String propertyNs;
            internal String soapActionGet;
            internal String soapActionSet;

            public void Dump(){
                Util.Log("WsdlMethodInfo.Dump");
                Util.Log("   soapAction "+soapAction);
                Util.Log("   methodName "+methodName);
                Util.Log("   ns "+methodNameNs);

                if (paramNamesOrder != null)
                {
                    foreach(String pname in paramNamesOrder)
                    Util.Log("   paramNamesOrder   name "+pname);

                }
                if (inputNames != null)
                {
                    for (int i=0; i<inputNames.Length; i++)
                        Util.Log("   inputparams   name "+inputNames[i]+" element "+inputElements[i]+" elementNs "+inputElementsNs[i]+" type "+inputTypes[i]+" typeNs "+inputTypesNs[i]);
                }

                if (outputNames != null)
                {
                    for (int i=0; i<outputNames.Length; i++)
                        Util.Log("   outputparams   name "+outputNames[i]+" element "+outputElements[i]+" elementNs "+outputElementsNs[i]+" type "+outputTypes[i]+" typeNs "+outputTypesNs[i]);
                }

                if (bProperty)
                {
                    Util.Log("   Property   name "+propertyName+" bGet "+bGet+" bSet "+bSet+" type "+propertyType+" ns "+propertyNs);
                    Util.Log("   action get "+soapActionGet+" set "+soapActionSet);

                }
            }
        }
    }
}
