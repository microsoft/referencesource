namespace System.Collections.Immutable
{
    internal static class ImmutableArray
    {
        public static ImmutableArray<T>.Builder CreateBuilder<T>(int capacity)
        {
            return new ImmutableArray<T>.Builder(capacity);
        }
    }

    internal struct ImmutableArray<T>
    {
        private readonly T[] _array;

        public ImmutableArray(T[] array)
        {
            _array = array;
        }

        public bool IsDefault { get { return _array == null; } }
        public int Length { get { return _array.Length; } }
        public T this[int index] { get { return _array[index]; } }
        public T[] UnderlyingArray { get { return _array; } }

        public static ImmutableArray<T> Empty = new ImmutableArray<T>(new T[0]);

        public T FirstOrDefault(Func<T, bool> predicate)
        {
            foreach (var v in _array)
            {
                if (predicate(v))
                {
                    return v;
                }
            }

            return default(T);
        }

        public void CopyTo(int sourceIndex, T[] destination, int destinationIndex, int length)
        {
            Array.Copy(_array, sourceIndex, destination, destinationIndex, length);
        }

        public sealed class Builder
        {
            private T[] _elements;
            private int _count;

            internal Builder(int capacity)
            {
                _elements = new T[capacity];
                _count = 0;
            }

            internal Builder()
                : this(8)
            {
            }

            public int Count
            {
                get
                {
                    return _count;
                }
            }

            public int Capacity
            {
                get { return _elements.Length; }
            }

            public T this[int index]
            {
                get
                {
                    if (index >= this.Count)
                    {
                        throw new IndexOutOfRangeException();
                    }

                    return _elements[index];
                }

                set
                {
                    if (index >= this.Count)
                    {
                        throw new IndexOutOfRangeException();
                    }

                    _elements[index] = value;
                }
            }

            public ImmutableArray<T> MoveToImmutable()
            {
                if (Capacity != Count)
                {
                    throw new InvalidOperationException();
                }

                T[] temp = _elements;
                _elements = ImmutableArray<T>.Empty._array;
                _count = 0;
                return new ImmutableArray<T>(temp);
            }

            public void Add(T item)
            {
                this.EnsureCapacity(this.Count + 1);
                _elements[_count++] = item;
            }

            private void EnsureCapacity(int capacity)
            {
                if (_elements.Length < capacity)
                {
                    int newCapacity = Math.Max(_elements.Length * 2, capacity);
                    Array.Resize(ref _elements, newCapacity);
                }
            }
        }
    }
}
