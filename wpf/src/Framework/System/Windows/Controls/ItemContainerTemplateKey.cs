//---------------------------------------------------------------------------
// <copyright file="ItemContainerTemplate.cs" company="Microsoft Corporation">
//     Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------------

namespace System.Windows.Controls
{
    /// <summary> Resource key for a ItemContainerTemplate</summary>
    public class ItemContainerTemplateKey : TemplateKey
    {
        /// <summary> Constructor</summary>
        /// <remarks>
        /// When constructed without dataType (e.g. in XAML),
        /// the DataType must be specified as a property.
        /// </remarks>
        public ItemContainerTemplateKey()
            : base(TemplateType.TableTemplate) // This should be TemplateType.ItemContainerTemplate
        {
        }

        /// <summary> Constructor</summary>
        public ItemContainerTemplateKey(object dataType)
            : base(TemplateType.TableTemplate, dataType) // This should be TemplateType.ItemContainerTemplate
        {
        }
    }
}
