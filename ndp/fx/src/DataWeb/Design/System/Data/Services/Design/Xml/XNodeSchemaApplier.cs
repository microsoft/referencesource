//---------------------------------------------------------------------
// <copyright file="XNodeSchemaApplier.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class used to make an XElement conform to a given
//      XML Schema.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Design.Xml
{
    #region Namespaces.

    using System.Diagnostics;
    using System;
    using System.Linq;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.Linq;
    using System.Collections.Generic;
    using System.Runtime.Versioning;

    #endregion Namespaces.

    /// <summary>Use this class to remove unexpected elements and attributes from an XDocument instance.</summary>
    internal class XNodeSchemaApplier
    {
        #region Private fields.

        /// <summary>Namespace manager for current scope.</summary>
        private readonly XmlNamespaceManager namespaceManager;

        /// <summary>Schemas used to predict expected elements and attributes.</summary>
        private readonly XmlSchemaSet schemas;

        /// <summary>XName for xsi:type.</summary>
        private readonly XName xsiTypeName;

        /// <summary>XName for xsi:nil.</summary>
        private readonly XName xsiNilName;

        /// <summary>Schema validator used to predict expected elements and attributes.</summary>
        private XmlSchemaValidator validator;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="XNodeSchemaApplier"/> instance.</summary>
        /// <param name="schemas">Schemas to use to predict elements and attributes.</param>
        private XNodeSchemaApplier(XmlSchemaSet schemas)
        {
            Debug.Assert(schemas != null, "schemas != null");

            this.schemas = schemas;
            XNamespace xsi = XNamespace.Get("http://www.w3.org/2001/XMLSchema-instance");
            this.xsiTypeName = xsi.GetName("type");
            this.xsiNilName = xsi.GetName("nil");
            this.namespaceManager = new XmlNamespaceManager(schemas.NameTable);
        }

        #endregion Constructors.

        #region Internal methods.

        /// <summary>
        /// Appends <paramref name="element"/> to the specified <paramref name="list"/>, creating as necessary.
        /// </summary>
        /// <typeparam name="T">List element type.</typeparam>
        /// <param name="list">List to add the element to, possibly null on entry.</param>
        /// <param name="element">Element to add to the list.</param>
        internal static void AppendWithCreation<T>(ref List<T> list, T element)
        {
            if (list == null)
            {
                list = new List<T>();
            }

            list.Add(element);
        }

        /// <summary>
        /// Applies the specified <paramref name="schemas"/> to remove unexpected elements and attributes from the 
        /// given <paramref name="element"/>.
        /// </summary>
        /// <param name="schemas">Set of schemas to apply.</param>
        /// <param name="element">Document to remove elements and attributes from.</param>
        internal static void Apply(XmlSchemaSet schemas, XElement element)
        {
            Debug.Assert(schemas != null, "schemas != null");
            Debug.Assert(element != null, "document != null");

            XNodeSchemaApplier applier = new XNodeSchemaApplier(schemas);
            applier.Validate(element);
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>Determines whether the specified <paramref name="element"/> is expected.</summary>
        /// <param name="element">Element to check.</param>
        /// <param name="elementName">
        /// <see cref="XmlQualifiedName"/> of the <paramref name="element"/> (passed to avoid recreation).
        /// </param>
        /// <param name="expected">Expected schema particle.</param>
        /// <returns>true if the element is expected; false otherwise.</returns>
        private static bool IsElementExpected(XElement element, XmlQualifiedName elementName, XmlSchemaParticle expected)
        {
            Debug.Assert(element != null, "element != null");
            Debug.Assert(elementName != null, "elementName != null");
            Debug.Assert(expected != null, "expected != null");
            Debug.Assert(
                ToQualifiedName(element.Name) == elementName,
                "ToQualifiedName(element.Name) == elementName -- otherwise the caller get the 'caching' wrong");

            // These are all possibilities for a particle.
            XmlSchemaGroupRef schemaGroupRef = expected as XmlSchemaGroupRef;
            XmlSchemaAny schemaAny = expected as XmlSchemaAny;
            XmlSchemaElement schemaElement = expected as XmlSchemaElement;
            XmlSchemaAll schemaAll = expected as XmlSchemaAll;
            XmlSchemaChoice schemaChoice = expected as XmlSchemaChoice;
            XmlSchemaSequence schemaSequence = expected as XmlSchemaSequence;

            Debug.Assert(schemaGroupRef == null, "schemaGroupRef == null -- the validator flattens this out as options.");
            Debug.Assert(schemaSequence == null, "schemaSequence == null -- the validator flattens this out and picks the right one in seq.");
            Debug.Assert(schemaAll == null, "schemaAll == null -- the validator flattens this out as options.");
            Debug.Assert(schemaChoice == null, "schemaChoice == null -- the validator flattens this out as options.");

            if (schemaAny != null)
            {
                Debug.Assert(
                    schemaAny.Namespace == "##other" || schemaAny.Namespace == "##any",
                    "schemaAny.Namespace == '##other' || '##any' -- otherwise CSDL XSD resource was changed");
                if (schemaAny.Namespace == "##any")
                {
                    return true;
                }
                else if (schemaAny.Namespace == "##other")
                {
                    string realElementNamespace = element.Name.NamespaceName;
                    if (realElementNamespace != GetTargetNamespace(expected))
                    {
                        return true;
                    }
                }
            }

            if (schemaElement != null)
            {
                if (schemaElement.QualifiedName == elementName)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>Gets the target namespace that applies to the specified <paramref name="schemaObject"/>.</summary>
        /// <param name="schemaObject">XML schema object for which to get target namespace.</param>
        /// <returns>Target namespace for the specified <paramref name="schemaObject"/> (never null).</returns>
        private static string GetTargetNamespace(XmlSchemaObject schemaObject)
        {
            Debug.Assert(schemaObject != null, "schemaObject != null");

            string result = null;
            do
            {
                XmlSchema schema = schemaObject as XmlSchema;
                if (schema != null)
                {
                    result = schema.TargetNamespace;
                    Debug.Assert(!String.IsNullOrEmpty(schema.TargetNamespace), "schema.TargetNamespace != null||'' -- otherwise this isn't CSDL");
                }
                else
                {
                    schemaObject = schemaObject.Parent;
                    Debug.Assert(schemaObject != null, "o != null -- otherwise the object isn't parented to a schema");
                }
            }
            while (result == null);

            return result;
        }

        /// <summary>Determines whether the specified <paramref name="element"/> is expected.</summary>
        /// <param name="element">Element to check.</param>
        /// <param name="expectedParticles">Expected schema particles (possibly empty).</param>
        /// <returns>true if the element is expected; false otherwise.</returns>
        private static bool IsElementExpected(XElement element, XmlSchemaParticle[] expectedParticles)
        {
            Debug.Assert(element != null, "element != null");
            Debug.Assert(expectedParticles != null, "expectedParticles != null");

            XmlQualifiedName elementName = ToQualifiedName(element.Name);
            foreach (var expected in expectedParticles)
            {
                if (IsElementExpected(element, elementName, expected))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>Determines whether the specified <paramref name="attribute"/> is expected.</summary>
        /// <param name="attribute">Attribute to check.</param>
        /// <param name="expectedAttributes">Expected attributes (possibly empty).</param>
        /// <param name="anyAttribute">anyAttribute schema for a complex type element (possibly null).</param>
        /// <returns>true if the attribute is expected; false otherwise.</returns>
        private static bool IsAttributeExpected(XAttribute attribute, XmlSchemaAnyAttribute anyAttribute, XmlSchemaAttribute[] expectedAttributes)
        {
            Debug.Assert(attribute != null, "attribute != null");
            Debug.Assert(expectedAttributes != null, "expectedAttributes != null");
            Debug.Assert(expectedAttributes.All(a => a.Form != XmlSchemaForm.Qualified), "expectedAttributes.All(a => a.Form != XmlSchemaForm.Qualified)");

            var name = ToQualifiedName(attribute.Name);
            if (name.Namespace.Length == 0)
            {
                foreach (var expected in expectedAttributes)
                {
                    if (expected.Name == name.Name)
                    {
                        return true;
                    }
                }
            }

            if (anyAttribute != null)
            {
                Debug.Assert(
                    anyAttribute.Namespace == "##any" || anyAttribute.Namespace == "##other",
                    "anyAttribute.Namespace == '##any' || '##other' -- otherwise CSDL XSD resource was changed");
                if (anyAttribute.Namespace == "##any")
                {
                    return true;
                }
                else
                {
                    string attributeNamespace = attribute.Name.NamespaceName;
                    if (attributeNamespace.Length > 0 && attributeNamespace != GetTargetNamespace(anyAttribute))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Return the <see cref="XmlQualifiedName"/> representation of the specified <paramref name="name"/>.
        /// </summary>
        /// <param name="name">XML name to return.</param>
        /// <returns>An <see cref="XmlQualifiedName"/> that represents the given <paramref name="name"/>.</returns>
        private static XmlQualifiedName ToQualifiedName(XName name)
        {
            Debug.Assert(name != null, "name != null");
            return new XmlQualifiedName(name.LocalName, name.NamespaceName);
        }

        /// <summary>Validates the specified <paramref name="element"/> object.</summary>
        /// <param name="element">Source object for validation (must be an element).</param>
        private void Validate(XElement element)
        {
            Debug.Assert(element != null, "element != null");

            XmlSchemaValidationFlags validationFlags = XmlSchemaValidationFlags.AllowXmlAttributes;
            this.PushAncestorsAndSelf(element.Parent);

            validator = new XmlSchemaValidator(schemas.NameTable, schemas, namespaceManager, validationFlags);
            validator.XmlResolver = null;
            validator.Initialize();
            this.ValidateElement(element);
            validator.EndValidation();
        }

        /// <summary>Pushes the specifed <paramref name="element"/> namespaces and those of ancestors.</summary>
        /// <param name="element">Element to push from - possibly null.</param>
        /// <remarks>
        /// Pushing in reverse order (up the tree rather than down the tree) is OK, because we check that
        /// the namespace local name hasn't been added yet. Use <see cref="PushElement"/> as we go down
        /// the tree to push/pop as usual.
        /// </remarks>
        private void PushAncestorsAndSelf(XElement element)
        {
            while (element != null)
            {
                foreach (XAttribute attribute in element.Attributes())
                {
                    if (attribute.IsNamespaceDeclaration)
                    {
                        string localName = attribute.Name.LocalName;
                        if (localName == "xmlns")
                        {
                            localName = string.Empty;
                        }

                        if (!namespaceManager.HasNamespace(localName))
                        {
                            namespaceManager.AddNamespace(localName, attribute.Value);
                        }
                    }
                }

                element = element.Parent as XElement;
            }
        }

        /// <summary>Pushes the specifed <paramref name="element" /> namespaces and those of ancestors.</summary>
        /// <param name="element">Element to push.</param>
        /// <param name="xsiType">The value for xsi:type on this element.</param>
        /// <param name="xsiNil">The value for xsi:nil on this element.</param>
        private void PushElement(XElement element, ref string xsiType, ref string xsiNil)
        {
            Debug.Assert(element != null, "e != null");
            namespaceManager.PushScope();
            foreach (XAttribute attribute in element.Attributes())
            {
                if (attribute.IsNamespaceDeclaration)
                {
                    string localName = attribute.Name.LocalName;
                    if (localName == "xmlns")
                    {
                        localName = string.Empty;
                    }

                    namespaceManager.AddNamespace(localName, attribute.Value);
                }
                else
                {
                    XName name = attribute.Name;
                    if (name == xsiTypeName)
                    {
                        xsiType = attribute.Value;
                    }
                    else if (name == xsiNilName)
                    {
                        xsiNil = attribute.Value;
                    }
                }
            }
        }

        /// <summary>Validates all attributes on the specified <paramref name="element"/>.</summary>
        /// <param name="element">Element to validate attributes on.</param>
        private void ValidateAttributes(XElement element)
        {
            Debug.Assert(element != null, "e != null");

            foreach (XAttribute attribute in element.Attributes())
            {
                if (!attribute.IsNamespaceDeclaration)
                {
                    validator.ValidateAttribute(attribute.Name.LocalName, attribute.Name.NamespaceName, attribute.Value, null);
                }
            }
        }

        //// SxS: This method does not expose any resources to the caller and passes null as resource names to 
        //// XmlSchemaValidator.ValidateElement (annotated with ResourceExposure(ResourceScope.None).
        //// It's OK to suppress the SxS warning.
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        [ResourceExposure(ResourceScope.None)]
        private void ValidateElement(XElement e)
        {
            Debug.Assert(e != null, "e != null");

            XmlSchemaInfo schemaInfo = new XmlSchemaInfo();
            string xsiType = null;
            string xsiNil = null;
            this.PushElement(e, ref xsiType, ref xsiNil);

            // The current element is always valid - otherwise we wouldn't have recursed into it in the first place.
            validator.ValidateElement(e.Name.LocalName, e.Name.NamespaceName, schemaInfo, xsiType, xsiNil, null, null);

            // When we have no schema element, then e was included but we don't know about it - it's an extension 
            // element, likely under CSDL documentation. We'll skip the whole thing in this case.
            if (schemaInfo.SchemaElement != null)
            {
                XmlSchemaComplexType schemaComplexType = schemaInfo.SchemaElement.ElementSchemaType as XmlSchemaComplexType;
                this.TrimAttributes(e, (schemaComplexType == null) ? null : schemaComplexType.AttributeWildcard);
                this.ValidateAttributes(e);
                validator.ValidateEndOfAttributes(null);

                this.TrimAndValidateNodes(e);
            }

            validator.ValidateEndElement(null);
            this.namespaceManager.PopScope();
        }

        /// <summary>Removes attributes from the specified <paramref name="element"/> if they're unexpected.</summary>
        /// <param name="element">Element to remove attributes from.</param>
        /// <param name="anyAttribute">anyAttribute schema for a complex type element (possibly null).</param>
        private void TrimAttributes(XElement element, XmlSchemaAnyAttribute anyAttribute)
        {
            Debug.Assert(element != null, "e != null");

            List<XAttribute> unexpectedAttributes = null;
            var expectedAttributes = validator.GetExpectedAttributes();
            foreach (XAttribute attribute in element.Attributes())
            {
                if (attribute.IsNamespaceDeclaration)
                {
                    continue;
                }

                if (!IsAttributeExpected(attribute, anyAttribute, expectedAttributes))
                {
                    AppendWithCreation(ref unexpectedAttributes, attribute);
                }
            }

            if (unexpectedAttributes != null)
            {
                foreach (var attribute in unexpectedAttributes)
                {
                    attribute.Remove();
                }
            }
        }

        /// <summary>
        /// Removes nodes from the specified <paramref name="parent"/> element and validates its nodes.
        /// </summary>
        /// <param name="parent"></param>
        /// <remarks>
        /// While it's cleaner to do this in two passes, trim then validate, like we do with attributes, we need to
        /// validate as we go for the validator to return sequence elements in the right order.
        /// </remarks>
        private void TrimAndValidateNodes(XElement parent)
        {
            Debug.Assert(parent != null, "parent != null");

            List<XNode> unexpectedNodes = null;
            XmlSchemaParticle[] expectedParticles = null;
            foreach (XNode node in parent.Nodes())
            {
                // expectedParticles will be null the first iteration and right after we validate,
                // when we potentially have something different to validate against.
                if (expectedParticles == null)
                {
                    expectedParticles = validator.GetExpectedParticles();
                }

                Debug.Assert(expectedParticles != null, "expectedParticles != null -- GetExpectedParticles should return empty at worst");
                XElement element = node as XElement;
                if (element != null)
                {
                    if (!IsElementExpected(element, expectedParticles))
                    {
                        AppendWithCreation(ref unexpectedNodes, element);
                    }
                    else
                    {
                        this.ValidateElement(element);
                        expectedParticles = null;
                    }
                }
                else
                {
                    XText text = node as XText;
                    if (text != null)
                    {
                        string s = text.Value;
                        if (s.Length > 0)
                        {
                            validator.ValidateText(s);
                            expectedParticles = null;
                        }
                    }
                }
            }

            if (unexpectedNodes != null)
            {
                foreach (var node in unexpectedNodes)
                {
                    node.Remove();
                }
            }
        }

        #endregion Private methods.
    }
}
