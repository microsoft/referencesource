//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//--------------------------------------------------------------------------
using System;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;

namespace System
{
    /// <summary>
    /// Helper so we can call some tuple methods recursively without knowing the underlying types.
    /// </summary>
    internal interface ITuple
    {
        string ToString(StringBuilder sb);
        int GetHashCode(IEqualityComparer comparer);
        int Size { get; }
    }

    /// <summary>
    ///     Provides static methods for creating tuple objects. 
    /// </summary>
    public static class Tuple
    {
        /// <summary>
        ///     Creates a new 1-tuple, or singleton.
        /// </summary>
        /// <typeparam name="T1">The type of the only component of the tuple.</typeparam>
        /// <param name="item1">The value of the only component of the tuple.</param>
        /// <returns>A tuple whose value is (item1).</returns>
        public static Tuple<T1> Create<T1>(T1 item1)
        {
            return new Tuple<T1>(item1);
        }

        /// <summary>
        ///     Creates a new 3-tuple, or pair.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <returns>An 2-tuple (pair) whose value is (item1, item2).</returns>
        public static Tuple<T1, T2> Create<T1, T2>(T1 item1, T2 item2)
        {
            return new Tuple<T1, T2>(item1, item2);
        }

        /// <summary>
        ///     Creates a new 3-tuple, or triple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <returns>An 3-tuple (triple) whose value is (item1, item2, item3).</returns>
        public static Tuple<T1, T2, T3> Create<T1, T2, T3>(T1 item1, T2 item2, T3 item3)
        {
            return new Tuple<T1, T2, T3>(item1, item2, item3);
        }

        /// <summary>
        ///     Creates a new 4-tuple, or quadruple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <returns>An 4-tuple (quadruple) whose value is (item1, item2, item3, item4).</returns>
        public static Tuple<T1, T2, T3, T4> Create<T1, T2, T3, T4>(T1 item1, T2 item2, T3 item3, T4 item4)
        {
            return new Tuple<T1, T2, T3, T4>(item1, item2, item3, item4);
        }

        /// <summary>
        ///     Creates a new 5-tuple, or quintuple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
        /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <returns>An 5-tuple (quintuple) whose value is (item1, item2, item3, item4, item5).</returns>
        public static Tuple<T1, T2, T3, T4, T5> Create<T1, T2, T3, T4, T5>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5)
        {
            return new Tuple<T1, T2, T3, T4, T5>(item1, item2, item3, item4, item5);
        }

