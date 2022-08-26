//---------------------------------------------------------------------
// <copyright file="HasStreamAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      CLR attribute to annotate that an Entity Type has a default stream property.
// </summary>
//
// @owner  jli
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System;

    /// <summary>
    /// Use this attribute to annotate that an Entity Type has a default stream property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = false)]
    public sealed class HasStreamAttribute : Attribute
    {
    }
}
