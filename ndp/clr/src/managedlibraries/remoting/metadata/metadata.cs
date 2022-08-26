// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    MetaDataServices
//<EMAIL>
// Author:  Gopal Kakivaya (GopalK)
//</EMAIL>
// Purpose: Defines APIs for Metadata
//
// Date:    April 01, 2000
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
    using System.Net;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Metadata;

    using Microsoft.CSharp;
    using System.CodeDom.Compiler;
    using System.Globalization;

    public class MetaData
    {

        public static void ConvertTypesToSchemaToFile(Type[] types, SdlType sdlType, String path)
        {
			Util.Log("MetaData.ConvertTypesToSchemaToFile 1 "+path);						
            ConvertTypesToSchemaToStream(types, sdlType, File.Create(path));
        } // ConvertTypesToSchemaToFile

        public static void ConvertTypesToSchemaToStream(Type[] types, SdlType sdlType, Stream outputStream)
        {
			Util.Log("MetaData.ConvertTypesToSchemaToFile 2 ");						
            // <


            ServiceType[] serviceTypes = new ServiceType[types.Length];
            for (int i=0; i<types.Length; i++)
                serviceTypes[i] = new ServiceType(types[i]);

            ConvertTypesToSchemaToStream(serviceTypes, sdlType, outputStream);

        } // ConvertTypesToSchemaToStream


        public static void ConvertTypesToSchemaToFile(ServiceType[] types, SdlType sdlType, String path)
        {
            Util.Log("MetaData.ConvertTypesToSchemaToFile 3 "+path);
            ConvertTypesToSchemaToStream(types, sdlType, File.Create(path));
        } // ConvertTypesToSchemaToFile


        public static void ConvertTypesToSchemaToStream(ServiceType[] serviceTypes, SdlType sdlType, Stream outputStream)
        {
            Util.Log("MetaData.ConvertTypesToSchemaToStream 4");			
            if (sdlType == SdlType.Sdl)
                throw new NotSupportedException(
                            String.Format(
                                CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Sdl generation is not supported")));
		  TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));
            SUDSGenerator sgen = new SUDSGenerator(serviceTypes, sdlType, tw);
            sgen.Generate();
            tw.Flush();

        } // ConvertTypesToSchemaToStream

        public static void RetrieveSchemaFromUrlToStream(String url, Stream outputStream)
        {
            WebRequest              Request;
            WebResponse             Response;
            Stream                  RespStream;
			Util.Log("MetaData.RetrieveSchemaFromUrlToStream "+url);						
            TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));

            Request = WebRequest.Create(url);
            Response = Request.GetResponse();
            RespStream = Response.GetResponseStream();
			StreamReader sr = new StreamReader(RespStream);
			tw.Write(sr.ReadToEnd());
            tw.Flush();
        }

        public static void RetrieveSchemaFromUrlToFile(String url, String path)
        {
			Util.Log("MetaData.RetrieveSchemaFromUrlToFile "+url+" file "+path);									
            RetrieveSchemaFromUrlToStream(url, File.Create(path));
        }




        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList, String proxyUrl, String proxyNamespace)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 1 "+outputDirectory+" proxyUrl "+proxyNamespace);									
            TextReader input;

            input = (TextReader) new StreamReader(inputStream);

            SUDSParser parser = new SUDSParser(input, outputDirectory, outCodeStreamList, proxyUrl, clientProxy, proxyNamespace);
            parser.Parse();
        }

        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList, String proxyUrl)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 3 "+outputDirectory);									
          ConvertSchemaStreamToCodeSourceStream(clientProxy, outputDirectory, inputStream, outCodeStreamList, proxyUrl, "");
        }

        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 2 "+outputDirectory);									
          ConvertSchemaStreamToCodeSourceStream(clientProxy, outputDirectory, inputStream, outCodeStreamList, "", "");
        }
        

        public static void ConvertCodeSourceStreamToAssemblyFile(ArrayList outCodeStreamList, String assemblyPath, String strongNameFilename)
        {
#if FEATURE_PAL
            throw new NotImplementedException("Not Implemented in Rotor");
#else
            Util.Log("MetaData.ConvertCodeSourceStreamToAssemblyFile "+assemblyPath);												
            CompilerResults results = null;


            String stfilename = "__Sn.cs";
            try
            {
                if (strongNameFilename != null)
                {
                    // Create strong name file with assembly attribute
                    if (assemblyPath != null)
                    {
                        int index = assemblyPath.LastIndexOf("\\");
                        if (index > 0)
                        {
                            stfilename = assemblyPath.Substring(0,index+1)+stfilename;
                        }
                    }
                    FileStream fs = new FileStream(stfilename, FileMode.Create, FileAccess.ReadWrite);
                    StreamWriter fsWriter = new StreamWriter(fs, new UTF8Encoding(false, true));
                    fsWriter.WriteLine("// CLR Remoting Autogenerated Key file (to create a key file use: sn -k tmp.key)");
                    fsWriter.WriteLine("using System;");
                    fsWriter.WriteLine("using System.Reflection;");
                    fsWriter.WriteLine("[assembly: AssemblyKeyFile(@\""+strongNameFilename+"\")]");
                    fsWriter.WriteLine("[assembly: AssemblyVersion(@\"1.0.0.1\")]");
                    fsWriter.Flush();
                    fsWriter.Close();
                    fs.Close();
                    outCodeStreamList.Add(stfilename);
                    Util.Log("MetaData.ConvertCodeSourceStreamToAssemblyFile key file "+stfilename);												
                }

                String[] sourceTexts = new String[outCodeStreamList.Count];
                String[] sourceTextNames = new String[outCodeStreamList.Count];

                int streamCount = 0; // used for naming sourceTexts streams

                for(int item=0;item<outCodeStreamList.Count;item++)
                {
                  Stream inputStream;
                  bool close=false;

                  if (outCodeStreamList[item] is String)
                  {
                    // it's a file
                    String filename = (String)outCodeStreamList[item];
                    sourceTextNames[item] = (String)filename;
					Util.Log("MetaData.ConvertCodeSourceStreamToAssemblyFile  filename "+filename);																	
                    inputStream = File.OpenRead(filename);
                    close = true;
                  }
                  else if (outCodeStreamList[item] is Stream)
                  {
                    // it's a stream
                    inputStream = (Stream)outCodeStreamList[item];
                    sourceTextNames[item] = "Stream" + (streamCount++);
                  }
                  else
                  {
                     throw new RemotingException(CoreChannel.GetResourceString("Remoting_UnknownObjectInCodeStreamList"));
                  }

				  StreamReader streamReader = new StreamReader(inputStream);
                  sourceTexts[item] = streamReader.ReadToEnd();

                  if (true == close)
                    inputStream.Close();
                }

                String target = assemblyPath;

                String[] imports = new String[5];
                imports[0] = "System.dll";
                imports[1] = "System.Runtime.Remoting.dll";
                imports[2] = "System.Data.dll";
                imports[3] = "System.Xml.dll";
                imports[4] = "System.Web.Services.dll";
 
				if (sourceTexts.Length > 0)
				{
                          CodeDomProvider csharpCompiler = new CSharpCodeProvider();
		        	     CompilerParameters compileParams = new CompilerParameters(imports, target, true);
	            	     compileParams.GenerateExecutable = false; // target:library
                          results = csharpCompiler.CompileAssemblyFromSource(compileParams, sourceTexts);
				}
            }

            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
            finally {
                File.Delete(stfilename);                
            }

            if (results.Errors.HasErrors)
            {
                CompilerErrorCollection errors = results.Errors;
                if (errors.Count > 0)
                {
                    foreach (CompilerError error in errors)
                    {
                        Console.WriteLine(error.ToString());  
                    }
                }
            }
#endif //!FEATURE_PAL
        }


