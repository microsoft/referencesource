//---------------------------------------------------------------------
// <copyright file="DataServiceProcessingPipeline.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Class declaring events for the data service processing pipeline
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Diagnostics;

    /// <summary>
    /// Class declaring the events for the data service processing pipeline
    /// </summary>
    public sealed class DataServiceProcessingPipeline
    {
#if DEBUG
        // ***************************************************************************************
        // The debug code here is for basic assertions on call orders if the request is successful.
        // ***************************************************************************************

        /// <summary>
        /// Number of times InvokeProcessingRequest() is called.
        /// </summary>
        private int ProcessingRequestInvokeCount;

        /// <summary>
        /// Number of times InvokeProcessedRequest() is called
        /// </summary>
        private int ProcessedRequestInvokeCount;

        /// <summary>
        /// Number of times InvokeProcessingChangeset() is called
        /// </summary>
        private int ProcessingChangesetInvokeCount;

        /// <summary>
        /// Number of times InvokeProcessedChangeset() is called
        /// </summary>
        private int ProcessedChangesetInvokeCount;

        /// <summary>
        /// Set to true if we have seen an exception in the current request or we are in the $metadata path
        /// </summary>
        /// <remarks>
        /// If the current request encounters an exception, there is no guarantee that all the
        /// events will be fired, we may skip the debug asserts when that happens.
        /// </remarks>
        internal bool SkipDebugAssert;

        /// <summary>
        /// Set to true if DataServiceProviderWrapper.GetService() is called.
        /// </summary>
        internal bool HasInstantiatedProviderInterfaces;

        /// <summary>
        /// Set to true if any of the service is disposed from WebUtil.Dispose()
        /// </summary>
        internal bool HasDisposedProviderInterfaces;

        /// <summary>
        /// Number of times DataService&lt;T&gt;.OnStartProcessingRequest() is called.
        /// </summary>
        internal int OnStartProcessingRequestInvokeCount;
#endif

        /// <summary>
        /// Request start event
        /// </summary>
        public event EventHandler<DataServiceProcessingPipelineEventArgs> ProcessingRequest;

        /// <summary>
        /// Request end event
        /// </summary>
        public event EventHandler<DataServiceProcessingPipelineEventArgs> ProcessedRequest;

        /// <summary>
        /// Change set start event
        /// </summary>
        public event EventHandler<EventArgs> ProcessingChangeset;

        /// <summary>
        /// Change set end event
        /// </summary>
        public event EventHandler<EventArgs> ProcessedChangeset;

#if DEBUG
        /// <summary>
        /// Assert ProcessingPipeline state before any event has been fired
        /// </summary>
        internal void AssertInitialDebugState()
        {
            Debug.Assert(!this.HasInstantiatedProviderInterfaces, "!this.HasInstantiatedProviderInterfaces");
            Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
            Debug.Assert(this.OnStartProcessingRequestInvokeCount == 0, "this.OnStartProcessingRequestInvokeCount == 0");
            Debug.Assert(this.ProcessingRequestInvokeCount == 0, "this.ProcessingRequestInvokeCount == 0");
            Debug.Assert(this.ProcessedRequestInvokeCount == 0, "this.ProcessedRequestInvokeCount == 0");
            Debug.Assert(this.ProcessingChangesetInvokeCount == 0, "this.ProcessingChangesetInvokeCount == 0");
            Debug.Assert(this.ProcessedChangesetInvokeCount == 0, "this.ProcessedChangesetInvokeCount == 0");
        }

        /// <summary>
        /// Assert ProcessingPipeline state at DataService&lt;T&gt;.OnStartProcessingRequest
        /// </summary>
        internal void AssertDebugStateAtOnStartProcessingRequest()
        {
            // If the current request encounters an exception, there is no guarantee that all the
            // events will be fired, we skip the debug asserts when that happens.
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(
                    this.OnStartProcessingRequestInvokeCount > 0 || !this.HasInstantiatedProviderInterfaces,
                    "this.OnStartProcessingRequestInvokeCount > 0 || !this.HasInstantiatedProviderInterfaces");
                Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
                Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
                Debug.Assert(this.ProcessedRequestInvokeCount == 0, "this.ProcessedRequestInvokeCount == 0");
                Debug.Assert(
                    this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount || this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount + 1,
                    "this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount || this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount + 1");
            }
        }

        /// <summary>
        /// Assert ProcessingPipeline state before disposing provider interfaces
        /// </summary>
        internal void AssertDebugStateAtDispose()
        {
            // If the current request encounters an exception, there is no guarantee that all the
            // events will be fired, we skip the debug asserts when that happens.
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(this.OnStartProcessingRequestInvokeCount > 0, "this.OnStartProcessingRequestInvokeCount > 0");
                Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
                Debug.Assert(this.ProcessedRequestInvokeCount == 1, "this.ProcessedRequestInvokeCount == 1");
                Debug.Assert(
                    this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount,
                    "this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount");
            }
        }

        /// <summary>
        /// Assert Processing Pipeline state during request processing
        /// </summary>
        /// <param name="dataService">data service instance</param>
        internal void AssertDebugStateDuringRequestProcessing(IDataService dataService)
        {
            // If the current request encounters an exception, there is no guarantee that all the
            // events will be fired, we skip the debug asserts when that happens.
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
                Debug.Assert(this.OnStartProcessingRequestInvokeCount > 0, "this.OnStartProcessingRequestInvokeCount > 0");
                Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
                Debug.Assert(
                    (this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount && dataService.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.GET) ||
                    (this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount + 1 && dataService.OperationContext.Host.AstoriaHttpVerb != AstoriaVerbs.GET),
                    "ProcessingChangesetInvokeCount must be 1 larger than ProcessedChangesetInvokeCount in CUD operation and they must be the same in a GET request.");
            }
        }

        /// <summary>
        /// Need to be able to reset the states since the caller can reuse the same service instance.
        /// </summary>
        internal void ResetDebugState()
        {
            this.HasInstantiatedProviderInterfaces = false;
            this.HasDisposedProviderInterfaces = false;
            this.OnStartProcessingRequestInvokeCount = 0;
            this.ProcessingRequestInvokeCount = 0;
            this.ProcessedRequestInvokeCount = 0;
            this.ProcessingChangesetInvokeCount = 0;
            this.ProcessedChangesetInvokeCount = 0;
        }
