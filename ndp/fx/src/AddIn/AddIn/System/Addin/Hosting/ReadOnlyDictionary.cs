// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ReadOnlyDictionary
**    A wrapper on a Dictionary that throws if any of the 
**    write methods or property setters are called.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;

namespace System.Collections.ObjectModel
{

[Serializable]
internal class ReadOnlyDictionary<TKey, TValue> : IDictionary<TKey, TValue>
{
    private IDictionary<TKey, TValue> m_dictionary;

    public ReadOnlyDictionary(IDictionary<TKey, TValue> dictionary)
    {
        if (dictionary == null)
            throw new ArgumentNullException("dictionary");
        System.Diagnostics.Contracts.Contract.EndContractBlock();

        m_dictionary = dictionary;
    }

    public void Add(TKey key, TValue value)
    {
        throw new NotSupportedException();
    }

    public bool ContainsKey(TKey key)
    {
        return m_dictionary.ContainsKey(key);
    }

    public bool Remove(TKey key)
    {
        throw new NotSupportedException();
    }

    public bool TryGetValue(TKey key, out TValue value)
    {
        return m_dictionary.TryGetValue(key, out value);
    }

    public TValue this[TKey key] 
    {
        get {
            return m_dictionary[key];
        }
        set {
            throw new NotSupportedException();
        }
    }

    public ICollection<TKey> Keys
    {
        get {
            return m_dictionary.Keys;
        }
    }

    public ICollection<TValue> Values
    {
        get {
            return m_dictionary.Values;
        }
    }

    public void Add(KeyValuePair<TKey, TValue> pair)
    {
        throw new NotSupportedException();
    }

    public void Clear()
    {
        throw new NotSupportedException();
    }

    public bool Contains(KeyValuePair<TKey, TValue> keyValuePair)
    {
        return m_dictionary.Contains(keyValuePair);
    }

    public void CopyTo(KeyValuePair<TKey, TValue>[] array, Int32 arrayIndex)
    {
        m_dictionary.CopyTo(array, arrayIndex);
    }

    public bool Remove(KeyValuePair<TKey, TValue> keyValuePair)
    {
        throw new NotSupportedException();
    }

    public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
    {
        return m_dictionary.GetEnumerator();
    }

    public Int32 Count
    {
        get {
            return m_dictionary.Count;
        }
    }

    public bool IsReadOnly
    {
        get {
            return true;
        }
    }

    IEnumerator System.Collections.IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }
}

}
