//---------------------------------------------------------------------
// <copyright file="Int32EqualityComparer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Non-default-based implementation of IEqualityComparer&lt;int&gt;.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Collections.Generic;

    /// <summary>This class implements IEqualityComparer for System.In32.</summary>
    /// <remarks>
    /// Using this class rather than EqualityComparer&lt;T&gt;.Default 
    /// saves from JIT'ing it in each AppDomain.
    /// </remarks>
    internal class Int32EqualityComparer : IEqualityComparer<int>
    {
        /// <summary>Empty constructor.</summary>
        internal Int32EqualityComparer()
        {
        }

        /// <summary>Checks whether two numbers are equal.</summary>
        /// <param name='x'>First number.</param><param name='y'>Second number.</param>
        /// <returns>true if x equals y; false otherwise.</returns>
        public bool Equals(int x, int y)
        {
            return x == y;
        }

        /// <summary>Gets a hash code for the specified number.</summary>
        /// <param name='obj'>Value.</param>
        /// <returns>The hash code for the specified value.</returns>
        public int GetHashCode(int obj)
        {
            return obj;
        }
    }
}
