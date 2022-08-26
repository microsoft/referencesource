//---------------------------------------------------------------------
// <copyright file="UpdateTracker.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class used to track updates for callbacks.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Collections.Generic;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Reflection;

    /// <summary>Provides a class used to track updates for callbacks.</summary>
    internal class UpdateTracker
    {
        #region Private fields.

        /// <summary>
        /// A dictionary of containers mapping to the changes on those 
        /// containers, each of which consists of an element and the
        /// action taken on it.
        /// </summary>
        private Dictionary<ResourceSetWrapper, Dictionary<object, UpdateOperations>> items;

        /// <summary>Underlying data service instance.</summary>
        private IDataService service;

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="UpdateTracker"/> instance.</summary>
        /// <param name="service">underlying data source instance.</param>
        private UpdateTracker(IDataService service)
        {
            this.service = service;
            this.items = new Dictionary<ResourceSetWrapper, Dictionary<object, UpdateOperations>>(ReferenceEqualityComparer<ResourceSetWrapper>.Instance);
        }

        /// <summary>Fires the notification for a single action.</summary>
        /// <param name="service">Service on which methods should be invoked.</param>
        /// <param name="target">Object to be tracked.</param>
        /// <param name="container">Container in which object is changed.</param>
        /// <param name="action">Action affecting target.</param>
        internal static void FireNotification(IDataService service, object target, ResourceSetWrapper container, UpdateOperations action)
        {
            Debug.Assert(service != null, "service != null");
            AssertActionValues(target, container);

            MethodInfo[] methods = container.ChangeInterceptors;
            if (methods != null)
            {
                object[] parameters = new object[2];
                parameters[0] = target;
                parameters[1] = action;
                for (int i = 0; i < methods.Length; i++)
                {
                    try
                    {
                        methods[i].Invoke(service.Instance, parameters);
                    }
                    catch (TargetInvocationException exception)
                    {
                        ErrorHandler.HandleTargetInvocationException(exception);
                        throw;
                    }
                }
            }
        }

        /// <summary>
        /// Create a new instance of update tracker
        /// </summary>
        /// <param name="service">underlying data service.</param>
        /// <returns>
        /// Returns a new instance of UpdateTracker.
        /// </returns>
        internal static UpdateTracker CreateUpdateTracker(IDataService service)
        {
            return new UpdateTracker(service);
        }

        /// <summary>Fires all notifications</summary>
        internal void FireNotifications()
        {
            object[] parameters = new object[2];
            foreach (var item in this.items)
            {
                MethodInfo[] methods = item.Key.ChangeInterceptors;
                Debug.Assert(methods != null, "methods != null - should not have been tracking changes to the container otherwise.");
                foreach (var element in item.Value)
                {
                    parameters[0] = this.service.Updatable.ResolveResource(element.Key);
                    parameters[1] = element.Value;
                    for (int i = 0; i < methods.Length; i++)
                    {
                        try
                        {
                            methods[i].Invoke(this.service.Instance, parameters);
                        }
                        catch (TargetInvocationException exception)
                        {
                            ErrorHandler.HandleTargetInvocationException(exception);
                            throw;
                        }
                    }
                }

                // Make elements elegible for garbage collection.
                item.Value.Clear();
            }

            // Make dictionary elegible for garbage collection.
            this.items = null;
        }

        /// <summary>
        /// Tracks the specified <paramref name="target"/> for a 
        /// given <paramref name="action "/> on the <paramref name="container"/>.
        /// </summary>
        /// <param name="target">Object to be tracked.</param>
        /// <param name="container">Container in which object is changed.</param>
        /// <param name="action">Action affecting target.</param>
        /// <remarks>
        /// If <paramref name="target"/> was already being tracked, the actions are OR'ed together.
        /// </remarks>
        internal void TrackAction(object target, ResourceSetWrapper container, UpdateOperations action)
        {
            AssertActionValues(target, container);
            Debug.Assert(this.items != null, "this.items != null - otherwise FireNotification has already been called");

            // If it won't be necessary for us to fire authorizatio methods,
            // skip tracking altogether.
            if (container.ChangeInterceptors == null)
            {
                return;
            }

            // Get the container for which the change has taken place.
            Dictionary<object, UpdateOperations> changedItems;
            if (!this.items.TryGetValue(container, out changedItems))
            {
                // In order to mantain backwards compatibility, we are going to use default comparer for V1
                // providers. However, for V2 providers we are going to do reference equality comparisons.
                if (this.service.Provider.IsV1Provider)
                {
                    changedItems = new Dictionary<object, UpdateOperations>(EqualityComparer<object>.Default);
                }
                else
                {
                    changedItems = new Dictionary<object, UpdateOperations>(ReferenceEqualityComparer<object>.Instance);
                }

                this.items.Add(container, changedItems);
            }

            UpdateOperations existingAction;
            if (changedItems.TryGetValue(target, out existingAction))
            {
                if ((action | existingAction) != existingAction)
                {
                    changedItems[target] = action | existingAction;
                }
            }
            else
            {
                changedItems.Add(target, action);
            }
        }

        /// <summary>Asserts valid value for tracking update actions.</summary>
        /// <param name="target">Object to be tracked.</param>
        /// <param name="container">Container in which object is changed.</param>
        [Conditional("DEBUG")]
        private static void AssertActionValues(object target, ResourceSetWrapper container)
        {
            Debug.Assert(target != null, "target != null");
            Debug.Assert(container != null, "container != null");
        }
    }
}
