//---------------------------------------------------------------------
// <copyright file="ServiceOperationParameter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to represent parameter information for service
//      operations.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System.Diagnostics;

    /// <summary>Use this type to represent a parameter on a service operation.</summary>
    [DebuggerVisualizer("ServiceOperationParameter={Name}")]
    public class ServiceOperationParameter
    {
        /// <summary>Parameter name.</summary>
        private readonly string name;

        /// <summary>Parameter type.</summary>
        private readonly ResourceType type;

        /// <summary>Is true, if the service operation parameter is set to readonly i.e. fully initialized and validated.
        /// No more changes can be made, after this is set to readonly.</summary>
        private bool isReadOnly;

        /// <summary>
        /// Initializes a new <see cref="ServiceOperationParameter"/>.
        /// </summary>
        /// <param name="name">Name of parameter.</param>
        /// <param name="parameterType">resource type of parameter value.</param>
        public ServiceOperationParameter(string name, ResourceType parameterType)
        {
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckArgumentNull(parameterType, "parameterType");

            if (parameterType.ResourceTypeKind != ResourceTypeKind.Primitive)
            {
                throw new ArgumentException(Strings.ServiceOperationParameter_TypeNotSupported(name, parameterType.FullName));
            }

            this.name = name;
            this.type = parameterType;
        }

        /// <summary>Name of parameter.</summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary>Type of parameter values.</summary>
        public ResourceType ParameterType
        {
            get { return this.type; }
        }

        /// <summary>
        /// PlaceHolder to hold custom state information about service operation parameter.
        /// </summary>
        public object CustomState
        {
            get;
            set;
        }

        /// <summary>
        /// Returns true, if this parameter has been set to read only. Otherwise returns false.
        /// </summary>
        public bool IsReadOnly
        {
            get { return this.isReadOnly; }
        }

        /// <summary>
        /// Sets this service operation parameter to readonly.
        /// </summary>
        public void SetReadOnly()
        {
            if (this.isReadOnly)
            {
                return;
            }

            this.isReadOnly = true;
        }
    }
}
