//---------------------------------------------------------------------
// <copyright file="AtomContentProperty.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that represents an ATOM entry property, with
// details on how it was parsed and interpreted.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Linq;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to represent a property for an ATOM entry.
    /// </summary>
    [DebuggerDisplay("AtomContentProperty {TypeName} {Name}")]
    internal class AtomContentProperty
    {
        /// <summary>Whether the property was marked as null.</summary>
        /// <remarks>This value will be assigned during XML parsing.</remarks>
        public bool IsNull 
        { 
            get; 
            set; 
        }

        /// <summary>Property name.</summary>
        /// <remarks>
        /// This value will be assigned during XML parsing.
        /// May be null if reading a top-level complex instance.
        /// </remarks>
        public string Name 
        { 
            get; 
            set; 
        }

        /// <summary>Type name for the property as found during serialization.</summary>
        /// <remarks>This value will be assigned during XML parsing. May be null.</remarks>
        public string TypeName 
        { 
            get; 
            set; 
        }

        /// <summary>Text value of property, as found during serialization.</summary>
        public string Text 
        { 
            get; 
            set; 
        }

        /// <summary>Sub-properties for this property; typically found for complex types.</summary>
        /// <remarks>
        /// This value will be assigned during XML parsing, but may be modified
        /// when Entity Property Mapping information is applied.
        /// 
        /// Note that for expanded entities, sub-properties will be found
        /// in the <see cref="Entry"/> value.
        /// </remarks>
        public List<AtomContentProperty> Properties 
        { 
            get; 
            set; 
        }

        /// <summary>Value for nested collection of entries found during parsing.</summary>
        public AtomFeed Feed 
        { 
            get; 
            set; 
        }

        /// <summary>Value for a nested ATOM entry as found during parsing.</summary>
        public AtomEntry Entry 
        { 
            get; 
            set; 
        }

        /// <summary>Materialized value for this property; null until materialization.</summary>
        /// <remarks>
        /// This property will be null after parsing, and is assigned a value
        /// during materialization (when type information is available/resolved).
        /// 
        /// This property will be set for primitive types only in the current
        /// materialization pipeline.
        /// 
        /// 


        public object MaterializedValue 
        { 
            get; 
            set; 
        }

        /// <summary>Ensures that the <see cref="Properties"/> property is not null.</summary>
        public void EnsureProperties()
        {
            if (this.Properties == null)
            {
                this.Properties = new List<AtomContentProperty>();
            }
        }
    }
}
