//---------------------------------------------------------------------
// <copyright file="ArraySet.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// a set, collection of unordered, distinct objects, implemented as an array
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;

    /// <summary>a set, collection of unordered, distinct objects, implemented as an array</summary>
    /// <typeparam name="T">element type</typeparam>
    [DebuggerDisplay("Count = {count}")]
    internal struct ArraySet<T> : IEnumerable<T> where T : class
    {
        /// <summary>item array of T</summary>
        private T[] items;

        /// <summary>count of elements in the items array</summary>
        private int count;

        /// <summary>number of Add and RemoveAt operations</summary>
        private int version;

        /// <summary>
        /// array set with an intial capacity
        /// </summary>
        /// <param name="capacity">initial capacity</param>
        public ArraySet(int capacity)
        {
            this.items = new T[capacity];
            this.count = 0;
            this.version = 0;
        }

        /// <summary>count of elements in the set</summary>
        public int Count
        {
            get { return this.count; }
        }

        /// <summary>get an item from an index in the set</summary>
        /// <param name="index">index to access</param>
        public T this[int index]
        {
            get
            {
                Debug.Assert(index < this.count);
                return this.items[index];
            }
        }

        /// <summary>add new element to the set</summary>
        /// <param name="item">element to add</param>
        /// <param name="equalityComparer">equality comparison function to avoid duplicates</param>
        /// <returns>true if actually added, false if a duplicate was discovered</returns>
        public bool Add(T item, Func<T, T, bool> equalityComparer)
        {
            if ((null != equalityComparer) && this.Contains(item, equalityComparer))
            {
                return false;
            }

            int index = this.count++;
            if ((null == this.items) || (index == this.items.Length))
            {   // grow array in size, with minimum size being 32
                Array.Resize<T>(ref this.items, Math.Min(Math.Max(index, 16), Int32.MaxValue / 2) * 2);
            }

            this.items[index] = item;
            unchecked
            {
                this.version++;
            }

            return true;
        }

        /// <summary>is the element contained within the set</summary>
        /// <param name="item">item to find</param>
        /// <param name="equalityComparer">comparer</param>
        /// <returns>true if the element is contained</returns>
        public bool Contains(T item, Func<T, T, bool> equalityComparer)
        {
            return (0 <= this.IndexOf(item, equalityComparer));
        }

        /// <summary>
        /// enumerator
        /// </summary>
        /// <returns>enumerator</returns>
        public IEnumerator<T> GetEnumerator()
        {
            for (int i = 0; i < this.count; ++i)
            {
                yield return this.items[i];
            }
        }

        /// <summary>
        /// enumerator
        /// </summary>
        /// <returns>enumerator</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
        
        /// <summary>Find the current index of element within the set</summary>
        /// <param name="item">item to find</param>
        /// <param name="comparer">comparision function</param>
        /// <returns>index of the item else (-1)</returns>
        public int IndexOf(T item, Func<T, T, bool> comparer)
        {
            return this.IndexOf(item, IdentitySelect, comparer);
        }

        /// <summary>Find the current index of element within the set</summary>
        /// <typeparam name="K">selected type</typeparam>
        /// <param name="item">item to find</param>
        /// <param name="select">selector for item to compare</param>
        /// <param name="comparer">item to compare</param>
        /// <returns>index of the item else (-1)</returns>
        public int IndexOf<K>(K item, Func<T, K> select, Func<K, K, bool> comparer)
        {
            T[] array = this.items;
            if (null != array)
            {
                int length = this.count;
                for (int i = 0; i < length; ++i)
                {
                    if (comparer(item, select(array[i])))
                    {
                        return i;
                    }
                }
            }

            return -1;
        }

        /// <summary>Remove the matched item from within the set</summary>
        /// <param name="item">item to find within the set</param>
        /// <param name="equalityComparer">comparer to find item to remove</param>
        /// <returns>the item that was actually contained else its default</returns>
        public T Remove(T item, Func<T, T, bool> equalityComparer)
        {
            int index = this.IndexOf(item, equalityComparer);
            if (0 <= index)
            {
                item = this.items[index];
                this.RemoveAt(index);
                return item;
            }

            return default(T);
        }

        /// <summary>Remove an item at a specific index from within the set</summary>
        /// <param name="index">index of item to remove within the set</param>
        public void RemoveAt(int index)
        {
            Debug.Assert(unchecked((uint)index < (uint)this.count), "index out of range");
            T[] array = this.items;
            int lastIndex = --this.count;
            array[index] = array[lastIndex];
            array[lastIndex] = default(T);

            if ((0 == lastIndex) && (256 <= array.Length))
            {
                this.items = null;
            }
            else if ((256 < array.Length) && (lastIndex < array.Length / 4))
            {   // shrink to half size when count is a quarter
                Array.Resize(ref this.items, array.Length / 2);
            }

            unchecked
            {
                this.version++;
            }
        }

        /// <summary>Sort array based on selected value out of item being stored</summary>
        /// <typeparam name="K">selected type</typeparam>
        /// <param name="selector">selector</param>
        /// <param name="comparer">comparer</param>
        public void Sort<K>(Func<T, K> selector, Func<K, K, int> comparer)
        {
            if (null != this.items)
            {
                SelectorComparer<K> scomp;
                scomp.Selector = selector;
                scomp.Comparer = comparer;
                Array.Sort<T>(this.items, 0, this.count, scomp);
            }
        }

        /// <summary>Sets the capacity to the actual number of elements in the ArraySet.</summary>
        public void TrimToSize()
        {
            Array.Resize(ref this.items, this.count);
        }

        /// <summary>identity selector, returns self</summary>
        /// <param name="arg">input</param>
        /// <returns>output</returns>
        private static T IdentitySelect(T arg)
        {
            return arg;
        }

        /// <summary>Compare selected value out of t</summary>
        /// <typeparam name="K">comparison type</typeparam>
        private struct SelectorComparer<K> : IComparer<T>
        {
            /// <summary>Select something out of T</summary>
            internal Func<T, K> Selector;

            /// <summary>Comparer of selected value</summary>
            internal Func<K, K, int> Comparer;

            /// <summary>Compare</summary>
            /// <param name="x">x</param>
            /// <param name="y">y</param>
            /// <returns>int</returns>
            int IComparer<T>.Compare(T x, T y)
            {
                return this.Comparer(this.Selector(x), this.Selector(y));
            }
        }
    }
}
