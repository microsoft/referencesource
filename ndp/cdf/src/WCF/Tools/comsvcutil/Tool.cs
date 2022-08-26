//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Description;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.ServiceModel;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using System.Configuration;

    [ComImport]
    [Guid("33CAF1A1-FCB8-472b-B45E-967448DED6D8")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IServiceSysTxnConfig
    {

    }

    [ComImport]
    [Guid("ecabb0c8-7f19-11d2-978e-0000f8757e2a")]
    class CServiceConfig { }
    public static class Tool
    {
        static Options options;
        internal static Options Options { get { return options; } }

        // const string DefaultBindingName = "HttpDuplexWindowsSecurityBinding";



        internal static Exception CreateArgumentException(string command, string arg, string message, Exception innerException)
        {
            return new ArgumentException(SR.GetString(SR.InvalidArg, command, arg, message), innerException);
        }

        internal static Exception CreateException(string message, Exception innerException)
        {
            return new ApplicationException(message, innerException);

        }

        // returns whether help was displayed
        static void DisplayHelp(Mode mode)
        {
            if (options.Mode == Mode.NotSpecified)
            {
                DisplayUsage();
            }
            else if (options.Mode == Mode.Install)
            {
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsage4, Cmd.Install, Abbr.Install));
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsageExamples));
                ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:* /hosting:complus");
                ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:TestComponent,ITest /hosting:was /webDirectory:testdir /mex");
                ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:TestComponent,ITest.{Method1} /hosting:was /webDirectory:testdir /mex");
                ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:TestComponent,ITest.{Method2,Method3} /hosting:was /webDirectory:testdir /mex");
            }
            else if (options.Mode == Mode.Uninstall)
            {
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsage5, Cmd.Uninstall, Abbr.Uninstall));
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsageExamples));
                ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:* /hosting:complus");
                ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:* /hosting:was /mex");
                ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:TestComponent,ITest.{Method1} /hosting:was /mex");
                ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:TestComponent,ITest.{Method2,Method3} /hosting:was /mex");

            }
            else if (options.Mode == Mode.List)
            {
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsage6, Cmd.List, Abbr.List));
                ToolConsole.WriteLine(SR.GetString(SR.HelpUsageExamples));
                ToolConsole.WriteLine("  ComSvcConfig.exe /list");
                ToolConsole.WriteLine("  ComSvcConfig.exe /list /hosting:complus");
                ToolConsole.WriteLine("  ComSvcConfig.exe /list /hosting:was");

            }
        }


        static void DisplayLogo()
        {
            // Using CommonResStrings.WcfTrademarkForCmdLine for the trademark: the proper resource for command line tools.
            Console.WriteLine(SR.GetString(SR.Logo, CommonResStrings.WcfTrademarkForCmdLine, ThisAssembly.InformationalVersion, CommonResStrings.CopyrightForCmdLine));
        }

        static void DisplayUsage()
        {
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage1));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage2, ThisAssembly.Title));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage3));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage4, Cmd.Install, Abbr.Install));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage5, Cmd.Uninstall, Abbr.Uninstall));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage6, Cmd.List, Abbr.List));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage7));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageApplication, Cmd.Application, Abbr.Application));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageInterface, Cmd.Contract, Abbr.Contract, "{", "}"));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageReferences, Cmd.AllowReferences, Abbr.AllowReferences));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageHosting, Cmd.Hosting, Abbr.Hosting));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageWebServer, Cmd.WebServer, Abbr.WebServer));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageWebDirectory, Cmd.WebDirectory, Abbr.WebDirectory));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageMexOption, Cmd.MetaData, Abbr.MetaData));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageGuidOption, Cmd.ID, Abbr.ID));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageNoLogo, Cmd.NoLogo, Abbr.NoLogo));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageVerbose, Cmd.Verbose, Abbr.Verbose));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsage8, "help"));
            ToolConsole.WriteLine(SR.GetString(SR.HelpUsageExamples));
            ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:* /hosting:complus");
            ToolConsole.WriteLine("  ComSvcConfig.exe /install /application:TestApp /contract:TestComponent,ITest /hosting:was /webDirectory:testdir /mex");
            ToolConsole.WriteLine("  ComSvcConfig.exe /list");
            ToolConsole.WriteLine("  ComSvcConfig.exe /list /hosting:complus");
            ToolConsole.WriteLine("  ComSvcConfig.exe /list /hosting:was");
            ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:* /hosting:complus");
            ToolConsole.WriteLine("  ComSvcConfig.exe /uninstall /application:OnlineStore /contract:* /hosting:was");
            ToolConsole.WriteLine("");
        }

        static void DoInstall()
        {
            ValidateAddParams();
            ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(options.Application);
            if (appInfo == null)
            {
                throw CreateArgumentException(Cmd.Application, options.Application, SR.GetString(SR.ApplicationNotFound, options.Application), null);
            }

            ValidateApplication(appInfo, options.Hosting);
            Guid sourceAppId = appInfo.ID;

            EndpointConfigContainer container = null;


            if (options.Hosting == Hosting.Complus)
            {
                container = ComplusEndpointConfigContainer.Get(options.Application, true);
                if (container == null)
                {
                    throw CreateArgumentException(Cmd.Application, options.Application, SR.GetString(SR.ApplicationNotFound, options.Application), null);
                }
            }
            else if (options.Hosting == Hosting.Was)
            {
                string webServer = null;
                if (options.WebServer != null)
                {
                    webServer = options.WebServer;
                }
                else
                {
                    webServer = WasEndpointConfigContainer.DefaultWebServer;
                }

                container = WasEndpointConfigContainer.Get(webServer, options.WebDirectory, options.Application);
                if (container == null)
                {
                    throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.WebDirectoryNotFound, options.WebDirectory), null);
                }
            }

            IList<ComponentDefinition<Guid>> guidComponents = null;
            if (options.AllComponents)
            {
                GetAllComponentsForAdd(appInfo, options.Mex, out guidComponents);
            }
            else
            {
                GetComponentsFromInputForAdd(appInfo, options.Components, options.Mex, container.HasEndpointsForApplication(sourceAppId), out guidComponents);
            }

            if (guidComponents.Count == 0)
            {
                if (String.Empty != options.MexOnlyComponent)
                    throw Tool.CreateException(SR.GetString(SR.MexOnlyComponentHasNoExposedInterface, options.MexOnlyComponent), null);
                else
                    throw Tool.CreateException(SR.GetString(SR.NoneOfTheComponentsSatisfiedTheAddCriteria), null);
            }

            List<EndpointConfig> endpointConfigs = new List<EndpointConfig>();

            foreach (ComponentDefinition<Guid> component in guidComponents)
            {
                ComAdminClassInfo componentInfo = appInfo.FindClass(component.Component.ToString("B"));
                Debug.Assert(componentInfo != null, "No component Found");
                string bindingType = null;
                string bindingName = null;
                if (!componentInfo.SupportsTransactionFlow)
                {
                    bindingType = container.DefaultBindingType;
                    bindingName = container.DefaultBindingName;
                }
                else
                {
                    bindingType = container.DefaultTransactionalBindingType;
                    bindingName = container.DefaultTransactionalBindingName;
                }
                foreach (InterfaceDefination<Guid> iInterface in component.Interfaces)
                {
                    Guid iid = iInterface.Interface;
                    EndpointConfig ec = null;
                    if (iid != typeof(IMetadataExchange).GUID)
                    {
                        string address = container.DefaultEndpointAddress(sourceAppId, component.Component, iid);
                        ec = new EndpointConfig(sourceAppId,
                                                               component.Component,
                                                               iid,
                                                               bindingType,
                                                               bindingName,
                                                               new Uri(address, UriKind.RelativeOrAbsolute),
                                                               false,
                                                               (List<string>)iInterface.Methods);
                    }
                    else
                    {
                        ec = new EndpointConfig(sourceAppId,
                                                           component.Component,
                                                           typeof(IMetadataExchange).GUID,
                                                           container.DefaultMexBindingType,
                                                           container.DefaultMexBindingName,
                                                           new Uri(container.DefaultMexAddress(sourceAppId, component.Component), UriKind.RelativeOrAbsolute),
                                                           true,
                                                           null);
                    }

                    endpointConfigs.Add(ec);
                }

            }



            try
            {
                container.Add(endpointConfigs);
                container.PrepareChanges();         // containers can throw from this                
            }
            catch (Exception e)
            {
                if (e is NullReferenceException || e is SEHException)
                {
                    throw;
                }
                container.AbortChanges();
                throw CreateException(SR.GetString(SR.ErrorDuringAdd, options.Application), e);
            }

            container.CommitChanges();
        }

        // Assumption is that application, if present, is in curly Guids form
        static List<EndpointConfigContainer> GetContainersForQueryOrRemove(Hosting hosting, string application, string webServer, string webDirectory)
        {
            List<EndpointConfigContainer> containers = new List<EndpointConfigContainer>();

            // first, get any complus-hosted endpointConfigs
            if (hosting == Hosting.Complus || hosting == Hosting.NotSpecified)
            {
                if (!string.IsNullOrEmpty(application))
                {
                    EndpointConfigContainer container = ComplusEndpointConfigContainer.Get(application);
                    if (container == null)
                    {
                        throw CreateArgumentException(Cmd.Application, options.Application, SR.GetString(SR.ApplicationNotFound, options.Application), null);
                    }

                    containers.Add(container);
                }
                else
                {
                    // query for all complus-hosted apps
                    List<ComplusEndpointConfigContainer> comContainers = ComplusEndpointConfigContainer.Get();
                    if (comContainers != null)
                    {
                        foreach (ComplusEndpointConfigContainer comContainer in comContainers)
                        {
                            containers.Add(comContainer);
                        }
                    }
                }
            }

            // then, get any was-hosted endpointConfigs
            if (hosting == Hosting.Was || hosting == Hosting.NotSpecified)
            {
                // specific webDirectory
                if (!string.IsNullOrEmpty(webDirectory))
                {
                    if (string.IsNullOrEmpty(webServer))
                    {
                        webServer = WasEndpointConfigContainer.DefaultWebServer;
                    }

                    EndpointConfigContainer container = WasEndpointConfigContainer.Get(webServer, webDirectory, application);
                    if (container == null)
                    {
                        throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.WebDirectoryNotFound, options.WebDirectory), null);
                    }

                    if (string.IsNullOrEmpty(application))
                    {
                        containers.Add(container);
                    }
                    else
                    {
                        if (container.HasEndpointsForApplication(new Guid(application)))
                        {
                            containers.Add(container);
                        }
                    }
                }
                else
                {
                    // no webDirectory specified.
                    // we will therefore look in all webDirs, in all webServers (unless one is specified)

                    List<WasEndpointConfigContainer> wasContainers = null;

                    if (!string.IsNullOrEmpty(webServer))
                    {
                        wasContainers = WasEndpointConfigContainer.Get(webServer, application); // all webDirs in a specific server
                    }
                    else
                    {
                        wasContainers = WasEndpointConfigContainer.Get(application);  // all webDirs in all servers
                    }

                    if (wasContainers != null)
                    {
                        foreach (WasEndpointConfigContainer container in wasContainers)
                        {
                            if (string.IsNullOrEmpty(application))
                            {
                                containers.Add(container);
                            }
                            else
                            {
                                if (container.HasEndpointsForApplication(new Guid(application)))
                                {
                                    containers.Add(container);
                                }
                            }
                        }
                    }
                }
            }

            return containers;
        }

        static void DisplayEndpointConfig(EndpointConfig config)
        {
            List<string> baseAddresses = null;

            if (config.Container != null)
            {
                baseAddresses = config.Container.GetBaseAddresses(config);
            }

            if (null == baseAddresses || 0 == baseAddresses.Count)
            {
                if (config.IsMexEndpoint)
                    ToolConsole.WriteQueryLine("          " + SR.GetString(SR.MexEndpointExposed, config.Address));
                else
                {
                    ToolConsole.WriteQueryLine("             " + SR.GetString(SR.BindingType, config.BindingType));
                    ToolConsole.WriteQueryLine("             " + SR.GetString(SR.BindingConfigurationName, config.BindingName));
                    ToolConsole.WriteQueryLine("             " + SR.GetString(SR.Address, config.Address));
                }
            }
            else
            {
                foreach (string s in baseAddresses)
                {
                    string addr = s + @"/" + config.Address;

                    if (config.IsMexEndpoint)
                        ToolConsole.WriteQueryLine("          " + SR.GetString(SR.MexEndpointExposed, addr));
                    else
                    {
                        ToolConsole.WriteQueryLine("             " + SR.GetString(SR.BindingType, config.BindingType));
                        ToolConsole.WriteQueryLine("             " + SR.GetString(SR.BindingConfigurationName, config.BindingName));
                        ToolConsole.WriteQueryLine("             " + SR.GetString(SR.Address, addr));
                    }
                }
            }

        }

        static void DoList()
        {
            ValidateQueryParams();

            string application = null;
            Guid appid;

            if (options.Application != null)
            {
                // Make sure that the application exists, and get its Guid


                if (!ComAdminWrapper.ResolveApplicationId(options.Application, out appid))
                {
                    throw CreateArgumentException(Cmd.Application, options.Application, SR.GetString(SR.ApplicationNotFound, options.Application), null);
                }

                application = appid.ToString("B");
            }

            List<EndpointConfig> endpointConfigs = new List<EndpointConfig>();

            List<EndpointConfigContainer> containers = GetContainersForQueryOrRemove(options.Hosting, application, options.WebServer, options.WebDirectory);

            if (containers != null)
            {
                foreach (EndpointConfigContainer container in containers)
                {
                    try
                    {
                        List<EndpointConfig> configs = null;
                        if (!string.IsNullOrEmpty(application))
                        {
                            configs = container.GetEndpointConfigs(new Guid(application));
                        }
                        else
                        {
                            configs = container.GetEndpointConfigs();
                        }

                        endpointConfigs.AddRange(configs);
                    }
#pragma warning suppress 56500 // covered by FxCOP
                    catch (Exception)
                    {
                        if (container is WasEndpointConfigContainer)
                            ToolConsole.WriteWarning(SR.GetString(SR.InvalidConfigFile, ((WasEndpointConfigContainer)container).ConfigFile.OriginalFileName));

                        if (container is ComplusEndpointConfigContainer)
                            ToolConsole.WriteWarning(SR.GetString(SR.InvalidConfigFile, ((ComplusEndpointConfigContainer)container).ConfigFile.OriginalFileName));
                    }
                }
            }


            Dictionary<Guid, Dictionary<Guid, Dictionary<Guid, List<EndpointConfig>>>> applicationToComponents = new Dictionary<Guid, Dictionary<Guid, Dictionary<Guid, List<EndpointConfig>>>>();
            foreach (EndpointConfig config in endpointConfigs)
            {
                Dictionary<Guid, Dictionary<Guid, List<EndpointConfig>>> componentToInterfaces = null;
                Dictionary<Guid, List<EndpointConfig>> interfacesForComponents = null;
                List<EndpointConfig> endpointsForInterface = null;

                if (!applicationToComponents.TryGetValue(config.Appid, out componentToInterfaces))
                {
                    componentToInterfaces = new Dictionary<Guid, Dictionary<Guid, List<EndpointConfig>>>();
                    applicationToComponents[config.Appid] = componentToInterfaces;
                }
                if (!componentToInterfaces.TryGetValue(config.Clsid, out interfacesForComponents))
                {
                    interfacesForComponents = new Dictionary<Guid, List<EndpointConfig>>();
                    componentToInterfaces[config.Clsid] = interfacesForComponents;
                }
                if (!interfacesForComponents.TryGetValue(config.Iid, out endpointsForInterface))
                {
                    endpointsForInterface = new List<EndpointConfig>();
                    interfacesForComponents[config.Iid] = endpointsForInterface;
                }
                endpointsForInterface.Add(config);
            }
            IEnumerator<KeyValuePair<Guid, Dictionary<Guid, Dictionary<Guid, List<EndpointConfig>>>>> enumerateApps = applicationToComponents.GetEnumerator();
            while (enumerateApps.MoveNext())
            {
                IEnumerator<KeyValuePair<Guid, Dictionary<Guid, List<EndpointConfig>>>> enumerateComponents = enumerateApps.Current.Value.GetEnumerator();
                ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(enumerateApps.Current.Key.ToString("B"));
                if (appInfo == null)
                    continue;
                ToolConsole.WriteQueryLine(SR.GetString(SR.EnumeratingComponentsForApplication, options.ShowGuids ? appInfo.ID.ToString("B") : appInfo.Name));

                foreach (EndpointConfigContainer container in containers)
                {
                    if (container.HasEndpointsForApplication(enumerateApps.Current.Key))
                    {
                        if (container is WasEndpointConfigContainer)
                        {
                            ToolConsole.WriteQueryLine("     " + SR.GetString(SR.WasHosting));
                            ToolConsole.WriteQueryLine("     " + SR.GetString(SR.ConfigFileName, ((WasEndpointConfigContainer)container).ConfigFile.OriginalFileName));
                        }
                        else
                        {
                            ToolConsole.WriteQueryLine("     " + SR.GetString(SR.ComplusHosting));
                            ToolConsole.WriteQueryLine("     " + SR.GetString(SR.ConfigFileName, ((ComplusEndpointConfigContainer)container).ConfigFile.OriginalFileName));
                        }
                    }
                }

                while (enumerateComponents.MoveNext())
                {
                    IEnumerator<KeyValuePair<Guid, List<EndpointConfig>>> enumerateInterfaces = enumerateComponents.Current.Value.GetEnumerator();
                    ComAdminClassInfo classInfo = appInfo.FindClass(enumerateComponents.Current.Key.ToString("B"));
                    if (classInfo == null)
                        continue;
                    ToolConsole.WriteQueryLine("     " + SR.GetString(SR.EnumeratingInterfacesForComponent, options.ShowGuids ? classInfo.Clsid.ToString("B") : classInfo.Name));

                    while (enumerateInterfaces.MoveNext())
                    {
                        ComAdminInterfaceInfo interfaceInfo = classInfo.FindInterface(enumerateInterfaces.Current.Key.ToString("B"));
                        if (interfaceInfo == null)
                        {
                            foreach (EndpointConfig config in enumerateInterfaces.Current.Value)
                            {
                                if (config.IsMexEndpoint)
                                {
                                    DisplayEndpointConfig(config);

                                    continue;
                                }
                            }
                        }
                        else
                        {
                            ToolConsole.WriteQueryLine("          " + SR.GetString(SR.EnumeratingEndpointsForInterfaces, options.ShowGuids ? interfaceInfo.Iid.ToString("B") : interfaceInfo.Name));

                            foreach (EndpointConfig config in enumerateInterfaces.Current.Value)
                                DisplayEndpointConfig(config);
                        }
                    }
                }
            }
        }

        static void DoUninstall()
        {
            ValidateRemoveParams();
            ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(options.Application);
            if (appInfo == null)
            {
                throw CreateArgumentException(Cmd.Application, options.Application, SR.GetString(SR.ApplicationNotFound, options.Application), null);
            }

            //ValidateApplication(appInfo, options.Hosting);
            Guid sourceAppId = appInfo.ID;
            string application = sourceAppId.ToString("B");
            IList<ComponentDefinition<Guid>> guidComponents = null;
            if (options.AllComponents)
            {
                GetAllComponentsForRemove(appInfo, out guidComponents);
            }
            else
            {
                GetComponentsFromInputForRemove(appInfo, options.Components, out guidComponents);
            }

            List<EndpointConfigContainer> containers = GetContainersForQueryOrRemove(options.Hosting, application, options.WebServer, options.WebDirectory);
            if (guidComponents.Count == 0)
                ToolConsole.WriteWarning(SR.GetString(SR.NoneOfTheComponentsSatisfiedTheRemoveCriteria));

            try
            {
                bool update = false;
                foreach (EndpointConfigContainer container in containers)
                {
                    List<EndpointConfig> endpointsToDelete = new List<EndpointConfig>();
                    List<EndpointConfig> endpointConfigs = container.GetEndpointConfigs(sourceAppId);
                    foreach (EndpointConfig endpointConfig in endpointConfigs)
                    {
                        if (ShouldDelete(endpointConfig, guidComponents))
                        {
                            endpointsToDelete.Add(endpointConfig);
                        }
                    }
                    if (endpointsToDelete.Count != 0)
                    {
                        container.Remove(endpointsToDelete);
                        update = true;
                    }

                }
                if (!update)
                    ToolConsole.WriteWarning(SR.GetString(SR.NoneOfConfigsFoundMatchTheCriteriaSpecifiedNothingWillBeRemoved));

                foreach (EndpointConfigContainer container in containers)
                {
                    container.PrepareChanges();                    // containers are allowed to throw from Prepare
                }
            }
            catch (Exception e)
            {
                if (e is NullReferenceException || e is SEHException)
                {
                    throw;
                }

                foreach (EndpointConfigContainer container in containers)
                {
                    container.AbortChanges();     // containers shouldn't throw from here
                }
                throw CreateException(SR.GetString(SR.ErrorDuringRemove), e);
            }

            // Commit time!
            foreach (EndpointConfigContainer container in containers)
            {
                container.CommitChanges();      // containers shouldn't throw from here
            }
        }

        static bool ShouldDelete(EndpointConfig endpointConfig, IList<ComponentDefinition<Guid>> guidComponents)
        {
            foreach (ComponentDefinition<Guid> component in guidComponents)
            {
                if (component.Component == endpointConfig.Clsid)
                {
                    foreach (InterfaceDefination<Guid> interfaceDef in component.Interfaces)
                    {
                        if (interfaceDef.Interface == endpointConfig.Iid)
                        {
                            endpointConfig.Methods = interfaceDef.Methods;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        static void EnsureUserIsAdministrator()
        {
            WindowsPrincipal principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
            if (!principal.IsInRole(WindowsBuiltInRole.Administrator))
            {
                throw CreateException(SR.GetString(SR.MustBeAnAdministrator), null);
            }
        }

        // returns strongly typed, verified components/interfaces in an application
        static void GetAllComponentsForAdd(ComAdminAppInfo appInfo, bool mex, out IList<ComponentDefinition<Guid>> outComps)
        {
            outComps = new List<ComponentDefinition<Guid>>();

            foreach (ComAdminClassInfo classInfo in appInfo.Classes)
            {
                ComponentDefinition<Guid> outComp;

                if (!ValidateClass(classInfo))
                {
                    continue;

                }

                outComp = new ComponentDefinition<Guid>(classInfo.Clsid);

                foreach (ComAdminInterfaceInfo interfaceInfo in classInfo.Interfaces)
                {
                    if (ComPlusTypeValidator.VerifyInterface(interfaceInfo, options.AllowReferences, classInfo.Clsid))
                        outComp.AddInterface(interfaceInfo.Iid, ComPlusTypeValidator.FetchAllMethodsForInterface(interfaceInfo));
                }
                if (mex && (outComp.Interfaces != null))
                    outComp.AddInterface(typeof(IMetadataExchange).GUID, null);
                if (outComp.Interfaces != null)
                    outComps.Add(outComp);
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.NoneOfTheSpecifiedInterfacesForComponentWereFoundSkipping, Tool.Options.ShowGuids ? classInfo.Clsid.ToString("B") : classInfo.Name));

            }
        }

        // returns strongly typed, verified components/interfaces in an application
        static void GetAllComponentsForRemove(ComAdminAppInfo appInfo, out IList<ComponentDefinition<Guid>> outComps)
        {
            outComps = new List<ComponentDefinition<Guid>>();

            foreach (ComAdminClassInfo classInfo in appInfo.Classes)
            {
                ComponentDefinition<Guid> outComp;


                outComp = new ComponentDefinition<Guid>(classInfo.Clsid);

                foreach (ComAdminInterfaceInfo interfaceInfo in classInfo.Interfaces)
                    outComp.AddInterface(interfaceInfo.Iid, null);
                outComp.AddInterface(typeof(IMetadataExchange).GUID, null);
                outComps.Add(outComp);
            }
        }


        // returns strongly typed, verified components, from loosely-typed (string) user inputs
        static void GetComponentsFromInputForAdd(ComAdminAppInfo appInfo, IList<ComponentDefinition<string>> inComps, bool mex, bool priorEndpointsExist, out IList<ComponentDefinition<Guid>> outComps)
        {
            string missingInterface = String.Empty;
            outComps = new List<ComponentDefinition<Guid>>();

            foreach (ComponentDefinition<string> inComp in inComps)
            {
                ComponentDefinition<Guid> outComp = null;

                ComAdminClassInfo classInfo = appInfo.FindClass(inComp.Component);

                if (classInfo == null)
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.CannotFindComponentInApplicationSkipping, inComp.Component, Tool.Options.ShowGuids ? appInfo.ID.ToString("B") : appInfo.Name));
                    continue;
                }

                if (!ValidateClass(classInfo))
                    continue;

                // Find existing componentDef if it was referenced in an earlier iteration
                foreach (ComponentDefinition<Guid> cd in outComps)
                {
                    if (cd.Component == classInfo.Clsid)
                    {
                        outComp = cd;
                    }
                }

                if (outComp == null)
                {
                    outComp = new ComponentDefinition<Guid>(classInfo.Clsid);
                }

                if (inComp.AllInterfaces)
                {
                    foreach (ComAdminInterfaceInfo interfaceInfo in classInfo.Interfaces)
                    {
                        if (ComPlusTypeValidator.VerifyInterface(interfaceInfo, options.AllowReferences, classInfo.Clsid))
                            outComp.AddInterface(interfaceInfo.Iid, ComPlusTypeValidator.FetchAllMethodsForInterface(interfaceInfo));

                    }
                    if ((outComp.Interfaces != null) && mex)
                        outComp.AddInterface(typeof(IMetadataExchange).GUID, null);


                }
                else
                {
                    foreach (InterfaceDefination<string> comInterface in inComp.Interfaces)
                    {
                        string itfName = comInterface.Interface;
                        if (itfName == typeof(IMetadataExchange).GUID.ToString("B"))
                        {
                            if (!mex)
                                outComp.AddInterface(typeof(IMetadataExchange).GUID, null);
                        }
                        else
                        {
                            ComAdminInterfaceInfo interfaceInfo = classInfo.FindInterface(itfName);
                            if (interfaceInfo == null)
                            {
                                ToolConsole.WriteWarning(SR.GetString(SR.CannotFindInterfaceInCatalogForComponentSkipping, itfName, inComp.Component));
                                missingInterface = itfName;
                                continue;
                            }
                            if (comInterface.AllMethods)
                            {
                                if (ComPlusTypeValidator.VerifyInterface(interfaceInfo, options.AllowReferences, classInfo.Clsid, true))
                                    outComp.AddInterface(interfaceInfo.Iid, ComPlusTypeValidator.FetchAllMethodsForInterface(interfaceInfo));
                                else
                                    throw CreateException(SR.GetString(SR.InvalidInterface), null);
                            }
                            else
                            {
                                if (ComPlusTypeValidator.VerifyInterfaceMethods(interfaceInfo, comInterface.Methods, options.AllowReferences, true))
                                    outComp.AddInterface(interfaceInfo.Iid, (List<string>)comInterface.Methods);
                                else
                                    throw CreateException(SR.GetString(SR.InvalidMethod), null);
                            }

                        }
                    }

                    if ((outComp.Interfaces != null) || priorEndpointsExist)
                    {
                        if (mex)
                            outComp.AddInterface(typeof(IMetadataExchange).GUID, null);
                    }

                }
                if (outComp.Interfaces != null)
                    outComps.Add(outComp);
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.NoneOfTheSpecifiedInterfacesForComponentWereFoundSkipping, inComp.Component));
            }

            if (outComps.Count == 0 && (!String.IsNullOrEmpty(missingInterface)))
                throw Tool.CreateException(SR.GetString(SR.NoComponentContainsInterface, missingInterface), null);
        }

        static void GetComponentsFromInputForRemove(ComAdminAppInfo appInfo, IList<ComponentDefinition<string>> inComps, out IList<ComponentDefinition<Guid>> outComps)
        {
            outComps = new List<ComponentDefinition<Guid>>();

            foreach (ComponentDefinition<string> inComp in inComps)
            {
                ComponentDefinition<Guid> outComp = null;

                ComAdminClassInfo classInfo = appInfo.FindClass(inComp.Component);

                if (classInfo == null)
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.CannotFindComponentInApplicationSkipping, inComp.Component, Tool.Options.ShowGuids ? appInfo.ID.ToString("B") : appInfo.Name));
                    continue;
                }


                // Find existing componentDef if it was referenced in an earlier iteration
                foreach (ComponentDefinition<Guid> cd in outComps)
                {
                    if (cd.Component == classInfo.Clsid)
                    {
                        outComp = cd;
                    }
                }

                if (outComp == null)
                {
                    outComp = new ComponentDefinition<Guid>(classInfo.Clsid);
                }

                if (inComp.AllInterfaces)
                {
                    foreach (ComAdminInterfaceInfo interfaceInfo in classInfo.Interfaces)
                        outComp.AddInterface(interfaceInfo.Iid, ComPlusTypeValidator.FetchAllMethodsForInterface(interfaceInfo, false));
                    outComp.AddInterface(typeof(IMetadataExchange).GUID, null);


                }
                else
                {
                    foreach (InterfaceDefination<string> comInterface in inComp.Interfaces)
                    {
                        string itfName = comInterface.Interface;
                        if (itfName == typeof(IMetadataExchange).GUID.ToString("B"))
                        {
                            outComp.AddInterface(typeof(IMetadataExchange).GUID, null);
                        }
                        else
                        {
                            ComAdminInterfaceInfo interfaceInfo = classInfo.FindInterface(itfName);
                            if (interfaceInfo == null)
                            {
                                ToolConsole.WriteWarning(SR.GetString(SR.CannotFindInterfaceInCatalogForComponentSkipping, itfName, inComp.Component));
                                continue;
                            }
                            if (comInterface.AllMethods)
                            {
                                outComp.AddInterface(interfaceInfo.Iid, ComPlusTypeValidator.FetchAllMethodsForInterface(interfaceInfo));
                            }
                            else
                            {

                                outComp.AddInterface(interfaceInfo.Iid, (List<string>)comInterface.Methods);
                            }

                        }
                    }
                }
                if (outComp.Interfaces != null)
                    outComps.Add(outComp);
                else
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.NoneOfTheSpecifiedInterfacesForComponentWereFoundSkipping, inComp.Component));
                }
            }
        }

        public static bool CheckForCorrectOle32()
        {
            Guid clsid = new Guid("0000032E-0000-0000-C000-000000000046");
            IPSFactoryBuffer psFac = SafeNativeMethods.DllGetClassObject(clsid, typeof(IPSFactoryBuffer).GUID) as IPSFactoryBuffer;
            object o1;
            object o2;
            try
            {
                psFac.CreateProxy(IntPtr.Zero, clsid, out o1, out o2);
            }
            catch (ArgumentException)
            {
                return true;
            }
            catch (COMException)
            {
                return false;
            }
            return false;
        }
        public static int Main(string[] args)
        {
            // make sure the text output displays properly on various languages
            Thread.CurrentThread.CurrentUICulture = CultureInfo.CurrentUICulture.GetConsoleFallbackUICulture();
            if ((System.Console.OutputEncoding.CodePage != 65001) &&
                 (System.Console.OutputEncoding.CodePage !=
                  Thread.CurrentThread.CurrentUICulture.TextInfo.OEMCodePage))
            {
                Thread.CurrentThread.CurrentUICulture = new CultureInfo("en-US");
            }

            object serviceConfig = new CServiceConfig();
            IServiceSysTxnConfig sysTxnconfing = serviceConfig as IServiceSysTxnConfig;
            if (sysTxnconfing == null)
            {
                ToolConsole.WriteError(SR.GetString(SR.WindowsFunctionalityMissing), "");
                return 1;
            }
            if ((Environment.OSVersion.Version.Major == 5) && (Environment.OSVersion.Version.Minor == 1))
            {
                if (!CheckForCorrectOle32())
                {
                    ToolConsole.WriteError(SR.GetString(SR.WindowsFunctionalityMissing), "");
                    return 1;
                }
            }
            try
            {
                EnsureUserIsAdministrator();
                Tool.options = Options.ParseArguments(args);
                ToolConsole.Verbose = options.Verbose;

                Run();
            }
            catch (ArgumentException ae)
            {
                ToolConsole.WriteError(ae);
                Console.WriteLine(SR.GetString(SR.MoreHelp, Abbr.Help));
                return 1;
            }
            catch (ApplicationException appException)
            {
                ToolConsole.WriteError(appException);
                return 1;
            }
            catch (Exception e)
            {
                if (e is NullReferenceException || e is SEHException)
                {
                    throw;
                }
                ToolConsole.WriteDetailedException(e, SR.GetString(SR.UnExpectedError));
                return 1;
            }
            return 0;
        }

        static void Run()
        {
            if (!options.NoLogo)
            {
                DisplayLogo();
            }

            if (options.Help)
            {
                ToolConsole.Verbose = false;  // For Help-mode, ignore quiet flag
                DisplayHelp(options.Mode);
                return;
            }

            switch (options.Mode)
            {
                case Mode.NotSpecified:
                    {
                        throw CreateArgumentException(Cmd.Mode, "", SR.GetString(SR.ArgumentRequired, Cmd.Mode), null);
                    }
                case Mode.Install:
                    {
                        DoInstall();
                        break;
                    }
                case Mode.Uninstall:
                    {
                        DoUninstall();
                        break;
                    }
                case Mode.List:
                    {
                        DoList();
                        break;
                    }
                default:
                    {
                        Debug.Assert(false, "unknown mode");
                        break;
                    }
            }
        }

        static void ValidateAddParams()
        {
            if (options.Application == null)
            {
                throw CreateArgumentException(Cmd.Application, null, SR.GetString(SR.ArgumentRequired, Cmd.Application), null);
            }

            if (!options.AllComponents && ((options.Components == null) || options.Components.Count == 0))
            {
                throw CreateArgumentException(Cmd.Contract, null, SR.GetString(SR.ArgumentRequired, Cmd.Contract), null);
            }

            switch (options.Hosting)
            {
                case Hosting.NotSpecified:
                    {
                        throw CreateArgumentException(Cmd.Hosting, null, SR.GetString(SR.ArgumentRequired, Cmd.Hosting), null);
                    }
                case Hosting.Complus:
                    {
                        if (options.WebDirectory != null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebDirectory), null);
                        }
                        if (options.WebServer != null)
                        {
                            throw CreateArgumentException(Cmd.WebServer, options.WebServer, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebServer), null);
                        }
                        break;
                    }
                case Hosting.Was:
                    {
                        if (options.WebDirectory == null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, null, SR.GetString(SR.ArgumentRequired, Cmd.WebDirectory), null);
                        }
                        break;
                    }
            }
        }

        static void ValidateRemoveParams()
        {
            if (options.Application == null)
            {
                throw CreateArgumentException(Cmd.Application, null, SR.GetString(SR.ArgumentRequired, Cmd.Application), null);
            }

            if (!options.AllComponents && ((options.Components == null) || options.Components.Count == 0))
            {
                throw CreateArgumentException(Cmd.Contract, null, SR.GetString(SR.ArgumentRequired, Cmd.Contract), null);
            }

            switch (options.Hosting)
            {
                case Hosting.NotSpecified:
                    {
                        if (options.WebDirectory != null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebDirectory), null);
                        }
                        break;

                    }
                case Hosting.Complus:
                    {
                        if (options.WebDirectory != null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebDirectory), null);
                        }
                        if (options.WebServer != null)
                        {
                            throw CreateArgumentException(Cmd.WebServer, options.WebServer, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebServer), null);
                        }
                        break;
                    }
                case Hosting.Was:
                    {
                        break;
                    }
            }
        }

        static void ValidateQueryParams()
        {

            if (options.AllComponents || ((options.Components != null) && options.Components.Count > 0))
            {
                throw CreateArgumentException(Cmd.Contract, null, SR.GetString(SR.ExclusiveOptionsSpecified, Cmd.Contract, Cmd.Mode + ":query"), null);
            }

            switch (options.Hosting)
            {
                case Hosting.NotSpecified:
                    {
                        if (options.WebDirectory != null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebDirectory), null);
                        }
                        break;

                    }
                case Hosting.Complus:
                    {
                        if (options.WebDirectory != null)
                        {
                            throw CreateArgumentException(Cmd.WebDirectory, options.WebDirectory, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebDirectory), null);
                        }
                        if (options.WebServer != null)
                        {
                            throw CreateArgumentException(Cmd.WebServer, options.WebServer, SR.GetString(SR.InvalidArgumentForHostingMode, Cmd.WebServer), null);
                        }
                        break;
                    }
                case Hosting.Was:
                    {
                        break;
                    }
            }
        }

        static void ValidateApplication(ComAdminAppInfo appInfo, Hosting hosting)
        {
            if (appInfo.IsSystemApplication)
            {
                throw CreateArgumentException(Cmd.Application, appInfo.Name, SR.GetString(SR.SystemApplicationsNotSupported), null);
            }

            if (hosting == Hosting.Complus)
            {
                if (!appInfo.IsServerActivated)
                {
                    throw CreateArgumentException(Cmd.Application, appInfo.Name, SR.GetString(SR.LibraryApplicationsNotSupported), null);
                }

                if (appInfo.IsAutomaticRecycling)
                {
                    throw CreateArgumentException(Cmd.Application, appInfo.Name, SR.GetString(SR.ProcessRecyclingNotSupported), null);
                }

                if (appInfo.IsProcessPooled)
                {
                    throw CreateArgumentException(Cmd.Application, appInfo.Name, SR.GetString(SR.ProcessPoolingNotSupported), null);
                }
            }
        }

        static bool ValidateClass(ComAdminClassInfo classInfo)
        {
            if (classInfo.IsPrivate)
            {
                ToolConsole.WriteWarning(SR.GetString(SR.CannotExposePrivateComponentsSkipping, Tool.Options.ShowGuids ? classInfo.Clsid.ToString("B") : classInfo.Name));
                return false;
            }
            return true;
        }



    }
}
