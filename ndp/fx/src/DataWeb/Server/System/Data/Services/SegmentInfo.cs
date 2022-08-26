//---------------------------------------------------------------------
// <copyright file="SegmentInfo.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Structure containing information about a segment (Uri is made
//      up of bunch of segments, each segment is seperated by '/' character)
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;

    #endregion Namespaces.

    /// <summary>Contains the information regarding a segment that makes up the uri</summary>
    [DebuggerDisplay("SegmentInfo={Identifier} -> {TargetKind} '{TargetResourceType.InstanceType}'")]
    internal class SegmentInfo
    {
        #region Private fields.

        /// <summary>Returns the identifier for this segment i.e. string part without the keys.</summary>
        private string identifier;

        /// <summary>Returns the values that constitute the key as specified in the request.</summary>
        private KeyInstance key;

        /// <summary>Returns the query that's being composed for this segment</summary>
        private IEnumerable requestEnumerable;

        /// <summary>Whether the segment targets a single result or not.</summary>
        private bool singleResult;

        /// <summary>resource set if applicable.</summary>
        private ResourceSetWrapper targetContainer;

        /// <summary>The type of resource targeted by this segment.</summary>
        private ResourceType targetResourceType;

        /// <summary>The kind of resource targeted by this segment.</summary>
        private RequestTargetKind targetKind;

        /// <summary>Returns the source for this segment</summary>
        private RequestTargetSource targetSource;

        /// <summary>Service operation being invoked.</summary>
        private ServiceOperationWrapper operation;

        /// <summary>Operation parameters.</summary>
        private object[] operationParameters;

        /// <summary>Returns the property that is being projected in this segment, if there's any.</summary>
        private ResourceProperty projectedProperty;

        #endregion Private fields.

        /// <summary>Empty constructor.</summary>
        internal SegmentInfo()
        {
        }

        /// <summary>Copy constructor.</summary>
        /// <param name="other">Another <see cref="SegmentInfo"/> to get a shallow copy of.</param>
        internal SegmentInfo(SegmentInfo other)
        {
            Debug.Assert(other != null, "other != null");
            this.Identifier = other.Identifier;
            this.Key = other.Key;
            this.Operation = other.Operation;
            this.OperationParameters = other.OperationParameters;
            this.ProjectedProperty = other.ProjectedProperty;
            this.RequestEnumerable = other.RequestEnumerable;
            this.SingleResult = other.SingleResult;
            this.TargetContainer = other.TargetContainer;
            this.TargetKind = other.TargetKind;
            this.TargetSource = other.TargetSource;
            this.targetResourceType = other.targetResourceType;
        }

        /// <summary>Returns the identifier for this segment i.e. string part without the keys.</summary>
        internal string Identifier
        {
            get { return this.identifier; }
            set { this.identifier = value; }
        }

        /// <summary>Returns the values that constitute the key as specified in the request.</summary>
        internal KeyInstance Key
        {
            get { return this.key; }
            set { this.key = value; }
        }

        /// <summary>Returns the query that's being composed for this segment</summary>
        internal IEnumerable RequestEnumerable
        {
            get { return this.requestEnumerable; }
            set { this.requestEnumerable = value; }
        }

        /// <summary>Whether the segment targets a single result or not.</summary>
        internal bool SingleResult
        {
            get { return this.singleResult; }
            set { this.singleResult = value; }
        }

        /// <summary>resource set if applicable.</summary>
        internal ResourceSetWrapper TargetContainer
        {
            get { return this.targetContainer; }
            set { this.targetContainer = value; }
        }

        /// <summary>The type of element targeted by this segment.</summary>
        internal ResourceType TargetResourceType
        {
            get { return this.targetResourceType; }
            set { this.targetResourceType = value; }
        }

        /// <summary>The kind of resource targeted by this segment.</summary>
        internal RequestTargetKind TargetKind
        {
            get { return this.targetKind; }
            set { this.targetKind = value; }
        }

        /// <summary>Returns the source for this segment</summary>
        internal RequestTargetSource TargetSource
        {
            get { return this.targetSource; }
            set { this.targetSource = value; }
        }

        /// <summary>Service operation being invoked.</summary>
        internal ServiceOperationWrapper Operation
        {
            get { return this.operation; }
            set { this.operation = value; }
        }

        /// <summary>Operation parameters.</summary>
        internal object[] OperationParameters
        {
            get { return this.operationParameters; }
            set { this.operationParameters  = value; }
        }

        /// <summary>Returns the property that is being projected in this segment, if there's any.</summary>
        internal ResourceProperty ProjectedProperty
        {
            get { return this.projectedProperty; }
            set { this.projectedProperty = value; }
        }

        /// <summary>Returns true if this segment has a key filter with values; false otherwise.</summary>
        internal bool HasKeyValues
        {
            get { return this.Key != null && !this.Key.IsEmpty; }
        }

        /// <summary>
        /// Determines whether the target kind is a direct reference to an element
        /// i.e. either you have a $value or you are accessing a resource via key property
        /// (/Customers(1) or /Customers(1)/BestFriend/Orders('Foo'). Either case the value
        /// cannot be null.
        /// </summary>
        /// <param name="kind">Kind of request to evaluate.</param>
        /// <returns>
        /// A characteristic of a direct reference is that if its value
        /// is null, a 404 error should be returned.
        /// </returns>
        internal bool IsDirectReference
        {
            get
            {
                return
                    this.TargetKind == RequestTargetKind.PrimitiveValue ||
                    this.TargetKind == RequestTargetKind.OpenPropertyValue ||
                    this.HasKeyValues;
            }
        }

        /// <summary>Returns the query for this segment, possibly null.</summary>
        internal IQueryable RequestQueryable
        {
            get
            {
                return this.RequestEnumerable as IQueryable;
            }
            
            set
            {
                this.RequestEnumerable = value;
            }
        }

#if DEBUG
        /// <summary>In DEBUG builds, ensures that invariants for the class hold.</summary>
        internal void AssertValid()
        {
            WebUtil.DebugEnumIsDefined(this.TargetKind);
            WebUtil.DebugEnumIsDefined(this.TargetSource);
            Debug.Assert(this.TargetKind != RequestTargetKind.Nothing, "targetKind != RequestTargetKind.Nothing");
            Debug.Assert(
                this.TargetContainer == null || this.TargetSource != RequestTargetSource.None,
                "'None' targets should not have a resource set.");
            Debug.Assert(
                this.TargetKind != RequestTargetKind.Resource || 
                this.TargetContainer != null || 
                this.TargetKind == RequestTargetKind.OpenProperty ||
                this.TargetSource == RequestTargetSource.ServiceOperation,
                "All resource targets (except for some service operations and open properties) should have a container.");
            Debug.Assert(
                this.TargetContainer == null || this.TargetContainer.ResourceType.IsAssignableFrom(this.TargetResourceType),
                "If targetContainer is assigned, it should be equal to (or assignable to) the segment's element type.");
            Debug.Assert(
                !String.IsNullOrEmpty(this.Identifier) || RequestTargetSource.None == this.TargetSource || RequestTargetKind.VoidServiceOperation == this.TargetKind,
                "identifier must not be empty or null except for none or void service operation");
        }
#endif
    }
}
