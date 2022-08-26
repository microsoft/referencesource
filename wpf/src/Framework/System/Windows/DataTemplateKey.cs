//---------------------------------------------------------------------------
//
// <copyright file="DataTemplateKey.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Resource key for a DataTemplate
//
//---------------------------------------------------------------------------

using System;
using System.Reflection;

namespace System.Windows
{
    /// <summary> Resource key for a DataTemplate</summary>
    public class DataTemplateKey : TemplateKey
    {
        /// <summary> Constructor</summary>
        /// <remarks>
        /// When constructed without dataType (e.g. in XAML),
        /// the DataType must be specified as a property.
        /// </remarks>
        public DataTemplateKey()
            : base(TemplateType.DataTemplate)
        {
        }

        /// <summary> Constructor</summary>
        public DataTemplateKey(object dataType)
            : base(TemplateType.DataTemplate, dataType)
        {
        }
    }
}

