// <copyright file="DebugBuildExtension.cs" company="Microsoft Corporation">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.Activities.Build.Debugger
{
    using System;
    using System.Activities.Debugger.Symbol;
    using System.CodeDom;
    using System.IO;
    using System.Runtime;
    using System.Xaml;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Tasks.Xaml;

    /// <summary>
    /// Build Extension for Workflow debugger support.
    /// </summary>
    public class DebugBuildExtension : IXamlBuildTypeGenerationExtension
    {
        private static string debugSymbolTypeFullName = typeof(DebugSymbol).FullName;

        /// <summary>
        /// Update debug symbol with check sum.
        /// </summary>
        /// <param name="classData">Class data to add the checksum.</param>
        /// <param name="buildContext">Build context</param>
        /// <returns>Whether the execution succeeds</returns>
        public bool Execute(ClassData classData, XamlBuildTypeGenerationExtensionContext buildContext)
        {
            string className = !string.IsNullOrEmpty(classData.Namespace) ? classData.Namespace + "." + classData.Name : classData.Name;
            buildContext.XamlBuildLogger.LogMessage(MessageImportance.Low, SR.InspectingClass(typeof(DebugBuildExtension).Name, className));
            this.UpdateDebugSymbol(classData, buildContext);
            return true;
        }

        private void UpdateDebugSymbol(ClassData classData, XamlBuildTypeGenerationExtensionContext buildContext)
        {
            string path = Path.GetFullPath(classData.FileName);
            try
            {
                using (XamlReader reader = classData.EmbeddedResourceXaml.GetReader())
                {
                    XamlNodeList newList = new XamlNodeList(reader.SchemaContext);
                    using (XamlWriter writer = newList.Writer)
                    {
                        bool nodesAvailable = reader.Read();
                        while (nodesAvailable)
                        {
                            if (reader.NodeType == XamlNodeType.StartMember)
                            {
                                writer.WriteNode(reader);
                                if (reader.Member.DeclaringType != null &&
                                    reader.Member.DeclaringType.UnderlyingType != null &&
                                    string.CompareOrdinal(reader.Member.DeclaringType.UnderlyingType.FullName, debugSymbolTypeFullName) == 0)
                                {
                                    reader.Read();
                                    string symbolString = reader.Value as string;
                                    if (!string.IsNullOrEmpty(symbolString))
                                    {
                                        WorkflowSymbol symbol = WorkflowSymbol.Decode(symbolString);
                                        symbol.FileName = path;
                                        symbol.CalculateChecksum();
                                        writer.WriteValue(symbol.Encode());
                                    }
                                    else
                                    {
                                        writer.WriteValue(reader.Value);
                                    }
                                }

                                nodesAvailable = reader.Read();
                            }
                            else
                            {
                                writer.WriteNode(reader);
                                nodesAvailable = reader.Read();
                            }
                        }
                    }

                    classData.EmbeddedResourceXaml = newList;
                }
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                buildContext.XamlBuildLogger.LogMessage(
                    MessageImportance.High,
                    SR.DebugBuildExtensionExceptionPrefix(
                        typeof(DebugBuildExtension).Name,
                        path,
                        e.Message));
            }
        }
    }
}
