//---------------------------------------------------------------------
// <copyright file="IExpandedResult.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface that supports getting expanded values
//      for associated $expand segments.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services
{
    /// <summary>
    /// This interface declares the methods required to support enumerators for results and for
    /// associated segments on a WCF Data Service $expand query option.
    /// </summary>
    public interface IExpandedResult
    {
        /// <summary>The element with expanded properties.</summary>
        object ExpandedElement
        { 
            get;
        }

        /// <summary>Gets the value for named property for the result.</summary>
        /// <param name="name">Name of property for which to get enumerable results.</param>
        /// <returns>The value for the named property of the result.</returns>
        /// <remarks>
        /// If the element returned in turn has properties which are expanded out-of-band
        /// of the object model, then the result will also be of type <see cref="IExpandedResult"/>,
        /// and the value will be available through <see cref="ExpandedElement"/>.
        /// </remarks>
        object GetExpandedPropertyValue(string name);
    }
}
