//---------------------------------------------------------------------
// <copyright file="NamespaceEmitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------

using System.CodeDom;
using System.Collections.Generic;
using System.Data.Metadata.Edm;
using System.Data.Services.Design;
using System.Linq;

namespace System.Data.EntityModel.Emitters
{
    /// <summary>
    /// This class is responsible for Emitting the code to create the CLR namespace container and assembly level attributes
    /// </summary>
    internal sealed class NamespaceEmitter : Emitter
    {
        #region Static Fields
        private static Pair<Type, CreateEmitter>[] EmitterCreators = new Pair<Type, CreateEmitter>[]
        {
            (new Pair<Type,CreateEmitter>(typeof(EntityType), delegate (ClientApiGenerator generator1, GlobalItem element) { return new EntityTypeEmitter(generator1,(EntityType)element); })),
            (new Pair<Type,CreateEmitter>(typeof(ComplexType), delegate (ClientApiGenerator generator2, GlobalItem element) { return new ComplexTypeEmitter(generator2,(ComplexType)element); })),
            (new Pair<Type,CreateEmitter>(typeof(EntityContainer), delegate (ClientApiGenerator generator3, GlobalItem element) { return new EntityContainerEmitter(generator3,(EntityContainer)element); })),            
            (new Pair<Type,CreateEmitter>(typeof(AssociationType), delegate (ClientApiGenerator generator4, GlobalItem element) { return new AssociationTypeEmitter(generator4,(AssociationType)element); })),            
        };
        #endregion

        #region Private Fields
        private string _targetFilePath;
        private string _namespacePrefix;
        #endregion

        #region Public Methods
        /// <summary>
        /// 
        /// </summary>
        /// <param name="generator"></param>
        public NamespaceEmitter(ClientApiGenerator generator, string namespacePrefix, string targetFilePath)
            : base(generator)
        {
            _targetFilePath = targetFilePath != null ? targetFilePath : string.Empty;
            _namespacePrefix = namespacePrefix;
        }

        /// <summary>
        /// Creates the CodeTypeDeclarations necessary to generate the code
        /// </summary>
        public void Emit()
        {
            Dictionary<string, string> usedClassName = new Dictionary<string, string>(StringComparer.Ordinal);
            HashSet<string> used = new HashSet<string>();
            used.Add("Edm");

            EntityContainer defaultContainer = this.Generator.EdmItemCollection
                                                       .GetItems<EntityContainer>()
                                                       .FirstOrDefault(c => IsDefaultContainer(c));
            this.Generator.DefaultContainerNamespace = this.Generator.GetContainerNamespace(defaultContainer);

            this.BuildNamespaceMap(this.Generator.DefaultContainerNamespace);

            foreach (EntityContainer container in this.Generator.EdmItemCollection.GetItems<EntityContainer>())
            {
                string namespaceName = this.Generator.GetContainerNamespace(container);

                used.Add(namespaceName);
                List<GlobalItem> items = new List<GlobalItem>();
                items.Add(container);
                foreach (GlobalItem element in Generator.GetSourceTypes())
                {
                    EdmType edmType = (element as EdmType);
                    if ((null != edmType) && (edmType.NamespaceName == namespaceName))
                    {
                        items.Add(edmType);
                    }
                }

                if (!string.IsNullOrEmpty(_namespacePrefix))
                {
                    if (string.IsNullOrEmpty(namespaceName) || IsDefaultContainer(container))
                    {
                        namespaceName = _namespacePrefix;
                    }
                    else
                    {
                        namespaceName = _namespacePrefix + "." + namespaceName;
                    }
                }

                Emit(usedClassName, namespaceName, items);
            }

            foreach (string namespaceName in (from x in this.Generator.EdmItemCollection.GetItems<EdmType>() select x.NamespaceName).Distinct())
            {
                if (used.Add(namespaceName))
                {
                    List<GlobalItem> items = new List<GlobalItem>();
                    foreach (GlobalItem element in Generator.GetSourceTypes())
                    {
                        EdmType edmType = (element as EdmType);
                        if ((null != edmType) && (edmType.NamespaceName == namespaceName))
                        {
                            items.Add(edmType);
                        }
                    }

                    if (0 < items.Count)
                    {
                        string clientNamespace = namespaceName;

                        if (_namespacePrefix != null)
                        {
                            clientNamespace = this.Generator.GetClientTypeNamespace(namespaceName);
                        }

                        this.Emit(usedClassName, clientNamespace, items);
                    }
                }
            }
        }

