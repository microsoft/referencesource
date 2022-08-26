// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: ContractListAdapter
**
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.AddIn.Contract;
using System.Runtime.Remoting;
using System.Diagnostics.Contracts;

namespace System.AddIn.Pipeline
{
    internal class ContractListAdapter<T, U> : IList<U>
    {
        private IListContract<T> m_listContract;
        private Converter<T, U> m_wrapper;
        private Converter<U, T> m_unwrapper;

        private ContractHandle m_contractHandle;

        public ContractListAdapter(IListContract<T> source, Converter<T, U> wrapper, Converter<U, T> unwrapper)
        {
            if (source == null)
                throw new ArgumentNullException("source");
            if (wrapper == null)
                throw new ArgumentNullException("wrapper");
            if (unwrapper == null)
                throw new ArgumentNullException("unwrapper");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            m_listContract = source;
            m_wrapper = wrapper;
            m_unwrapper = unwrapper;

            m_contractHandle = new ContractHandle(m_listContract);
        }

        public void Add(U item)
        {
            m_listContract.Add(m_unwrapper(item));
        }

        public void Clear()
        {
            m_listContract.Clear();
        }

        public U this[int index] {
            get
            {
                return m_wrapper(m_listContract.GetItem(index));
            }
            set
            {
                m_listContract.SetItem(index, m_unwrapper(value));
            }
        }

        public bool Contains(U item)
        {
            return m_listContract.Contains(m_unwrapper(item));
        }

        public void CopyTo(U[] destination, int index)
        {
            // need to check Count >= destination.Length then copy wrapped (U) instances to the destination array
            if (destination == null)
                throw new ArgumentNullException("destination");
            if (index < 0)
                throw new ArgumentOutOfRangeException("index");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            int listContractCount = m_listContract.GetCount();
            if (index > destination.Length - listContractCount)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            for (int i = 0; i < listContractCount; i++)
            {
                destination[index++]=m_wrapper(m_listContract.GetItem(i));
            }
        }

        public int IndexOf(U item)
        {
            return m_listContract.IndexOf(m_unwrapper(item));
        }

        public void Insert(int index, U item)
        {
            m_listContract.Insert(index, m_unwrapper(item));
        }

        public bool Remove(U item)
        {
            return m_listContract.Remove(m_unwrapper(item));
        }

        public void RemoveAt(int index)
        {
            m_listContract.RemoveAt(index);
        }

        public U GetItem(int index)
        {
            T item = m_listContract.GetItem(index);
            return m_wrapper(item);
        }

        public void SetItem(int index, U item)
        {
            m_listContract.SetItem(index, m_unwrapper(item));
        }

        public int Count
        {
            get
            {
                return m_listContract.GetCount();
            }
        }

        public bool IsReadOnly
        {
            get
            {
                return m_listContract.GetIsReadOnly();
            }
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        public IEnumerator<U> GetEnumerator()
        {
            return new ContractEnumeratorAdapter<T, U>(m_listContract.GetEnumeratorContract(), m_wrapper);
        }

    }

    internal class ContractEnumeratorAdapter<T, U> : IEnumerator<U>
    {
        private IEnumeratorContract<T> m_enumerator;
        private Converter<T,U> m_wrapper;
        private ContractHandle m_contractHandle;

        public ContractEnumeratorAdapter(IEnumeratorContract<T> enumerator, Converter<T,U> wrapper)
        {
            if (enumerator == null)
                throw new ArgumentNullException("enumerator");
            if (wrapper == null)
                throw new ArgumentNullException("wrapper");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            m_enumerator = enumerator;
            m_wrapper = wrapper;

            m_contractHandle = new ContractHandle((IContract)m_enumerator);
        }

        public U Current
        {
            get
            {
                T val = m_enumerator.GetCurrent();
                return m_wrapper(val);
            }
        }

        object System.Collections.IEnumerator.Current
        {
            get
            {
                return Current;
            }
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
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        // Dispose(bool disposing) executes in two distinct scenarios.
        // If disposing equals true, the method has been called directly
        // or indirectly by a user's code. Managed and unmanaged resources
        // can be disposed.
        // If disposing equals false, the method has been called by the 
        // runtime from inside the finalizer and you should not reference 
        // other objects. Only unmanaged resources can be disposed.
        void Dispose(bool disposing)
        {
            if (m_contractHandle != null )
            {
                if (disposing)
                {
                    try
                    {
                        m_contractHandle.Dispose();

                        if (m_enumerator != null)
                            m_enumerator.Dispose();

                    }
                    catch (AppDomainUnloadedException) { }
                    catch (RemotingException) { }
                }

                m_contractHandle = null;
            } 
        }

        // Dispose pattern - here for completeness
        ~ContractEnumeratorAdapter()
        {
            Dispose(false);
        }
    }
}

