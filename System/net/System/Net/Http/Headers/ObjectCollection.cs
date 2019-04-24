using System.Collections.ObjectModel;

namespace System.Net.Http.Headers
{
    // List<T> allows 'null' values to be added. This is not what we want so we use a custom Collection<T> derived 
    // type to throw if 'null' gets added. Collection<T> internally uses List<T> which comes at some cost. In addition
    // Collection<T>.Add() calls List<T>.InsertItem() which is an O(n) operation (compared to O(1) for List<T>.Add()).
    // This type is only used for very small collections (1-2 items) to keep the impact of using Collection<T> small.
    internal class ObjectCollection<T> : Collection<T> where T : class
    {
        private static readonly Action<T> defaultValidator = CheckNotNull;

        private Action<T> validator;

        public ObjectCollection()
            : this(defaultValidator)
        {
        }

        public ObjectCollection(Action<T> validator)
        {            
            this.validator = validator;
        }

        protected override void InsertItem(int index, T item)
        {
            if (validator != null)
            {
                validator(item);
            }
            base.InsertItem(index, item);
        }

        protected override void SetItem(int index, T item)
        {
            if (validator != null)
            {
                validator(item);
            }
            base.SetItem(index, item);
        }

        private static void CheckNotNull(T item)
        {
            // null values cannot be added to the collection.
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
        }
    }
}