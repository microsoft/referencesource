//---------------------------------------------------------------------
// <copyright file="DataServiceProtocolVersion.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Enum to represent the choice of protocol version that the service may decide to support.
// </summary>
//
// @owner pawelka
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    /// <summary>
    /// This enum represents the choice of protocol version that the service may decide to support.
    /// </summary>
    public enum DataServiceProtocolVersion
    {
        /// <summary>Version 1 </summary>
        V1, 

        /// <summary>Version 2</summary>
        V2
    }
}