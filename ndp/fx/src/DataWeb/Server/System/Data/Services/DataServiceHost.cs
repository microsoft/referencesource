//---------------------------------------------------------------------
// <copyright file="DataServiceHost.cs" company="Microsoft">
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
    #region Namespaces.

    using System;
    using System.ServiceModel.Web;

    #endregion Namespaces.

    /// <summary>
    /// This structure supports the .NET Framework infrastructure and is 
    /// not intended to be used directly from your code.
    /// </summary>
    /// <internal>
    /// Provides a host for services of type DataService.
    /// </internal>
    [CLSCompliant(false)]
    public class DataServiceHost : WebServiceHost
    {
        /// <summary>
        /// Initializes a new instance of the ServiceHost class with the type
        /// of service and its base addresses specified.
        /// </summary>
        /// <param name="serviceType">The type of hosted service.</param>
        /// <param name="baseAddresses">An array of type Uri that contains the base addresses for the hosted service.</param>
        public DataServiceHost(Type serviceType, Uri[] baseAddresses)
            : base(serviceType, baseAddresses)
        {
        }
    }
}