/*
/target:module -> Name="target", Value="module"
/target:library -> Name="target", Value="library"
/main:MyClass -> Name="m", Value="MyClass"
/debug+ -> Name="debug", Value=true

// The dictionary of options takes almost ALL of the normal command-line options, but only using their 'short form' and without the preceding slash or dash.
*/

        public static void ConvertCodeSourceFileToAssemblyFile(String codePath, String assemblyPath, String strongNameFilename)
        {
			Util.Log("MetaData.ConvertCodeSourceFileToAssemblyFile codePath "+codePath+" assemblyPath "+assemblyPath);												
            ArrayList arrayList = new ArrayList();
            arrayList.Add(codePath);
            ConvertCodeSourceStreamToAssemblyFile(arrayList, assemblyPath, strongNameFilename);
        }

        // Helpers

        public static void SaveStreamToFile(Stream inputStream, String path)
        {
			Util.Log("MetaData.SaveStreamToFile "+path);			
            Stream outputStream = File.Create(path);
            TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));
			StreamReader sr = new StreamReader(inputStream);
			tw.Write(sr.ReadToEnd());
            tw.Flush();
            tw.Close();
            outputStream.Close();
        }


    } // class MetaData
        
        
    public class ServiceType
        {
        private Type    _type;  // Type of object being exported.
        private String  _url;   // This may be null if no address is available.
        
        public ServiceType(Type type)
        {
            _type = type;
            _url = null;
        } // ServiceType

        public ServiceType(Type type, String url)
            {
            _type = type;
            _url = url;
        } // ServiceType

        public Type   ObjectType { get { return _type; } }
        public String Url { get { return _url; } }        
       
    } // ServiceType
        
        

} // namespace System.Runtime.Remoting.MetadataServices

