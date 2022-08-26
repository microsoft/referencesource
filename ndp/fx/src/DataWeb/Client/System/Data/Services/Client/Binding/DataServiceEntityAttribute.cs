//---------------------------------------------------------------------
// <copyright file="DataServiceEntityAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      EntitySetAttribute class
// </summary>
//
//---------------------------------------------------------------------
namespace System.Data.Services.Common
{
    /// <summary>
    /// This attribute allows users to specify an entity set name with a client type.
    /// </summary>
    /// <remarks>
    /// This attribute is generated only when there is one entity set associated with the type.
    /// When there are more than one entity set associated with the type, then the entity set
    /// name can be passed in through the EntitySetNameResolver event.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public sealed class EntitySetAttribute : System.Attribute
    {
        /// <summary>
        /// The entity set name.
        /// </summary>
        private readonly string entitySet;

        /// <summary>
        /// Construct a EntitySetAttribute
        /// </summary>
        /// <param name="entitySet">The entity set name.</param>
        public EntitySetAttribute(string entitySet)
        {
            this.entitySet = entitySet;
        }

        /// <summary>
        /// The entity set name.
        /// </summary>
        public string EntitySet
        {
            get
            {
                return this.entitySet;
            }
        }
    }
}
