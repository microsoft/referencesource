//---------------------------------------------------------------------
// <copyright file="ReadingWritingEntityEventArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Event args for the event fired during reading or writing of
// an entity serialization/deserialization
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Diagnostics;
    using System.Xml.Linq;

    /// <summary>
    /// Event args for the event fired during reading or writing of
    /// an entity serialization/deserialization
    /// </summary>
    public sealed class ReadingWritingEntityEventArgs : EventArgs
    {
        /// <summary>The entity being (de)serialized</summary>
        private object entity;

        /// <summary>The ATOM entry data to/from the network</summary>
        private XElement data;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="entity">The entity being (de)serialized</param>
        /// <param name="data">The ATOM entry data to/from the network</param>
        internal ReadingWritingEntityEventArgs(object entity, XElement data)
        {
            this.entity = entity;
            this.data = data;
        }

        /// <summary>The entity being (de)serialized</summary>
        public object Entity
        {
            get { return this.entity; }
        }

        /// <summary>The ATOM entry data to/from the network</summary>
        public XElement Data
        {
            [DebuggerStepThrough]
            get { return this.data; }
        }
    }
}
