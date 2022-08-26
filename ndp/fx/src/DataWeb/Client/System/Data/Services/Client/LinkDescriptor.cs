//---------------------------------------------------------------------
// <copyright file="LinkDescriptor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// represents the association between two entities
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Diagnostics;

    /// <summary>
    /// represents the association between two entities
    /// </summary>
    [DebuggerDisplay("State = {state}")]
    public sealed class LinkDescriptor : Descriptor
    {
        #region Fields

        /// <summary>equivalence comparer</summary>
        internal static readonly System.Collections.Generic.IEqualityComparer<LinkDescriptor> EquivalenceComparer = new Equivalent();

        /// <summary>source entity</summary>
        private object source;

        /// <summary>name of property on source entity that references the target entity</summary>
        private string sourceProperty;

        /// <summary>target entity</summary>
        private object target;

        #endregion
        
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="source">Source entity</param>
        /// <param name="sourceProperty">Navigation property on the source entity</param>
        /// <param name="target">Target entity</param>
        internal LinkDescriptor(object source, string sourceProperty, object target)
            : this(source, sourceProperty, target,  EntityStates.Unchanged)
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="source">Source entity</param>
        /// <param name="sourceProperty">Navigation property on the source entity</param>
        /// <param name="target">Target entity</param>
        /// <param name="state">The link state</param>
        internal LinkDescriptor(object source, string sourceProperty, object target, EntityStates state)
            : base(state)
        {
            this.source = source;
            this.sourceProperty = sourceProperty;
            this.target = target;
        }

#region Public Properties

        /// <summary>target entity</summary>
        public object Target
        {
            get { return this.target; }
        }

        /// <summary>source entity</summary>
        public object Source
        {
            get { return this.source; }
        }

        /// <summary>name of property on source entity that references the target entity</summary>
        public string SourceProperty
        {
            get { return this.sourceProperty; }
        }

#endregion
        
        /// <summary>this is a link</summary>
        internal override bool IsResource
        {
            get { return false; }
        }

        /// <summary>
        /// If the current instance of link descriptor is equivalent to the parameters supplied
        /// </summary>
        /// <param name="src">The source entity</param>
        /// <param name="srcPropName">The source property name</param>
        /// <param name="targ">The target entity</param>
        /// <returns>true if equivalent</returns>
        internal bool IsEquivalent(object src, string srcPropName, object targ)
        {
            return (this.source == src &&
                this.target == targ &&
                this.sourceProperty == srcPropName);
        }

        /// <summary>equivalence comparer</summary>
        private sealed class Equivalent : System.Collections.Generic.IEqualityComparer<LinkDescriptor>
        {
            /// <summary>are two LinkDescriptors equivalent, ignore state</summary>
            /// <param name="x">link descriptor x</param>
            /// <param name="y">link descriptor y</param>
            /// <returns>true if equivalent</returns>
            public bool Equals(LinkDescriptor x, LinkDescriptor y)
            {
                return (null != x) && (null != y) && x.IsEquivalent(y.source, y.sourceProperty, y.target);
            }

            /// <summary>compute hashcode for LinkDescriptor</summary>
            /// <param name="obj">link descriptor</param>
            /// <returns>hashcode</returns>
            public int GetHashCode(LinkDescriptor obj)
            {
                return (null != obj) ? (obj.Source.GetHashCode() ^ ((null != obj.Target) ? obj.Target.GetHashCode() : 0) ^ obj.SourceProperty.GetHashCode()) : 0;
            }
        }
    }
}
