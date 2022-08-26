//---------------------------------------------------------------------
// <copyright file="DataServicePagingProviderWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Wrapper for IDataServicePagingProvider discovery by service instance.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    /// <summary>Wrapper for IDataServicePagingProvider interface discovery.</summary>
    internal sealed class DataServicePagingProviderWrapper
    {
        /// <summary>IDataServicePagingProvider interface for the service.</summary>
        private IDataServicePagingProvider pagingProvider;

        /// <summary>Service instance.</summary>
        private IDataService service;

        /// <summary>Was interface already requested.</summary>
        private bool checkedForIDataServicePagingProvider;

        /// <summary>Constructor.</summary>
        /// <param name="serviceInstance">Service instance.</param>
        public DataServicePagingProviderWrapper(IDataService serviceInstance)
        {
            this.service = serviceInstance;
        }

        /// <summary>Gives reference to IDataServicePagingProvider interface implemented by the service.</summary>
        public IDataServicePagingProvider PagingProviderInterface
        {
            get
            {
                if (!this.checkedForIDataServicePagingProvider)
                {
                    this.pagingProvider = this.service.Provider.GetService<IDataServicePagingProvider>(this.service);
                    this.checkedForIDataServicePagingProvider = true;
                }

                return this.pagingProvider;
            }
        }

        /// <summary>Is custom paging enabled for the service for query processing.</summary>
        public bool IsCustomPagedForQuery
        {
            get
            {
                return this.PagingProviderInterface != null;
            }
        }

        /// <summary>Do we need to handle custom paging during serialization.</summary>
        public bool IsCustomPagedForSerialization
        {
            get
            {
                if (!this.checkedForIDataServicePagingProvider)
                {
                    return false;
                }
                else
                {
                    return this.pagingProvider != null;
                }
            }
        }

        /// <summary>
        /// Dispose the pagingProvider provider instance
        /// </summary>
        internal void DisposeProvider()
        {
            if (this.pagingProvider != null)
            {
                WebUtil.Dispose(this.pagingProvider);
                this.pagingProvider = null;
            }
        }
    }
}