#endif

        /// <summary>
        /// Invoke request start event
        /// </summary>
        /// <param name="sender">Sender, i.e. data service instance.</param>
        /// <param name="e">event arg</param>
        internal void InvokeProcessingRequest(object sender, DataServiceProcessingPipelineEventArgs e)
        {
#if DEBUG
            this.AssertInitialDebugState();
            this.ProcessingRequestInvokeCount++;
#endif
            if (this.ProcessingRequest != null)
            {
                this.ProcessingRequest(sender, e);
            }
        }

        /// <summary>
        /// Invoke request end event
        /// </summary>
        /// <param name="sender">Sender, i.e. data service instance.</param>
        /// <param name="e">event arg</param>
        internal void InvokeProcessedRequest(object sender, DataServiceProcessingPipelineEventArgs e)
        {
#if DEBUG
            Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
            Debug.Assert(this.OnStartProcessingRequestInvokeCount > 0, "this.OnStartProcessingRequestInvokeCount > 0");
            Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
            Debug.Assert(this.ProcessedRequestInvokeCount == 0, "this.ProcessedRequestInvokeCount == 0");
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount, "this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount");
            }

            this.ProcessedRequestInvokeCount++;
#endif
            if (this.ProcessedRequest != null)
            {
                this.ProcessedRequest(sender, e);
            }
        }

        /// <summary>
        /// Invoke change set start event
        /// </summary>
        /// <param name="sender">Sender, i.e. data service instance.</param>
        /// <param name="e">event arg</param>
        internal void InvokeProcessingChangeset(object sender, EventArgs e)
        {
#if DEBUG
            Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
            Debug.Assert(this.OnStartProcessingRequestInvokeCount > 0, "this.OnStartProcessingRequestInvokeCount > 0");
            Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
            Debug.Assert(this.ProcessedRequestInvokeCount == 0, "this.ProcessedRequestInvokeCount == 0");
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount, "this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount");
            }

            this.ProcessingChangesetInvokeCount++;
#endif
            if (this.ProcessingChangeset != null)
            {
                this.ProcessingChangeset(sender, e);
            }
        }

        /// <summary>
        /// Invoke change set end event
        /// </summary>
        /// <param name="sender">Sender, i.e. data service instance.</param>
        /// <param name="e">event arg</param>
        internal void InvokeProcessedChangeset(object sender, EventArgs e)
        {
#if DEBUG
            Debug.Assert(!this.HasDisposedProviderInterfaces, "!this.HasDisposedProviderInterfaces");
            Debug.Assert(this.OnStartProcessingRequestInvokeCount > 0, "this.OnStartProcessingRequestInvokeCount > 0");
            Debug.Assert(this.ProcessingRequestInvokeCount == 1, "this.ProcessingRequestInvokeCount == 1");
            Debug.Assert(this.ProcessedRequestInvokeCount == 0, "this.ProcessedRequestInvokeCount == 0");
            if (!this.SkipDebugAssert)
            {
                Debug.Assert(this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount + 1, "this.ProcessingChangesetInvokeCount == this.ProcessedChangesetInvokeCount + 1");
            }

            this.ProcessedChangesetInvokeCount++;
#endif
            if (this.ProcessedChangeset != null)
            {
                this.ProcessedChangeset(sender, e);
            }
        }
    }
}
