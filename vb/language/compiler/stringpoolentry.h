//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Type safe wrapper over a STRING*
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
// Case insensitive version of a StringPoolEntry
//
//-------------------------------------------------------------------------------------------------
template <bool CaseSensitive>
struct _StringPoolEntry
{
    _StringPoolEntry() : m_pData(NULL)
    {

    }

    _StringPoolEntry(const STRING* pString) : m_pData(pString)
    {

    }

    inline
    bool IsNullOrEmpty() const
    {
        return StringPool::IsNullOrEmpty(m_pData);
    }

    inline 
    size_t Length() const
    {
        VSASSERT(m_pData, "StringPoolEntry::Length on a null string");
        return StringPool::StringLength(m_pData);
    }

    inline
    const STRING* GetData() const
    {
        return m_pData;
    }

    //-------------------------------------------------------------------------------------------------
    //
    // This is a bad function.  All STRING* are inherently const but much of our code base does not
    // propagate this and to ease the transition this function is provided
    //
    //-------------------------------------------------------------------------------------------------
    inline
    STRING* GetDataNonConst() const
    {
        return const_cast<STRING*>(m_pData);
    }

    inline
    unsigned long GetHashCode() const
    {
        return StringPool::HashValOfString(m_pData);
    }

    inline
    operator const STRING*() const
    {
        return m_pData;
    }

    inline
    bool operator ==(const _StringPoolEntry<CaseSensitive>& other) const
    {
        return StringPool::IsEqual(m_pData, other.m_pData, CaseSensitive);
    }

    inline
    bool operator ==( _In_z_ STRING* pOther) const
    {
        return StringPool::IsEqual(m_pData, pOther, CaseSensitive);
    }

    inline
    bool operator !=(const _StringPoolEntry<CaseSensitive>& other) const
    {
        return !StringPool::IsEqual(m_pData, other.m_pData, CaseSensitive);
    }

    inline
    bool operator<(const _StringPoolEntry<CaseSensitive>& other) const
    {
        return StringPool::Compare(m_pData, other.m_pData, CaseSensitive) < 0;
    }

    inline
    bool operator>(const _StringPoolEntry<CaseSensitive>& other) const
    {
        return StringPool::Compare(m_pData, other.m_pData, CaseSensitive) > 0;
    }

    const STRING* m_pData;
};

typedef _StringPoolEntry<false> StringPoolEntry;
typedef _StringPoolEntry<true> StringPoolEntryCaseSensitive;

//-------------------------------------------------------------------------------------------------
//
// Hashing function to allow entries in the DynamicHashTable
//
//-------------------------------------------------------------------------------------------------
template<>
class DefaultHashFunction<StringPoolEntry>
{
public:
    unsigned long GetHashCode(const StringPoolEntry& value) const
    {
        return value.GetHashCode();
    }
    bool AreEqual(const StringPoolEntry& val1, const StringPoolEntry& val2) const
    {
        return val1 == val2;
    }
};

template<>
class DefaultHashFunction<StringPoolEntryCaseSensitive>
{
public:
    unsigned long GetHashCode(const StringPoolEntry& value) const
    {
        return value.GetHashCode();
    }
    bool AreEqual(const StringPoolEntry& val1, const StringPoolEntry& val2) const
    {
        return val1 == val2;
    }
};


