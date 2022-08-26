//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{

    using System;

    using System.ServiceModel.Channels;
    using System.Globalization;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ServiceModel;
    using System.Reflection;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;


    static class ComPlusTypeValidator
    {
        static Guid IID_Object = new Guid("{65074F7F-63C0-304E-AF0A-D51741CB4A8D}");
        static Guid IID_IDisposable = new Guid("{805D7A98-D4AF-3F0F-967F-E5CF45312D2C}");
        static Guid IID_IManagedObject = new Guid("{C3FCC19E-A970-11D2-8B5A-00A0C9B7C9C4}");
        static Guid IID_IProcessInitializer = new Guid("{1113F52D-DC7F-4943-AED6-88D04027E32A}");
        static Guid IID_IRemoteDispatch = new Guid("{6619A740-8154-43BE-A186-0319578E02DB}");
        static Guid IID_IServicedComponentInfo = new Guid("{8165B19E-8D3A-4D0B-80C8-97DE310DB583}");
        static Guid IID_IComponentRegistrar = new Guid("{A817E7A2-43FA-11D0-9E44-00AA00B6770A}");

        static internal bool DerivesFromServicedComponent(Guid iid, Guid clsid)
        {
            Type typeOfInterfaceResolver = typeof(Message).Assembly.GetType("System.ServiceModel.ComIntegration.TypeCacheManager");

            object resolver = typeOfInterfaceResolver.InvokeMember("Provider", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty, null, null, null, CultureInfo.InvariantCulture);
            object[] args = new object[3] { iid, false, true };
            Assembly asm = null;
            try
            {
                asm = typeOfInterfaceResolver.InvokeMember("ResolveAssemblyFromIID", BindingFlags.InvokeMethod | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, resolver, args, CultureInfo.InvariantCulture) as Assembly;
            }
            catch (TargetInvocationException e)
            {
                if (e.GetBaseException() is System.IO.FileNotFoundException)
                    throw;

                Console.WriteLine(iid + ", " + clsid);

                return false;
            }
            bool inherits = false;
            foreach (Type t in asm.GetTypes())
            {
                if (t.GUID == clsid)
                {
                    Type baseType = t.BaseType;
                    while (null != baseType)
                    {
                        if (typeof(ServicedComponent) == baseType)
                        {
                            inherits = true;
                            break;
                        }

                        baseType = baseType.BaseType;
                    }
                }
            }

            return inherits;

        }

        static internal bool VerifyInterface(ComAdminInterfaceInfo interfaceInfo, bool allowReferences, Guid clsid)
        {
            return VerifyInterface(interfaceInfo, allowReferences, clsid, false);
        }

        static internal bool VerifyInterface(ComAdminInterfaceInfo interfaceInfo, bool allowReferences, Guid clsid, bool produceError)
        {
            if (IsInternalInterface(interfaceInfo.Iid))
            {
                if (produceError)
                    ToolConsole.WriteError(SR.GetString(SR.IsInternalInterfaceAndCannotBeExposedOverWebService, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name), "");
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.IsInternalInterfaceAndCannotBeExposedOverWebService, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name));

                return false;
            }
            Type typeOfInterfaceResolver = typeof(Message).Assembly.GetType("System.ServiceModel.ComIntegration.TypeCacheManager");

            object resolver = typeOfInterfaceResolver.InvokeMember("Provider", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty, null, null, null, CultureInfo.InvariantCulture);
            object[] args = new object[1] { interfaceInfo.Iid };
            Type typeOfInterface = null;
            try
            {
                typeOfInterface = typeOfInterfaceResolver.InvokeMember("VerifyType", BindingFlags.InvokeMethod | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, resolver, args, CultureInfo.InvariantCulture) as Type;
            }
            catch (TargetInvocationException e)
            {
                if (e.GetBaseException() is System.IO.FileNotFoundException)
                    throw CreateDescriptiveException((System.IO.FileNotFoundException)e.GetBaseException());

                string exceptionMessage = SR.GetString(SR.TypeResolutionForInterfaceFailed, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, e.InnerException.Message);

                if (DerivesFromServicedComponent(interfaceInfo.Iid, clsid))
                    exceptionMessage += " " + SR.GetString(SR.ClassInterfacesNotSupported);

                if (produceError)
                    ToolConsole.WriteError(exceptionMessage, "");
                else
                    ToolConsole.WriteNonVerboseWarning(exceptionMessage);

                return false;
            }

            MethodInfo[] methods = typeOfInterface.GetMethods();
            if (methods.Length == 0)
            {
                if (produceError)
                    ToolConsole.WriteError(SR.GetString(SR.InterfaceHasNoMethods, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name), "");
                else
                    ToolConsole.WriteNonVerboseWarning(SR.GetString(SR.InterfaceHasNoMethods, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name));

                return false;
            }
            string typeMismatchDetails;
            foreach (MethodInfo method in methods)
            {
                foreach (ParameterInfo parameter in method.GetParameters())
                {
                    Type typeOfParam = parameter.ParameterType;
                    if (typeOfParam.IsByRef)
                        typeOfParam = typeOfParam.GetElementType();

                    if (!IsValidParameter(typeOfParam, parameter, allowReferences, out typeMismatchDetails))
                    {
                        if (produceError)
                            ToolConsole.WriteError(SR.GetString(SR.ParameterOfMethodInInterfaceHasANonCompliantType, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, method.Name, parameter.Name, typeMismatchDetails), "");
                        else
                            ToolConsole.WriteNonVerboseWarning(SR.GetString(SR.ParameterOfMethodInInterfaceHasANonCompliantType, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, method.Name, parameter.Name, typeMismatchDetails));

                        return false;
                    }
                }
                if (!IsValidParameter(method.ReturnType, method.ReturnTypeCustomAttributes, allowReferences, out typeMismatchDetails))
                {
                    if (produceError)
                        ToolConsole.WriteError(SR.GetString(SR.InvalidWebServiceReturnValue, method.ReturnType.Name, method.Name, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, typeMismatchDetails), "");
                    else
                        ToolConsole.WriteNonVerboseWarning(SR.GetString(SR.InvalidWebServiceReturnValue, method.ReturnType.Name, method.Name, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, typeMismatchDetails));

                    return false;
                }
            }
            return true;
        }

        public static List<string> FetchAllMethodsForInterface(ComAdminInterfaceInfo interfaceInfo)
        {
            return FetchAllMethodsForInterface(interfaceInfo, true);
        }

        public static List<string> FetchAllMethodsForInterface(ComAdminInterfaceInfo interfaceInfo, bool produceWarning)
        {
            Type typeOfInterfaceResolver = typeof(Message).Assembly.GetType("System.ServiceModel.ComIntegration.TypeCacheManager");

            object resolver = typeOfInterfaceResolver.InvokeMember("Provider", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty, null, null, null, CultureInfo.InvariantCulture);
            object[] args = new object[1] { interfaceInfo.Iid };
            Type typeOfInterface = null;
            try
            {
                typeOfInterface = typeOfInterfaceResolver.InvokeMember("VerifyType", BindingFlags.InvokeMethod | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, resolver, args, CultureInfo.InvariantCulture) as Type;
            }
            catch (TargetInvocationException e)
            {
                if (e.GetBaseException() is System.IO.FileNotFoundException)
                    throw CreateDescriptiveException((System.IO.FileNotFoundException)e.GetBaseException());

                if (produceWarning)
                    ToolConsole.WriteWarning(SR.GetString(SR.TypeResolutionForInterfaceFailed, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, e.InnerException.Message));

                return null;
            }
            MethodInfo[] methods = typeOfInterface.GetMethods();
            if (methods.Length == 0)
                return null;
            else
            {
                List<string> methodNames = new List<string>();
                foreach (MethodBase method in methods)
                    methodNames.Add(method.Name);
                return methodNames;

            }


        }

        static Exception CreateDescriptiveException(System.IO.FileNotFoundException oldException)
        {
            return new Exception(oldException.Message + " " + SR.GetString(SR.InstallInGAC, oldException.FileName));
        }

        public static bool VerifyInterfaceMethods(ComAdminInterfaceInfo interfaceInfo, IList<string> methodNames, bool allowReferences, bool produceError)
        {
            if (IsInternalInterface(interfaceInfo.Iid))
            {
                if (produceError)
                    ToolConsole.WriteError(SR.GetString(SR.IsInternalInterfaceAndCannotBeExposedOverWebService, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name), "");
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.IsInternalInterfaceAndCannotBeExposedOverWebService, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name));

                return false;
            }
            Type typeOfInterfaceResolver = typeof(Message).Assembly.GetType("System.ServiceModel.ComIntegration.TypeCacheManager");

            object resolver = typeOfInterfaceResolver.InvokeMember("Provider", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty, null, null, null, CultureInfo.InvariantCulture);
            object[] args = new object[1] { interfaceInfo.Iid };
            Type typeOfInterface = null;
            try
            {
                typeOfInterface = typeOfInterfaceResolver.InvokeMember("VerifyType", BindingFlags.InvokeMethod | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, resolver, args, CultureInfo.InvariantCulture) as Type;
            }
            catch (TargetInvocationException e)
            {
                if (e.GetBaseException() is System.IO.FileNotFoundException)
                    throw CreateDescriptiveException((System.IO.FileNotFoundException)e.GetBaseException());

                if (produceError)
                    ToolConsole.WriteError(SR.GetString(SR.TypeResolutionForInterfaceFailed, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, e.InnerException.Message), "");
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.TypeResolutionForInterfaceFailed, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, e.InnerException.Message));

                return false;
            }
            MethodInfo[] methods = typeOfInterface.GetMethods();
            if (methods.Length == 0)
            {
                if (produceError)
                    ToolConsole.WriteError(SR.GetString(SR.InterfaceHasNoMethods, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name), "");
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.InterfaceHasNoMethods, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name));
                return false;
            }
            string typeMismatchDetails;
            bool found = false;
            foreach (string methodName in methodNames)
            {
                found = false;
                foreach (MethodInfo method in methods)
                {
                    if (methodName == method.Name)
                    {
                        found = true;
                        foreach (ParameterInfo parameter in method.GetParameters())
                        {
                            Type typeOfParam = parameter.ParameterType;
                            if (typeOfParam.IsByRef)
                                typeOfParam = typeOfParam.GetElementType();

                            if (!IsValidParameter(typeOfParam, parameter, allowReferences, out typeMismatchDetails))
                            {
                                if (produceError)
                                    ToolConsole.WriteError(SR.GetString(SR.ParameterOfMethodInInterfaceHasANonCompliantType, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, method.Name, parameter.Name, typeMismatchDetails), "");
                                else
                                    ToolConsole.WriteWarning(SR.GetString(SR.ParameterOfMethodInInterfaceHasANonCompliantType, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, method.Name, parameter.Name, typeMismatchDetails));

                                return false;
                            }
                        }
                        if (!IsValidParameter(method.ReturnType, method.ReturnTypeCustomAttributes, allowReferences, out typeMismatchDetails))
                        {
                            if (produceError)
                                ToolConsole.WriteError(SR.GetString(SR.InvalidWebServiceReturnValue, method.ReturnType.Name, method.Name, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, typeMismatchDetails), "");
                            else
                                ToolConsole.WriteWarning(SR.GetString(SR.InvalidWebServiceReturnValue, method.ReturnType.Name, method.Name, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, typeMismatchDetails));

                            return false;
                        }
                    }
                }
                if (!found)
                {
                    throw Tool.CreateException(SR.GetString(SR.MethodNotFoundOnInterface, Tool.Options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name, methodName), null);
                }
            }
            return true;


        }
        public static bool IsInternalInterface(Guid iid)
        {

            if (iid == IID_Object ||
                iid == IID_IDisposable ||
                iid == IID_IManagedObject ||
                iid == IID_IProcessInitializer ||
                iid == IID_IRemoteDispatch ||
                iid == IID_IServicedComponentInfo ||
                iid == IID_IComponentRegistrar ||
                iid.ToString("D").ToLowerInvariant().EndsWith("c000-000000000046", StringComparison.Ordinal)) //other ole/com standard interfaces
            {
                return true;
            }

            return false;
        }


        public static bool IsValidParameter(Type type, ICustomAttributeProvider attributeProvider, bool allowReferences, out string typeMismatchDetails)
        {
            typeMismatchDetails = type.ToString() + " ";
            object[] attributes = attributeProvider.GetCustomAttributes(typeof(MarshalAsAttribute), true);

            foreach (MarshalAsAttribute attr in attributes)
            {
                UnmanagedType marshalAs = attr.Value;
                if (marshalAs == UnmanagedType.IDispatch ||
                    marshalAs == UnmanagedType.Interface ||
                    marshalAs == UnmanagedType.IUnknown)
                {
                    if (!allowReferences)
                        typeMismatchDetails += SR.GetString(SR.HasMarshalAsAttributeOfType, marshalAs);

                    return allowReferences;
                }
            }

            XsdDataContractExporter exporter = new XsdDataContractExporter();
            if (!exporter.CanExport(type))
            {
                typeMismatchDetails += SR.GetString(SR.CannotBeExportedByDataContractExporter);
                return false;
            }

            return true;
        }
    }
}