        private void Emit(Dictionary<string, string> usedClassName, string namespaceName, List<GlobalItem> items)
        {
            // it is a valid scenario for namespaceName to be empty
            //string namespaceName = Generator.SourceObjectNamespaceName;
            Generator.SourceEdmNamespaceName = namespaceName;

            // emit the namespace definition
            CodeNamespace codeNamespace = new CodeNamespace(namespaceName);

            // output some boiler plate comments
            string comments = Strings.NamespaceComments(
                System.IO.Path.GetFileName(_targetFilePath),
                DateTime.Now.ToString(System.Globalization.CultureInfo.CurrentCulture));
            CommentEmitter.EmitComments(CommentEmitter.GetFormattedLines(comments, false), codeNamespace.Comments, false);
            CompileUnit.Namespaces.Add(codeNamespace);

            // Emit the classes in the schema
            foreach (GlobalItem element in items)
            {
                if (AddElementNameToCache(element, usedClassName))
                {
                    SchemaTypeEmitter emitter = CreateElementEmitter(element);
                    CodeTypeDeclarationCollection typeDeclaration = emitter.EmitApiClass();
                    if (typeDeclaration.Count > 0)
                    {
                        codeNamespace.Types.AddRange(typeDeclaration);
                    }
                }
            }

            Generator.SourceEdmNamespaceName = null;
        }

        #endregion

        #region Private Properties
        /// <summary>
        /// Gets the compile unit (top level codedom object)
        /// </summary>
        /// <value></value>
        private CodeCompileUnit CompileUnit
        {
            get
            {
                return Generator.CompileUnit;
            }
        }
        #endregion

        private bool AddElementNameToCache(GlobalItem element, Dictionary<string, string> cache)
        {
            if (element.BuiltInTypeKind == BuiltInTypeKind.EntityContainer)
            {
                return TryAddNameToCache((element as EntityContainer).Name, element.BuiltInTypeKind.ToString(), cache);
            }
            else if (element.BuiltInTypeKind == BuiltInTypeKind.EntityType ||
                element.BuiltInTypeKind == BuiltInTypeKind.ComplexType ||
                element.BuiltInTypeKind == BuiltInTypeKind.AssociationType)
            {
                return TryAddNameToCache((element as StructuralType).Name, element.BuiltInTypeKind.ToString(), cache);
            }
            return true;
        }

        private bool TryAddNameToCache(string name, string type, Dictionary<string, string> cache)
        {
            if (!cache.ContainsKey(name))
            {
                cache.Add(name, type);
            }
            else
            {
                this.Generator.AddError(Strings.DuplicateClassName(type, name, cache[name]), ModelBuilderErrorCode.DuplicateClassName, EdmSchemaErrorSeverity.Error);
                return false;
            }
            return true;
        }

        /// <summary>
        /// Create an Emitter for a schema type element
        /// </summary>
        /// <param name="element"></param>
        /// <returns></returns>
        private SchemaTypeEmitter CreateElementEmitter(GlobalItem element)
        {
            Type typeOfElement = element.GetType();
            foreach (Pair<Type, CreateEmitter> pair in EmitterCreators)
            {
                if (pair.First.IsAssignableFrom(typeOfElement))
                    return pair.Second(Generator, element);
            }
            return null;
        }

        private delegate SchemaTypeEmitter CreateEmitter(ClientApiGenerator generator, GlobalItem item);

        /// <summary>
        /// Reponsible for relating two objects together into a pair
        /// </summary>
        /// <typeparam name="T1"></typeparam>
        /// <typeparam name="T2"></typeparam>
        private class Pair<T1, T2>
        {
            public T1 First;
            public T2 Second;
            internal Pair(T1 first, T2 second)
            {
                First = first;
                Second = second;
            }
        }

        private bool IsDefaultContainer(EntityContainer container)
        {
            if (container == null) return false;

            MetadataProperty prop;
            if (container.MetadataProperties.TryGetValue("http://schemas.microsoft.com/ado/2007/08/dataservices/metadata:IsDefaultEntityContainer", false, out prop))
            {
                return prop != null &&
                       prop.Value != null &&
                       string.Equals(prop.Value.ToString(), "true", StringComparison.OrdinalIgnoreCase);
            }

            return false;
        }

        private void BuildNamespaceMap(string defaultContainerNamespace)
        {
            if (!string.IsNullOrEmpty(this.Generator.NamespacePrefix))
            {
                var namespaceMap = this.Generator.GetSourceTypes()
                                                 .OfType<EdmType>()
                                                 .Select(et => et.NamespaceName)
                                                 .Distinct()
                                                 .Select(ns => new
                                                 {
                                                     ServiceNamespace = ns,
                                                     ClientNamespace = ((ns == defaultContainerNamespace)
                                                                              ? _namespacePrefix
                                                                              : _namespacePrefix + "." + ns)
                                                 });
                foreach (var i in namespaceMap)
                {
                    this.Generator.NamespaceMap.Add(i.ServiceNamespace, i.ClientNamespace);
                }
            }
        }
    }
}
