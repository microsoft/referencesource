//---------------------------------------------------------------------
// <copyright file="ServiceOperation.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to represent custom operations on services.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;

    /// <summary>Use this class to represent a custom service operation.</summary>
    [DebuggerVisualizer("ServiceOperation={Name}")]
    public class ServiceOperation
    {
        /// <summary>Protocol (for example HTTP) method the service operation responds to.</summary>
        private readonly string method;

        /// <summary>In-order parameters for this operation.</summary>
        private readonly ReadOnlyCollection<ServiceOperationParameter> parameters;

        /// <summary>Kind of result expected from this operation.</summary>
        private readonly ServiceOperationResultKind resultKind;

        /// <summary>Type of element of the method result.</summary>
        private readonly ResourceType resultType;

        /// <summary>Empty parameter collection.</summary>
        private static ReadOnlyCollection<ServiceOperationParameter> emptyParameterCollection = new ReadOnlyCollection<ServiceOperationParameter>(new ServiceOperationParameter[0]);

        /// <summary>MIME type specified on primitive results, possibly null.</summary>
        private string mimeType;

        /// <summary>Entity set from which entities are read, if applicable.</summary>
        private ResourceSet resourceSet;

        /// <summary>name of the service operation.</summary>
        private string name;

        /// <summary>Is true, if the service operation is set to readonly i.e. fully initialized and validated. No more changes can be made,
        /// after the service operation is set to readonly.</summary>
        private bool isReadOnly;

        /// <summary>
        /// Initializes a new <see cref="ServiceOperation"/> instance.
        /// </summary>
        /// <param name="name">name of the service operation.</param>
        /// <param name="resultKind">Kind of result expected from this operation.</param>
        /// <param name="resultType">Type of element of the method result.</param>
        /// <param name="resultSet">EntitySet of the result expected from this operation.</param>
        /// <param name="method">Protocol (for example HTTP) method the service operation responds to.</param>
        /// <param name="parameters">In-order parameters for this operation.</param>
        public ServiceOperation(
            string name,
            ServiceOperationResultKind resultKind,
            ResourceType resultType,
            ResourceSet resultSet,
            string method,
            IEnumerable<ServiceOperationParameter> parameters)
        {
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckServiceOperationResultKind(resultKind, "resultKind");
            WebUtil.CheckStringArgumentNull(method, "method");

            if ((resultKind == ServiceOperationResultKind.Void && resultType != null) ||
                (resultKind != ServiceOperationResultKind.Void && resultType == null))
            {
                throw new ArgumentException(Strings.ServiceOperation_ResultTypeAndKindMustMatch("resultKind", "resultType", ServiceOperationResultKind.Void));
            }

            if ((resultType == null || resultType.ResourceTypeKind != ResourceTypeKind.EntityType) && resultSet != null)
            {
                throw new ArgumentException(Strings.ServiceOperation_ResultSetMustBeNull("resultSet", "resultType"));
            }

            if (resultType != null && resultType.ResourceTypeKind == ResourceTypeKind.EntityType && (resultSet == null || !resultSet.ResourceType.IsAssignableFrom(resultType)))
            {
                throw new ArgumentException(Strings.ServiceOperation_ResultTypeAndResultSetMustMatch("resultType", "resultSet"));
            }

            if (method != XmlConstants.HttpMethodGet && method != XmlConstants.HttpMethodPost)
            {
                throw new ArgumentException(Strings.ServiceOperation_NotSupportedProtocolMethod(method, name));
            }

            this.name = name;
            this.resultKind = resultKind;
            this.resultType = resultType;
            this.resourceSet = resultSet;
            this.method = method;
            if (parameters == null)
            {
                this.parameters = ServiceOperation.emptyParameterCollection;
            }
            else
            {
                this.parameters = new ReadOnlyCollection<ServiceOperationParameter>(new List<ServiceOperationParameter>(parameters));
                HashSet<string> paramNames = new HashSet<string>(StringComparer.Ordinal);
                foreach (ServiceOperationParameter p in this.parameters)
                {
                    if (!paramNames.Add(p.Name))
                    {
                        throw new ArgumentException(Strings.ServiceOperation_DuplicateParameterName(p.Name), "parameters");
                    }
                }
            }
        }

        /// <summary>Protocol (for example HTTP) method the service operation responds to.</summary>
        public string Method
        {
            get { return this.method; }
        }

        /// <summary>MIME type specified on primitive results, possibly null.</summary>
        public string MimeType
        {
            get
            {
                return this.mimeType;
            }

            set
            {
                this.ThrowIfSealed();
                if (String.IsNullOrEmpty(value))
                {
                    throw new InvalidOperationException(Strings.ServiceOperation_MimeTypeCannotBeEmpty(this.Name));
                }

                if (!WebUtil.IsValidMimeType(value))
                {
                    throw new InvalidOperationException(Strings.ServiceOperation_MimeTypeNotValid(value, this.Name));
                }

                this.mimeType = value;
            }
        }

        /// <summary>Name of the service operation.</summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary>Returns all the parameters for the given service operations./// </summary>
        public ReadOnlyCollection<ServiceOperationParameter> Parameters
        {
            get { return this.parameters; }
        }

        /// <summary>Kind of result expected from this operation.</summary>
        public ServiceOperationResultKind ResultKind
        {
            get { return this.resultKind; }
        }

        /// <summary>Element of result type.</summary>
        /// <remarks>
        /// Note that if the method returns an IEnumerable&lt;string&gt;, 
        /// this property will be typeof(string).
        /// </remarks>
        public ResourceType ResultType
        {
            get { return this.resultType; }
        }

        /// <summary>
        /// PlaceHolder to hold custom state information about service operation.
        /// </summary>
        public object CustomState
        {
            get;
            set;
        }

        /// <summary>
        /// Returns true, if this service operation has been set to read only. Otherwise returns false.
        /// </summary>
        public bool IsReadOnly
        {
            get { return this.isReadOnly; }
        }

        /// <summary>Entity set from which entities are read (possibly null).</summary>
        public ResourceSet ResourceSet
        {
            get { return this.resourceSet; }
        }

        /// <summary>
        /// Set this service operation to readonly.
        /// </summary>
        public void SetReadOnly()
        {
            if (this.isReadOnly)
            {
                return;
            }

            foreach (ServiceOperationParameter parameter in this.Parameters)
            {
                parameter.SetReadOnly();
            }

            this.isReadOnly = true;
        }

        /// <summary>
        /// Throws an InvalidOperationException if this service operation is already set to readonly.
        /// </summary>
        internal void ThrowIfSealed()
        {
            if (this.isReadOnly)
            {
                throw new InvalidOperationException(Strings.ServiceOperation_Sealed(this.Name));
            }
        }
    }
}
