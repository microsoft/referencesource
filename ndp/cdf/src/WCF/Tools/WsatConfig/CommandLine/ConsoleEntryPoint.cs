//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Reflection;
    using System.Globalization;
    using System.Diagnostics;
    using System.Threading;
    using System.Security.Cryptography.X509Certificates;

    class ConsoleEntryPoint
    {
        enum Operations
        {
            None = 0,
            Help,
            Change
        }

        // the configuration received from the user
        WsatConfiguration newConfig;

        // the current configuration loaded from the registry
        WsatConfiguration previousConfig;

        // possible operations
        Operations operation;
        
        // restart required
        bool restartRequired = false;
        bool showRequired = false;

        // DTC cluster server name
        string virtualServer;

        // console entry point
        [STAThread]
        public static int Main(String[] args)
        {
            try
            {
                ValidateUICulture();

                PrintBanner();

                ConsoleEntryPoint tool = new ConsoleEntryPoint(args);
                tool.Run();
                return 0;
            }
            catch (WsatAdminException wsatEx)
            {
                Console.WriteLine();
                Console.WriteLine(wsatEx.Message);
                return (int)wsatEx.ErrorCode;
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                Console.WriteLine(SR.GetString(SR.UnexpectedError, e.Message));
                return (int)WsatAdminErrorCode.UNEXPECTED_ERROR;
            }
        }

        static void PrintBanner()
        {
            // Using CommonResStrings.WcfTrademarkForCmdLine for the trademark: the proper resource for command line tools.
            Console.WriteLine();
            Console.WriteLine(SR.GetString(SR.ConsoleBannerLine01));
            Console.WriteLine(SR.GetString(SR.ConsoleBannerLine02, CommonResStrings.WcfTrademarkForCmdLine, ThisAssembly.InformationalVersion));
            Console.WriteLine(SR.GetString(SR.ConsoleBannerLine03, CommonResStrings.CopyrightForCmdLine));
        }

        static void PrintUsage()
        {
            OptionUsage.Print();
        }

        ConsoleEntryPoint(string[] argv)
        {
            List<string> arguments = PrescanArgs(argv);
            if (this.operation != Operations.Help)
            {
                previousConfig = new WsatConfiguration(null, this.virtualServer, null, true);
                previousConfig.LoadFromRegistry();
                newConfig = new WsatConfiguration(null, this.virtualServer, previousConfig, true);
                ParseArgs(arguments);
            }
        }

        // Execute the user's command
        void Run()
        {
            switch (operation)
            {
                case Operations.Change:
                    newConfig.ValidateThrow();
                    if (restartRequired)
                    {
                        Console.WriteLine(SR.GetString(SR.InfoRestartingMSDTC));
                    }
                    newConfig.Save(restartRequired);
                    if (restartRequired)
                    {
                        Console.WriteLine(SR.GetString(SR.InfoRestartedMSDTC));
                    }
                    if (newConfig.IsClustered)
                    {
                        Console.WriteLine(SR.GetString(SR.ClusterConfigUpdatedSuccessfully));
                    }
                    else
                    {
                        Console.WriteLine(SR.GetString(SR.ConfigUpdatedSuccessfully));
                    }
                    break;
                case Operations.None:
                    // does nothing
                    break;
                case Operations.Help:
                    // fall through
                default:
                    PrintUsage();
                    break; 
            }

            if (operation != Operations.Change && restartRequired)
            {
                try
                {
                    MsdtcWrapper msdtc = newConfig.GetMsdtcWrapper();
                    Console.WriteLine(SR.GetString(SR.InfoRestartingMSDTC));
                    msdtc.RestartDtcService();
                    Console.WriteLine(SR.GetString(SR.InfoRestartedMSDTC));
                }
                catch (WsatAdminException)
                {
                    throw;
                }
#pragma warning suppress 56500
                catch (Exception e)
                {
                    if (Utilities.IsCriticalException(e))
                    {
                        throw;
                    }
                    throw new WsatAdminException(WsatAdminErrorCode.DTC_RESTART_ERROR, SR.GetString(SR.ErrorRestartMSDTC), e);
                }
            }

            if (this.showRequired)
            {
                Console.WriteLine();
                Console.WriteLine(SR.GetString(SR.ConsoleShowInformation));
                Console.WriteLine(operation == Operations.Change ? this.newConfig.ToString() : this.previousConfig.ToString());
            }
        }

        List<string> PrescanArgs(string[] argv)
        {
            List<string> arguments = new List<string>();

            if (argv.Length < 1)
            {
                this.operation = Operations.Help;
            }
            else
            {
                this.operation = Operations.None;
                foreach (string rawArg in argv)
                {
                    // -?, -h, -help
                    string arg = ProcessArg(rawArg);
                    if (Utilities.SafeCompare(arg, CommandLineOption.Help) ||
                        Utilities.SafeCompare(arg, CommandLineOption.Help_short1) ||
                        Utilities.SafeCompare(arg, CommandLineOption.Help_short2))
                    {
                        this.operation = Operations.Help;
                        break;
                    }

                    // -show
                    if (Utilities.SafeCompare(arg, CommandLineOption.Show))
                    {
                        this.showRequired = true;
                        continue;
                    }

                    // -restart
                    if (Utilities.SafeCompare(arg, CommandLineOption.Restart))
                    {
                        this.restartRequired = true;
                        continue;
                    }

                    // -virtualServer
                    string value;
                    if (Utilities.SafeCompare(ArgumentsParser.ExtractOption(arg, out value), CommandLineOption.ClusterVirtualServer))
                    {
                        this.virtualServer = value;
                        continue;
                    }
                    arguments.Add(arg);
                }
            }
            return arguments;
        }

        void ParseArgs(List<string> arguments)
        {
            ArgumentsParser parser = new ArgumentsParser(newConfig);

            foreach (string arg in arguments)
            {
                if (parser.ParseOptionAndArgument(arg))
                {
                    this.operation = Operations.Change;
                }
                else
                {
                    // otherwise, we have encountered something we dont recognize
                    throw new WsatAdminException(WsatAdminErrorCode.INVALID_ARGUMENT, SR.GetString(SR.ErrorArgument));
                }
            }

            parser.ValidateArgumentsThrow();
        }

        // strip off the switch prefix and get the real argument
        static string ProcessArg(string rawArg)
        {
            if (String.IsNullOrEmpty(rawArg))
            {
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_ARGUMENT, SR.GetString(SR.ErrorArgument));
            }

            if (rawArg[0] != '/' && rawArg[0] != '-')
            {
                throw new WsatAdminException(WsatAdminErrorCode.INVALID_ARGUMENT, SR.GetString(SR.ErrorArgument));
            }

            return rawArg.Substring(1);
        }

        // Since we are outputing to the console, force the current UI culture to be the
        // console fallback UI culture since the console cannot support all code pages for
        // certain cultures (i.e. it will print garbage characters).  Unfortunately, there
        // are some console fallback cultures that still use code pages incompatible with
        // the console.  Catch those cases and fall back to English because ASCII is widely
        // accepted in OEM code pages.
        static void ValidateUICulture()
        {
            Thread.CurrentThread.CurrentUICulture = CultureInfo.CurrentUICulture.GetConsoleFallbackUICulture();

            if ((System.Console.OutputEncoding.CodePage != Encoding.UTF8.CodePage) &&
                (System.Console.OutputEncoding.CodePage != Thread.CurrentThread.CurrentUICulture.TextInfo.OEMCodePage))
            {
                Thread.CurrentThread.CurrentUICulture = new CultureInfo("en-US");
            }
        }
    }
}
