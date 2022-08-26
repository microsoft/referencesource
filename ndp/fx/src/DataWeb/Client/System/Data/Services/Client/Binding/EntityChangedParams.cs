//---------------------------------------------------------------------
// <copyright file="EntityChangedParams.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   EntityChangedParams class
// </summary>
//
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>Encapsulates the arguments of EntityChanged delegate</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704", Justification = "Name gets too long with Parameters")]
    public sealed class EntityChangedParams
    {
        #region Fields
        
        /// <summary>Context associated with the BindingObserver.</summary>
        private readonly DataServiceContext context;
        
        /// <summary>The entity object that has changed.</summary>
        private readonly object entity;
        
        /// <summary>The property of the entity that has changed.</summary>
        private readonly string propertyName;
        
        /// <summary>The current value of the target property.</summary>
        private readonly object propertyValue;

        /// <summary>Entity set to which the entity object belongs</summary>
        private readonly string sourceEntitySet;
        
        /// <summary>Entity set to which the target propertyValue entity belongs</summary>
        private readonly string targetEntitySet;

        #endregion

        #region Constructor
        
        /// <summary>
        /// Construct an EntityChangedParams object.
        /// </summary>
        /// <param name="context">Context to which the entity and propertyValue belong.</param>
        /// <param name="entity">The entity object that has changed.</param>
        /// <param name="propertyName">The property of the target entity object that has changed.</param>
        /// <param name="propertyValue">The current value of the entity property.</param>
        /// <param name="sourceEntitySet">Entity set to which the entity object belongs</param>
        /// <param name="targetEntitySet">Entity set to which the target propertyValue entity belongs</param>
        internal EntityChangedParams(
            DataServiceContext context,
            object entity,
            string propertyName,
            object propertyValue,
            string sourceEntitySet,
            string targetEntitySet)
        {
            this.context = context;
            this.entity = entity;
            this.propertyName = propertyName;
            this.propertyValue = propertyValue;
            this.sourceEntitySet = sourceEntitySet;
            this.targetEntitySet = targetEntitySet;
        }
        
        #endregion

        #region Properties

        /// <summary>Context associated with the BindingObserver.</summary>
        public DataServiceContext Context
        {
            get { return this.context; }
        }

        /// <summary>The entity object that has changed.</summary>
        public object Entity
        {
            get { return this.entity; }
        }

        /// <summary>The property of the target entity object that has changed.</summary>
        public string PropertyName
        {
            get { return this.propertyName; }
        }

        /// <summary>The current value of the entity property.</summary>
        public object PropertyValue
        {
            get { return this.propertyValue; }
        }

        /// <summary>Entity set to which the entity object belongs</summary>
        public string SourceEntitySet
        {
            get { return this.sourceEntitySet; }
        }

        /// <summary>Entity set to which the target propertyValue entity belongs</summary>
        public string TargetEntitySet
        {
            get { return this.targetEntitySet; }
        }
        
        #endregion
    }
}
