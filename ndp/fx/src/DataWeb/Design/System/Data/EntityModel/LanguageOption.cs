//---------------------------------------------------------------------
// <copyright file="LanguageOption.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       jeffreed
// @backupOwner dsimons
//---------------------------------------------------------------------

namespace System.Data.Services.Design
{
    /// <summary>
    /// Generate code targeting a specific .Net Framework language.
    /// </summary>
    public enum LanguageOption
    {
        /// <summary>Generate code for C# language.</summary>
        GenerateCSharpCode = 0,

        /// <summary>Generate code for Visual Basic language.</summary>
        GenerateVBCode = 1,
    }    

    /// <summary>
    /// Generate code targeting a specific WCF Data Services version.
    /// </summary>
    public enum DataServiceCodeVersion
    {
        /// <summary>Generate code targeting WCF Data Services v1.0.</summary>
        /// <remarks>First version of WCF Data Services.</remarks>
        V1 = 0,

        /// <summary>Generate code targeting WCF Data Services v2.0</summary>
        /// <remarks>Second version of WCF Data Services.</remarks>
        V2 = 1,
    }
}
