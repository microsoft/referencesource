// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    WsdlWriter.cs
//<EMAIL>
// Author:  Peter de Jong (Microsoft)
//</EMAIL>
// Purpose: Defines WsdlWriter that parses a given Wsdl document
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
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Metadata; 
    using System.Runtime.Serialization;
    using System.Runtime.Remoting.Channels; // This is so we can get the resource strings.
    using System.Globalization;

    // This class generates SUDS documents
    internal class WsdlGenerator
    {
        // Constructor
        internal WsdlGenerator(Type[] types, TextWriter output)
        {
            Util.Log("WsdlGenerator.WsdlGenerator 1");
            _textWriter = output;
            _queue = new Queue();
            _name = null;
            _namespaces = new ArrayList();
            _dynamicAssembly = null;
            _serviceEndpoint = null;
            for (int i=0;i<types.Length;i++)
            {
                if (types[i] != null)
                {
                    if (types[i].BaseType != null)
                    {
                        Util.Log("WsdlGenerator.WsdlGenerator ProcessTypeAttributes 1 "+types[i]);
                        ProcessTypeAttributes(types[i]);
                        _queue.Enqueue(types[i]);
                    }
                }
            }
        }

        // Constructor
        internal WsdlGenerator(Type[] types, SdlType sdlType, TextWriter output)
        {
            Util.Log("WsdlGenerator.WsdlGenerator 2");
            _textWriter = output;
            _queue = new Queue();
            _name = null;
            _namespaces = new ArrayList();
            _dynamicAssembly = null;
            _serviceEndpoint = null;
            for (int i=0;i<types.Length;i++)
            {
                if (types[i] != null)
                {
                    if (types[i].BaseType != null)
                    {
                        Util.Log("WsdlGenerator.WsdlGenerator ProcessTypeAttributes 2 "+types[i].BaseType);
                        ProcessTypeAttributes(types[i]);
                        _queue.Enqueue(types[i]);
                    }
                }
            }
        }

        // Constructor
        internal WsdlGenerator(Type[] types, TextWriter output, Assembly assembly, String url)
        : this(types, output)
        {
            Util.Log("WsdlGenerator.WsdlGenerator 3 "+url);         
            _dynamicAssembly = assembly;
            _serviceEndpoint = url;
        }

        // Constructor
        internal WsdlGenerator(Type[] types, SdlType sdlType, TextWriter output, Assembly assembly, String url)
        : this(types, output)
        {
            Util.Log("WsdlGenerator.WsdlGenerator 4 "+url);         
            _dynamicAssembly = assembly;
            _serviceEndpoint = url;
        }

        internal WsdlGenerator(ServiceType[] serviceTypes, SdlType sdlType, TextWriter output)
        {
            Util.Log("WsdlGenerator.WsdlGenerator 5 ");
            _textWriter = output;
            _queue = new Queue();
            _name = null;
            _namespaces = new ArrayList();
            _dynamicAssembly = null;
            _serviceEndpoint = null;

            for (int i=0; i<serviceTypes.Length; i++)
            {
                if (serviceTypes[i] != null)
                {
                    if (serviceTypes[i].ObjectType.BaseType != null)
                    {
                        Util.Log("WsdlGenerator.WsdlGenerator ProcessTypeAttributes 3 objectType "+serviceTypes[i].ObjectType+" basetype "+serviceTypes[i].ObjectType.BaseType);
                        ProcessTypeAttributes(serviceTypes[i].ObjectType);
                        _queue.Enqueue(serviceTypes[i].ObjectType);
                    }
                }

                // Associate serviceEndpoint with type. A type can have multiple serviceEndpoints
                if (serviceTypes[i].Url != null)
                {
                    if (_typeToServiceEndpoint == null)
                        _typeToServiceEndpoint = new Hashtable(10);
                    if (_typeToServiceEndpoint.ContainsKey(serviceTypes[i].ObjectType.Name))
                    {
                        ArrayList serviceEndpoints = (ArrayList)_typeToServiceEndpoint[serviceTypes[i].ObjectType.Name];
                        serviceEndpoints.Add(serviceTypes[i].Url);
                    }
                    else
                    {
                        ArrayList serviceEndpoints = new ArrayList(10);
                        serviceEndpoints.Add(serviceTypes[i].Url);
                        _typeToServiceEndpoint[serviceTypes[i].ObjectType.Name] = serviceEndpoints;
                    }

                }
            }
        }

        internal static void QualifyName(StringBuilder sb, String ns, String name)
        {
            if (!(ns == null || ns.Length == 0))
            {
                sb.Append(ns);
                sb.Append('.');
            }
            sb.Append(name);
        }


        internal static String RefName(Type type)
        {
            String refName = type.Name;

            if (!(type.IsPublic || type.IsNotPublic))
            {
                Util.Log("WsdlGenerator.WsdlGenerator RefName nested "+type);
                // nested name
                refName = type.FullName;
                int index = refName.LastIndexOf('.');
                if (index > 0)
                {
                    // nested type, type.Name returns full type rather then simple type
                    refName = refName.Substring(index+1);
                }
                refName = refName.Replace('+', '.');
            }

            return refName;
        }

        internal void ProcessTypeAttributes(Type type)
        {
            // Check to see if the xsd and xsi schema types should be 1999 instead of 2000. This is a temporary fix for an interop problem
            SoapTypeAttribute att = InternalRemotingServices.GetCachedSoapAttribute(type) as SoapTypeAttribute;
            if (att != null)
            {
                SoapOption soapOption = att.SoapOptions;
                if ((soapOption &= SoapOption.Option1) == SoapOption.Option1)
                    _xsdVersion = XsdVersion.V1999;
                else if ((soapOption &= SoapOption.Option2) == SoapOption.Option2)
                    _xsdVersion = XsdVersion.V2000;
                else
                    _xsdVersion = XsdVersion.V2001;
            }
            Util.Log("WsdlGenerator.ProcessTypeAttributes "+type+" SoapOptions "+((Enum)att.SoapOptions).ToString()+" _xsdVersion "+((Enum)_xsdVersion).ToString());         

        }


        // Generates SUDS
        internal void Generate()
        {
            Util.Log("WsdlGenerator.Generate");         
            // Generate the trasitive closure of the types reachable from
            // the supplied types
            while (_queue.Count > 0)
            {
                // Dequeue from not yet seen queue
                Type type = (Type) _queue.Dequeue();
                ProcessType(type);
            }

            // At this point we have the complete list of types
            // to be processed. Resolve cross references between
            // them
            Resolve();

            // At this stage, we are ready to print the schemas
            PrintWsdl();

            // Flush cached buffers
            _textWriter.Flush();

            return;
        }

        internal void ProcessType(Type type)
        {
            // Check if the type was encountered earlier
            String ns;
            Assembly assem;
            bool bInteropType = GetNSAndAssembly(type, out ns, out assem);
            Util.Log("WsdlGenerator.ProcessType Dequeue "+type+" ns "+ns+" assem "+assem);                         
            XMLNamespace xns = LookupNamespace(ns, assem);
            if (xns != null)
            {
                String searchName = WsdlGenerator.RefName(type);

                if (xns.LookupSchemaType(searchName) != null)
                {
                    return;
                }
            }
            else
            {
                xns = AddNamespace(ns, assem, bInteropType);
            }
            _typeToInteropNS[type] = xns;
            
            if (!type.IsArray)
            {
                // Check if type needs to be represented as a SimpleSchemaType
                SimpleSchemaType ssType = SimpleSchemaType.GetSimpleSchemaType(type, xns, false);
                Util.Log("WsdlGenerator.ProcessType simpleType "+ssType);

                if (ssType != null)
                {
                    // Add to namespace as a SimpleSchemaType
                    xns.AddSimpleSchemaType(ssType);
                }
                else
                {
                    // Check for the first MarshalByRef type
                    bool bUnique = false;
                    String connectURL = null;
                    Hashtable connectTypeToServiceEndpoint = null;
                    if (_name == null && s_marshalByRefType.IsAssignableFrom(type))
                    {
                        Util.Log("WsdlGenerator.ProcessType need new type "+type+" typename "+type.Name);          
                        _name = type.Name;
                        _targetNS = xns.Namespace;
                        _targetNSPrefix = xns.Prefix;
                        connectURL = _serviceEndpoint;
                        connectTypeToServiceEndpoint = _typeToServiceEndpoint;
                        bUnique = true;
                    }

                    RealSchemaType rsType = new RealSchemaType(type, xns, connectURL, connectTypeToServiceEndpoint, bUnique, this);
                    // Add to namespace as a RealSchemaType
                    xns.AddRealSchemaType(rsType);
                    // Enqueue types reachable from this type
                    EnqueueReachableTypes(rsType);
                }
            }
        }

        // Adds types reachable from the given type
        private void EnqueueReachableTypes(RealSchemaType rsType)
        {
            Util.Log("WsdlGenerator.EnqueueReachableTypes "+rsType.Name+" "+rsType.XNS.Name);           
            // Get the XML namespace object
            XMLNamespace xns = rsType.XNS;

            // Process base type
            if (rsType.Type.BaseType != null)
            {
                if (rsType.Type.BaseType != s_valueType || rsType.Type.BaseType != s_objectType)
                    AddType(rsType.Type.BaseType, GetNamespace(rsType.Type.BaseType));
            }

            // Check if this is a suds type
            bool bSUDSType = rsType.Type.IsInterface ||
                             s_marshalByRefType.IsAssignableFrom(rsType.Type) ||
                             s_delegateType.IsAssignableFrom(rsType.Type);
            if (bSUDSType)
            {
                Util.Log("WsdlGenerator.EnqueueReachableTypes suds type "+rsType.Name+" "+rsType.XNS.Name);           
                // Process fields
                FieldInfo[] fields = rsType.GetInstanceFields();
                for (int i=0;i<fields.Length;i++)
                {
                    if (fields[i].FieldType == null)
                        continue;
                    AddType(fields[i].FieldType, xns);
                }

                // Process implemented interfaces
                Type[] interfaces = rsType.GetIntroducedInterfaces();
                if (interfaces.Length > 0)
                {
                    for (int i=0;i<interfaces.Length;i++)
                    {
                        Util.Log("WsdlGenerator.EnqueueReachableTypes Interfaces "+interfaces[i].Name+" "+xns.Name);                        
                        AddType(interfaces[i], xns);
                    }
                }

                ProcessMethods(rsType);

            }
            else
            {
                // Process fields
                FieldInfo[] fields = rsType.GetInstanceFields();
                for (int i=0;i<fields.Length;i++)
                {
                    if (fields[i].FieldType == null)
                        continue;
                    AddType(fields[i].FieldType, xns);
                }
            }

            return;
        }

        private void ProcessMethods(RealSchemaType rsType)
        {
            Util.Log("WsdlGenerator.ProcessMethods "+rsType);                       
            XMLNamespace xns = rsType.XNS;
            MethodInfo[] methods = rsType.GetIntroducedMethods();
            if (methods.Length > 0)
            {
                String methodNSString = null;
                XMLNamespace methodXNS = null;  

                if (xns.IsInteropType)
                {
                    methodNSString = xns.Name;
                    methodXNS = xns;
                }
                else
                {
                    StringBuilder sb = new StringBuilder();
                    WsdlGenerator.QualifyName(sb, xns.Name, rsType.Name);
                    methodNSString = sb.ToString();
                    methodXNS = AddNamespace(methodNSString, xns.Assem);
                    xns.DependsOnSchemaNS(methodXNS, false);
                }

                for (int i=0;i<methods.Length;i++)
                {
                    MethodInfo method = methods[i];
                    Util.Log("WsdlGenerator.ProcessMethods methods "+method.Name+" "+methodXNS.Name);
                    AddType(method.ReturnType, methodXNS);
                    ParameterInfo[] parameters = method.GetParameters();
                    for (int j=0;j<parameters.Length;j++)
                        AddType(parameters[j].ParameterType, methodXNS); 
                }
            }
        }


        // Adds the given type if it has not been encountered before
        private void AddType(Type type, XMLNamespace xns)
        {
            Util.Log("WsdlGenerator.AddType "+type+" ns "+xns.Namespace);                      
            //  System.Array says that it has element type, but returns null
            //         when asked for the element type.<




            // Need to get underlying element type
            // For arrays of arrays, want element, not embedded array
            Type elementType = type.GetElementType();
            Type nextelementType = elementType;
            while(nextelementType != null)
            {
                nextelementType = elementType.GetElementType();
                if (nextelementType != null)
                    elementType = nextelementType;
                
            }
            Util.Log("WsdlGenerator.AddType elementType "+type+" elementType "+elementType);                      

            if (elementType != null)
                EnqueueType(elementType, xns);


            if (!type.IsArray && !type.IsByRef)
                EnqueueType(type, xns);

            if (!(type.IsPublic || type.IsNotPublic))
            {
                // nested type, enqueue parent
                String refTypeName = type.FullName;
                int index = refTypeName.IndexOf("+");
                if (index > 0)
                {
                    String parentName = refTypeName.Substring(0, index);
                    Assembly assembly = type.Module.Assembly;
                    Util.Log("WsdlGenerator.AddType parentName "+parentName+" assembly "+assembly);
                    Type parentType = assembly.GetType(parentName, true);
                    Util.Log("WsdlGenerator.AddType parentType "+parentType);
                    if (parentType == null)
                    {
                        //Error nested type
                    }
                    EnqueueType(parentType, xns);
                }
            }
        }

        private void EnqueueType(Type type, XMLNamespace xns)
        {
            Util.Log("WsdlGenerator.EnqueueType "+type+" ns "+xns.Namespace);                      
            if (!type.IsPrimitive || type == s_charType) //char is not a xsd type
            {
                String ns;
                Assembly assem;
                XMLNamespace dependsOnNS = null;

                bool bInteropType = GetNSAndAssembly(type, out ns, out assem);

                // Lookup the namespace
                dependsOnNS = LookupNamespace(ns, assem);
                // Creat a new namespace if neccessary
                if (dependsOnNS == null)
                    dependsOnNS = AddNamespace(ns, assem, bInteropType);

                // The supplied namespace depends directly on the namespace of the type
                String typeString = SudsConverter.MapClrTypeToXsdType(type); //see if this is a xsd type
                if (type.IsInterface || typeString != null || type == s_voidType)
                {
                    // Interfaces aren't in schema section
                    // Any xsd type
                    xns.DependsOnSchemaNS(dependsOnNS, false); 
                }
                else
                    xns.DependsOnSchemaNS(dependsOnNS, true);



                // Enqueue the type if does not belong to system namespace
                if (!type.FullName.StartsWith("System."))
                {
                    Util.Log("WsdlGenerator.EnqueueType place on queue "+type+" ns "+xns.Namespace);                      
                    _queue.Enqueue(type);
                }
            }
        }

        private static bool GetNSAndAssembly(Type type, out String ns, out Assembly assem)
        {
            Util.Log("WsdlGenerator.GetNSAndAssembly enter "+type);

            String xmlNamespace = null;
            String xmlElement = null;
            bool bInterop = false;
            SoapServices.GetXmlElementForInteropType(type, out xmlElement, out xmlNamespace);
            if (xmlNamespace != null)
            {
                ns = xmlNamespace;
                assem = type.Module.Assembly;
                bInterop = true;
            }
            else
            {
                // Return the namespace and assembly in which the type is defined
                ns = type.Namespace;
                assem = type.Module.Assembly;
                bInterop = false;
            }

            Util.Log("WsdlGenerator.GetNSAndAssembly exit ns "+ns+" assem "+assem+" bInterop "+bInterop);
            return bInterop;
        }

        private XMLNamespace LookupNamespace(String name, Assembly assem)
        {
            Util.Log("WsdlGenerator.LookupNamespace "+name);                        
            for (int i=0;i<_namespaces.Count;i++)
            {
                XMLNamespace xns = (XMLNamespace) _namespaces[i];
                if (name == xns.Name)
                    return(xns);
            }

            return(null);
        }

        private XMLNamespace AddNamespace(String name, Assembly assem)
        {
            return AddNamespace(name, assem, false);
        }

        private XMLNamespace AddNamespace(String name, Assembly assem, bool bInteropType)
        {
            Util.Log("WsdlGenerator.AddNamespace "+name);                       
            Debug.Assert(LookupNamespace(name, assem) == null, "Duplicate Type found");

            XMLNamespace xns = new XMLNamespace(name, assem,
                                                _serviceEndpoint,
                                                _typeToServiceEndpoint,
                                                "ns" + _namespaces.Count,
                                                bInteropType, this);
            _namespaces.Add(xns);

            return(xns);
        }

        private XMLNamespace GetNamespace(Type type)
        {
            Util.Log("WsdlGenerator.GetNamespace "+type);
            String ns = null;
            Assembly assem = null;
            bool bInteropType = GetNSAndAssembly(type, out ns, out assem);

            XMLNamespace xns = LookupNamespace(ns, assem);
            if (xns == null)
            {
                xns = AddNamespace(ns, assem, bInteropType);
            }
            return xns;
        }


        private void Resolve()
        {
            Util.Log("WsdlGenerator.Resolve ");                     
            for (int i=0;i<_namespaces.Count;i++)
                ((XMLNamespace) _namespaces[i]).Resolve();

            return;
        }

        private void PrintWsdl()
        {
            if (_targetNS == null || _targetNS.Length == 0)
            {
                // No marshalbyRef object so use another target
                // Find a namespace
                if (_namespaces.Count > 0)
                    _targetNS =  ((XMLNamespace)_namespaces[0]).Namespace;
                else
                    _targetNS = "http://schemas.xmlsoap.org/wsdl/";
            }

            String indent = "";
            String indent1 = IndentP(indent);
            String indent2 = IndentP(indent1);
            String indent3 = IndentP(indent2);
            String indent4 = IndentP(indent3);

            StringBuilder sb = new StringBuilder(256);          
            Util.Log("WsdlGenerator.PrintWsdl");
            _textWriter.WriteLine("<?xml version='1.0' encoding='UTF-8'?>");
            sb.Length = 0;
            sb.Append("<definitions ");
            if (_name != null)
            {
                sb.Append("name='");
                sb.Append(_name);
                sb.Append("' ");
            }
            sb.Append("targetNamespace='");
            sb.Append(_targetNS);
            sb.Append("'");
            _textWriter.WriteLine(sb);              

            PrintWsdlNamespaces(_textWriter, sb, indent3);
    	    // See if there are any schema information to print.
    
    	    bool bPrintTypeSection = false;
                for (int i=0;i<_namespaces.Count;i++)
                {
                   if (((XMLNamespace) _namespaces[i]).CheckForSchemaContent())
    	       {	
    		   bPrintTypeSection = true;
    		   break;
    	       }
                }
    
    
    	    if (bPrintTypeSection)
    	    {
                PrintTypesBeginWsdl(_textWriter, sb, indent1);
    
                for (int i=0;i<_namespaces.Count;i++)
                {
           		    if (((XMLNamespace) _namespaces[i]).CheckForSchemaContent())
        		    {
                        Util.Log("WsdlGenerator.PrintWsdl call PrintWsdlNamespaces "+((XMLNamespace) _namespaces[i]).Namespace);                            
                        ((XMLNamespace) _namespaces[i]).PrintSchemaWsdl(_textWriter, sb, indent2);
                    }
                }
    
                PrintTypesEndWsdl(_textWriter, sb, indent1);
            }
            
            ArrayList refNames = new ArrayList(25);

            for (int i=0;i<_namespaces.Count;i++)
                ((XMLNamespace) _namespaces[i]).PrintMessageWsdl(_textWriter, sb, indent1, refNames);

            PrintServiceWsdl(_textWriter, sb, indent1, refNames);

            _textWriter.WriteLine("</definitions>");

            return;
        }


        private void PrintWsdlNamespaces(TextWriter textWriter, StringBuilder sb, String indent)
        {
            Util.Log("WsdlGenerator.PrintWsdlNamespaces");          
            sb.Length = 0;
            sb.Append(indent);
            sb.Append("xmlns='http://schemas.xmlsoap.org/wsdl/'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);
            sb.Append("xmlns:tns='");
            sb.Append(_targetNS);
            sb.Append("'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("xmlns:xsd='");
            sb.Append(SudsConverter.GetXsdVersion(_xsdVersion));
            sb.Append("'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("xmlns:xsi='");
            sb.Append(SudsConverter.GetXsiVersion(_xsdVersion));
            sb.Append("'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("xmlns:suds='http://www.w3.org/2000/wsdl/suds'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("xmlns:wsdl='http://schemas.xmlsoap.org/wsdl/'");
            textWriter.WriteLine(sb);

            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("xmlns:soapenc='http://schemas.xmlsoap.org/soap/encoding/'");
            textWriter.WriteLine(sb);


            Hashtable usedNames = new Hashtable(10);
            for (int i=0;i<_namespaces.Count;i++)
                ((XMLNamespace) _namespaces[i]).PrintDependsOnWsdl(_textWriter, sb, indent, usedNames);

            // This should be last because it closes off the definitions element
            sb.Length = 0;
            sb.Append(indent);                      
            sb.Append("xmlns:soap='http://schemas.xmlsoap.org/wsdl/soap/'>");
            textWriter.WriteLine(sb);


        }

        private void PrintTypesBeginWsdl(TextWriter textWriter, StringBuilder sb, String indent)
        {
            Util.Log("WsdlGenerator.PrintTypesBeginWsdl");          
            sb.Length = 0;
            sb.Append(indent);
            sb.Append("<types>");
            textWriter.WriteLine(sb);
        }

        private void PrintTypesEndWsdl(TextWriter textWriter, StringBuilder sb, String indent)
        {
            Util.Log("WsdlGenerator.PrintTypesEndWsdl");            
            sb.Length = 0;
            sb.Append(indent);          
            sb.Append("</types>");
            textWriter.WriteLine(sb);           
        }

        internal void PrintServiceWsdl(TextWriter textWriter, StringBuilder sb, String indent, ArrayList refNames)
        {
            Util.Log("WsdlGenerator.PrintServiceWsdl");         
            String indent1 = IndentP(indent);               
            String indent2 = IndentP(indent1);
            String indent3 = IndentP(indent2);                                                      
            sb.Length = 0;
            sb.Append("\n");
            sb.Append(indent);
            sb.Append("<service name='");
            sb.Append(_name);
            sb.Append("Service'");
            sb.Append(">");
            textWriter.WriteLine(sb);

            for (int i=0; i<refNames.Count; i++)
            {
                if (((_typeToServiceEndpoint != null) && (_typeToServiceEndpoint.ContainsKey(refNames[i]))) ||
                    (_serviceEndpoint != null))
                {
            
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("<port name='");
                    sb.Append(refNames[i]);
                    sb.Append("Port'");
                    sb.Append(" ");
                    sb.Append("binding='tns:");
                    sb.Append(refNames[i]);
                    sb.Append("Binding");
                    sb.Append("'>");
                    textWriter.WriteLine(sb);               
    
                    if ((_typeToServiceEndpoint != null) && (_typeToServiceEndpoint.ContainsKey(refNames[i])))
                    {
                        foreach (String url in (ArrayList)_typeToServiceEndpoint[refNames[i]])
                        {
                            sb.Length = 0;
                            sb.Append(indent2);
                            sb.Append("<soap:address location='");
                            sb.Append(UrlEncode(url));
                            sb.Append("'/>");
                            textWriter.WriteLine(sb);
                        }
    
                    }
                    else if (_serviceEndpoint != null)
                    {
                        sb.Length = 0;
                        sb.Append(indent2);
                        sb.Append("<soap:address location='");
                        sb.Append(_serviceEndpoint);
                        sb.Append("'/>");
                        textWriter.WriteLine(sb);
                    }
    
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("</port>");
                    textWriter.WriteLine(sb);
                }
            }
            sb.Length = 0;
            sb.Append(indent);
            sb.Append("</service>");
            textWriter.WriteLine(sb);               
        }

        private String UrlEncode(String url)
        {
            if (url == null || url.Length == 0)
                return url;

            int index = url.IndexOf("&amp;");
            if (index > -1)
            {
                // Assume it's encoded
                return url;
            }

            index = url.IndexOf('&');
            if (index > -1)
            {
                return url.Replace("&", "&amp;");
            }

            return url;
        }


        // Private fields
        private TextWriter _textWriter;
        internal Queue _queue;
        private String _name;
        private String _targetNS;
        private String _targetNSPrefix;
        private ArrayList _namespaces;
        private Assembly _dynamicAssembly;
        private String _serviceEndpoint; //service endpoint for all types
        private XsdVersion _xsdVersion; // Temporary, specifies what xsd and xsi schema to put out
        internal Hashtable _typeToServiceEndpoint; //service endpoint for each type
        internal Hashtable _typeToInteropNS = new Hashtable(); // If interop type, then XMLNamespace the type is in.

        private static Type s_marshalByRefType = typeof(System.MarshalByRefObject);
        private static Type s_contextBoundType = typeof(System.ContextBoundObject);
        private static Type s_delegateType = typeof(System.Delegate);
        private static Type s_valueType = typeof(System.ValueType);
        private static Type s_objectType = typeof(System.Object);
        private static Type s_charType = typeof(System.Char);
        private static Type s_voidType = typeof(void);

        private static Type s_remotingClientProxyType = typeof(System.Runtime.Remoting.Services.RemotingClientProxy);
        private static SchemaBlockType blockDefault = SchemaBlockType.SEQUENCE;

        /***************************************************************
         **
         ** Private classes used by SUDS generator
         **
         ***************************************************************/
        private interface IAbstractElement
        {
            void Print(TextWriter textWriter, StringBuilder sb, String indent);
        }

        private class EnumElement : IAbstractElement
        {
            internal EnumElement(String value)
            {
                Util.Log("EnumElement.EnumElement "+value);
                _value = value;
            }

            public void Print(TextWriter textWriter, StringBuilder sb, String indent)
            {
                Util.Log("EnumElement.Print");
                sb.Length = 0;
                sb.Append(indent);
                sb.Append("<enumeration value='");
                sb.Append(_value);
                sb.Append("'/>");
                textWriter.WriteLine(sb);
                return;
            }

            // Fields
            private String _value;
        }

        /*
        private class ComplexContent : IAbstractElement
        {
            internal ComplexContent()
            {
            }

            internal void Add(Restriction restriction)
            {
                _restriction = restriction;
            }

            public void Print(TextWriter textWriter, StringBuilder sb, String indent)
            {
                Util.Log("EnumElement.Print");
                sb.Length = 0;
                sb.Append(indent);
                sb.Append("<enumeration value='");
                sb.Append(_value);
                sb.Append("'/>");
                textWriter.WriteLine(sb);
                return;
            }

            Restriction _restriction;
        }
        */


        private class Restriction : Particle
        {
            internal enum RestrictionType 
            {
                None = 0,
                Array = 1,
                Enum = 2
            }

            internal Restriction()
            {
                Util.Log("Restriction.Restriction ");
            }

            internal Restriction(String baseName, XMLNamespace baseNS)
            {
                Util.Log("Restriction.Restriction "+baseName+" "+baseNS.Namespace);
                _baseName = baseName;
                _baseNS = baseNS;
            }

            internal void AddArray(SchemaAttribute attribute)
            {
                Util.Log("Restriction.AddArray ");
                _rtype = RestrictionType.Array;
                _attribute = attribute;
            }

            public override String Name()
            {
                return _baseName;
            }

            public override void Print(TextWriter textWriter, StringBuilder sb, String indent)
            {
                Util.Log("Restriction.Print "+_baseName);
                String indent1 = IndentP(indent);
                sb.Length = 0;
                sb.Append(indent);
                if (_rtype == RestrictionType.Array)
                    sb.Append("<restriction base='soapenc:Array'>");
                else if (_rtype == RestrictionType.Enum)
                {
                    sb.Append("<restriction base='xsd:string'>");
                }
                else
                {
                    sb.Append("<restriction base='");
                    sb.Append(_baseNS.Prefix);
                    sb.Append(':');
                    sb.Append(_baseName);
                    sb.Append("'>");
                }
                textWriter.WriteLine(sb);
                foreach (IAbstractElement elem in _abstractElms)
                  elem.Print(textWriter, sb, IndentP(indent1));

                if (_attribute != null)
                    _attribute.Print(textWriter, sb, IndentP(indent1));
                sb.Length = 0;
                sb.Append(indent);
                sb.Append("</restriction>");
                textWriter.WriteLine(sb);
            }

            String _baseName;
            XMLNamespace _baseNS;
            internal RestrictionType _rtype;
            SchemaAttribute _attribute;
            internal ArrayList _abstractElms = new ArrayList();
        }

        private class SchemaAttribute : IAbstractElement
        {
            internal SchemaAttribute()
            {
                Util.Log("SchemaAttribute ");
            }

            internal void AddArray(String wireQname)
            {
                Util.Log("SchemaAttribute wireQname "+wireQname);
                _wireQname = wireQname;
            }

            public void Print(TextWriter textWriter, StringBuilder sb, String indent)
            {
                sb.Length = 0;
                sb.Append(indent);
                sb.Append("<attribute ref='soapenc:arrayType'");
                sb.Append(" wsdl:arrayType ='");
                sb.Append(_wireQname);
                sb.Append("'/>");
                textWriter.WriteLine(sb);
                return;
            }

            // Fields
            private String _wireQname;

        }

        private abstract class Particle : IAbstractElement
        {
            protected Particle(){}
            public abstract String Name();
            public abstract void Print(TextWriter textWriter, StringBuilder sb, String indent);
        }

        private class SchemaElement : Particle
        {
            internal SchemaElement(String name, Type type, bool bEmbedded, XMLNamespace xns)
            : base(){
                Util.Log("SchemaElement.SchemaElement Particle "+name+" type "+type+" bEmbedded "+bEmbedded);
                _name = name;
                _typeString = null;
                _schemaType = SimpleSchemaType.GetSimpleSchemaType(type, xns, true);                
                _typeString = RealSchemaType.TypeName(type, bEmbedded, xns);
            }

            public override String Name()
            {
                return _name;
            }

            public override void Print(TextWriter textWriter, StringBuilder sb, String indent){
                Util.Log("SchemaElement.Print "+_name+" _schemaType "+_schemaType+" _typeString "+_typeString );
                String indent1 = IndentP(indent);
                sb.Length = 0;
                sb.Append(indent);
                sb.Append("<element name='");
                sb.Append(_name);
                if (_schemaType != null && !(_schemaType is SimpleSchemaType && ((SimpleSchemaType)_schemaType).Type.IsEnum))
                {
                    sb.Append("'>");
                    textWriter.WriteLine(sb);
                    _schemaType.PrintSchemaType(textWriter, sb, IndentP(indent1), true);

                    sb.Length = 0;
                    sb.Append(indent);
                    sb.Append("</element>");
                }
                else
                {
                    if (_typeString != null)
                    {
                        sb.Append("' type='");
                        sb.Append(_typeString);
                        sb.Append('\'');
                    }
                    sb.Append("/>");
                }
                textWriter.WriteLine(sb);

                return;
            }

            // Fields
            private String _name;
            private String _typeString;
            private SchemaType _schemaType;
        }

        private abstract class SchemaType
        {
            internal abstract void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous);
        }

        private class SimpleSchemaType : SchemaType
        {
            private SimpleSchemaType(Type type, XMLNamespace xns)
            {
                Util.Log("SimpleSchemaType.SimpleSchemaType "+type+" xns "+((xns != null) ? xns.Name : "Null"));
                _type = type;
                _xns = xns;
                _abstractElms = new ArrayList();

                _fullRefName = WsdlGenerator.RefName(type);
            }

            internal Type Type
            {
                get{ return(_type);}
            }

            internal String FullRefName
            {
                get{ return _fullRefName;}
            }

            internal String BaseName{
                get{ return(_baseName);}
            }

            internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous){
                Util.Log("SimpleSchemaType.PrintSchemaType _type.Name "+_type.Name);
                sb.Length = 0;
                sb.Append(indent);
                if (bAnonymous == false)
                {
                    sb.Append("<simpleType name='");
                    sb.Append(FullRefName);
                    sb.Append("'");
                    if (BaseName != null)
                    {
                        sb.Append(" base='");
                        sb.Append(BaseName);
                        sb.Append("'");
                    }

                    if (_restriction._rtype == Restriction.RestrictionType.Enum)
                    {
                        sb.Append(" suds:enumType='");
                        sb.Append(_restriction.Name());
                        sb.Append("'");
                    }
                }
                else
                {
                    if (BaseName != null)
                    {
                        sb.Append("<simpleType base='");
                        sb.Append(BaseName);
                        sb.Append("'");
                    }
                    else
                        sb.Append("<simpleType");
                }

                bool bEmpty = (_abstractElms.Count == 0 && _restriction == null);
                if (bEmpty)
                    sb.Append("/>");
                else
                    sb.Append(">");
                textWriter.WriteLine(sb);
                if (bEmpty)
                    return;

                if (_abstractElms.Count > 0)
                {
                    for (int i=0;i<_abstractElms.Count; i++)
                        ((IAbstractElement) _abstractElms[i]).Print(textWriter, sb, IndentP(indent));
                }

                if (_restriction != null)
                    _restriction.Print(textWriter, sb, IndentP(indent));

                textWriter.Write(indent);
                textWriter.WriteLine("</simpleType>");

                return;
            }


            internal static SimpleSchemaType GetSimpleSchemaType(Type type, XMLNamespace xns, bool fInline)
            {
                Util.Log("SimpleSchemaType.GetSimpleSchemaType "+type+" xns "+xns.Name);                

                SimpleSchemaType ssType = null;
                if (type.IsEnum)
                {
                    ssType = new SimpleSchemaType(type, xns);
                    String baseName = RealSchemaType.TypeName(Enum.GetUnderlyingType(type), true, xns);
                    ssType._restriction = new Restriction(baseName, xns);
                    String[] values = Enum.GetNames(type);
                    for (int i=0;i<values.Length;i++)
                        ssType._restriction._abstractElms.Add(new EnumElement(values[i]));
                    ssType._restriction._rtype = Restriction.RestrictionType.Enum;
                }
                else
                {
                }
                return(ssType);
            }


            private Type _type;
            internal String _baseName = null; //get rid of warning, not used for now
            private XMLNamespace _xns;
            internal Restriction _restriction;
            private String _fullRefName;
            private ArrayList _abstractElms = new ArrayList();
        }


        private abstract class ComplexSchemaType : SchemaType
        {
            internal ComplexSchemaType(String name, bool bSealed)
            {
                _name = name;
                _fullRefName = _name;
                _blockType = SchemaBlockType.ALL;
                _baseName = null;
                _elementName = name;
                _bSealed = bSealed;
                _particles = new ArrayList();
                _abstractElms = new ArrayList();
            }

            internal ComplexSchemaType(String name, SchemaBlockType blockType, bool bSealed)
            {
                _name = name;
                _fullRefName = _name;
                _blockType = blockType;
                _baseName = null;
                _elementName = name;
                _bSealed = bSealed;
                _particles = new ArrayList();
                _abstractElms = new ArrayList();
            }

            internal ComplexSchemaType(Type type)
            {
                Util.Log("ComplexSchemaType.ComplexSchemaType "+type);              
                _blockType = SchemaBlockType.ALL;
                _type = type;
                Init();
            }

            private void Init()
            {
                _name = _type.Name;
                _bSealed = _type.IsSealed;
                _baseName = null;
                _elementName = _name;
                _particles = new ArrayList();
                _abstractElms = new ArrayList();
                _fullRefName = WsdlGenerator.RefName(_type);
            }

            internal String Name{
                get{ return(_name);}
            }

            internal String FullRefName
            {
                get{ return(_fullRefName);}
            }

            protected String BaseName{
                get{ return(_baseName);}
                set{ _baseName = value;}
            }

            internal String ElementName{
                get{ return(_elementName);}
                set{ _elementName = value;}
            }


            protected bool IsSealed{
                get{ return(_bSealed);}
            }

            protected bool IsEmpty{
                get{
                    return((_abstractElms.Count == 0) &&
                           (_particles.Count == 0));
                }
            }

            internal void AddParticle(Particle particle){
                Util.Log("ComplexSchemaType.AddParticle "+particle.Name());                
                _particles.Add(particle);
            }

            protected void PrintBody(TextWriter textWriter, StringBuilder sb, String indent){
                Util.Log("ComplexSchemaType.PrintBody "+_name);             
                int particleCount = _particles.Count;
                String indent1 = IndentP(indent);
                String indent2 = IndentP(indent1);
                if (particleCount > 0)
                {
                    bool bPrintBlockElms = /*(particleCount > 1) && */(WsdlGenerator.blockDefault != _blockType);
                    if (bPrintBlockElms)
                    {
                        sb.Length = 0;
                        sb.Append(indent1);
                        sb.Append(schemaBlockBegin[(int) _blockType]);
                        textWriter.WriteLine(sb);
                    }

                    for (int i=0;i<particleCount; i++)
                        ((Particle) _particles[i]).Print(textWriter, sb, IndentP(indent2));

                    if (bPrintBlockElms)
                    {
                        sb.Length = 0;
                        sb.Append(indent1);
                        sb.Append(schemaBlockEnd[(int) _blockType]);
                        textWriter.WriteLine(sb);
                    }
                }

                int abstractElmCount = _abstractElms.Count;
                for (int i=0;i<abstractElmCount; i++)
                    ((IAbstractElement) _abstractElms[i]).Print(textWriter, sb, IndentP(indent));

                return;
            }

            private String _name;
            private Type _type;
            private String _fullRefName;
            private String _baseName;
            private String _elementName;
            private bool _bSealed;
            private SchemaBlockType _blockType;
            private ArrayList _particles;
            private ArrayList _abstractElms;

            static private String[] schemaBlockBegin = { "<all>", "<sequence>", "<choice>", "<complexContent>"};
            static private String[] schemaBlockEnd = { "</all>", "</sequence>", "</choice>", "</complexContent>"};
        }

        private class PhonySchemaType : ComplexSchemaType
        {
            internal PhonySchemaType(String name) : base(name, true)
            {
                Util.Log("PhonySchemaType.PhonySchemaType "+name);              
                _numOverloadedTypes = 0;
            }

            internal int OverloadedType(){
                Util.Log("PhonySchemaType.OverLoadedTypeType");             
                return(++_numOverloadedTypes);
            }

            internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous){
                Util.Log("PhonySchemaType.PrintSchemaType"); 
                Debug.Assert(bAnonymous == true, "PhonySchemaType should always be printed as anonymous types");

                // Wsdl Phony is not printed, instead the message section contains the parameters.
                return;
            }
            private int _numOverloadedTypes;
            internal ArrayList _inParamTypes;
            internal ArrayList _inParamNames;
            internal ArrayList _outParamTypes;
            internal ArrayList _outParamNames;
            internal ArrayList _paramNamesOrder;
            internal String _returnType;
            internal String _returnName;
        }

        private class ArraySchemaType : ComplexSchemaType
        {
            internal ArraySchemaType(Type type, String name, SchemaBlockType blockType, bool bSealed)
            : base(name, blockType, bSealed)
            {
                Util.Log("ArraySchemaType.ArrayComplexSchemaType");  
                _type = type;
            }

            internal Type Type
            {
                get { return _type;}
            }


            internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous)
            {
                Util.Log("ArrayType.PrintSchemaType");   
                String indent1 = IndentP(indent);
                sb.Length = 0;
                sb.Append(indent); 
                sb.Append("<complexType name='");
                sb.Append(FullRefName);
                sb.Append("\'>");
                textWriter.WriteLine(sb);
                PrintBody(textWriter, sb, indent1);
                sb.Length = 0;
                sb.Append(indent); 
                sb.Append("</complexType>");
                textWriter.WriteLine(sb);

                return;
            }

            private Type _type;
        }

        private class RealSchemaType : ComplexSchemaType
        {
            internal RealSchemaType(Type type, XMLNamespace xns, String serviceEndpoint, Hashtable typeToServiceEndpoint, bool bUnique, WsdlGenerator WsdlGenerator)
            : base(type)
            {
                Util.Log("RealSchemaType.RealSchemaType "+type+" xns "+xns.Name+" serviceEndpoint "+serviceEndpoint+" bUnique "+bUnique);               
                _type = type;
                _serviceEndpoint = serviceEndpoint;
                _typeToServiceEndpoint = typeToServiceEndpoint;
                _bUnique = bUnique;
                _WsdlGenerator = WsdlGenerator;
                _bStruct = type.IsValueType;
                _xns = xns;
                _implIFaces = null;
                _iFaces = null;
                _methods = null;
                _fields = null;
                _methodTypes = null;

                _nestedTypes = type.GetNestedTypes();
                if (_nestedTypes != null)
                {
                    foreach (Type ntype in _nestedTypes)
                    {
                        Util.Log("RealSchemaType.RealSchemaType nested classes"+ntype);
                        _WsdlGenerator.AddType(ntype, xns);
                    }
                }
            }

            internal Type Type{
                get{ return(_type);}
            }

            internal XMLNamespace XNS{
                get{ return(_xns);}
            }

            internal bool IsUnique{
                get{ return(_bUnique);}
            }

            internal bool IsSUDSType{
                /*
                get{ return((_fields == null) &&
                            ((_iFaces.Length > 0) || (_methods.Length > 0) ||
                             (_type.IsInterface) || (s_delegateType.IsAssignableFrom(_type))));}
                             */
                get{ return((_iFaces != null && _iFaces.Length > 0) || 
                            (_methods != null && _methods.Length > 0) ||
                             (_type != null && _type.IsInterface) || 
                            (s_delegateType != null && s_delegateType.IsAssignableFrom(_type)));}
            }

            internal Type[] GetIntroducedInterfaces(){
                Util.Log("RealSchemaType.GetIntroducedInterfaces");
                Debug.Assert(_iFaces == null, "variable set");
                _iFaces = GetIntroducedInterfaces(_type);
                return(_iFaces);
            }

            internal MethodInfo[] GetIntroducedMethods(){
                Util.Log("RealSchemaType.GetIntroducedMethods");                
                Debug.Assert(_methods == null, "variable set");
                _methods = GetIntroducedMethods(_type, ref _methodAttributes);
                _methodTypes = new String[2*_methods.Length];
                return(_methods);
            }

            internal FieldInfo[] GetInstanceFields(){
                Util.Log("RealSchemaType.GetInstanceFields");               
                Debug.Assert(_fields == null, "variable set");
                //Debug.Assert(!WsdlGenerator.s_marshalByRefType.IsAssignableFrom(_type), "Invalid Type");
                _fields = GetInstanceFields(_type);
                return(_fields);
            }

            private bool IsNotSystemDefinedRoot(Type type, Type baseType)
            {
                if (!type.IsInterface &&
                    !type.IsValueType &&
                    baseType != null &&
                    baseType.BaseType != null &&
                    baseType != WsdlGenerator.s_marshalByRefType &&
                    baseType != WsdlGenerator.s_valueType &&
                    baseType != WsdlGenerator.s_objectType &&
                    baseType != WsdlGenerator.s_contextBoundType &&
                    baseType != WsdlGenerator.s_remotingClientProxyType &&
                    baseType.FullName != "System.EnterpriseServices.ServicedComponent" &&
                    baseType.FullName != "System.__ComObject")
                    return true;
                else 
                    return false;
            }

            internal void Resolve(StringBuilder sb){
                Util.Log("RealSchemaType.Resolve "+_type);             
                sb.Length = 0;

                // Check if this is a suds type
                bool bSUDSType = IsSUDSType;


                // Resolve base type eliminating system defined roots of the class heirarchy

                Type baseType = _type.BaseType;
                if (IsNotSystemDefinedRoot(_type, baseType))
                {                    
                    Util.Log("RealSchemaType.Resolve Not System Defined root "+baseType);                                                     
                    XMLNamespace xns = _WsdlGenerator.GetNamespace(baseType);
                    Debug.Assert(xns != null, "Namespace should have been found");
                    sb.Append(xns.Prefix);
                    sb.Append(':');
                    sb.Append(baseType.Name);
                    BaseName = sb.ToString();
                    if (bSUDSType)
                        _xns.DependsOnSUDSNS(xns);
                    Type ltype= _type; 
                    Type lbaseType = ltype.BaseType;
                    while(lbaseType != null && IsNotSystemDefinedRoot(ltype, lbaseType))
                    {
                        if (_typeToServiceEndpoint != null && !_typeToServiceEndpoint.ContainsKey(lbaseType.Name) && _typeToServiceEndpoint.ContainsKey(ltype.Name))
                        {
                            // type contains endpoints, but baseType doesn't, so assign the type's endpoints to the baseType.
                            // This is needed when a child has endpoints but the parent doesn't. A cast to the parent won't 
                            // find the object's endpoint
                            _typeToServiceEndpoint[lbaseType.Name] = _typeToServiceEndpoint[ltype.Name];
                        }
                        ltype = lbaseType;
                        lbaseType = ltype.BaseType;
                    }

                    Util.Log("RealSchemaType.Resolve Not System Defined root BaseName "+BaseName);                                 
                }

                // The element definition of this type depends on itself
                _xns.DependsOnSchemaNS(_xns, false);

                if (bSUDSType)
                {
                    Util.Log("RealSchemaType.Resolve AddRealSUDSType  "+_type);                                                     
                    _xns.AddRealSUDSType(this);

                    // Resolve interfaces introduced by this type
                    if (_iFaces.Length > 0)
                    {
                        _implIFaces = new String[_iFaces.Length];
                        for (int i=0;i<_iFaces.Length;i++)
                        {
                            String ns;
                            Assembly assem;
                            Util.Log("RealSchemaType.Resolve iFace  "+_iFaces[i].Name);                                                                                     
                            bool bInteropType = WsdlGenerator.GetNSAndAssembly(_iFaces[i], out ns, out assem);
                            XMLNamespace xns = _xns.LookupSchemaNamespace(ns, assem);
                            Debug.Assert(xns != null, "SchemaType should have been found");
                            sb.Length = 0;
                            sb.Append(xns.Prefix);
                            sb.Append(':');
                            sb.Append(_iFaces[i].Name);
                            _implIFaces[i] = sb.ToString();
                            _xns.DependsOnSUDSNS(xns);
                        }
                    }

                    // Resolve methods introduced by this type
                    if (_methods.Length > 0)
                    {
                        String useNS = null;
                        if (_xns.IsInteropType)
                            useNS = _xns.Name;
                        else
                        {
                            sb.Length = 0;
                            WsdlGenerator.QualifyName(sb, _xns.Name, Name);
                            useNS = sb.ToString();
                        }
                        XMLNamespace methodXNS = _xns.LookupSchemaNamespace(useNS, _xns.Assem);
                        Debug.Assert(methodXNS != null, "Namespace is null");
                        _xns.DependsOnSUDSNS(methodXNS);
                        _phony = new PhonySchemaType[_methods.Length];
                        for (int i=0;i<_methods.Length;i++)
                        {
                            // Process the request
                            MethodInfo method = _methods[i];
                            String methodRequestName = method.Name;

                            ParameterInfo[] parameters = method.GetParameters();
                            PhonySchemaType methodRequest = new PhonySchemaType(methodRequestName);

                            // Wsdl 
                            Util.Log("RealSchemaType.Resolve Wsdl  "+methodRequestName);
                            methodRequest._inParamTypes = new ArrayList(10); 
                            methodRequest._inParamNames = new ArrayList(10); 
                            methodRequest._outParamTypes = new ArrayList(10);
                            methodRequest._outParamNames = new ArrayList(10);
                            methodRequest._paramNamesOrder = new ArrayList(10);

                            int paramNameCnt = 0;
                            foreach (ParameterInfo param in parameters){
                                bool bmarshalIn = false;
                                bool bmarshalOut = false;
                                methodRequest._paramNamesOrder.Add(param.Name);
                                ParamInOut(param, out bmarshalIn, out bmarshalOut);

                                Type type = param.ParameterType;
                                String paramName = param.Name;
                                if (paramName == null || paramName.Length == 0)
                                    paramName = "param"+paramNameCnt++;

                                // Find the Wsdl type name

                                String stringType = TypeName(type, true, methodXNS);
                                // add to the method parameters
                                if (bmarshalIn)
                                {
                                    methodRequest._inParamNames.Add(paramName);
                                    methodRequest._inParamTypes.Add(stringType);
                                }

                                if (bmarshalOut)
                                {
                                    methodRequest._outParamNames.Add(paramName);
                                    methodRequest._outParamTypes.Add(stringType);
                                }
                            }


                            methodXNS.AddPhonySchemaType(methodRequest);
                            _phony[i] = methodRequest;
                            _methodTypes[2*i] = methodRequest.ElementName;

                            if (!RemotingServices.IsOneWay(method))
                            {
                                // Process response (look at custom attributes to get values

                                String returnName = null;
                                SoapMethodAttribute soapAttribute = (SoapMethodAttribute)InternalRemotingServices.GetCachedSoapAttribute(method);
                                if (soapAttribute.ReturnXmlElementName != null)
                                    returnName = soapAttribute.ReturnXmlElementName;
                                else
                                    returnName = "return";

                                String responseName = null;
                                if (soapAttribute.ResponseXmlElementName != null)
                                    responseName = soapAttribute.ResponseXmlElementName;
                                else
                                    responseName = methodRequestName + "Response";

                                PhonySchemaType methodResponse = new PhonySchemaType(responseName);

                                //Wsdl type
                                // out paramters alread processed above.
                                // return name stored in methodRequest PhonySchemaType
                                methodRequest._returnName = returnName;
                                Type returnType = method.ReturnType;

                                if (!((returnType == null) || (returnType == typeof(void))))
                                {
                                    methodRequest._returnType = TypeName(returnType, true, methodXNS);
                                }
                                methodXNS.AddPhonySchemaType(methodResponse);
                                _methodTypes[2*i+1] = methodResponse.ElementName;
                            }
                        }
                    }
                }

                // Resolve fields
                if (_fields != null)
                {
                    for (int i=0;i<_fields.Length;i++)
                    {
                        FieldInfo field = _fields[i];
                        Debug.Assert(!field.IsStatic, "Static field");
                        Type fieldType = field.FieldType;
                        if (fieldType == null)
                            fieldType = typeof(Object);
                        Util.Log("RealSchemaType.Resolve fields  "+field.Name+" type "+fieldType);                                                                                                              
                        AddParticle(new SchemaElement(field.Name, fieldType, false, _xns));
                    }
                }

                // Resolve attribute elements                
                return;
            }

            private void ParamInOut(ParameterInfo param, out bool bMarshalIn, out bool bMarshalOut)
            {
                bool bIsIn = param.IsIn;    // [In]
                bool bIsOut = param.IsOut;  // [Out]  note: out int a === [Out] ref int b

                bool bIsByRef = param.ParameterType.IsByRef; // (ref or normal)      

                bMarshalIn = false;
                bMarshalOut = false;
                if (bIsByRef)
                {
                    if (bIsIn == bIsOut)
                    {
                        // "ref int a" or "[In, Out] ref int a"
                        bMarshalIn = true;
                        bMarshalOut = true;
                    }
                    else
                    {
                        // "[In] ref int a" or "out int a"
                        bMarshalIn = bIsIn;     
                        bMarshalOut = bIsOut;  
                    }
                }
                else
                {
                    // "int a" or "[In, Out] a"
                    bMarshalIn = true;     
                    bMarshalOut = bIsOut;
                }
                Util.Log("RealSchemaType.ParamInOut "+param.Name+" ref,in,out "+bIsByRef+","+bIsIn+","+bIsOut+" bMarshalIn,bMarshalOut "+bMarshalIn+","+bMarshalOut);             
            }


            internal override void PrintSchemaType(TextWriter textWriter, StringBuilder sb, String indent, bool bAnonymous)
            {
                Util.Log("RealSchemaType.PrintSchemaType");             
                if (bAnonymous == false)
                {
                    sb.Length = 0;
                    sb.Append(indent);
                    sb.Append("<element name='");
                    sb.Append(ElementName);
                    sb.Append("' type='");
                    sb.Append(_xns.Prefix);
                    sb.Append(':');
                    sb.Append(FullRefName);
                    sb.Append("'/>");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indent);
                if (bAnonymous == false)
                {
                    sb.Append("<complexType name='");
                    sb.Append(FullRefName);
                    sb.Append('\'');
                }
                else
                {
                    sb.Append("<complexType ");
                }
                if (BaseName != null)
                {
                    sb.Append(" base='");
                    sb.Append(BaseName);
                    sb.Append('\'');
                }
                if ((IsSealed == true) &&
                    (bAnonymous == false))
                    sb.Append(" final='#all'");
                bool bEmpty = IsEmpty;
                if (bEmpty)
                    sb.Append("/>");
                else
                    sb.Append('>');
                textWriter.WriteLine(sb);
                if (bEmpty)
                    return;

                base.PrintBody(textWriter, sb, indent);

                textWriter.Write(indent);
                textWriter.WriteLine("</complexType>");

                return;
            }


            internal void PrintMessageWsdl(TextWriter textWriter, StringBuilder sb, String indent, ArrayList refNames)
            {
                Util.Log("RealSchemaType.PrintMessageWsdl "+Name);

                String indent1 = IndentP(indent);
                String indent2 = IndentP(indent1);
                String indent3 = IndentP(indent2);
                String ns = null;
                String nsPrefix = null;
                MethodInfo method = null;
                String methodName = null;
                String overloadedName = null;
                bool bIsOneWay = false;

                String useNS = null;
                if (_xns.IsInteropType)
                    useNS = _xns.Name;
                else
                {
                    sb.Length = 0;

                    WsdlGenerator.QualifyName(sb, _xns.Name, Name);
                    useNS = sb.ToString();
                }
                XMLNamespace methodXns = _xns.LookupSchemaNamespace(useNS, _xns.Assem);

                //Debug.Assert(methodXns != null, "Namespace is null");

                int methodsLength = 0;
                if (_methods != null)
                    methodsLength = _methods.Length;

                if (methodsLength > 0)
                {
                    ns = methodXns.Namespace;
                    nsPrefix = methodXns.Prefix;
                }

                refNames.Add(Name);

                for (int i=0;i<methodsLength;i++)
                {
                    method = _methods[i];
                    bIsOneWay = RemotingServices.IsOneWay(method);
                    methodName = PrintMethodName(method);
                    sb.Length = 0;
                    WsdlGenerator.QualifyName(sb, Name, _methodTypes[2*i]);
                    overloadedName = sb.ToString();

                    // Message element
                    sb.Length = 0;
                    sb.Append("\n");
                    sb.Append(indent);
                    sb.Append("<message name='");
                    sb.Append(overloadedName+"Input");
                    sb.Append("'>");
                    textWriter.WriteLine(sb);

                    PhonySchemaType phony = _phony[i];


                    if (phony._inParamTypes != null)
                    {
                        for (int iparam=0; iparam<phony._inParamTypes.Count; iparam++)
                        {
                            sb.Length = 0;
                            sb.Append(indent1);
                            sb.Append("<part name='");
                            sb.Append(phony._inParamNames[iparam]);
                            sb.Append("' type='");
                            sb.Append(phony._inParamTypes[iparam]);
                            sb.Append("'/>");
                            textWriter.WriteLine(sb);
                        }

                        sb.Length = 0;
                        sb.Append(indent);
                        sb.Append("</message>");
                        textWriter.WriteLine(sb);

                        if (!bIsOneWay)
                        {
                            sb.Length = 0;
                            sb.Append(indent);
                            sb.Append("<message name='");
                            sb.Append(overloadedName+"Output");
                            sb.Append("'>");
                            textWriter.WriteLine(sb);

                            if (phony._returnType != null || phony._outParamTypes != null)
                            {
                                if (phony._returnType != null)
                                {
                                    sb.Length = 0;
                                    sb.Append(indent1);
                                    sb.Append("<part name='");
                                    sb.Append(phony._returnName);
                                    sb.Append("' type='");
                                    sb.Append(phony._returnType);
                                    sb.Append("'/>");
                                    textWriter.WriteLine(sb);
                                }

                                if (phony._outParamTypes != null)
                                {
                                    for (int iparam=0; iparam<phony._outParamTypes.Count; iparam++)
                                    {
                                        sb.Length = 0;
                                        sb.Append(indent1);
                                        sb.Append("<part name='");
                                        sb.Append(phony._outParamNames[iparam]);
                                        sb.Append("' type='");
                                        sb.Append(phony._outParamTypes[iparam]);
                                        sb.Append("'/>");
                                        textWriter.WriteLine(sb);
                                    }
                                }
                            }

                            sb.Length = 0;
                            sb.Append(indent);
                            sb.Append("</message>");
                            textWriter.WriteLine(sb);
                        }
                    }
                }

                // PortType Element
                sb.Length = 0;
                sb.Append("\n");                    
                sb.Append(indent);
                sb.Append("<portType name='");
                sb.Append(Name);
                sb.Append("PortType");
                sb.Append("'>");
                textWriter.WriteLine(sb);

                for (int i=0;i<methodsLength;i++)
                {
                    method = _methods[i];
                    PhonySchemaType phony = _phony[i];

                    bIsOneWay = RemotingServices.IsOneWay(method);                  
                    methodName = PrintMethodName(method);
                    sb.Length = 0;                    
                    sb.Append("tns:");
                    WsdlGenerator.QualifyName(sb, Name, _methodTypes[2*i]);
                    overloadedName = sb.ToString();

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("<operation name='");
                    sb.Append(methodName);
                    sb.Append("'");
                    if (phony != null && phony._paramNamesOrder.Count > 0)
                    {

                        sb.Append(" parameterOrder='");
                        bool bfirst = true;
                        foreach (String param in phony._paramNamesOrder)
                        {
                            if (!bfirst)
                                sb.Append(" ");
                            sb.Append(param);
                            bfirst = false;
                        }
                        sb.Append("'");
                    }

                    sb.Append(">");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("<input name='");
                    sb.Append(_methodTypes[2*i]);
                    sb.Append("Request' ");                 
                    sb.Append("message='");
                    sb.Append(overloadedName);
                    sb.Append("Input");
                    sb.Append("'/>");
                    textWriter.WriteLine(sb);

                    if (!bIsOneWay)
                    {
                        sb.Length = 0;
                        sb.Append(indent2);
                        sb.Append("<output name='");
                        sb.Append(_methodTypes[2*i]);
                        sb.Append("Response' ");                    

                        sb.Append("message='");
                        sb.Append(overloadedName);
                        sb.Append("Output");
                        sb.Append("'/>");
                        textWriter.WriteLine(sb);
                    }

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("</operation>");
                    textWriter.WriteLine(sb);
                }

                sb.Length = 0;
                sb.Append(indent);
                sb.Append("</portType>");
                textWriter.WriteLine(sb);


                // Binding 
                sb.Length = 0;
                sb.Append("\n");                    
                sb.Append(indent);
                sb.Append("<binding name='");
                sb.Append(Name);
                sb.Append("Binding");
                sb.Append("' ");
                sb.Append("type='tns:");                
                sb.Append(Name);
                sb.Append("PortType");
                sb.Append("'>");
                textWriter.WriteLine(sb);

                sb.Length = 0;
                sb.Append(indent1);
                sb.Append("<soap:binding style='rpc' transport='http://schemas.xmlsoap.org/soap/http'/>");
                textWriter.WriteLine(sb);

                if (_type.IsInterface || IsSUDSType)
                    PrintSuds(_type, _implIFaces, _nestedTypes, textWriter, sb, indent); // Some namespaces have no suds types


                if (!_xns.IsClassesPrinted)
                {
                    for (int i=0;i<_xns._realSchemaTypes.Count;i++)
                    {
                        RealSchemaType rsType = (RealSchemaType) _xns._realSchemaTypes[i];
                        Type type = rsType._type;
                        Util.Log("RealSchemaType.PrintMessageWsd suds realSchemaType "+type);
                        if (!rsType.Type.IsInterface && !rsType.IsSUDSType)
                        {
                            Util.Log("RealSchemaType.PrintMessageWsd suds realSchemaType 2 "+type.BaseType+" "+typeof(MulticastDelegate).IsAssignableFrom(type));;

                            Type[] iFaces = GetIntroducedInterfaces(rsType._type);
                            String[] implIFaces = null;
                            bool bUsedFaces = false;
                            if (iFaces.Length > 0)
                            {
                                implIFaces = new String[iFaces.Length];
                                for (int j=0;i<iFaces.Length;i++)
                                {
                                    String fns;
                                    Assembly fassem;
                                    Util.Log("RealSchemaType.PrintMessageWsdl iFace  "+iFaces[j].Name);
                                    bool bInteropType = WsdlGenerator.GetNSAndAssembly(iFaces[j], out fns, out fassem);
                                    XMLNamespace xns = _xns.LookupSchemaNamespace(fns, fassem);
                                    Debug.Assert(xns != null, "SchemaType should have been found");
                                    sb.Length = 0;
                                    sb.Append(xns.Prefix);
                                    sb.Append(':');
                                    sb.Append(iFaces[j].Name);
                                    implIFaces[j] = sb.ToString();
                                    if (implIFaces[j].Length > 0)
                                        bUsedFaces = true;
                                }
                            }
                            if (!bUsedFaces)
                                implIFaces = null;

                            PrintSuds(type, implIFaces, rsType._nestedTypes, textWriter, sb, indent);
                        }
                    }
                    _xns.IsClassesPrinted = true;
                }


                for (int i=0;i<methodsLength;i++)
                {
                    method = _methods[i];
                    methodName = PrintMethodName(method);
                    bIsOneWay = RemotingServices.IsOneWay(method);                                      

                    //binding operation
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("<operation name='");
                    sb.Append(methodName);
                    sb.Append("'>");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("<soap:operation soapAction='");
                    String soapAction = SoapServices.GetSoapActionFromMethodBase(method);
                    if ((soapAction != null) || (soapAction.Length > 0))
                    {
                        sb.Append(soapAction);
                    }
                    else
                    {
                        sb.Append(ns);
                        sb.Append('#');
                        sb.Append(methodName);
                    }
                    sb.Append("'/>");
                    textWriter.WriteLine(sb);

                    if (_methodAttributes != null && (i < _methodAttributes.Length) && _methodAttributes[i] != null)
                    {
                        // Suds for method attributes
                        // Attributes are only for public methods,
                        //  _method contains public and additional qualified interface methods
                        // The public methods are at the beginning of _methods
                        sb.Length = 0;
                        sb.Append(indent2);
                        sb.Append("<suds:method attributes='");
                        sb.Append(_methodAttributes[i]);
                        sb.Append("'/>");
                        textWriter.WriteLine(sb);
                    }

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("<input name='");
                    sb.Append(_methodTypes[2*i]);
                    sb.Append("Request'>");                 
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent3);
                    sb.Append("<soap:body use='encoded' encodingStyle='http://schemas.xmlsoap.org/soap/encoding/' namespace='");
                    String interopNamespace = SoapServices.GetXmlNamespaceForMethodCall(method);
		            if (interopNamespace == null)
			            sb.Append(ns);
		            else
			            sb.Append(interopNamespace);
                    sb.Append("'/>");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("</input>");
                    textWriter.WriteLine(sb);

                    if (!bIsOneWay)
                    {
                        sb.Length = 0;
                        sb.Append(indent2);
                        sb.Append("<output name='");
                        sb.Append(_methodTypes[2*i]);
                        sb.Append("Response'>");                                        
                        textWriter.WriteLine(sb);

                        sb.Length = 0;
                        sb.Append(indent3);
                        sb.Append("<soap:body use='encoded' encodingStyle='http://schemas.xmlsoap.org/soap/encoding/' namespace='");
                        interopNamespace = SoapServices.GetXmlNamespaceForMethodResponse(method);
			            if (interopNamespace == null)
			                sb.Append(ns);
			            else
			                sb.Append(interopNamespace);
                        sb.Append("'/>");
                        textWriter.WriteLine(sb);

                        sb.Length = 0;
                        sb.Append(indent2);
                        sb.Append("</output>");
                        textWriter.WriteLine(sb);
                    }

                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append("</operation>");
                    textWriter.WriteLine(sb);
                }

                sb.Length=0;
                sb.Append(indent);
                sb.Append("</binding>");
                textWriter.WriteLine(sb);                       
            }

            private void PrintSuds(Type type, String[] implIFaces, Type[] nestedTypes, TextWriter textWriter, StringBuilder sb, String indent)
            {
                Util.Log("RealSchemaType.PrintSuds  "+type+" implIFaces "+implIFaces+" nestedTypes "+nestedTypes);
                String indent1 = IndentP(indent);
                String indent2 = IndentP(indent1);
                String indent3 = IndentP(indent2);

                String sudsEnd = null;
                // Type, interface, extends information
                sb.Length = 0;
                sb.Append(indent1);
                if (type.IsInterface)
                {
                    sb.Append("<suds:interface type='");
                    sudsEnd = "</suds:interface>";


                }
                else if (type.IsValueType)
                {
                    sb.Append("<suds:struct type='");
                    sudsEnd = "</suds:struct>";
                }
                else
                {
                    sb.Append("<suds:class type='");
                    sudsEnd = "</suds:class>";
                }
                sb.Append(_xns.Prefix);
                sb.Append(':');
                sb.Append(WsdlGenerator.RefName(type));
                sb.Append("'");

                Type baseType = type.BaseType;
                if (IsNotSystemDefinedRoot(type, baseType))
                {
                    XMLNamespace xns = _WsdlGenerator.GetNamespace(baseType);
                    sb.Append(" extends='");
                    sb.Append(xns.Prefix);
                    sb.Append(':');
                    sb.Append(baseType.Name);
                    sb.Append("'");

                }

                if (baseType != null && baseType.FullName == "System.EnterpriseServices.ServicedComponent")
                    sb.Append(" rootType='ServicedComponent'");
                else if (typeof(Delegate).IsAssignableFrom(type) || typeof(MulticastDelegate).IsAssignableFrom(type))
                    sb.Append(" rootType='Delegate'");
                else if (typeof(MarshalByRefObject).IsAssignableFrom(type))
                    sb.Append(" rootType='MarshalByRefObject'");
                else if (typeof(ISerializable).IsAssignableFrom(type))
                    sb.Append(" rootType='ISerializable'");

                if (implIFaces == null && nestedTypes == null)
                    sb.Append("/>");
                else
                    sb.Append(">");

                textWriter.WriteLine(sb);

                String extendAttribute = null;
                if (type.IsInterface)
                    extendAttribute = "<suds:extends type='";
                else
                    extendAttribute = "<suds:implements type='";


                if (implIFaces != null)
                {
                    for (int j=0;j<implIFaces.Length;j++)
                    {
                        if (!(implIFaces[j] == null || implIFaces[j] == String.Empty))
                        {
                            sb.Length = 0;
                            sb.Append(indent2);                                 
                            sb.Append(extendAttribute);
                            sb.Append(implIFaces[j]);
                            sb.Append("'/>");
                            textWriter.WriteLine(sb);
                        }
                    }
                }

                if (nestedTypes != null)
                {
                    for (int j=0;j<nestedTypes.Length;j++)
                    {
                            sb.Length = 0;
                            sb.Append(indent2);                                 
                            sb.Append("<suds:nestedType name='");
                            sb.Append(nestedTypes[j].Name);
                            sb.Append("' type='");
                            sb.Append(_xns.Prefix);
                            sb.Append(':');
                            sb.Append(WsdlGenerator.RefName(nestedTypes[j]));
                            sb.Append("'/>");
                            textWriter.WriteLine(sb);
                    }
                }

                if (implIFaces != null || nestedTypes != null)
                {
                    sb.Length = 0;
                    sb.Append(indent1);
                    sb.Append(sudsEnd);
                    textWriter.WriteLine(sb);
                }
            }


            private static String ProcessArray(Type type, XMLNamespace xns)
            {
                Util.Log("RealSchemaType.ProcessArray Enter "+type);
                String qname = null;
                bool bbinary = false;
                Type elementType = type.GetElementType();
                String elementTypeName = "ArrayOf";
                while (elementType.IsArray)
                {
                    elementTypeName = elementTypeName+"ArrayOf";
                    elementType = elementType.GetElementType();
                }

                qname = RealSchemaType.TypeName(elementType, true, xns);
                int index = qname.IndexOf(":");
                String prefix = qname.Substring(0, index);
                String wireName = qname.Substring(index+1);
                Util.Log("RealSchemaType.ProcessArray qname "+qname+" wirename "+wireName);
                int rank =  type.GetArrayRank();
                String rankStr = "";
                if (rank > 1)
                    rankStr = rank.ToString(CultureInfo.InvariantCulture);
                String csname =elementTypeName+wireName.Substring(0,1).ToUpper(CultureInfo.InvariantCulture)+wireName.Substring(1)+rankStr;
                csname = csname.Replace('+','N'); // need to get rid of + in nested classes
                ArraySchemaType ast = xns.LookupArraySchemaType(csname); 
                if (ast == null)
                {
                    ArraySchemaType cstype = new ArraySchemaType(type, csname, SchemaBlockType.ComplexContent, false);
                    Restriction restriction = new Restriction();
                    SchemaAttribute attribute = new SchemaAttribute();
                    if (bbinary)
                        attribute.AddArray(qname);
                    else
                    {
                        String arrayTypeName = type.Name;
                        index = arrayTypeName.IndexOf("[");
                        attribute.AddArray(qname+arrayTypeName.Substring(index));
                    }

                    restriction.AddArray(attribute);
                    cstype.AddParticle(restriction);
                    xns.AddArraySchemaType(cstype);
                }

                String returnStr = xns.Prefix+":"+csname;
                Util.Log("RealSchemaType.ProcessArray Exit "+returnStr);
                return returnStr;
            }



            internal static String TypeName(Type type, bool bEmbedded, XMLNamespace thisxns)
            {
                Util.Log("RealSchemaType.TypeName entry "+type+" bEmbedded "+bEmbedded+" xns "+thisxns.Name);              
                String typeName = null;
                if (type.IsArray)
                    return ProcessArray(type, thisxns);

                String clrTypeName = WsdlGenerator.RefName(type);
                Type clrType = type;

                // If ref type the name ends in &
                if (type.IsByRef)
                {
                    clrType = type.GetElementType(); 
                    clrTypeName = WsdlGenerator.RefName(clrType);
                    if (clrType.IsArray)
                        return ProcessArray(clrType, thisxns);
                }

                typeName = SudsConverter.MapClrTypeToXsdType(clrType);

                if (typeName == null)
                {
                    String ns = type.Namespace;
                    Assembly assem = type.Module.Assembly; 
                    XMLNamespace xns = null;
                    Util.Log("RealSchemaType.TypeName realNS "+ns);
                    xns = (XMLNamespace)thisxns.Generator._typeToInteropNS[type];

                    if (xns == null)
                    {
                        xns = thisxns.LookupSchemaNamespace(ns, assem);
                        if (xns == null)
                        {
                            xns = thisxns.Generator.LookupNamespace(ns,assem);
                            if (xns == null)
                            {
                                xns = thisxns.Generator.AddNamespace(ns, assem);
                            }
                            thisxns.DependsOnSchemaNS(xns, false);
                        }
                        Util.Log("RealSchemaType.TypeName depended NS with assem equals "+xns.Name);
                    }
                    StringBuilder sb = new StringBuilder(256);

                    sb.Append(xns.Prefix);
                    sb.Append(':');
                    sb.Append(clrTypeName);
                    typeName = sb.ToString();
                }

                Util.Log("RealSchemaType.TypeName exit "+typeName);
                return typeName;
            }
            

            static private Type[] GetIntroducedInterfaces(Type type)
            {
                ArrayList ifaceA = new ArrayList();
                Type[] typeA = type.GetInterfaces();
                // remove system interfaces.
                foreach (Type itype in typeA)
                {
                    if (!itype.FullName.StartsWith("System."))
                    {
                        ifaceA.Add(itype);
                        Util.Log("RealSchemaType.GetIntroducedInterfaces "+type+" Interfaces "+itype);
                    }
                }

                Util.Log("RealSchemaType.GetIntroducedInterfaces "+type+" typeInterface? "+type.IsInterface+" number of interfaces "+typeA.Length);               

                Type[] ifaceTypes = new Type[ifaceA.Count];
                for(int i=0; i<ifaceA.Count; i++)
                   ifaceTypes[i] = (Type)ifaceA[i];
                return ifaceTypes;
            }

            static private void FindMethodAttributes(Type type, MethodInfo[] infos, ref String[] methodAttributes, BindingFlags bFlags)
            {
                Util.Log("RealSchemaType.FindMethodAttributes Enter "+type);
                Type baseType = type;
                ArrayList inherit = new ArrayList();
                while (true)
                {
                    baseType = baseType.BaseType;

                    Util.Log("RealSchemaType.FindMethodAttributes baseType "+baseType);
                    if (baseType != null && !baseType.FullName.StartsWith("System."))
                        inherit.Add(baseType);
                    else
                        break;
                }

                StringBuilder sb = new StringBuilder();
                for(int i=0; i<infos.Length; i++)
                {
                    MethodBase info = (MethodBase)infos[i];
                    sb.Length = 0;
                    MethodAttributes ma = info.Attributes;
                    bool bVirtual = info.IsVirtual;
                    bool bNewSlot = ((ma & MethodAttributes.NewSlot) == MethodAttributes.NewSlot);
                    Util.Log("RealSchemaType.FindMethodAttributes "+info.Name+" bVirtual "+bVirtual+" bNewSlot "+bNewSlot+" hidebysig "+info.IsHideBySig);
                    if (info.IsPublic)
                        sb.Append("public");
                    else if (info.IsFamily)
                        sb.Append("protected");
                    else if (info.IsAssembly)
                        sb.Append("internal");
                    
                    // See if method hides inherited methods
                    bool bHides = false;
                    for (int j=0; j<inherit.Count; j++)
                    {
                        baseType = (Type)inherit[j];
                        ParameterInfo[] paramInfos = info.GetParameters();
                        Type[] types = new Type[paramInfos.Length];
                        for (int itype=0; itype<types.Length; itype++)
                        {
                            types[itype] = paramInfos[itype].ParameterType;
                        }
                        MethodInfo baseInfo = baseType.GetMethod(info.Name, types);
                        if (baseInfo != null)
                        {
                            // Hides
                            if (sb.Length > 0)
                                sb.Append(" ");
                            if (bNewSlot || baseInfo.IsFinal)
                                sb.Append("new");
                            else if (baseInfo.IsVirtual && bVirtual)
                                sb.Append("override");
                            else 
                                sb.Append("new");
                            bHides = true;
                            break;
                        }
                    }
                    if (!bHides && bVirtual)
                    {
                        if (sb.Length > 0)
                            sb.Append(" ");
                        sb.Append("virtual");
                    }

                    if (sb.Length > 0)
                    {
                        methodAttributes[i] = sb.ToString();
                        Util.Log("RealSchemaType.FindMethodAttributes Exit "+info.Name+" "+methodAttributes[i]);     
                    }
                }
            }

            static private MethodInfo[] GetIntroducedMethods(Type type, ref String[] methodAttributes)
            {
                Util.Log("RealSchemaType.GetIntroducedMethods "+type);     


                // Methods in the class are either the class public methods or interface qualified methods

                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public;
                MethodInfo[] methodInfos = type.GetMethods(bFlags); //public methods (including unqualified interface methods)

                if (type.IsInterface)
                    return methodInfos;

                // Find method attributes for public  methods
                methodAttributes = new String[methodInfos.Length];
                FindMethodAttributes(type, methodInfos, ref methodAttributes, bFlags);

                // Get any class methods which are interface qualifed methods.
                // interface qualified methods are not public in the metadata.
                ArrayList additionalInfos = new ArrayList();
                Type[] itypeA = type.GetInterfaces();
                foreach (Type itype in itypeA )
                {
                    InterfaceMapping im = type.GetInterfaceMap(itype);
                    foreach (MethodInfo mi in im.TargetMethods)
                    {
                        if (!mi.IsPublic && type.GetMethod(mi.Name, bFlags | BindingFlags.NonPublic) != null)
                        {
                            additionalInfos.Add(mi);
                        }
                    }
                }

                // Combine all the methodinfos into one structure
                MethodInfo[] finalMethodInfos = null;
                if (additionalInfos.Count > 0)
                {
                    finalMethodInfos = new MethodInfo[methodInfos.Length + additionalInfos.Count];
                    for(int i=0; i<methodInfos.Length; i++)
                        finalMethodInfos[i] = methodInfos[i];
                    for(int i=0; i<additionalInfos.Count; i++)
                        finalMethodInfos[methodInfos.Length+i] = (MethodInfo)additionalInfos[i];
                }
                else
                    finalMethodInfos = methodInfos;

                return finalMethodInfos;
            }

            internal static String PrintMethodName(MethodInfo methodInfo)
            {
                String methodName = methodInfo.Name;
                int lastDot = 0;
                int prevDot = 0;

                for (int i=0; i<methodName.Length; i++)
                {
                    if (methodName[i] == '.')
                    {
                        prevDot = lastDot;
                        lastDot = i;
                    }
                }

                String iname = methodName;

                if (prevDot > 0)
                    iname = methodName.Substring(prevDot+1);

                return iname;
            }

            static private FieldInfo[] GetInstanceFields(Type type){
                Util.Log("RealSchemaType.GetIntroducedFields "+type);                               

                BindingFlags bFlags = BindingFlags.DeclaredOnly | BindingFlags.Instance |
                                      BindingFlags.Public;

                if (!s_marshalByRefType.IsAssignableFrom(type))
                    bFlags |= BindingFlags.NonPublic;

                FieldInfo[] fields = type.GetFields(bFlags);
                Util.Log("RealSchemaType.GetIntroducedFields length "+fields.Length);                               
                int actualLength = fields.Length;
                if (actualLength == 0)
                    return(emptyFieldSet);

                for (int i=0;i<fields.Length;i++)
                {
                    Util.Log("RealSchemaType.GetInstanceFields field "+fields[i].Name+" "+fields[i].FieldType+" type "+type);
                    if (fields[i].IsStatic)
                    {
                        Debug.Assert(false, "Static Field");
                        Util.Log("RealSchemaType.GetInstanceFields field  static "+fields[i].FieldType);
                        --actualLength;
                        fields[i] = fields[actualLength];
                        fields[actualLength] = null;
                    }
                }

                if (actualLength < fields.Length)
                {
                    FieldInfo[] ifields = new FieldInfo[actualLength];
                    Array.Copy(fields, ifields, actualLength);
                    Util.Log("RealSchemaType.GetInstanceFields adjust length "+actualLength);
                    return(ifields);
                }

                return(fields);
            }

            // Instance fields
            private WsdlGenerator _WsdlGenerator;
            private Type _type;
            private String _serviceEndpoint;
            private Hashtable _typeToServiceEndpoint;
            private bool _bUnique;
            private XMLNamespace _xns;
            private bool _bStruct;
            private String[] _implIFaces;

            private Type[] _iFaces;
            private MethodInfo[] _methods;
            private String[] _methodAttributes;
            private String[] _methodTypes;
            private FieldInfo[] _fields;
            private PhonySchemaType[] _phony;
            internal Type[] _nestedTypes;

            // Static fields
            private static Type[] emptyTypeSet = new Type[0];
            private static MethodInfo[] emptyMethodSet = new MethodInfo[0];
            private static FieldInfo[] emptyFieldSet = new FieldInfo[0];
        }

        private class XMLNamespace
        {
            internal XMLNamespace(String name, Assembly assem, String serviceEndpoint, Hashtable typeToServiceEndpoint, String prefix, bool bInteropType, WsdlGenerator generator ){
                Util.Log("XMLNamespace.XMLNamespace Enter "+name+" serviceEndpoint "+serviceEndpoint+" prefix "+prefix+" bInteropType "+bInteropType);
                _name = name;
                _assem = assem;
                _bUnique = false;
                _bInteropType = bInteropType;
                _generator = generator;
                StringBuilder sb = new StringBuilder(256);
                Assembly systemAssembly = typeof(String).Module.Assembly;

                // Remove leading . for an empty namespace

                if (!_bInteropType)
                {
                    if (assem == systemAssembly)
                    {
                        sb.Append(SoapServices.CodeXmlNamespaceForClrTypeNamespace(name, null));
                    }
                    else if (assem != null)
                    {
                        sb.Append(SoapServices.CodeXmlNamespaceForClrTypeNamespace(name, assem.FullName));
                    }
                }
                else
                {
                    sb.Append(name);
                }
                _namespace = sb.ToString();
                _prefix = prefix;
                _dependsOnSchemaNS = new ArrayList();
                _realSUDSTypes = new ArrayList();
                _dependsOnSUDSNS = new ArrayList();
                _realSchemaTypes = new ArrayList();
                _phonySchemaTypes = new ArrayList();
                _simpleSchemaTypes = new ArrayList();
                _arraySchemaTypes = new ArrayList();
                _xnsImports = new ArrayList();
                _serviceEndpoint = serviceEndpoint;
                _typeToServiceEndpoint = typeToServiceEndpoint;
                Util.Log("XMLNamespace.XMLNamespace exit "+_namespace);
            }

            internal String Name{
                get{ return(_name);}
            }
            internal Assembly Assem{
                get{ return(_assem);}
            }
            internal String Prefix{
                get{ return(_prefix);}
            }
            internal String Namespace{
                get{ return(_namespace);}
            }

            internal bool IsInteropType{
                get{return(_bInteropType);}
            }

            internal WsdlGenerator Generator
            {
                get {return (_generator);}
            }

            internal bool IsClassesPrinted
            {
                get {return _bClassesPrinted;}
                set {_bClassesPrinted = value;}
            }


            //internal XMLNamespace(String name, Assembly assem, String serviceEndpoint, Hashtable typeToServiceEndpoint, String prefix, bool bInteropType, WsdlGenerator generator ){

            /*
            internal SchemaType LookupSchemaType(Type type)
            {
                return (SchemaType)_typeToSchemaType[type];
            }

            internal Type LookupType(SchemaType stype)
            {
                return (Type)_schemaTypeToType[stype];
            }
            */
	        
            internal Type LookupSchemaType(String name)
            {
                Type returnType = null;
                RealSchemaType rsType = LookupRealSchemaType(name);
                if (rsType != null)
                    returnType = rsType.Type;

                SimpleSchemaType ssType = LookupSimpleSchemaType(name);
                if (ssType != null)
                    returnType = ssType.Type;

                ArraySchemaType asType = LookupArraySchemaType(name);
                if (asType != null)
                    returnType = asType.Type;

                Util.Log("XMLNamespace.LookupSchemaType "+name+" return "+returnType);
                return(returnType);
            }

            internal SimpleSchemaType LookupSimpleSchemaType(String name){
                Util.Log("XMLNamespace.LookupSimpleSchemaType "+name);              
                for (int i=0;i<_simpleSchemaTypes.Count;i++)
                {
                    SimpleSchemaType ssType = (SimpleSchemaType) _simpleSchemaTypes[i];
                    if (ssType.FullRefName == name)
                        return(ssType);
                }

                return(null);
            }
            
            internal bool CheckForSchemaContent()
	        {
                if (_arraySchemaTypes.Count > 0 ||
                    _simpleSchemaTypes.Count > 0)
		            return true;

    		    if (_realSchemaTypes.Count ==  0)
    		        return false;
    
    		    bool bRealSchema = false;
    		    for (int i=0;i<_realSchemaTypes.Count;i++)
    		    {
    		        RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
    		        if (!rsType.Type.IsInterface && !rsType.IsSUDSType)
    		        {
        			    bRealSchema = true;
        	    		break;
    	    	    }
    		    }
    
    		    if (bRealSchema)
    		        return true;
    		    else
    		        return false;
    	    }
            
            internal RealSchemaType LookupRealSchemaType(String name){
                Util.Log("XMLNamespace.LookupRealSchemaType "+name);                                
                Debug.Assert(_phonySchemaTypes.Count == 0, "PhonyTypes present");
                for (int i=0;i<_realSchemaTypes.Count;i++)
                {
                    RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
                    if (rsType.FullRefName == name)
                        return(rsType);
                }

                return(null);
            }

            internal ArraySchemaType LookupArraySchemaType(String name){
                Util.Log("XMLNamespace.LookupArraySchemaType "+name);                                
                //Debug.Assert(_phonySchemaTypes.Count == 0, "PhonyTypes present");
                for (int i=0;i<_arraySchemaTypes.Count;i++)
                {
                    ArraySchemaType asType = (ArraySchemaType) _arraySchemaTypes[i];
                    if (asType.Name == name)
                        return(asType);
                }

                return(null);
            }

            internal void AddRealSUDSType(RealSchemaType rsType){
                Util.Log("XMLNamespace.AddRealSUDSType "+rsType.Type);                              
                _realSUDSTypes.Add(rsType);
                //_typeToSchemaType[rsType.Type] = rsType;
                //_schemaTypeToType[rsType] = rsType.Type;
                return;
            }

            internal void AddRealSchemaType(RealSchemaType rsType){
                Util.Log("XMLNamespace.AddRealSchemaType "+rsType.Type);                              
                Debug.Assert(LookupRealSchemaType(rsType.Name) == null, "Duplicate Type found");
                _realSchemaTypes.Add(rsType);
                if (rsType.IsUnique)
                    _bUnique = true;
                //_typeToSchemaType[rsType.Type] = rsType;
                //_schemaTypeToType[rsType] = rsType.Type;
                return;
            }

            internal void AddArraySchemaType(ArraySchemaType asType){
                Debug.Assert(LookupArraySchemaType(asType.Name) == null, "Duplicate Type found");
                _arraySchemaTypes.Add(asType);
                //_typeToSchemaType[asType.Type] = asType;
                //_schemaTypeToType[asType] = asType.Type;
                return;
            }
            
            internal void AddSimpleSchemaType(SimpleSchemaType ssType){
                Util.Log("XMLNamespace.AddSimpleSchemaType "+ssType.Type);                              
                Debug.Assert(LookupSimpleSchemaType(ssType.Type.Name) == null, "Duplicate Type found");
                _simpleSchemaTypes.Add(ssType);
                //_typeToSchemaType[ssType.Type] = ssType;
                //_schemaTypeToType[ssType] = ssType.Type;
                return;
            }

            internal PhonySchemaType LookupPhonySchemaType(String name){
                Util.Log("XMLNamespace.LookupPhonySchemaType "+name);                                               
                for (int i=0;i<_phonySchemaTypes.Count;i++)
                {
                    PhonySchemaType type = (PhonySchemaType) _phonySchemaTypes[i];
                    if (type.Name == name)
                        return(type);
                }

                return(null);
            }

            internal void AddPhonySchemaType(PhonySchemaType phType){
                Util.Log("XMLNamespace.AddPhonySchemaType "+phType.Name);                                                               
                PhonySchemaType overloadedType = LookupPhonySchemaType(phType.Name);
                if (overloadedType != null)
                    phType.ElementName = phType.Name + overloadedType.OverloadedType();
                _phonySchemaTypes.Add(phType);

                return;
            }

            internal XMLNamespace LookupSchemaNamespace(String ns, Assembly assem){
                Util.Log("XMLNamespace.LookupSchemaNamespace "+ns);                                                             
                for (int i=0;i<_dependsOnSchemaNS.Count;i++)
                {
                    XMLNamespace xns = (XMLNamespace) _dependsOnSchemaNS[i];
                    if ((xns.Name == ns) && (xns.Assem == assem))
                        return(xns);
                }

                return(null);
            }

            internal void DependsOnSchemaNS(XMLNamespace xns, bool bImport){
                Util.Log("XMLNamespace.DependsOnSchemaNS "+Namespace+" depends on "+xns.Namespace+" bImport "+bImport);                                                              
                if (LookupSchemaNamespace(xns.Name, xns.Assem) != null)
                    return;

                _dependsOnSchemaNS.Add(xns);
                if (bImport && Namespace != xns.Namespace)
                    _xnsImports.Add(xns);
                return;
            }

            private XMLNamespace LookupSUDSNamespace(String ns, Assembly assem){
                Util.Log("XMLNamespace.LookupSUDSNamespace "+ns);                                                               
                for (int i=0;i<_dependsOnSUDSNS.Count;i++)
                {
                    XMLNamespace xns = (XMLNamespace) _dependsOnSUDSNS[i];
                    if ((xns.Name == ns) && (xns.Assem == assem))
                        return(xns);
                }

                return(null);
            }

            internal void DependsOnSUDSNS(XMLNamespace xns){
                Util.Log("XMLNamespace.DependsOnSUDSNS "+xns.Name+" "+xns.Assem);
                if (LookupSUDSNamespace(xns.Name, xns.Assem) != null)
                    return;

                _dependsOnSUDSNS.Add(xns);
                return;
            }

            internal void Resolve(){
                Util.Log("XMLNamespace.Resolve");                                                               
                StringBuilder sb = new StringBuilder(256);
                for (int i=0;i<_realSchemaTypes.Count;i++)
                    ((RealSchemaType) _realSchemaTypes[i]).Resolve(sb);

                return;
            }

            internal void PrintDependsOnWsdl(TextWriter textWriter, StringBuilder sb, String indent, Hashtable usedNames)
            {
                Util.Log("XMLNamespace.PrintDependsOn "+_name+" targetNameSpace "+Namespace);                                                                                                   
                if (_dependsOnSchemaNS.Count > 0)
                {
                    for (int i=0;i<_dependsOnSchemaNS.Count;i++)
                    {
                        XMLNamespace xns = (XMLNamespace) _dependsOnSchemaNS[i];
                        if (!usedNames.ContainsKey(xns.Prefix))
                        {
                            usedNames[xns.Prefix] = null;
                            sb.Length = 0;
                            sb.Append(indent);
                            sb.Append("xmlns:");                            
                            sb.Append(xns.Prefix);
                            sb.Append("='");
                            sb.Append(xns.Namespace);
                            sb.Append("'");
                            textWriter.WriteLine(sb);
                        }
                    }
                }
            }            

            internal void PrintSchemaWsdl(TextWriter textWriter, StringBuilder sb, String indent){
                Util.Log("XMLNamespace.PrintSchemaWsdl "+Namespace+" _realSchemaTypes.Count "+_realSchemaTypes.Count);                      
                // Print schema types

                bool bReal = false; 

                /*
                for(int i=0;i<_realSchemaTypes.Count;i++)
                {
                    RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
                    if(!rsType.Type.IsInterface && !rsType.IsSUDSType)
                        bReal = true;
                }
                */

                if ((_simpleSchemaTypes.Count > 0) || (_realSchemaTypes.Count > 0) || (_arraySchemaTypes.Count > 0))
                {
                    bReal = true;
                }

                // Print schema types
                if (bReal)
                {
                    // schema begin
                    String indent1 = IndentP(indent);
                    String indent2 = IndentP(indent1);
                    String indent3 = IndentP(indent2);
                    String indent4 = IndentP(indent3);                                  
                    sb.Length = 0;
                    sb.Append(indent);
                    sb.Append("<schema ");
                    sb.Append("targetNamespace='");
                    sb.Append(Namespace);
                    sb.Append("'");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);
                    sb.Append("xmlns='");
                    sb.Append(SudsConverter.GetXsdVersion(_generator._xsdVersion));
                    sb.Append("'");
                    textWriter.WriteLine(sb);

                    sb.Length = 0;
                    sb.Append(indent2);         
                    sb.Append("elementFormDefault='unqualified' attributeFormDefault='unqualified'>");
                    textWriter.WriteLine(sb);

                    // Write import statements
                    foreach (XMLNamespace xns in _xnsImports)
                    {
                        sb.Length = 0;
                        sb.Append(indent1);
                        sb.Append("<import namespace='");
                        sb.Append(xns.Namespace);
                        sb.Append("'/>");
                        textWriter.WriteLine(sb);
                    }


                    for (int i=0;i<_simpleSchemaTypes.Count;i++)
                    {
                        SimpleSchemaType ssType = (SimpleSchemaType) _simpleSchemaTypes[i];
                        ssType.PrintSchemaType(textWriter, sb, indent1, false);
                    }

                    for (int i=0;i<_realSchemaTypes.Count;i++)
                    {
                        RealSchemaType rsType = (RealSchemaType) _realSchemaTypes[i];
                        if (!rsType.Type.IsInterface && !rsType.IsSUDSType)
                            rsType.PrintSchemaType(textWriter, sb, indent1, false);
                    }

                    for (int i=0;i<_arraySchemaTypes.Count;i++)
                    {
                        ArraySchemaType asType = (ArraySchemaType) _arraySchemaTypes[i];
                        asType.PrintSchemaType(textWriter, sb, indent1, false);
                    }


                    /*
                    for(int i=0;i<_phonySchemaTypes.Count;i++)
                    {
                        PhonySchemaType psType = (PhonySchemaType) _phonySchemaTypes[i];
                        psType.PrintSchemaType(textWriter, sb, indent1, true);
                    }
                    */

                    sb.Length = 0;
                    sb.Append(indent);
                    sb.Append("</schema>");
                    textWriter.WriteLine(sb);


                }
            }

            internal void PrintMessageWsdl(TextWriter textWriter, StringBuilder sb, String indent, ArrayList refNames)
            {
                Util.Log("XmlNamespace.PrintMessageWsdl");
                for (int i=0;i<_realSUDSTypes.Count;i++)
                    ((RealSchemaType) _realSUDSTypes[i]).PrintMessageWsdl(textWriter, sb, indent, refNames);
                
                if (_realSUDSTypes.Count == 0 && _realSchemaTypes.Count > 0)
                {
                    // If no suds type, we still generate a binding section to print the Suds Extendsions
                    // We only need to do this once, because all the realschema types will be placed into
                    // one binding
                    ((RealSchemaType) _realSchemaTypes[0]).PrintMessageWsdl(textWriter, sb, indent, new ArrayList());
                }
            }

            // Fields
            private String _name;
            private Assembly _assem;
            private String _namespace;
            private String _prefix;
 // disable csharp compiler warning #0414: field assigned unused value
#pragma warning disable 0414
           internal bool _bUnique; 
#pragma warning restore 0414
            private ArrayList _dependsOnSUDSNS;
            private ArrayList _realSUDSTypes;
            private ArrayList _dependsOnSchemaNS;
            internal ArrayList _realSchemaTypes;
            private ArrayList _phonySchemaTypes;
            private ArrayList _simpleSchemaTypes;
            private ArrayList _arraySchemaTypes;
            private bool _bInteropType;
            private String _serviceEndpoint;
            private Hashtable _typeToServiceEndpoint;
            private WsdlGenerator _generator;
            private ArrayList _xnsImports;
            private bool _bClassesPrinted = false;
            //private Hashtable _typeToSchemaType = new Hashtable();
            //private Hashtable _schemaTypeToType = new Hashtable();


        }

        internal static String IndentP(String indentStr){
            return indentStr+"    ";
        }

    }
}











        
