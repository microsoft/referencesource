// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections.Generic;
using System.Diagnostics;

// Include Silverlight's managed resources
#if SILVERLIGHT
using System.Core;
#endif //SILVERLIGHT

namespace System.Linq
{
    public static partial class Enumerable
    {
        public static IEnumerable<TSource> Append<TSource>(this IEnumerable<TSource> source, TSource element)
        {
            if (source == null)
            {
                throw Error.ArgumentNull(nameof(source));
            }

            AppendPrependIterator<TSource> appendable = source as AppendPrependIterator<TSource>;
            if (appendable != null)
            {
                return appendable.Append(element);
            }

            return new AppendPrepend1Iterator<TSource>(source, element, appending: true);
        }

        public static IEnumerable<TSource> Prepend<TSource>(this IEnumerable<TSource> source, TSource element)
        {
            if (source == null)
            {
                throw Error.ArgumentNull(nameof(source));
            }

            AppendPrependIterator<TSource> appendable = source as AppendPrependIterator<TSource>;
            if (appendable != null)
            {
                return appendable.Prepend(element);
            }

            return new AppendPrepend1Iterator<TSource>(source, element, appending: false);
        }

        /// <summary>
        /// Represents the insertion of one or more items before or after an <see cref="IEnumerable{TSource}"/>.
        /// </summary>
        /// <typeparam name="TSource">The type of the source enumerable.</typeparam>
        private abstract class AppendPrependIterator<TSource> : Iterator<TSource>, IIListProvider<TSource>
        {
            protected readonly IEnumerable<TSource> _source;
            protected IEnumerator<TSource> enumerator;

            protected AppendPrependIterator(IEnumerable<TSource> source)
            {
                Debug.Assert(source != null);
                _source = source;
            }

            protected void GetSourceEnumerator()
            {
                Debug.Assert(enumerator == null);
                enumerator = _source.GetEnumerator();
            }

            public abstract AppendPrependIterator<TSource> Append(TSource item);

            public abstract AppendPrependIterator<TSource> Prepend(TSource item);

            protected bool LoadFromEnumerator()
            {
                if (enumerator.MoveNext())
                {
                    current = enumerator.Current;
                    return true;
                }

                Dispose();
                return false;
            }

            public override void Dispose()
            {
                if (enumerator != null)
                {
                    enumerator.Dispose();
                    enumerator = null;
                }

                base.Dispose();
            }

            public override IEnumerable<TResult> Select<TResult>(Func<TSource, TResult> selector)
            {
                return new SelectEnumerableIterator<TSource, TResult>(this, selector);
            }

            public override IEnumerable<TSource> Where(Func<TSource, bool> predicate)
            {
                return new WhereEnumerableIterator<TSource>(this, predicate);
            }

            public abstract TSource[] ToArray();

            public abstract List<TSource> ToList();

            public abstract int GetCount(bool onlyIfCheap);
        }

        /// <summary>
        /// Represents the insertion of an item before or after an <see cref="IEnumerable{TSource}"/>.
        /// </summary>
        /// <typeparam name="TSource">The type of the source enumerable.</typeparam>
        private class AppendPrepend1Iterator<TSource> : AppendPrependIterator<TSource>
        {
            private readonly TSource _item;
            private readonly bool _appending;

            public AppendPrepend1Iterator(IEnumerable<TSource> source, TSource item, bool appending)
                : base(source)
            {
                _item = item;
                _appending = appending;
            }

            public override Iterator<TSource> Clone() => new AppendPrepend1Iterator<TSource>(_source, _item, _appending);

            public override bool MoveNext()
            {
                switch (state)
                {
                    case 1:
                        state = 2;
                        if (!_appending)
                        {
                            current = _item;
                            return true;
                        }

                        goto case 2;
                    case 2:
                        GetSourceEnumerator();
                        state = 3;
                        goto case 3;
                    case 3:
                        if (LoadFromEnumerator())
                        {
                            return true;
                        }

                        if (_appending)
                        {
                            current = _item;
                            return true;
                        }

                        break;
                }

                Dispose();
                return false;
            }

