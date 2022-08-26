// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    SudsParser.cs
//<EMAIL>
// Author:  Gopal Kakivaya (GopalK)
//</EMAIL>
// Purpose: Defines SUDSParser that parses a given SUDS document
//          and generates types defined in it.
//
// Date:    April 01, 2000
// Revised: November 15, 2000 (Wsdl) <EMAIL>Microsoft</EMAIL>
//
//============================================================
namespace System.Runtime.Remoting.MetadataServices
{
    using System;
    using System.IO;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Collections;

    // Represents exceptions thrown by the SUDSParser
    [Serializable]
    public class SUDSParserException : Exception
    {
        internal SUDSParserException(String message)
        : base(message)
        {
        }

        protected SUDSParserException(SerializationInfo info, StreamingContext context): base(info, context) {}
    }

    // Represents a block type of a complex type
	[Serializable]
    internal enum SchemaBlockType { ALL, SEQUENCE, CHOICE, ComplexContent}

    // This class parses SUDS documents
    internal class SUDSParser
    {
        WsdlParser wsdlParser;
                                                        
        // Main parser
        internal SUDSParser(TextReader input, String outputDir, ArrayList outCodeStreamList, String locationURL, bool bWrappedProxy, String proxyNamespace)
        {
			Util.Log("SUDSParser.SUDSParser outputDir "+outputDir+" locationURL "+locationURL+" bWrappedProxy "+bWrappedProxy+" proxyNamespace "+proxyNamespace);
            Util.LogInput(ref input);
            wsdlParser = new WsdlParser(input, outputDir, outCodeStreamList, locationURL, bWrappedProxy, proxyNamespace);
        }

        internal void Parse()
        {
			Util.Log("SUDSParser.Parse");
            wsdlParser.Parse();
        }

    }
}