        /// <summary>
        ///     Creates a new 6-tuple, or sextuple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
        /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
        /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        /// <returns>An 6-tuple (sextuple) whose value is (item1, item2, item3, item4, item5, item6).</returns>
        public static Tuple<T1, T2, T3, T4, T5, T6> Create<T1, T2, T3, T4, T5, T6>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6)
        {
            return new Tuple<T1, T2, T3, T4, T5, T6>(item1, item2, item3, item4, item5, item6);
        }

        /// <summary>
        ///     Creates a new 7-tuple, or septuple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
        /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
        /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
        /// <typeparam name="T7">The type of the seventh component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        /// <param name="item7">The value of the seventh component of the tuple.</param>
        /// <returns>An 7-tuple (septuple) whose value is (item1, item2, item3, item4, item5, item6, item7).</returns>
        public static Tuple<T1, T2, T3, T4, T5, T6, T7> Create<T1, T2, T3, T4, T5, T6, T7>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7)
        {
            return new Tuple<T1, T2, T3, T4, T5, T6, T7>(item1, item2, item3, item4, item5, item6, item7);
        }

        /// <summary>
        ///     Creates a new 8-tuple, or octuple.
        /// </summary>
        /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
        /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
        /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
        /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
        /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
        /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
        /// <typeparam name="T7">The type of the seventh component of the tuple.</typeparam>
        /// <typeparam name="T8">The type of the eighth component of the tuple.</typeparam>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        /// <param name="item7">The value of the seventh component of the tuple.</param>
        /// <param name="item8">The value of the eighth component of the tuple.</param>
        /// <returns>An 8-tuple (octuple) whose value is (item1, item2, item3, item4, item5, item6, item7, item8).</returns>
        public static Tuple<T1, T2, T3, T4, T5, T6, T7, Tuple<T8>> Create<T1, T2, T3, T4, T5, T6, T7, T8>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7, T8 item8)
        {
            return new Tuple<T1, T2, T3, T4, T5, T6, T7, Tuple<T8>>(item1, item2, item3, item4, item5, item6, item7, new Tuple<T8>(item8));
        }

        // From System.Web.Util.HashCodeCombiner
        internal static int CombineHashCodes(int h1, int h2)
        {
            return (((h1 << 5) + h1) ^ h2);
        }

        internal static int CombineHashCodes(int h1, int h2, int h3)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2), h3);
        }

        internal static int CombineHashCodes(int h1, int h2, int h3, int h4)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2), CombineHashCodes(h3, h4));
        }

        internal static int CombineHashCodes(int h1, int h2, int h3, int h4, int h5)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2, h3, h4), h5);
        }

        internal static int CombineHashCodes(int h1, int h2, int h3, int h4, int h5, int h6)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2, h3, h4), CombineHashCodes(h5, h6));
        }

        internal static int CombineHashCodes(int h1, int h2, int h3, int h4, int h5, int h6, int h7)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2, h3, h4), CombineHashCodes(h5, h6, h7));
        }

        internal static int CombineHashCodes(int h1, int h2, int h3, int h4, int h5, int h6, int h7, int h8)
        {
            return CombineHashCodes(CombineHashCodes(h1, h2, h3, h4), CombineHashCodes(h5, h6, h7, h8));
        }
    }

    /// <summary>
    ///     Represents a 1-tuple, or singleton. 
    /// </summary>
    /// <typeparam name="T1">The type of the tuple's only component.</typeparam>
    public class Tuple<T1> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {
        private readonly T1 m_Item1;

        /// <summary>
        ///     Gets the value of the tuple object's single component. 
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's single component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1}"/> class.
        /// </summary>
        /// <param name="item1">The value of the current tuple object's single component.</param>
        public Tuple(T1 item1)
        {
            m_Item1 = item1;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default);
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1> objTuple = other as Tuple<T1>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1> objTuple = other as Tuple<T1>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            return comparer.Compare(m_Item1, objTuple.m_Item1);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return comparer.GetHashCode(m_Item1);
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 1;
            }
        }
    }

    /// <summary>
    ///     Represents an 2-tuple, or pair.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    public class Tuple<T1, T2> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        public Tuple(T1 item1, T2 item2)
        {
            m_Item1 = item1;
            m_Item2 = item2;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2> objTuple = other as Tuple<T1, T2>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2> objTuple = other as Tuple<T1, T2>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            return comparer.Compare(m_Item2, objTuple.m_Item2);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 2;
            }
        }
    }

    /// <summary>
    ///     Represents an 3-tuple, or triple.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    public class Tuple<T1, T2, T3> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        public Tuple(T1 item1, T2 item2, T3 item3)
        {
            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3> objTuple = other as Tuple<T1, T2, T3>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3> objTuple = other as Tuple<T1, T2, T3>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            return comparer.Compare(m_Item3, objTuple.m_Item3);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 3;
            }
        }
    }

    /// <summary>
    ///     Represents an 4-tuple, or quadruple.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
    public class Tuple<T1, T2, T3, T4> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;
        private readonly T4 m_Item4;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fourth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fourth component.
        /// </value>
        public T4 Item4 { get { return m_Item4; } }
        
        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3,T4}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        public Tuple(T1 item1, T2 item2, T3 item3, T4 item4)
        {
            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
            m_Item4 = item4;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3, T4> objTuple = other as Tuple<T1, T2, T3, T4>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3) && comparer.Equals(m_Item4, objTuple.m_Item4);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3, T4> objTuple = other as Tuple<T1, T2, T3, T4>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            c = comparer.Compare(m_Item3, objTuple.m_Item3);

            if (c != 0) return c;

            return comparer.Compare(m_Item4, objTuple.m_Item4);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(", ");
            sb.Append(m_Item4);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 4;
            }
        }
    }

    /// <summary>
    ///     Represents an 5-tuple, or quintuple.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
    /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
    public class Tuple<T1, T2, T3, T4, T5> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;
        private readonly T4 m_Item4;
        private readonly T5 m_Item5;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fourth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fourth component.
        /// </value>
        public T4 Item4 { get { return m_Item4; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fifth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fifth component.
        /// </value>
        public T5 Item5 { get { return m_Item5; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3,T4,T5}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        public Tuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5)
        {
            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
            m_Item4 = item4;
            m_Item5 = item5;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3, T4, T5> objTuple = other as Tuple<T1, T2, T3, T4, T5>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3) && comparer.Equals(m_Item4, objTuple.m_Item4) && comparer.Equals(m_Item5, objTuple.m_Item5);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3, T4, T5> objTuple = other as Tuple<T1, T2, T3, T4, T5>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            c = comparer.Compare(m_Item3, objTuple.m_Item3);

            if (c != 0) return c;

            c = comparer.Compare(m_Item4, objTuple.m_Item4);

            if (c != 0) return c;

            return comparer.Compare(m_Item5, objTuple.m_Item5);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(", ");
            sb.Append(m_Item4);
            sb.Append(", ");
            sb.Append(m_Item5);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 5;
            }
        }
    }

    /// <summary>
    ///     Represents an 6-tuple, or sextuple.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
    /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
    /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
    public class Tuple<T1, T2, T3, T4, T5, T6> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;
        private readonly T4 m_Item4;
        private readonly T5 m_Item5;
        private readonly T6 m_Item6;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fourth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fourth component.
        /// </value>
        public T4 Item4 { get { return m_Item4; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fifth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fifth component.
        /// </value>
        public T5 Item5 { get { return m_Item5; } }

        /// <summary>
        ///     Gets the value of the current tuple object's sixth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's sixth component.
        /// </value>
        public T6 Item6 { get { return m_Item6; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3,T4,T5,T6}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        public Tuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6)
        {
            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
            m_Item4 = item4;
            m_Item5 = item5;
            m_Item6 = item6;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3, T4, T5, T6> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3) && comparer.Equals(m_Item4, objTuple.m_Item4) && comparer.Equals(m_Item5, objTuple.m_Item5) && comparer.Equals(m_Item6, objTuple.m_Item6);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3, T4, T5, T6> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            c = comparer.Compare(m_Item3, objTuple.m_Item3);

            if (c != 0) return c;

            c = comparer.Compare(m_Item4, objTuple.m_Item4);

            if (c != 0) return c;

            c = comparer.Compare(m_Item5, objTuple.m_Item5);

            if (c != 0) return c;

            return comparer.Compare(m_Item6, objTuple.m_Item6);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(", ");
            sb.Append(m_Item4);
            sb.Append(", ");
            sb.Append(m_Item5);
            sb.Append(", ");
            sb.Append(m_Item6);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 6;
            }
        }
    }

    /// <summary>
    ///     Represents an 7-tuple, or septuple.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
    /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
    /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
    /// <typeparam name="T7">The type of the seventh component of the tuple.</typeparam>
    public class Tuple<T1, T2, T3, T4, T5, T6, T7> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;
        private readonly T4 m_Item4;
        private readonly T5 m_Item5;
        private readonly T6 m_Item6;
        private readonly T7 m_Item7;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fourth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fourth component.
        /// </value>
        public T4 Item4 { get { return m_Item4; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fifth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fifth component.
        /// </value>
        public T5 Item5 { get { return m_Item5; } }

        /// <summary>
        ///     Gets the value of the current tuple object's sixth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's sixth component.
        /// </value>
        public T6 Item6 { get { return m_Item6; } }

        /// <summary>
        ///     Gets the value of the current tuple object's seventh component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's seventh component.
        /// </value>
        public T7 Item7 { get { return m_Item7; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3,T4,T5,T6,T7}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        /// <param name="item7">The value of the seventh component of the tuple.</param>
        public Tuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7)
        {
            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
            m_Item4 = item4;
            m_Item5 = item5;
            m_Item6 = item6;
            m_Item7 = item7;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3, T4, T5, T6, T7> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6, T7>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3) && comparer.Equals(m_Item4, objTuple.m_Item4) && comparer.Equals(m_Item5, objTuple.m_Item5) && comparer.Equals(m_Item6, objTuple.m_Item6) && comparer.Equals(m_Item7, objTuple.m_Item7);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3, T4, T5, T6, T7> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6, T7>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            c = comparer.Compare(m_Item3, objTuple.m_Item3);

            if (c != 0) return c;

            c = comparer.Compare(m_Item4, objTuple.m_Item4);

            if (c != 0) return c;

            c = comparer.Compare(m_Item5, objTuple.m_Item5);

            if (c != 0) return c;

            c = comparer.Compare(m_Item6, objTuple.m_Item6);

            if (c != 0) return c;

            return comparer.Compare(m_Item7, objTuple.m_Item7);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7));
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }

        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(", ");
            sb.Append(m_Item4);
            sb.Append(", ");
            sb.Append(m_Item5);
            sb.Append(", ");
            sb.Append(m_Item6);
            sb.Append(", ");
            sb.Append(m_Item7);
            sb.Append(")");
            return sb.ToString();
        }

        int ITuple.Size
        {
            get
            {
                return 7;
            }
        }
    }

    /// <summary>
    ///     Represents an n-tuple, where n is 8 or greater.
    /// </summary>
    /// <typeparam name="T1">The type of the first component of the tuple.</typeparam>
    /// <typeparam name="T2">The type of the second component of the tuple.</typeparam>
    /// <typeparam name="T3">The type of the third component of the tuple.</typeparam>
    /// <typeparam name="T4">The type of the fourth component of the tuple.</typeparam>
    /// <typeparam name="T5">The type of the fifth component of the tuple.</typeparam>
    /// <typeparam name="T6">The type of the sixth component of the tuple.</typeparam>
    /// <typeparam name="T7">The type of the seventh component of the tuple.</typeparam>
    /// <typeparam name="TRest">Any generic Tuple object that defines the types of the tuple's remaining components.</typeparam>
    public class Tuple<T1, T2, T3, T4, T5, T6, T7, TRest> : IStructuralEquatable, IStructuralComparable, IComparable, ITuple
    {

        private readonly T1 m_Item1;
        private readonly T2 m_Item2;
        private readonly T3 m_Item3;
        private readonly T4 m_Item4;
        private readonly T5 m_Item5;
        private readonly T6 m_Item6;
        private readonly T7 m_Item7;
        private readonly TRest m_Rest;

        /// <summary>
        ///     Gets the value of the current tuple object's first component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's first component.
        /// </value>
        public T1 Item1 { get { return m_Item1; } }

        /// <summary>
        ///     Gets the value of the current tuple object's second component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's second component.
        /// </value>
        public T2 Item2 { get { return m_Item2; } }

        /// <summary>
        ///     Gets the value of the current tuple object's third component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's third component.
        /// </value>
        public T3 Item3 { get { return m_Item3; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fourth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fourth component.
        /// </value>
        public T4 Item4 { get { return m_Item4; } }

        /// <summary>
        ///     Gets the value of the current tuple object's fifth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's fifth component.
        /// </value>
        public T5 Item5 { get { return m_Item5; } }

        /// <summary>
        ///     Gets the value of the current tuple object's sixth component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's sixth component.
        /// </value>
        public T6 Item6 { get { return m_Item6; } }

        /// <summary>
        ///     Gets the value of the current tuple object's seventh component.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's seventh component.
        /// </value>
        public T7 Item7 { get { return m_Item7; } }

        /// <summary>
        ///     Gets the current tuple object's remaining components.
        /// </summary>
        /// <value>
        ///     The value of the current tuple object's remaining components.
        /// </value>
        public TRest Rest { get { return m_Rest; } }

        /// <summary>
        ///     Initializes a new instance of the <see cref="Tuple{T1,T2,T3,T4,T5,T6,T7,TRest}"/> class.
        /// </summary>
        /// <param name="item1">The value of the first component of the tuple.</param>
        /// <param name="item2">The value of the second component of the tuple.</param>
        /// <param name="item3">The value of the third component of the tuple.</param>
        /// <param name="item4">The value of the fourth component of the tuple.</param>
        /// <param name="item5">The value of the fifth component of the tuple.</param>
        /// <param name="item6">The value of the sixth component of the tuple.</param>
        /// <param name="item7">The value of the seventh component of the tuple.</param>
        /// <param name="rest">Any generic Tuple object that contains the values of the tuple's remaining components.</param>
        /// <exception cref="ArgumentException">
        ///     rest is not a generic Tuple object.
        /// </exception>
        public Tuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7, TRest rest)
        {
            if (!(rest is ITuple))
            {
                throw new ArgumentException(Strings.ArgumentException_TupleLastArgumentNotATuple);
            }

            m_Item1 = item1;
            m_Item2 = item2;
            m_Item3 = item3;
            m_Item4 = item4;
            m_Item5 = item5;
            m_Item6 = item6;
            m_Item7 = item7;
            m_Rest = rest;
        }

        /// <summary>
        ///     Returns a value that indicates whether the current tuple object is equal to a specified object.
        /// </summary>
        /// <param name="obj">The object to compare with this instance.</param>
        /// <returns>true if the current instance is equal to the specified object; otherwise, false.</returns>
        public override Boolean Equals(Object obj)
        {
            return ((IStructuralEquatable)this).Equals(obj, EqualityComparer<Object>.Default); ;
        }

        Boolean IStructuralEquatable.Equals(Object other, IEqualityComparer comparer)
        {
            if (other == null) return false;

            Tuple<T1, T2, T3, T4, T5, T6, T7, TRest> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6, T7, TRest>;

            if (objTuple == null)
            {
                return false;
            }

            return comparer.Equals(m_Item1, objTuple.m_Item1) && comparer.Equals(m_Item2, objTuple.m_Item2) && comparer.Equals(m_Item3, objTuple.m_Item3) && comparer.Equals(m_Item4, objTuple.m_Item4) && comparer.Equals(m_Item5, objTuple.m_Item5) && comparer.Equals(m_Item6, objTuple.m_Item6) && comparer.Equals(m_Item7, objTuple.m_Item7) && comparer.Equals(m_Rest, objTuple.m_Rest);
        }

        Int32 IComparable.CompareTo(Object obj)
        {
            return ((IStructuralComparable)this).CompareTo(obj, Comparer<Object>.Default);
        }

        Int32 IStructuralComparable.CompareTo(Object other, IComparer comparer)
        {
            if (other == null) return 1;

            Tuple<T1, T2, T3, T4, T5, T6, T7, TRest> objTuple = other as Tuple<T1, T2, T3, T4, T5, T6, T7, TRest>;

            if (objTuple == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, Strings.ArgumentException_TupleIncorrectType, GetType().ToString()), "other");
            }

            int c = 0;

            c = comparer.Compare(m_Item1, objTuple.m_Item1);

            if (c != 0) return c;

            c = comparer.Compare(m_Item2, objTuple.m_Item2);

            if (c != 0) return c;

            c = comparer.Compare(m_Item3, objTuple.m_Item3);

            if (c != 0) return c;

            c = comparer.Compare(m_Item4, objTuple.m_Item4);

            if (c != 0) return c;

            c = comparer.Compare(m_Item5, objTuple.m_Item5);

            if (c != 0) return c;

            c = comparer.Compare(m_Item6, objTuple.m_Item6);

            if (c != 0) return c;

            c = comparer.Compare(m_Item7, objTuple.m_Item7);

            if (c != 0) return c;

            return comparer.Compare(m_Rest, objTuple.m_Rest);
        }

        /// <summary>
        ///     Calculates the hash code for the current tuple object.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return ((IStructuralEquatable)this).GetHashCode(EqualityComparer<Object>.Default);
        }

        Int32 IStructuralEquatable.GetHashCode(IEqualityComparer comparer)
        {
            // We want to have a limited hash in this case.  We'll use the last 8 elements of the tuple
            ITuple t = (ITuple)m_Rest;
            if (t.Size >= 8) { return t.GetHashCode(comparer); }

            // In this case, the rest memeber has less than 8 elements so we need to combine some our elements with the elements in rest
            int k = 8 - t.Size;
            switch (k)
            {
                case 1:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 2:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 3:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 4:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 5:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 6:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
                case 7:
                    return Tuple.CombineHashCodes(comparer.GetHashCode(m_Item1), comparer.GetHashCode(m_Item2), comparer.GetHashCode(m_Item3), comparer.GetHashCode(m_Item4), comparer.GetHashCode(m_Item5), comparer.GetHashCode(m_Item6), comparer.GetHashCode(m_Item7), t.GetHashCode(comparer));
            }
            //Contract.Assert(false, "Missed all cases for computing Tuple hash code");
            return -1;
        }

        Int32 ITuple.GetHashCode(IEqualityComparer comparer)
        {
            return ((IStructuralEquatable)this).GetHashCode(comparer);
        }
        
        /// <summary>
        ///     Returns a string that represents the value of this tuple instance.
        /// </summary>
        /// <returns>The string representation of this tuple object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("(");
            return ((ITuple)this).ToString(sb);
        }

        string ITuple.ToString(StringBuilder sb)
        {
            sb.Append(m_Item1);
            sb.Append(", ");
            sb.Append(m_Item2);
            sb.Append(", ");
            sb.Append(m_Item3);
            sb.Append(", ");
            sb.Append(m_Item4);
            sb.Append(", ");
            sb.Append(m_Item5);
            sb.Append(", ");
            sb.Append(m_Item6);
            sb.Append(", ");
            sb.Append(m_Item7);
            sb.Append(", ");
            return ((ITuple)m_Rest).ToString(sb);
        }

        int ITuple.Size
        {
            get
            {
                return 7 + ((ITuple)m_Rest).Size;
            }
        }
    }
}