            public override AppendPrependIterator<TSource> Append(TSource item)
            {
                if (_appending)
                {
                    return new AppendPrependN<TSource>(_source, null, new SingleLinkedNode<TSource>(_item).Add(item), prependCount: 0, appendCount: 2);
                }
                else
                {
                    return new AppendPrependN<TSource>(_source, new SingleLinkedNode<TSource>(_item), new SingleLinkedNode<TSource>(item), prependCount: 1, appendCount: 1);
                }
            }

            public override AppendPrependIterator<TSource> Prepend(TSource item)
            {
                if (_appending)
                {
                    return new AppendPrependN<TSource>(_source, new SingleLinkedNode<TSource>(item), new SingleLinkedNode<TSource>(_item), prependCount: 1, appendCount: 1);
                }
                else
                {
                    return new AppendPrependN<TSource>(_source, new SingleLinkedNode<TSource>(_item).Add(item), null, prependCount: 2, appendCount: 0);
                }
            }

            private TSource[] LazyToArray()
            {
                Debug.Assert(GetCount(onlyIfCheap: true) == -1);

                var builder = new LargeArrayBuilder<TSource>(initialize: true);
                
                if (!_appending)
                {
                    builder.SlowAdd(_item);
                }

                builder.AddRange(_source);

                if (_appending)
                {
                    builder.SlowAdd(_item);
                }

                return builder.ToArray();
            }

            public override TSource[] ToArray()
            {
                int count = GetCount(onlyIfCheap: true);
                if (count == -1)
                {
                    return LazyToArray();
                }

                TSource[] array = new TSource[count];
                int index;
                if (_appending)
                {
                    index = 0;
                }
                else
                {
                    array[0] = _item;
                    index = 1;
                }

                EnumerableHelpers.Copy(_source, array, index, count - 1);

                if (_appending)
                {
                    array[array.Length - 1] = _item;
                }

                return array;
            }

            public override List<TSource> ToList()
            {
                int count = GetCount(onlyIfCheap: true);
                List<TSource> list = count == -1 ? new List<TSource>() : new List<TSource>(count);
                if (!_appending)
                {
                    list.Add(_item);
                }

                list.AddRange(_source);
                if (_appending)
                {
                    list.Add(_item);
                }

                return list;
            }

            public override int GetCount(bool onlyIfCheap)
            {
                IIListProvider<TSource> listProv = _source as IIListProvider<TSource>;
                if (listProv != null)
                {
                    int count = listProv.GetCount(onlyIfCheap);
                    return count == -1 ? -1 : count + 1;
                }

                return !onlyIfCheap || _source is ICollection<TSource> ? _source.Count() + 1 : -1;
            }
        }

        /// <summary>
        /// Represents the insertion of multiple items before or after an <see cref="IEnumerable{TSource}"/>.
        /// </summary>
        /// <typeparam name="TSource">The type of the source enumerable.</typeparam>
        private class AppendPrependN<TSource> : AppendPrependIterator<TSource>
        {
            private readonly SingleLinkedNode<TSource> _prepended;
            private readonly SingleLinkedNode<TSource> _appended;
            private readonly int _prependCount;
            private readonly int _appendCount;
            private SingleLinkedNode<TSource> _node;

            public AppendPrependN(IEnumerable<TSource> source, SingleLinkedNode<TSource> prepended, SingleLinkedNode<TSource> appended, int prependCount, int appendCount)
                : base(source)
            {
                Debug.Assert(prepended != null || appended != null);
                Debug.Assert(prependCount > 0 || appendCount > 0);
                Debug.Assert(prependCount + appendCount >= 2);
                Debug.Assert((prepended?.GetCount() ?? 0) == prependCount);
                Debug.Assert((appended?.GetCount() ?? 0) == appendCount);

                _prepended = prepended;
                _appended = appended;
                _prependCount = prependCount;
                _appendCount = appendCount;
            }

            public override Iterator<TSource> Clone() => new AppendPrependN<TSource>(_source, _prepended, _appended, _prependCount, _appendCount);

