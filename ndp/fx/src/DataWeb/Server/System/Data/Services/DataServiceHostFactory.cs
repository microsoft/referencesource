//---------------------------------------------------------------------
// <copyright file="DataServiceHostFactory.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a way to hook up a WCF service as a raw HTTP handler.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.ServiceModel;
    using System.ServiceModel.Activation;

    /// <summary>
    /// This structure supports the .NET Framework infrastructure and is 
    /// not intended to be used directly from your code.
    /// </summary>
    /// <internal>
    /// This class is used to hook up a WCF as a raw HTTP handler.
    /// </internal>
    public class DataServiceHostFactory : ServiceHostFactory
    {
        /// <summary>
        /// Creates a ServiceHost for a specified type of service with a specific base address.
        /// </summary>
        /// <param name="serviceType">Specifies the type of service to host.</param>
        /// <param name="baseAddresses">The Array of type Uri that contains the base addresses for the service hosted.</param>
        /// <returns>A ServiceHost for the type of service specified with a specific base address.</returns>
        protected override ServiceHost CreateServiceHost(Type serviceType, Uri[] baseAddresses)
        {
            return new DataServiceHost(serviceType, baseAddresses);
        }
    }
}
