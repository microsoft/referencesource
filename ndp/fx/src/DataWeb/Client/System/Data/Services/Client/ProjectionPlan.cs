//---------------------------------------------------------------------
// <copyright file="ProjectionPlan.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that encapsulates how to materialize an ATOM
// entry using a projection.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Diagnostics;

    #endregion Namespaces.

    /// <summary>Use this class to store a materialization plan used with projections.</summary>
    internal class ProjectionPlan
    {
#if DEBUG
        /// <summary>Source projection for this plan.</summary>
        internal System.Linq.Expressions.Expression SourceProjection
        {
            get;
            set;
        }

        /// <summary>Target projection for this plan.</summary>
        internal System.Linq.Expressions.Expression TargetProjection
        {
            get;
            set;
        }
#endif

        /// <summary>Last segment type for query.</summary>
        /// <remarks>This typically matches the expected element type at runtime.</remarks>
        internal Type LastSegmentType
        {
            get;
            set;
        }

        /// <summary>Provides a method to materialize a payload.</summary>
        internal Func<object, object, Type, object> Plan 
        { 
            get;
            set;
        }

        /// <summary>Expected type to project.</summary>
        internal Type ProjectedType
        {
            get;
            set;
        }

#if DEBUG
        /// <summary>Returns a string representation for this object.</summary>
        /// <returns>A string representation for this object, suitable for debugging.</returns>
        public override string ToString()
        {
            return "Plan - projection: " + this.SourceProjection + "\r\nBecomes: " + this.TargetProjection;
        }
#endif

        /// <summary>Runs this plan.</summary>
        /// <param name="materializer">Materializer under which materialization should happen.</param>
        /// <param name="entry">Root entry to materialize.</param>
        /// <param name="expectedType">Expected type for the <paramref name="entry"/>.</param>
        /// <returns>The materialized object.</returns>
        internal object Run(AtomMaterializer materializer, AtomEntry entry, Type expectedType)
        {
            Debug.Assert(materializer != null, "materializer != null");
            Debug.Assert(entry != null, "entry != null");

            return this.Plan(materializer, entry, expectedType);
        }
    }
}