            public override bool MoveNext()
            {
                switch (state)
                {
                    case 1:
                        _node = _prepended;
                        state = 2;
                        goto case 2;
                    case 2:
                        if (_node != null)
                        {
                            current = _node.Item;
                            _node = _node.Linked;
                            return true;
                        }

                        GetSourceEnumerator();
                        state = 3;
                        goto case 3;
                    case 3:
                        if (LoadFromEnumerator())
                        {
                            return true;
                        }

                        if (_appended == null)
                        {
                            return false;
                        }

                        enumerator = _appended.GetEnumerator(_appendCount);
                        state = 4;
                        goto case 4;
                    case 4:
                        return LoadFromEnumerator();
                }

                Dispose();
                return false;
            }

            public override AppendPrependIterator<TSource> Append(TSource item)
            {
                var appended = _appended != null ? _appended.Add(item) : new SingleLinkedNode<TSource>(item);
                return new AppendPrependN<TSource>(_source, _prepended, appended, _prependCount, _appendCount + 1);
            }

            public override AppendPrependIterator<TSource> Prepend(TSource item)
            {
                var prepended = _prepended != null ? _prepended.Add(item) : new SingleLinkedNode<TSource>(item);
                return new AppendPrependN<TSource>(_source, prepended, _appended, _prependCount + 1, _appendCount);
            }

            private TSource[] LazyToArray()
            {
                Debug.Assert(GetCount(onlyIfCheap: true) == -1);

                var builder = new SparseArrayBuilder<TSource>(initialize: true);

                if (_prepended != null)
                {
                    builder.Reserve(_prependCount);
                }

                builder.AddRange(_source);

                if (_appended != null)
                {
                    builder.Reserve(_appendCount);
                }

                TSource[] array = builder.ToArray();

                int index = 0;
                for (SingleLinkedNode<TSource> node = _prepended; node != null; node = node.Linked)
                {
                    array[index++] = node.Item;
                }

                index = array.Length - 1;
                for (SingleLinkedNode<TSource> node = _appended; node != null; node = node.Linked)
                {
                    array[index--] = node.Item;
                }

                return array;
            }

            public override TSource[] ToArray()
            {
                int count = GetCount(onlyIfCheap: true);
                if (count == -1)
                {
                    return LazyToArray();
                }

                TSource[] array = new TSource[count];
                int index = 0;
                for (SingleLinkedNode<TSource> node = _prepended; node != null; node = node.Linked)
                {
                    array[index] = node.Item;
                    ++index;
                }

                ICollection<TSource> sourceCollection = _source as ICollection<TSource>;
                if (sourceCollection != null)
                {
                    sourceCollection.CopyTo(array, index);
                }
                else
                {
                    foreach (TSource item in _source)
                    {
                        array[index] = item;
                        ++index;
                    }
                }

                index = array.Length;
                for (SingleLinkedNode<TSource> node = _appended; node != null; node = node.Linked)
                {
                    --index;
                    array[index] = node.Item;
                }

                return array;
            }

            public override List<TSource> ToList()
            {
                int count = GetCount(onlyIfCheap: true);
                List<TSource> list = count == -1 ? new List<TSource>() : new List<TSource>(count);
                for (SingleLinkedNode<TSource> node = _prepended; node != null; node = node.Linked)
                {
                    list.Add(node.Item);
                }

                list.AddRange(_source);
                if (_appended != null)
                {
                    IEnumerator<TSource> e = _appended.GetEnumerator(_appendCount);
                    while (e.MoveNext())
                    {
                        list.Add(e.Current);
                    }
                }

                return list;
            }

            public override int GetCount(bool onlyIfCheap)
            {
                IIListProvider<TSource> listProv = _source as IIListProvider<TSource>;
                if (listProv != null)
                {
                    int count = listProv.GetCount(onlyIfCheap);
                    return count == -1 ? -1 : count + _appendCount + _prependCount;
                }

                return !onlyIfCheap || _source is ICollection<TSource> ? _source.Count() + _appendCount + _prependCount : -1;
            }
        }
    }

