// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: ListContractAdapter 
**
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.AddIn.Contract;
using System.Diagnostics.Contracts;

namespace System.AddIn.Pipeline
{
    internal class ListContractAdapter<T, U> : ContractBase, IListContract<U>
    {
        private IList<T> m_list;
        private Converter<T, U> m_wrapper;
        private Converter<U, T> m_unwrapper; 

        public ListContractAdapter(IList<T> source, Converter<T, U> wrapper, Converter<U, T> unwrapper)
        {
            if (source == null)
                throw new ArgumentNullException("source");
            if (wrapper == null)
                throw new ArgumentNullException("wrapper");
            if (unwrapper == null)
                throw new ArgumentNullException("unwrapper");
            if (!typeof(U).IsSerializable && !typeof(IContract).IsAssignableFrom(typeof(U)))
                throw new ArgumentException(Res.TypeShouldBeSerializableOrIContract, typeof(U).Name);
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            m_list = source;
            m_wrapper = wrapper;
            m_unwrapper = unwrapper;
        }

        public void Add(U item)
        {
            m_list.Add(m_unwrapper(item));
        }

        public void Clear()
        {
            m_list.Clear();
        }

        public bool Contains(U item)
        {
            return m_list.Contains(m_unwrapper(item));
        }

        public int IndexOf(U item)
        {
            return m_list.IndexOf(m_unwrapper(item));
        }

        public void Insert(int index, U item)
        {
            m_list.Insert(index, m_unwrapper(item));
        }

        public bool Remove(U item)
        {
            return m_list.Remove(m_unwrapper(item));
        }

        public void RemoveAt(int index)
        {
            m_list.RemoveAt(index);
        }

        public U GetItem(int index)
        {
            return m_wrapper(m_list[index]);
        }

        public void SetItem(int index, U item)
        {
            m_list[index] = m_unwrapper(item);
        }

        public int GetCount()
        {
            return m_list.Count;
        }

        public bool GetIsReadOnly()
        {
            return m_list.IsReadOnly;
        }

        public IEnumeratorContract<U> GetEnumeratorContract()
        {
            return new ListEnumeratorAdapter<T, U>(m_list.GetEnumerator(), m_wrapper);
        }
    }

    internal class ListEnumeratorAdapter<T, U> : ContractBase, IEnumeratorContract<U>
    {
        IEnumerator<T> m_enumerator;
        Converter<T,U> m_wrapper;

        public ListEnumeratorAdapter(IEnumerator<T> enumerator, Converter<T,U> wrapper)
        {
            if (enumerator == null)
                throw new ArgumentNullException("enumerator");
            if (wrapper == null)
                throw new ArgumentNullException("wrapper");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            m_enumerator = enumerator;
            m_wrapper = wrapper;
        }

        public U GetCurrent()
        {
            return m_wrapper(m_enumerator.Current);
        }

        public bool MoveNext()
        {
            return m_enumerator.MoveNext();
        }

        public void Reset()
        {
            m_enumerator.Reset();
        }

        public void Dispose()
        {
            m_enumerator.Dispose();
        }
    }
}

