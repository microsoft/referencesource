//---------------------------------------------------------------------
// <copyright file="ReferenceEqualityComparer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can compare objects for reference equality.
// </summary>
//---------------------------------------------------------------------

//// #define NON_GENERIC_AVAILABLE

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq.Expressions;
    using System.Text;
    using System.Collections;

    #endregion Namespaces.

    /// <summary>Equality comparer implementation that uses reference equality.</summary>
    internal class ReferenceEqualityComparer : IEqualityComparer
    {
        #region Private fields.

#if NON_GENERIC_AVAILABLE
        /// <summary>Singleton instance (non-generic, as opposed to the one in ReferenceEqualityComparer&lt;T&gt;.</summary>
        private static ReferenceEqualityComparer nonGenericInstance;
#endif

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ReferenceEqualityComparer"/> instance.</summary>
        protected ReferenceEqualityComparer()
        {
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>Determines whether two objects are the same.</summary>
        /// <param name="x">First object to compare.</param>
        /// <param name="y">Second object to compare.</param>
        /// <returns>true if both are the same; false otherwise.</returns>
        bool IEqualityComparer.Equals(object x, object y)
        {
            return object.ReferenceEquals(x, y);
        }

        /// <summary>Serves as hashing function for collections.</summary>
        /// <param name="obj">Object to hash.</param>
        /// <returns>
        /// Hash code for the object; shouldn't change through the lifetime 
        /// of <paramref name="obj"/>.
        /// </returns>
        int IEqualityComparer.GetHashCode(object obj)
        {
            if (obj == null)
            {
                return 0;
            }

            return obj.GetHashCode();
        }

#if NON_GENERIC_AVAILABLE
        /// <summary>Singleton instance (non-generic, as opposed to the one in ReferenceEqualityComparer&lt;T&gt;.</summary>
        internal ReferenceEqualityComparer NonGenericInstance
        {
            get
            {
                if (nonGenericInstance == null)
                {
                    ReferenceEqualityComparer comparer = new ReferenceEqualityComparer();
                    System.Threading.Interlocked.CompareExchange(ref nonGenericInstance, comparer, null);
                }

                return nonGenericInstance;
            }
        }
#endif

        #endregion Properties.
    }

    /// <summary>
    /// Use this class to compare objects by reference in collections such as 
    /// dictionary or hashsets.
    /// </summary>
    /// <typeparam name="T">Type of objects to compare.</typeparam>
    /// <remarks>
    /// Typically accesses statically as eg 
    /// ReferenceEqualityComparer&lt;Expression&gt;.Instance.
    /// </remarks>
    internal sealed class ReferenceEqualityComparer<T> : ReferenceEqualityComparer, IEqualityComparer<T>
    {
        #region Private fields.

        /// <summary>Single instance per 'T' for comparison.</summary>
        private static ReferenceEqualityComparer<T> instance;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ReferenceEqualityComparer"/> instance.</summary>
        private ReferenceEqualityComparer() : base()
        {
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>Returns a singleton instance for this comparer type.</summary>
        internal static ReferenceEqualityComparer<T> Instance
        {
            get
            {
                if (instance == null)
                {
                    Debug.Assert(!typeof(T).IsValueType, "!typeof(T).IsValueType -- can't use reference equality in a meaningful way with value types");
                    ReferenceEqualityComparer<T> newInstance = new ReferenceEqualityComparer<T>();
                    System.Threading.Interlocked.CompareExchange(ref instance, newInstance, null);
                }

                return instance;
            }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Determines whether two objects are the same.</summary>
        /// <param name="x">First object to compare.</param>
        /// <param name="y">Second object to compare.</param>
        /// <returns>true if both are the same; false otherwise.</returns>
        public bool Equals(T x, T y)
        {
            return object.ReferenceEquals(x, y);
        }

        /// <summary>Serves as hashing function for collections.</summary>
        /// <param name="obj">Object to hash.</param>
        /// <returns>
        /// Hash code for the object; shouldn't change through the lifetime 
        /// of <paramref name="obj"/>.
        /// </returns>
        public int GetHashCode(T obj)
        {
            if (obj == null)
            {
                return 0;
            }

            return obj.GetHashCode();
        }

        #endregion Methods.
    }
}
