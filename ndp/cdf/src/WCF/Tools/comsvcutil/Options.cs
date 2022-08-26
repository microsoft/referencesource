//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Description;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Reflection;
    using System.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using Microsoft.Tools.ServiceModel;

    enum Mode
    {
        NotSpecified,
        Install,
        Uninstall,
        List
    }

    enum Hosting
    {
        NotSpecified,
        Complus,
        Was
    }

    static class Cmd
    {
        internal const string Application = "application";
        internal const string Help = "help";
        internal const string Hosting = "hosting";
        internal const string Contract = "contract";
        internal const string Mode = "mode";
        internal const string NoLogo = "nologo";
        internal const string Verbose = "verbose";
        internal const string WebDirectory = "webDirectory";
        internal const string WebServer = "webSite";
        internal const string ID = "id";
        internal const string MetaData = "mex";
        internal const string Install = "install";
        internal const string List = "list";
        internal const string Uninstall = "uninstall";
        internal const string AllowReferences = "allowreferences";
    }

    static class Abbr
    {
        internal const string Application = "a";
        internal const string Help = "?";
        internal const string Hosting = "h";
        internal const string Contract = "c";
        internal const string Mode = "m";
        internal const string NoLogo = "n";
        internal const string Verbose = "v";
        internal const string WebDirectory = "d";
        internal const string WebServer = "w";
        internal const string ID = "k";
        internal const string MetaData = "x";
        internal const string Install = "i";
        internal const string List = "l";
        internal const string Uninstall = "u";
        internal const string AllowReferences = "r";
    }
    class InterfaceDefination<T>
    {
        T iinterface;
        List<string> methods;


        public InterfaceDefination(T iinterface, List<string> methods)
        {
            this.iinterface = iinterface;
            this.methods = methods;

        }
        public bool AllMethods
        {
            get
            {
                if (methods == null)
                    return true;
                else
                    return false;
            }
        }
        public T Interface { get { return this.iinterface; } }
        public IList<string> Methods { get { return this.methods; } }

    }

    class ComponentDefinition<T>
    {
        bool allInterfaces;
        T component;
        List<InterfaceDefination<T>> interfaces;

        public bool AllInterfaces { get { return this.allInterfaces; } }
        public T Component { get { return this.component; } }
        public IList<InterfaceDefination<T>> Interfaces { get { return this.interfaces; } }

        public ComponentDefinition(T component)
        {
            this.allInterfaces = false;
            this.component = component;
            this.interfaces = null;
        }

        public void AddInterface(T itf, List<string> methods)
        {
            if (this.AllInterfaces)
            {
                throw Tool.CreateArgumentException(Cmd.Contract, this.component.ToString() + "," + itf.ToString(), SR.GetString(SR.AllInterfacesAlreadySelected, this.component.ToString()), null);
            }
            if (this.interfaces == null)
            {
                this.interfaces = new List<InterfaceDefination<T>>();
            }
            InterfaceDefination<T> interfaceDefination = new InterfaceDefination<T>(itf, methods);
            this.interfaces.Add(interfaceDefination);
        }

        public void SetAllInterfaces()
        {
            if (this.interfaces != null || (this.allInterfaces == true))
            {
                throw Tool.CreateArgumentException(Cmd.Contract, this.component.ToString() + ",*", SR.GetString(SR.CannotSpecifyAllInterfaces), null);
            }

            this.allInterfaces = true;
        }
    }

    // This class does just the conversion of command line options to strongly-typed params
    // It does only very basic checking (i.e. allowable interface, etc.)
    // But most other checking (i.e. what params are allowed for what modes), is left to the consumer
    class Options
    {
        bool allComponents;
        string application;
        bool help;
        Hosting hosting;
        IList<ComponentDefinition<string>> components;
        Mode mode;
        bool noLogo;
        bool verbose;
        string webDirectory;
        string webServer;
        bool showGuids;
        bool allowReferences;

        bool mex;

        string mexOnlyComponent = String.Empty;

        internal bool AllComponents { get { return this.allComponents; } }
        internal string Application { get { return this.application; } }
        internal IList<ComponentDefinition<string>> Components { get { return this.components; } }
        internal bool Help { get { return this.help; } }
        internal Hosting Hosting { get { return this.hosting; } }
        internal Mode Mode { get { return this.mode; } }
        internal bool NoLogo { get { return this.noLogo; } }
        internal bool Verbose { get { return this.verbose; } }
        internal string WebDirectory { get { return this.webDirectory; } }
        internal string WebServer { get { return this.webServer; } }
        internal bool ShowGuids { get { return showGuids; } }
        internal bool AllowReferences { get { return allowReferences; } }

        internal bool Mex { get { return mex; } }

        internal string MexOnlyComponent { get { return this.mexOnlyComponent; } }


        static CommandSwitch[] switches = new CommandSwitch[] {
                new CommandSwitch(Cmd.Application, Abbr.Application, SwitchType.SingletonValue),
                new CommandSwitch(Cmd.Help, Abbr.Help, SwitchType.Flag),
                new CommandSwitch(Cmd.Hosting, Abbr.Hosting, SwitchType.SingletonValue), 
                new CommandSwitch(Cmd.Contract, Abbr.Contract, SwitchType.SingletonValue),
                new CommandSwitch(Cmd.Mode, Abbr.Mode, SwitchType.SingletonValue),
                new CommandSwitch(Cmd.NoLogo, Abbr.NoLogo, SwitchType.Flag),
                new CommandSwitch(Cmd.Verbose, Abbr.Verbose, SwitchType.Flag),
                new CommandSwitch(Cmd.WebDirectory, Abbr.WebDirectory, SwitchType.SingletonValue),
                new CommandSwitch(Cmd.WebServer, Abbr.WebServer, SwitchType.SingletonValue),
                new CommandSwitch(Cmd.ID, Abbr.ID, SwitchType.Flag),
                new CommandSwitch(Cmd.MetaData, Abbr.MetaData, SwitchType.Flag),
                new CommandSwitch(Cmd.Install, Abbr.Install, SwitchType.Flag),
                new CommandSwitch(Cmd.Uninstall, Abbr.Uninstall, SwitchType.Flag),
                new CommandSwitch(Cmd.List, Abbr.List, SwitchType.Flag),
                new CommandSwitch(Cmd.AllowReferences, Abbr.AllowReferences, SwitchType.Flag)
        };

        Options(Mode mode, ArgumentDictionary arguments)
        {
            if (arguments == null)
            {
                help = true;
                return;
            }
            this.mode = mode;
            // Application
            if (arguments.ContainsArgument(Cmd.Application))
            {
                this.application = arguments.GetArgument(Cmd.Application);
            }

            // Help
            this.help = arguments.ContainsArgument(Cmd.Help);

            // Hosting
            this.hosting = Hosting.NotSpecified;

            if (arguments.ContainsArgument(Cmd.Hosting))
            {
                string argValue = arguments.GetArgument(Cmd.Hosting);
                if (string.Equals(argValue, Enum.GetName(typeof(Hosting), Hosting.Complus), StringComparison.OrdinalIgnoreCase))
                {
                    this.hosting = Hosting.Complus;
                }
                else if (string.Equals(argValue, Enum.GetName(typeof(Hosting), Hosting.Was), StringComparison.OrdinalIgnoreCase))
                {
                    if (WasAdminWrapper.IsIISInstalled())
                        this.hosting = Hosting.Was;
                    else
                        throw Tool.CreateException(SR.GetString(SR.IISNotInstalled, argValue), null);
                }
                else
                {
                    throw Tool.CreateException(SR.GetString(SR.UnknownHostingSpecified, argValue), null);
                }
            }

            this.mex = arguments.ContainsArgument(Cmd.MetaData);

            // Interface
            this.components = null;
            this.allComponents = false;
            if (arguments.ContainsArgument(Cmd.Contract))
            {
                IList<string> argValues = arguments.GetArguments(Cmd.Contract);
                ParseInterfaces(argValues);
            }



            // NoLogo
            this.noLogo = arguments.ContainsArgument(Cmd.NoLogo);
            if (this.noLogo && arguments.Count == 1)
                this.help = true;

            // Verbose
            this.verbose = arguments.ContainsArgument(Cmd.Verbose);

            // WebDirectory
            if (arguments.ContainsArgument(Cmd.WebDirectory))
            {
                this.webDirectory = arguments.GetArgument(Cmd.WebDirectory);
            }

            // WebServer
            if (arguments.ContainsArgument(Cmd.WebServer))
            {
                this.webServer = arguments.GetArgument(Cmd.WebServer);
            }

            this.showGuids = arguments.ContainsArgument(Cmd.ID);

            this.allowReferences = arguments.ContainsArgument(Cmd.AllowReferences);
        }

        internal static Options ParseArguments(string[] args)
        {

            Mode mode = Mode.NotSpecified;
            Options options = null;
            if (args.Length > 0)
            {
                ArgumentDictionary arguments = CommandParser.ParseCommand(args, switches);

                if (arguments.ContainsArgument(Cmd.Install))
                {
                    mode = Mode.Install;
                }

                if (arguments.ContainsArgument(Cmd.Uninstall))
                {
                    if (mode != Mode.NotSpecified)
                        throw Tool.CreateException(SR.GetString(SR.MultipleModeArguments), null);

                    mode = Mode.Uninstall;
                }

                if (arguments.ContainsArgument(Cmd.List))
                {
                    if (mode != Mode.NotSpecified)
                        throw Tool.CreateException(SR.GetString(SR.MultipleModeArguments), null);

                    mode = Mode.List;
                }
                options = new Options(mode, arguments);

            }
            else
                return new Options(mode, null);

            if (!options.Help && (mode == Mode.NotSpecified))
                throw Tool.CreateException(SR.GetString(SR.ModeArgumentMissing), null);

            return options;

        }

        void ParseInterfaces(IList<string> argValues)
        {
            this.components = null;
            this.allComponents = false;

            // Handle the case of allComponents first
            if (argValues.Count == 1 && argValues[0] == "*")
            {
                this.allComponents = true;
                return;
            }

            // So much for that; this point on, we cant have allComponents set to true

            Dictionary<string, ComponentDefinition<string>> comps = new Dictionary<string, ComponentDefinition<string>>();
            foreach (string argVal in argValues)
            {
                string comp = null;
                string itf = null;
                ComponentDefinition<string> compDef = null;

                if (argVal.Contains(","))
                {
                    string[] argParts = argVal.Split(new char[] { ',' }, 2);
                    comp = argParts[0];
                    itf = argParts[1];
                }
                else if (!mex)
                {
                    // No comma, well this is malformed then
                    throw Tool.CreateArgumentException(Cmd.Contract, argVal, SR.GetString(SR.MalformedInterfaceString), null);
                }
                else if (mex)
                {
                    comp = argVal;
                    itf = typeof(IMetadataExchange).GUID.ToString("B");
                    if (String.Empty == mexOnlyComponent)
                        mexOnlyComponent = argVal;
                }

                if (comp.Equals("*"))
                {
                    throw Tool.CreateArgumentException(Cmd.Contract, argVal, SR.GetString(SR.CannotSpecifyInterfaceForAllComponents), null);
                }

                if (!comps.TryGetValue(comp, out compDef))
                {
                    compDef = new ComponentDefinition<string>(comp);
                    comps.Add(comp, compDef);
                }
                if (itf.Equals("*"))
                {
                    compDef.SetAllInterfaces();
                }
                else
                {
                    string[] elements = itf.Split(new char[] { '.' }, 2);
                    // Just interface specified
                    if (elements.Length == 1)
                    {
                        compDef.AddInterface(itf, null);
                    }
                    else
                    {


                        itf = elements[0];
                        string methods = elements[1];
                        if (methods == "*")
                            compDef.AddInterface(itf, null);
                        else
                        {
                            if ((methods[0] != '{') && (methods[methods.Length - 1] != '}'))
                                throw Tool.CreateException(SR.GetString(SR.BadMethodParameter, argVal), null);
                            methods = methods.Substring(1, methods.Length - 2);
                            string[] methodNames = methods.Split(new char[] { ',' });
                            if (methodNames.Length == 0)
                                throw Tool.CreateException(SR.GetString(SR.NoMethodsSpecifiedInArgument, argVal), null);
                            List<string> methodList = new List<string>();
                            foreach (string method in methodNames)
                                methodList.Add(method);
                            compDef.AddInterface(itf, methodList);
                        }
                    }
                }
            }

            this.components = new List<ComponentDefinition<string>>(comps.Count);
            foreach (ComponentDefinition<string> compDef in comps.Values)
            {
                this.components.Add(compDef);
            }
        }
    }
}
