//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

[module: System.Diagnostics.CodeAnalysis.SuppressMessage(System.Runtime.FxCop.Category.Performance, System.Runtime.FxCop.Rule.AvoidUncalledPrivateCode, Scope = "member", Target = "Microsoft.Activities.Build.SR.get_InvalidValueForDisableWorkflowCompiledExpressions():System.String", Justification = "This resource string is only referenced from the targets file.")]

namespace Microsoft.Activities.Build.Expressions
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Build.Tasks.Xaml;
    using System.Activities;
    using System.Activities.XamlIntegration;
    using System.Reflection;
    using System.IO;
    using System.Runtime;
    using System.CodeDom.Compiler;
    using Microsoft.Activities.Build.Validation;

    public class ExpressionsBuildExtension : IXamlBuildTypeInspectionExtension
    {
        static string fileNameSuffix = "_CompiledExpressionRoot";
        XamlBuildTypeInspectionExtensionContext buildContext;
        List<string> generatedFiles;
        List<Tuple<string, bool>> messages;

        public ExpressionsBuildExtension()
        {
        }

        public bool Execute(XamlBuildTypeInspectionExtensionContext buildContext)
        {
            if (buildContext == null)
            {
                throw FxTrace.Exception.AsError(new ArgumentNullException("buildContext"));
            }

            this.buildContext = buildContext;
            string deferredValidationErrorsFilePath = Path.Combine(this.buildContext.OutputPath, ValidationBuildExtension.DeferredValidationErrorsFileName);
            if (string.Equals(this.buildContext.Language, "VB", StringComparison.OrdinalIgnoreCase) && File.Exists(deferredValidationErrorsFilePath))
            {                
                List<ValidationBuildExtension.Violation> violations = ReportDeferredValidationErrorsTask.LoadDeferredValidationErrors(deferredValidationErrorsFilePath);
                if (violations != null && violations.Count > 0)
                {                    
                    // ValidationBuildExtension must have run prior to ExpressionBuildExtension.
                    // If ValidationBuildExtension had cached any validation errors including VB Hosted compiler errors, 
                    //  then we do not generate the compiled expression code for VB.
                    return true;
                }
            }
            
            this.generatedFiles = new List<string>();
            this.messages = new List<Tuple<string, bool>>();

            try
            {
                bool success = Execute();

                foreach (string fileName in this.generatedFiles)
                {
                    buildContext.AddGeneratedFile(fileName);
                }

                foreach (Tuple<string, bool> message in this.messages)
                {
                    if (message.Item2)
                    {
                        buildContext.XamlBuildLogger.LogError(message.Item1);
                    }
                    else
                    {
                        buildContext.XamlBuildLogger.LogMessage(message.Item1);
                    }
                }

                return success;
            }
            catch (BadImageFormatException bex)
            {
                buildContext.XamlBuildLogger.LogWarning(SR.BadImageFormat_Expression(bex.FileName));

                // We don't want to add the generated files to the project, since compilation was incomplete;
                // so we're responsible for cleaning them up ourselves.
                try
                {
                    foreach (string fileName in this.generatedFiles)
                    {
                        File.Delete(fileName);
                    }
                }
                catch (IOException)
                {
                    // cleanup is best-effort
                }

                return true;
            }
        }

        bool Execute()
        {
            Assembly localAssembly = Utilities.GetLocalAssembly(buildContext, SR.LocalAssemblyNotLoaded_Expressions);            

            foreach (Type type in Utilities.GetTypes(localAssembly))
            {
                if (Utilities.IsTypeAuthoredInXaml(type))
                {
                    Exception ctorException;
                    Activity activity = Utilities.CreateActivity(type, out ctorException);
                    if (ctorException != null)
                    {
                        LogError(SR.ExpressionBuildExtensionConstructorFailed(type.FullName, ctorException != null ? ctorException.Message : string.Empty));
                    }
                    else if (activity != null)
                    {
                        string activityName = activity.GetType().Name;

                        string activityNamespace = "";
                        string fullActivityNamespace = activity.GetType().Namespace; 
                        if (string.Equals(buildContext.Language, "VB", StringComparison.OrdinalIgnoreCase))
                        {
                            if (string.IsNullOrWhiteSpace(buildContext.RootNamespace))
                            {
                                activityNamespace = fullActivityNamespace;
                            }
                            else
                            {
                                int firstIndex = fullActivityNamespace.IndexOf(buildContext.RootNamespace, StringComparison.Ordinal);                                
                                if (firstIndex != -1)
                                {
                                    int subStringIndex = firstIndex + buildContext.RootNamespace.Length + 1;
                                    if (subStringIndex < fullActivityNamespace.Length)
                                    {
                                        activityNamespace = fullActivityNamespace.Substring(subStringIndex);
                                    }
                                    else
                                    {
                                        activityNamespace = "";
                                    }
                                }
                                else
                                {
                                    activityNamespace = "";
                                }
                            }
                        }
                        else
                        {
                            activityNamespace = fullActivityNamespace;
                        }
                        

                        TextExpressionCompiler compiler = new TextExpressionCompiler(
                            new TextExpressionCompilerSettings()
                            {
                                Activity = activity,
                                ActivityName = activityName,
                                ActivityNamespace = activityNamespace,
                                Language = buildContext.Language,
                                RootNamespace = buildContext.RootNamespace,
                                LogSourceGenerationMessage = LogMessage,
                                AlwaysGenerateSource = false
                            });
                        
                        string filePath = Path.GetFullPath(buildContext.OutputPath);
                        string codeFileName = Path.Combine(filePath, activityNamespace + "_" + activityName + fileNameSuffix + "." + CodeDomProvider.CreateProvider(this.buildContext.Language).FileExtension);

                        bool fileWritten = false;
                        using (StreamWriter fileStream = new StreamWriter(codeFileName))
                        {
                            try
                            {
                                fileWritten = compiler.GenerateSource(fileStream);
                            }
                            catch (Exception ex)
                            {
                                if (Fx.IsFatal(ex))
                                {
                                    throw;
                                }
                                LogError(ex.Message);
                            }
                        }

                        if (fileWritten)
                        {
                            // Batch up all file generation until the end, because we don't want to emit 
                            // any source if we can't complete compilation due to an unloadable reference.
                            this.generatedFiles.Add(codeFileName);
                        }
                    }
                }
            }
            return true;
        }

        // Batch up all errors and warnings, because we don't want to emit any messages
        // if we can't complete compilation due to an unloadable reference.
        void LogMessage(string message)
        {
            this.messages.Add(Tuple.Create(message, false));
        }

        void LogError(string message)
        {
            this.messages.Add(Tuple.Create(message, true));
        }
    }
}
