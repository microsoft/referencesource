//---------------------------------------------------------------------
// <copyright file="UpdatableWrapper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//     Wraps all the calls to IUpdatable interface.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Linq;
    using System.Diagnostics;
    using System.Collections.Generic;
    using System.Data.Services.Providers;

    /// <summary>
    /// This class wraps all the calls to IUpdatable interface.
    /// </summary>
    internal class UpdatableWrapper
    {
        /// <summary> instance implementation of IUpdatable.</summary>
        private IUpdatable updateProvider;

        /// <summary> data service instance.</summary>
        private IDataService service;

        /// <summary>
        /// creates an instance of UpdatableWrapper, which wraps all the calls to IUpdatable interface.
        /// </summary>
        /// <param name="serviceInstance">instance of the data service.</param>
        internal UpdatableWrapper(IDataService serviceInstance)
        {
            this.service = serviceInstance;
        }

        /// <summary>
        /// Get the instance of IUpdatable.
        /// </summary>
        private IUpdatable UpdateProvider
        {
            get
            {
                if (this.updateProvider == null)
                {
                    // We need to call IUpdatable only for V1 providers. For ObjectContextServiceProvider, this always returns null.
                    // Hence basically, this call is only for reflection service provider.
                    if (this.service.Provider.IsV1Provider)
                    {
                        this.updateProvider = this.service.Provider.GetService<IUpdatable>((IDataService)this.service.Instance);
                    }

                    if (this.updateProvider == null)
                    {
                        this.updateProvider = this.service.Provider.GetService<IDataServiceUpdateProvider>((IDataService)this.service.Instance);
                    }
                }

                if (this.updateProvider == null)
                {
                    if (this.service.Provider.IsV1Provider)
                    {
                        throw DataServiceException.CreateMethodNotImplemented(Strings.UpdatableWrapper_MissingIUpdatableForV1Provider);
                    }
                    else
                    {
                        throw DataServiceException.CreateMethodNotImplemented(Strings.UpdatableWrapper_MissingUpdateProviderInterface);
                    }
                }

                return this.updateProvider;
            }
        }

        /// <summary>
        /// Creates the resource of the given type and belonging to the given container
        /// </summary>
        /// <param name="containerName">container name to which the resource needs to be added</param>
        /// <param name="fullTypeName">full type name i.e. Namespace qualified type name of the resource</param>
        /// <returns>object representing a resource of given type and belonging to the given container</returns>
        internal object CreateResource(string containerName, string fullTypeName)
        {
            // container name can be null for complex types
            // type name can be null when we don't know the type - e.g. DELETE /Customers(1) 
            // In case of inheritance we don't know the actual type of customer instance.
            object resource = this.UpdateProvider.CreateResource(containerName, fullTypeName);
            if (resource == null)
            {
                throw new InvalidOperationException(Strings.BadProvider_CreateResourceReturnedNull);
            }

            return resource;
        }

        /// <summary>
        /// Gets the resource of the given type that the query points to
        /// </summary>
        /// <param name="query">query pointing to a particular resource</param>
        /// <param name="fullTypeName">full type name i.e. Namespace qualified type name of the resource</param>
        /// <returns>object representing a resource of given type and as referenced by the query</returns>
        internal object GetResource(IQueryable query, string fullTypeName)
        {
            Debug.Assert(query != null, "query != null");
            try
            {
                // TypeName can be null since for open types, we may not know the type yet.
                return this.UpdateProvider.GetResource(query, fullTypeName);
            }
            catch (ArgumentException e)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidUriSpecified, e);
            }
        }

        /// <summary>
        /// Resets the value of the given resource to its default value
        /// </summary>
        /// <param name="resource">resource whose value needs to be reset</param>
        /// <returns>same resource with its value reset</returns>
        internal object ResetResource(object resource)
        {
            Debug.Assert(resource != null, "resource != null");
            object resetResource = this.UpdateProvider.ResetResource(resource);
            if (resetResource == null)
            {
                throw new InvalidOperationException(Strings.BadProvider_ResetResourceReturnedNull);
            }

            return resetResource;
        }

        /// <summary>
        /// If the provider implements IConcurrencyProvider, then this method passes the etag values
        /// to the provider, otherwise compares the etag itself.
        /// </summary>
        /// <param name="resourceCookie">etag values for the given resource.</param>
        /// <param name="container">container for the given resource.</param>
        internal void SetETagValues(object resourceCookie, ResourceSetWrapper container)
        {
            Debug.Assert(resourceCookie != null, "resourceCookie != null");
            Debug.Assert(container != null, "container != null");
            DataServiceHostWrapper host = this.service.OperationContext.Host;
            Debug.Assert(String.IsNullOrEmpty(host.RequestIfNoneMatch), "IfNoneMatch header cannot be specified for Update/Delete operations");

            // Resolve the cookie first to the actual resource type
            object actualEntity = this.ResolveResource(resourceCookie);
            Debug.Assert(actualEntity != null, "actualEntity != null");

            ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(this.service.Provider, actualEntity);
            Debug.Assert(resourceType != null, "resourceType != null");

            IList<ResourceProperty> etagProperties = this.service.Provider.GetETagProperties(container.Name, resourceType);

            if (etagProperties.Count == 0)
            {
                if (!String.IsNullOrEmpty(host.RequestIfMatch))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.Serializer_NoETagPropertiesForType);
                }

                // If the type has no etag properties, then we do not need to do any etag checks
                return;
            }

            // If the provider implements IConcurrencyProvider, then we need to call the provider
            // and pass the etag values. Else, we need to compare the etag values ourselves.
            IDataServiceUpdateProvider concurrencyProvider = this.updateProvider as IDataServiceUpdateProvider;
            if (concurrencyProvider != null)
            {
                bool? checkForEquality = null;
                IEnumerable<KeyValuePair<string, object>> etagValues = null;
                if (!String.IsNullOrEmpty(host.RequestIfMatch))
                {
                    checkForEquality = true;
                    etagValues = ParseETagValue(etagProperties, host.RequestIfMatch);
                }
                else
                {
                    etagValues = new KeyValuePair<string, object>[0];
                }

                concurrencyProvider.SetConcurrencyValues(resourceCookie, checkForEquality, etagValues);
            }
            else if (String.IsNullOrEmpty(host.RequestIfMatch))
            {
                throw DataServiceException.CreateBadRequestError(Strings.DataService_CannotPerformOperationWithoutETag(resourceType.FullName));
            }
            else if (host.RequestIfMatch != XmlConstants.HttpAnyETag)
            {
                // Compare If-Match header value with the current etag value, if the If-Match header value is not equal to '*'
                string etagValue = WebUtil.GetETagValue(resourceCookie, resourceType, etagProperties, this.service, false /*getMethod*/);
                Debug.Assert(!String.IsNullOrEmpty(etagValue), "etag value can never be null");

                if (etagValue != host.RequestIfMatch)
                {
                    throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch);
                }
            }
        }

        /// <summary>
        /// Sets the value of the given property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="propertyValue">value of the property</param>
        internal void SetValue(object targetResource, string propertyName, object propertyValue)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            this.UpdateProvider.SetValue(targetResource, propertyName, propertyValue);
        }

        /// <summary>
        /// Gets the value of the given property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <returns>the value of the property for the given target resource</returns>
        internal object GetValue(object targetResource, string propertyName)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            return this.UpdateProvider.GetValue(targetResource, propertyName);
        }

        /// <summary>
        /// Sets the value of the given reference property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="propertyValue">value of the property</param>
        internal void SetReference(object targetResource, string propertyName, object propertyValue)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            this.UpdateProvider.SetReference(targetResource, propertyName, propertyValue);
        }

        /// <summary>
        /// Adds the given value to the collection
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="resourceToBeAdded">value of the property which needs to be added</param>
        internal void AddReferenceToCollection(object targetResource, string propertyName, object resourceToBeAdded)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            this.UpdateProvider.AddReferenceToCollection(targetResource, propertyName, resourceToBeAdded);
        }

        /// <summary>
        /// Removes the given value from the collection
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="resourceToBeRemoved">value of the property which needs to be removed</param>
        internal void RemoveReferenceFromCollection(object targetResource, string propertyName, object resourceToBeRemoved)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");
            this.UpdateProvider.RemoveReferenceFromCollection(targetResource, propertyName, resourceToBeRemoved);
        }

        /// <summary>
        /// Delete the given resource
        /// </summary>
        /// <param name="targetResource">resource that needs to be deleted</param>
        internal void DeleteResource(object targetResource)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            this.UpdateProvider.DeleteResource(targetResource);
        }

        /// <summary>
        /// Saves all the pending changes made till now
        /// </summary>
        internal void SaveChanges()
        {
            this.UpdateProvider.SaveChanges();
        }

        /// <summary>
        /// Returns the actual instance of the resource represented by the given resource object
        /// </summary>
        /// <param name="resource">object representing the resource whose instance needs to be fetched</param>
        /// <returns>The actual instance of the resource represented by the given resource object</returns>
        internal object ResolveResource(object resource)
        {
            Debug.Assert(resource != null, "resource != null");
            object resolvedResource = this.UpdateProvider.ResolveResource(resource);
            if (resolvedResource == null)
            {
                throw new InvalidOperationException(Strings.BadProvider_ResolveResourceReturnedNull);
            }

            return resolvedResource;
        }

        /// <summary>
        /// Revert all the pending changes.
        /// </summary>
        internal void ClearChanges()
        {
            this.UpdateProvider.ClearChanges();
        }

        /// <summary>
        /// Dispose the update provider instance
        /// </summary>
        internal void DisposeProvider()
        {
            if (this.updateProvider != null)
            {
                WebUtil.Dispose(this.updateProvider);
                this.updateProvider = null;
            }
        }

        /// <summary>
        /// Parse the given etag value in the If-Match request header.
        /// </summary>
        /// <param name="etagProperties">List of etag properties for the type whose etag values we are parsing.</param>
        /// <param name="ifMatchHeaderValue">value of the If-Match header as specified in the request.</param>
        /// <returns>returns the etag value as a list containing the property name and its corresponding value. If the If-Match header value is '*', then returns an empty collection.</returns>
        private static IEnumerable<KeyValuePair<string, object>> ParseETagValue(IList<ResourceProperty> etagProperties, string ifMatchHeaderValue)
        {
            Debug.Assert(etagProperties != null && etagProperties.Count != 0, "There must be atleast one etag property specified");
            Debug.Assert(!String.IsNullOrEmpty(ifMatchHeaderValue), "IfMatch header cannot be null");

            if (ifMatchHeaderValue == XmlConstants.HttpAnyETag)
            {
                // if the value is '*', then we return an empty IEnumerable.
                return new KeyValuePair<string, object>[0];
            }

            Debug.Assert(ifMatchHeaderValue.StartsWith(XmlConstants.HttpWeakETagPrefix, StringComparison.Ordinal), "If-Match header must be properly formatted - this check is done in DataService.CheckETagValues method");
            Debug.Assert(ifMatchHeaderValue.Length >= XmlConstants.HttpWeakETagPrefix.Length + 1, "If-Match header must be properly formatted - this check is done in DataService.CheckETagValues method");

            // Just get the etag value - we need to ignore the 'W/"' and the last '"' character from the etag
            string strippedETag = ifMatchHeaderValue.Substring(XmlConstants.HttpWeakETagPrefix.Length, ifMatchHeaderValue.Length - XmlConstants.HttpWeakETagPrefix.Length - 1);

            KeyInstance keyInstance = null;
            bool success;
            Exception innerException = null;

            // In V1, when we didn't have IConcurrencyProvider interface, we always used to compute the
            // latest etag from the entity instance we got from IUpdatable.GetResource and then comparing
            // it with the If-Match request header. Hence all invalid cases always used to throw 
            // DataServiceException with 412, since the etags didn't match.
            // In V1.5, we have added the support for IConcurrencyProvider, which means we need to parse
            // the etag values and parse it to the provider, if it has implement this interface. To avoid
            // breaking changes, we need to catch all parsing errors and report them as 412 instead of 400
            // to avoid it from becoming a breaking change.
            try
            {
                success = KeyInstance.TryParseNullableTokens(Uri.UnescapeDataString(strippedETag), out keyInstance);
            }
            catch (DataServiceException e)
            {
                success = false;
                innerException = e;
            }

            if (!success)
            {
                // We could have throwed BadRequest here since the etag value is not properly formattted. But since
                // we used to do throw 412 in V1, keeping it that way to avoid breaking change.
                throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch, innerException);
            }

            if (keyInstance.PositionalValues.Count != etagProperties.Count)
            {
                // We could have throwed BadRequest here since the etag value is not properly formattted. But since
                // we used to do throw 412 in V1, keeping it that way to avoid breaking change.
                throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch);
            }

            KeyValuePair<string, object>[] etagPropertyInfo = new KeyValuePair<string, object>[etagProperties.Count];
            for (int i = 0; i < etagPropertyInfo.Length; i++)
            {
                ResourceProperty etagProperty = etagProperties[i];
                object propertyValue = null;
                string value = (string)keyInstance.PositionalValues[i];

                if (value != XmlConstants.NullLiteralInETag)
                {
                    // The reason we need to catch the Overflow Exception here is because of the Bug #679728.
                    // In V1, when we didn't have IConcurrencyProvider interface, we always used to compute the
                    // latest etag from the entity instance we got from IUpdatable.GetResource and then comparing
                    // it with the If-Match request header. Hence all invalid cases always used to throw 
                    // DataServiceException with 412, since the etags didn't match.
                    // In V1.5, we have added the support for IConcurrencyProvider, which means we need to parse
                    // the etag values and parse it to the provider, if it has implement this interface. To avoid
                    // breaking changes, we need to catch all parsing errors and report them as 412 instead of 400
                    // to avoid it from becoming a breaking change.
                    try
                    {
                        success = System.Data.Services.Parsing.WebConvert.TryKeyStringToPrimitive(value, etagProperty.Type, out propertyValue);
                    }
                    catch (OverflowException e)
                    {
                        success = false;
                        innerException = e;
                    }

                    if (!success)
                    {
                        // We could have throwed BadRequest here since the etag value is not properly formattted. But since
                        // we used to do throw 412 in V1, keeping it that way to avoid breaking change.
                        throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch, innerException);
                    }
                }

                etagPropertyInfo[i] = new KeyValuePair<string, object>(etagProperties[i].Name, propertyValue);
            }

            return etagPropertyInfo;
        }
    }
}
