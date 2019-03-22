//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace Microsoft.Activities.Build
{
    using System;
    using System.Activities;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Xaml;
    using System.Xaml.Schema;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Tasks.Xaml;
    using System.Runtime;
    using System.Text;

    class BeforeInitializeComponentExtension : IXamlBuildTypeGenerationExtension
    {
        const string FileNameSuffix = "BeforeInitializeComponentHelper";

        XamlSchemaContext schemaContext;

        public bool Execute(ClassData classData, XamlBuildTypeGenerationExtensionContext buildContext)
        {
            string className = !string.IsNullOrEmpty(classData.Namespace) ? classData.Namespace + "." + classData.Name : classData.Name;
            buildContext.XamlBuildLogger.LogMessage(MessageImportance.Low, SR.InspectingClass(typeof(BeforeInitializeComponentExtension).Name, className));

            this.schemaContext = classData.EmbeddedResourceXaml.Writer.SchemaContext;

            if (!IsAssignableTo(classData.BaseType, typeof(Activity)))
            {
                return true;
            }

            if (ArePartialMethodsSupported(buildContext))
            {
                buildContext.XamlBuildLogger.LogMessage(MessageImportance.Low, SR.GeneratingBeforeInitializeComponent(typeof(BeforeInitializeComponentExtension).Name, className));

                CodeCompileUnit myCodeCompileUnit = GenerateCompileUnit(classData.Namespace ?? string.Empty, classData, buildContext.Language);
                WriteGeneratedCode(classData, buildContext, myCodeCompileUnit, buildContext.Language);
            }
            else
            {
                buildContext.XamlBuildLogger.LogWarning(SR.UnsupportedLanguage(typeof(BeforeInitializeComponentExtension).Name, className));
            }

            return true;
        }

        static void WriteGeneratedCode(ClassData classData, XamlBuildTypeGenerationExtensionContext buildContext, CodeCompileUnit compileUnit, string language)
        {
            using (CodeDomProvider codeDomProvider = CodeDomProvider.CreateProvider(buildContext.Language))
            {
                string codeFileName = string.Format(CultureInfo.InvariantCulture, "{0}_{1}_{2}.{3}", classData.Namespace, classData.Name, FileNameSuffix, codeDomProvider.FileExtension);
                string codeFilePath = Path.Combine(buildContext.OutputPath, codeFileName);

                using (StreamWriter fileStream = new StreamWriter(codeFilePath))
                {
                    using (IndentedTextWriter tw = new IndentedTextWriter(fileStream))
                    {
                        codeDomProvider.GenerateCodeFromCompileUnit(compileUnit, tw, new CodeGeneratorOptions());
                    }
                }

                buildContext.AddGeneratedFile(codeFilePath);

                // Generate a resource file that contains the name of the resource holding the XAML.
                string resourceFilePath = Path.Combine(buildContext.OutputPath, GenerateHelperResourceFilename(classData, buildContext, language));

                string xamlResourceName = classData.EmbeddedResourceFileName;

                using (StreamWriter fileStream = new StreamWriter(resourceFilePath))
                {
                    // The first line of the resource is the Xaml resource name.
                    fileStream.WriteLine(xamlResourceName);
                    // The second line of the resource is the full class name of the Xaml helper class.
                    // In VB, that name is prepended with the root namespace.
                    string helperClassName = null;
                    if (string.Equals(language, "VB", StringComparison.OrdinalIgnoreCase))
                    {
                        helperClassName = string.Format(CultureInfo.InvariantCulture, "{0}.{1}", buildContext.RootNamespace, classData.HelperClassFullName);
                    }
                    else
                    {
                        helperClassName = classData.HelperClassFullName;
                    }
                    fileStream.WriteLine(helperClassName);
                }
                buildContext.AddGeneratedResourceFile(resourceFilePath);

            }
        }

        static string GenerateHelperResourceFilename(ClassData classData, XamlBuildTypeGenerationExtensionContext buildContext, string language)
        {
            // Generate a resource file that contains the name of the resource holding the XAML.
            // [<rootns][.][namespace][_]classname_FileNameSuffix.txt
            StringBuilder builder = new StringBuilder();
            if (string.Equals(language, "VB", StringComparison.OrdinalIgnoreCase))
            {

                if (!string.IsNullOrWhiteSpace(buildContext.RootNamespace))
                {
                    builder.Append(buildContext.RootNamespace);
                }

                if (!string.IsNullOrWhiteSpace(classData.Namespace))
                {
                    if (builder.Length > 0)
                    {
                        builder.Append(".");
                    }
                    builder.Append(classData.Namespace);
                }

                if (builder.Length > 0)
                {
                    builder.Append("_");
                }
            }
            else
            {
                if (!string.IsNullOrWhiteSpace(classData.Namespace))
                {
                    builder.Append(classData.Namespace);
                }

                if (builder.Length > 0)
                {
                    builder.Append("_");
                }
            }
            builder.Append(string.Format(CultureInfo.InvariantCulture, "{0}_{1}.{2}", classData.Name, FileNameSuffix, "txt"));
            return builder.ToString();
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters,
            Justification = "The string literals are code snippets, not localizable values.")]
        static CodeCompileUnit GenerateCompileUnit(string clrNamespace, ClassData classData, string language)
        {
            CodeTypeDeclaration typeDeclaration = new CodeTypeDeclaration
            {
                Name = classData.Name,
                IsPartial = true,
                TypeAttributes = classData.IsPublic ? TypeAttributes.Public : TypeAttributes.NotPublic,
            };

            // Partial declarations are only supported in C# and VB
            string namespaceAndClassNameSnippet = null;
            if (string.Equals(language, "C#", StringComparison.OrdinalIgnoreCase))
            {
                namespaceAndClassNameSnippet = CodeDomSnippets.BeforeInitializeComponentCS;
            }
            else if (string.Equals(language, "VB", StringComparison.OrdinalIgnoreCase))
            {
                namespaceAndClassNameSnippet = CodeDomSnippets.BeforeInitializeComponentVB;
            }
            else
            {
                throw Fx.AssertAndThrow("This method should only have been called for VB or C#");
            }

            namespaceAndClassNameSnippet = namespaceAndClassNameSnippet.Replace("ClassNameGoesHere", classData.Name);
            typeDeclaration.Members.Add(new CodeSnippetTypeMember(namespaceAndClassNameSnippet));

            return new CodeCompileUnit
            {
                Namespaces =
                {
                    new CodeNamespace
                    {
                        Name = clrNamespace,
                        Types =
                        {
                            typeDeclaration
                        }
                    }
                }
            };
        }

        bool IsAssignableTo(XamlType type, Type assignableTo)
        {
            XamlType liveXamlType = this.schemaContext.GetXamlType(assignableTo);
            XamlType rolXamlType = this.schemaContext.GetXamlType(new XamlTypeName(liveXamlType));
            if (rolXamlType == null)
            {
                // assignableTo is not in the project references, so project must not be deriving from it
                return false;
            }
            return type.CanAssignTo(rolXamlType);
        }

        bool ArePartialMethodsSupported(XamlBuildTypeGenerationExtensionContext buildContext)
        {
            return string.Equals(buildContext.Language, "C#", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(buildContext.Language, "VB", StringComparison.OrdinalIgnoreCase);
        }

        static class CodeDomSnippets
        {
            public const string BeforeInitializeComponentCS =
@"    partial void BeforeInitializeComponent(ref bool isInitialized)
    {
        if (isInitialized == true) {
            return;
        }

        System.Activities.XamlIntegration.ActivityXamlServices.InitializeComponent(
            typeof(ClassNameGoesHere),
            this
        );

        // Setting this will turn InitializeComponent into a no-op.
        isInitialized = true;
    }
";

            public const string BeforeInitializeComponentVB =
@"    Private Sub BeforeInitializeComponent(ByRef isInitialized as Boolean)
        If (isInitialized)
            return
        End If

        System.Activities.XamlIntegration.ActivityXamlServices.InitializeComponent(
            GetType(ClassNameGoesHere),
            Me
        )

        isInitialized = true
    End Sub
";
        }
    }
}

