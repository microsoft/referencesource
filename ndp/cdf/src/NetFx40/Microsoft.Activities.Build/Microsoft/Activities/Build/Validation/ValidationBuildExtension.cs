//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace Microsoft.Activities.Build.Validation
{
    using System;
    using System.Activities;
    using System.Activities.Debugger;
    using System.Activities.Debugger.Symbol;
    using System.Activities.Validation;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Reflection;
    using System.Runtime;
    using System.Runtime.Serialization;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Tasks.Xaml;
    using Microsoft.Build.Utilities;

    [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUninstantiatedInternalClasses,
            Justification = "This type is used in targets file. It is instantiated while running XBT pass 2 extensions.")]
    class ValidationBuildExtension : IXamlBuildTypeInspectionExtension
    {
        public const string DeferredValidationErrorsFileName = "AC2C1ABA-CCF6-44D4-8127-588FD4D0A860-DeferredValidationErrors.xml";

        XamlBuildTypeInspectionExtensionContext buildContext;
        List<Violation> violations;

        public ValidationBuildExtension()
        {
        }

        public bool Execute(XamlBuildTypeInspectionExtensionContext buildContext)
        {
            if (buildContext == null)
            {
                throw FxTrace.Exception.AsError(new ArgumentNullException("buildContext"));
            }
            this.buildContext = buildContext;
            this.violations = new List<Violation>();

            try
            {
                this.Execute();

                // Delay validation report and always returns true in order to let CoreCompile to compile first.
                // CoreCompile will report expression compile errors and duplicated errors will be merged by Microsoft.VisualStudio.Activities.BuildHelper later.
                // ReportValidationBuildExtensionErrors will report validation errors after CoreCompile done.
                this.WriteViolationsToDeferredErrorsFile();

                return true;
            }
            catch (BadImageFormatException bex)
            {
                buildContext.XamlBuildLogger.LogWarning(SR.BadImageFormat_Validation(bex.FileName));
                return true;
            }
        }

        void WriteViolationsToDeferredErrorsFile()
        {
            // OutputPath here is the intermediate output path
            string filePath = Path.Combine(this.buildContext.OutputPath, DeferredValidationErrorsFileName);
            using (FileStream stream = new FileStream(filePath, FileMode.Create, FileAccess.Write))
            {
                DataContractSerializer serializer = new DataContractSerializer(typeof(List<Violation>));
                serializer.WriteObject(stream, this.violations);
            }
        }

        void Execute()
        {
            Assembly localAssembly = Utilities.GetLocalAssembly(this.buildContext, SR.LocalAssemblyNotLoaded);

            foreach (Type type in Utilities.GetTypes(localAssembly))
            {
                // Check if the type is authored in xaml
                if (Utilities.IsTypeAuthoredInXaml(type))
                {
                    // Check if the file is marked with SkipWorkflowValidation = true
                    ITaskItem inputTaskItem = null;
                    buildContext.MarkupItemsByTypeName.TryGetValue(type.FullName, out inputTaskItem);
                    if (!SkipValidationForFile(inputTaskItem))
                    {
                        string fileName = inputTaskItem != null ? inputTaskItem.ItemSpec : String.Empty;
                        Exception ex;
                        Activity activity = Utilities.CreateActivity(type, out ex);
                        if (ex != null)
                        {
                            string message = SR.ValidationBuildExtensionConstructorFailed(type.FullName, ex != null ? ex.Message : string.Empty);
                            this.violations.Add(new Violation(fileName, message));
                        }
                        else if (activity != null)
                        {
                            this.Validate(activity, fileName);
                        }
                    }
                }
            }
        }

        static IDictionary<Activity, Activity> GetParentChildRelationships(Activity activity)
        {
            IDictionary<Activity, Activity> parentChildMappings = new Dictionary<Activity, Activity>();
            InternalGetParentChildRelationships(activity, null, parentChildMappings);
            return parentChildMappings;
        }

        static void InternalGetParentChildRelationships(Activity activity, Activity parent, IDictionary<Activity, Activity> parentChildMappings)
        {
            if (!parentChildMappings.ContainsKey(activity))
            {
                // the very first parent declaring the child activity
                // and the rest are parent to reference child relationships
                parentChildMappings.Add(activity, parent);
            }

            foreach (Activity child in WorkflowInspectionServices.GetActivities(activity))
            {
                InternalGetParentChildRelationships(child, activity, parentChildMappings);
            }
        }

        void Validate(Activity activity, string fileName)
        {
            List<ValidationError> validationErrors = new List<ValidationError>();

            ValidationSettings settings = new ValidationSettings()
            {
                SkipValidatingRootConfiguration = true
            };
            ValidationResults results = null;
            try
            {
                results = ActivityValidationServices.Validate(activity, settings);
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                ValidationError error = new ValidationError(SR.ValidationBuildExtensionExceptionPrefix(typeof(ValidationBuildExtension).Name, activity.DisplayName, e.Message));
                validationErrors.Add(error);
            }

            if (results != null)
            {
                validationErrors.AddRange(results.Errors);
                validationErrors.AddRange(results.Warnings);
            }

            if (validationErrors.Count > 0)
            {
                Dictionary<object, SourceLocation> sourceLocations;

                sourceLocations = GetErrorInformation(activity);

                IDictionary<Activity, Activity> parentChildMappings = GetParentChildRelationships(activity);

                Activity errorSource;
                foreach (ValidationError violation in validationErrors)
                {
                    bool foundSourceLocation = false;
                    SourceLocation violationLocation = null;

                    errorSource = violation.Source;

                    if (sourceLocations != null)
                    {
                        if (violation.SourceDetail != null)
                        {
                            foundSourceLocation = sourceLocations.TryGetValue(violation.SourceDetail, out violationLocation);
                        }
                        // SourceLocation points to the erroneous activity
                        // If the errorneous activity does not have SourceLocation attached, 
                        // for instance, debugger does not attach SourceLocations for expressions
                        // then the SourceLocation points to the first parent activity in the 
                        // parent chain which has SourceLocation attached.

                        while (!foundSourceLocation && errorSource != null)
                        {
                            foundSourceLocation = sourceLocations.TryGetValue(errorSource, out violationLocation);
                            if (!foundSourceLocation)
                            {
                                Activity parent;
                                if (!parentChildMappings.TryGetValue(errorSource, out parent))
                                {
                                    parent = null;
                                }
                                errorSource = parent;
                            }
                        }
                    }

                    this.violations.Add(new Violation(fileName, violation, violationLocation));
                }
            }
        }

        Dictionary<object, SourceLocation> GetErrorInformation(Activity activity)
        {
            Dictionary<object, SourceLocation> sourceLocations = null;

            Activity implementationRoot = null;
            IEnumerable<Activity> children = WorkflowInspectionServices.GetActivities(activity);
            foreach (Activity child in children)
            {
                // Check if the child is the root of the activity's implementation 
                // When an activity is an implementation child of another activity, the IDSpace for 
                // the implementation child is different than it's parent activity and parent's public
                // children. The IDs for activities in the root activity's IDSpace are 1, 2
                // etc and for the root implementation child it is 1.1 and for its implementation
                // child it is 1.1.1 and so on.
                // As the activity can have just one implementation root, we just check 
                // for '.' to identify the root of the implementation.
                if (child.Id.Contains("."))
                {
                    implementationRoot = child;
                    break;
                }
            }

            if (implementationRoot == null)
            {
                return sourceLocations;
            }

            // We use the workflow debug symbol to get the line and column number information for a
            // erroneous activity.
            // We do not rely on the workflow debug symbol to get the file name. This is to enable cases 
            // where the xaml was hand written outside of the workflow designer. The hand written xaml 
            // file will not have the workflow debug symbol unless it was saved in the workflow designer.
            string symbolString = DebugSymbol.GetSymbol(implementationRoot) as String;

            if (!string.IsNullOrEmpty(symbolString))
            {
                try
                {
                    WorkflowSymbol wfSymbol = WorkflowSymbol.Decode(symbolString);
                    if (wfSymbol != null)
                    {
                        sourceLocations = SourceLocationProvider.GetSourceLocations(activity, wfSymbol);
                    }
                }
                catch (Exception e)
                {
                    if (Fx.IsFatal(e))
                    {
                        throw;
                    }
                    // Ignore invalid symbol.
                }
            }
            return sourceLocations;
        }

        bool SkipValidationForFile(ITaskItem inputTaskItem)
        {
            if (inputTaskItem == null)
            {
                return false;
            }

            string skipWorkflowValidationValue = inputTaskItem.GetMetadata("SkipWorkflowValidation");
            if (String.IsNullOrEmpty(skipWorkflowValidationValue) || String.Equals(skipWorkflowValidationValue, "false", StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }
            else if (String.Equals(skipWorkflowValidationValue, "true", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            else
            {
                this.buildContext.XamlBuildLogger.LogWarning(SR.InvalidValueForSkipWorkflowValidation, null);
                return false;
            }
        }

        [DataContract]
        internal class Violation
        {
            private const int DefaultSourceLocationValue = 1;

            public Violation(string fileName, string message)
                : this(fileName, message, false, null)
            {
            }

            public Violation(string fileName, ValidationError validationError, SourceLocation sourceLoation)
                : this(fileName, validationError.Message, validationError.IsWarning, sourceLoation)
            {
            }

            public Violation(string fileName, string message, bool isWarning, SourceLocation sourceLocation)
            {
                this.FileName = fileName;
                this.Message = message;
                this.IsWarning = isWarning;
                if (sourceLocation != null)
                {
                    this.StartLineNumber = sourceLocation.StartLine;
                    this.StartColumnNumber = sourceLocation.StartColumn;
                    this.EndLineNumber = sourceLocation.EndLine;
                    this.EndColumnNumber = sourceLocation.EndColumn;
                }
                else
                {
                    this.StartLineNumber = DefaultSourceLocationValue;
                    this.StartColumnNumber = DefaultSourceLocationValue;
                    this.EndLineNumber = DefaultSourceLocationValue;
                    this.EndColumnNumber = DefaultSourceLocationValue;
                }
            }

            [DataMember]
            public string FileName { get; private set; }

            [DataMember]
            public string Message { get; private set; }

            [DataMember]
            public bool IsWarning { get; private set; }

            [DataMember]
            public int StartLineNumber { get; private set; }

            [DataMember]
            public int StartColumnNumber { get; private set; }

            [DataMember]
            public int EndLineNumber { get; private set; }

            [DataMember]
            public int EndColumnNumber { get; private set; }

            public void Emit(TaskLoggingHelper logger)
            {
                if (this.IsWarning)
                {
                    logger.LogWarning(null, null, null, this.FileName, this.StartLineNumber, this.StartColumnNumber, this.EndLineNumber, this.EndColumnNumber, this.Message, null);
                }
                else
                {
                    logger.LogError(null, null, null, this.FileName, this.StartLineNumber, this.StartColumnNumber, this.EndLineNumber, this.EndColumnNumber, this.Message, null);
                }
            }
        }
    }
}
