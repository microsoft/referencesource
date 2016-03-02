//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//--------------------------------------------------------------------------
using System;
using System.Text;
using System.Collections;
using System.Collections.Generic;

namespace System.Collections
{
    /// <summary>
    ///     Supports the structural comparison of collection objects.
    /// </summary>
    public interface IStructuralComparable
    {
        /// <summary>
        ///     Determines whether the current collection object precedes, occurs in the same position as, or follows another object in the sort order.
        /// </summary>
        /// <param name="other">The object to compare with the current instance.</param>
        /// <param name="comparer">An object that compares members of the current collection object with the corresponding members of other.</param>
        /// <returns>An integer that indicates the relationship of the current collection object to other.</returns>
        /// <exception cref="ArgumentException">
        ///     This instance and other are not the same type.
        /// </exception>
        int CompareTo(object other, IComparer comparer);
    }
}