    /// <summary>
    /// An iterator that can produce an array or <see cref="List{TElement}"/> through an optimized path.
    /// </summary>
    internal interface IIListProvider<TElement> : IEnumerable<TElement>
    {
        /// <summary>
        /// Produce an array of the sequence through an optimized path.
        /// </summary>
        /// <returns>The array.</returns>
        TElement[] ToArray();
 
        /// <summary>
        /// Produce a <see cref="List{TElement}"/> of the sequence through an optimized path.
        /// </summary>
        /// <returns>The <see cref="List{TElement}"/>.</returns>
        List<TElement> ToList();
 
        /// <summary>
        /// Returns the count of elements in the sequence.
        /// </summary>
        /// <param name="onlyIfCheap">If true then the count should only be calculated if doing
        /// so is quick (sure or likely to be constant time), otherwise -1 should be returned.</param>
        /// <returns>The number of elements.</returns>
        int GetCount(bool onlyIfCheap);
    }

    /// <summary>
    /// An immutable node in a singly-linked list of items.
    /// </summary>
    /// <typeparam name="TSource">The type of the node's item.</typeparam>
    internal sealed class SingleLinkedNode<TSource>
    {
        /// <summary>
        /// Constructs a tail node.
        /// </summary>
        /// <param name="item">The item to place in the tail node.</param>
        public SingleLinkedNode(TSource item)
        {
            Item = item;
        }

        /// <summary>
        /// Constructs a node linked to the specified node.
        /// </summary>
        /// <param name="linked">The linked node.</param>
        /// <param name="item">The item to place in this node.</param>
        private SingleLinkedNode(SingleLinkedNode<TSource> linked, TSource item)
        {
            Debug.Assert(linked != null);
            Linked = linked;
            Item = item;
        }

        /// <summary>
        /// The item held by this node.
        /// </summary>
        public TSource Item { get; }

        /// <summary>
        /// The next node in the singly-linked list.
        /// </summary>
        public SingleLinkedNode<TSource> Linked { get; }

        /// <summary>
        /// Creates a new node that holds the specified item and is linked to this node.
        /// </summary>
        /// <param name="item">The item to place in the new node.</param>
        public SingleLinkedNode<TSource> Add(TSource item) => new SingleLinkedNode<TSource>(this, item);

        /// <summary>
        /// Gets the number of items in this and subsequent nodes by walking the linked list.
        /// </summary>
        public int GetCount()
        {
            int count = 0;
            for (SingleLinkedNode<TSource> node = this; node != null; node = node.Linked)
            {
                count++;
            }

            return count;
        }

        /// <summary>
        /// Gets an <see cref="IEnumerator{TSource}"/> that enumerates the items of this node's singly-linked list in reverse.
        /// </summary>
        /// <param name="count">The number of items in this node.</param>
        public IEnumerator<TSource> GetEnumerator(int count)
        {
            return ((IEnumerable<TSource>)ToArray(count)).GetEnumerator();
        }

        /// <summary>
        /// Gets the node at a logical index by walking the linked list.
        /// </summary>
        /// <param name="index">The logical index.</param>
        /// <remarks>
        /// The caller should make sure <paramref name="index"> is less than this node's count.
        /// </remarks>
        public SingleLinkedNode<TSource> GetNode(int index)
        {
            Debug.Assert(index >= 0 && index < GetCount());

            SingleLinkedNode<TSource> node = this;
            for (; index > 0; index--)
            {
                node = node.Linked;
            }

            Debug.Assert(node != null);
            return node;
        }

        /// <summary>
        /// Returns an <see cref="T:TSource[]"/> that contains the items of this node's singly-linked list in reverse.
        /// </summary>
        /// <param name="count">The number of items in this node.</param>
        public TSource[] ToArray(int count)
        {
            Debug.Assert(count == GetCount());

            TSource[] array = new TSource[count];
            int index = count;
            for (SingleLinkedNode<TSource> node = this; node != null; node = node.Linked)
            {
                --index;
                array[index] = node.Item;
            }

            Debug.Assert(index == 0);
            return array;
        }
    }
}
