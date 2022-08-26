//---------------------------------------------------------------------
// <copyright file="ExpandedWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a base class implementing IExpandedResult over 
//      projections.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Internal
{
    using System;

    /// <summary>Provides a base class implementing IExpandedResult over projections.</summary>
    /// <typeparam name="TExpandedElement">Type of element whose properties are expanded.</typeparam>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public abstract class ExpandedWrapper<TExpandedElement>: IExpandedResult
    {
        #region Fields.

        /// <summary>Text description of properties, in comma-separated format.</summary>
        private string description;

        /// <summary>Element whose properties are being expanded.</summary>
        private TExpandedElement expandedElement;

        /// <summary>Parsed property names.</summary>
        private string[] propertyNames;

        #endregion Fields.

        #region Public properties.

        /// <summary>Text description of properties, in comma-separated format.</summary>
        public string Description
        {
            get
            { 
                return this.description;
            }

            set
            {
                this.description = WebUtil.CheckArgumentNull(value, "value");
                this.propertyNames = WebUtil.StringToSimpleArray(this.description);
                int expectedLength = this.GetType().GetGenericArguments().Length - 1;
                if (this.propertyNames.Length != expectedLength)
                {
                    throw new InvalidOperationException(
                        Strings.BasicExpandProvider_DescriptionInitializerCountMismatch(value, expectedLength));
                }
            }
        }

        /// <summary>Element whose properties are being expanded.</summary>
        public TExpandedElement ExpandedElement
        {
            get { return this.expandedElement; }
            set { this.expandedElement = value; }
        }

        /// <summary>The element with expanded properties.</summary>
        object IExpandedResult.ExpandedElement
        {
            get { return ProjectedWrapper.ProcessResultInstance(this.ExpandedElement); }
        }

        #endregion Public properties.

        #region Methods.

        /// <summary>Gets the value for named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        /// <remarks>
        /// If the element returned in turn has properties which are expanded out-of-band
        /// of the object model, then the result will also be of type <see cref="IExpandedResult"/>,
        /// and the value will be available through <see cref="ExpandedElement"/>.
        /// A special case is the handling of $skiptoken property. In case the $skiptoken property does not
        /// exist on the current wrapper object, instead of throw-ing we return null which will
        /// be an indication to the caller that the property does not exist.
        /// </remarks>
        public object GetExpandedPropertyValue(string name)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }
            
            if (this.propertyNames == null)
            {
                throw new InvalidOperationException(Strings.BasicExpandProvider_ExpandedPropertiesNotInitialized);
            }
            
            int nameIndex = -1;
            for (int i = 0; i < this.propertyNames.Length; i++)
            {
                if (this.propertyNames[i] == name)
                {
                    nameIndex = i;
                    break;
                }
            }
            
            if (nameIndex == -1 && name == XmlConstants.HttpQueryStringSkipToken)
            {
                return null;
            }

            object resource = this.InternalGetExpandedPropertyValue(nameIndex);

            resource = ProjectedWrapper.ProcessResultInstance(resource);
            return ProjectedWrapper.ProcessResultEnumeration(resource);
        }

        /// <summary>Gets the value for the property for the result.</summary>
        /// <param name="nameIndex">Index of property for which to get enumerable results.</param>
        /// <returns>The value for the property of the result.</returns>
        protected abstract object InternalGetExpandedPropertyValue(int nameIndex);

        #endregion Methods.
    }

    #region ExpandedWrapper types with numbered properties.

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <typeparam name="TProperty7">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6, TProperty7> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty7 ProjectedProperty7 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            if (nameIndex == 7) return this.ProjectedProperty7;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <typeparam name="TProperty7">Type of projected property.</typeparam>
    /// <typeparam name="TProperty8">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6, TProperty7, TProperty8> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty7 ProjectedProperty7 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty8 ProjectedProperty8 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            if (nameIndex == 7) return this.ProjectedProperty7;
            if (nameIndex == 8) return this.ProjectedProperty8;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <typeparam name="TProperty7">Type of projected property.</typeparam>
    /// <typeparam name="TProperty8">Type of projected property.</typeparam>
    /// <typeparam name="TProperty9">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6, TProperty7, TProperty8, TProperty9> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty7 ProjectedProperty7 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty8 ProjectedProperty8 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty9 ProjectedProperty9 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            if (nameIndex == 7) return this.ProjectedProperty7;
            if (nameIndex == 8) return this.ProjectedProperty8;
            if (nameIndex == 9) return this.ProjectedProperty9;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <typeparam name="TProperty7">Type of projected property.</typeparam>
    /// <typeparam name="TProperty8">Type of projected property.</typeparam>
    /// <typeparam name="TProperty9">Type of projected property.</typeparam>
    /// <typeparam name="TProperty10">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6, TProperty7, TProperty8, TProperty9, TProperty10> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }
        
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty7 ProjectedProperty7 { get; set; }
        
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty8 ProjectedProperty8 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty9 ProjectedProperty9 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty10 ProjectedProperty10 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            if (nameIndex == 7) return this.ProjectedProperty7;
            if (nameIndex == 8) return this.ProjectedProperty8;
            if (nameIndex == 9) return this.ProjectedProperty9;
            if (nameIndex == 10) return this.ProjectedProperty10;
            throw Error.NotSupported();
        }
    }

    /// <summary>Provides a wrapper over an element expanded with projections.</summary>
    /// <typeparam name="TExpandedElement">Type of expanded element.</typeparam>
    /// <typeparam name="TProperty0">Type of projected property.</typeparam>
    /// <typeparam name="TProperty1">Type of projected property.</typeparam>
    /// <typeparam name="TProperty2">Type of projected property.</typeparam>
    /// <typeparam name="TProperty3">Type of projected property.</typeparam>
    /// <typeparam name="TProperty4">Type of projected property.</typeparam>
    /// <typeparam name="TProperty5">Type of projected property.</typeparam>
    /// <typeparam name="TProperty6">Type of projected property.</typeparam>
    /// <typeparam name="TProperty7">Type of projected property.</typeparam>
    /// <typeparam name="TProperty8">Type of projected property.</typeparam>
    /// <typeparam name="TProperty9">Type of projected property.</typeparam>
    /// <typeparam name="TProperty10">Type of projected property.</typeparam>
    /// <typeparam name="TProperty11">Type of projected property.</typeparam>
    /// <remarks>This class supports the WCF Data Services infrastructure and is not meant to be used directly from your code.</remarks>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA903", Justification = "Type is already under System namespace.")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1005", Justification = "More than two parameter types used for wide projections.")]
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    public sealed class ExpandedWrapper<TExpandedElement, TProperty0, TProperty1, TProperty2, TProperty3, TProperty4, TProperty5, TProperty6, TProperty7, TProperty8, TProperty9, TProperty10, TProperty11> : ExpandedWrapper<TExpandedElement>
    {
        /// <summary>Gets or sets a projected property.</summary>
        public TProperty0 ProjectedProperty0 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty1 ProjectedProperty1 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty2 ProjectedProperty2 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty3 ProjectedProperty3 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty4 ProjectedProperty4 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty5 ProjectedProperty5 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty6 ProjectedProperty6 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty7 ProjectedProperty7 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty8 ProjectedProperty8 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty9 ProjectedProperty9 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty10 ProjectedProperty10 { get; set; }

        /// <summary>Gets or sets a projected property.</summary>
        public TProperty11 ProjectedProperty11 { get; set; }

        /// <summary>Gets the value for the named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        protected override object InternalGetExpandedPropertyValue(int nameIndex)
        {
            if (nameIndex == 0) return this.ProjectedProperty0;
            if (nameIndex == 1) return this.ProjectedProperty1;
            if (nameIndex == 2) return this.ProjectedProperty2;
            if (nameIndex == 3) return this.ProjectedProperty3;
            if (nameIndex == 4) return this.ProjectedProperty4;
            if (nameIndex == 5) return this.ProjectedProperty5;
            if (nameIndex == 6) return this.ProjectedProperty6;
            if (nameIndex == 7) return this.ProjectedProperty7;
            if (nameIndex == 8) return this.ProjectedProperty8;
            if (nameIndex == 9) return this.ProjectedProperty9;
            if (nameIndex == 10) return this.ProjectedProperty10;
            if (nameIndex == 11) return this.ProjectedProperty11;
            throw Error.NotSupported();
        }
    }

    #endregion ExpandedWrapper types with numbered properties.
}
