//-----------------------------------------------------------------------------
//
// <copyright file="PackageRelationshipSelectorType.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  PackageRelationshipSelectorType enum - lists all the possible types based on which
//  PackageRelationships can be selected. Currently we define just two of these -
//  1. Id
//  2. Type
//
// History:
//  07/27/2005: SarjanaS: Initial creation.
//
//-----------------------------------------------------------------------------

namespace System.IO.Packaging
{
    /// <summary>
    /// Enum to represent the different selector types for PackageRelationshipSelector  
    /// </summary>
    public enum PackageRelationshipSelectorType : int
    {
            /// <summary>
            /// Id
            /// </summary>
            Id = 0,

            /// <summary>
            /// Type 
            /// </summary>
            Type = 1            
    }
}
