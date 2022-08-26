//---------------------------------------------------------------------
// <copyright file="IExceptionWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      An interface for objects that can write exceptions.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    /// <summary>Interface for objects than can write exception descriptions.</summary>
    internal interface IExceptionWriter
    {
        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        void WriteException(HandleExceptionArgs args);
    }
}