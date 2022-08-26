//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Thin wrapper around a raw string buffer
//
//-------------------------------------------------------------------------------------------------

#pragma once

template<typename T>
class _RawStringBuffer
{
public:
    _RawStringBuffer()
    {
    }

    explicit _RawStringBuffer(size_t countNoNullTerminator) 
    {
        AllocateNonNullCharacterCount(countNoNullTerminator);
    }

    size_t GetCharacterCount() const 
    { 
        return m_buffer.GetCount(); 
    }

    size_t GetNonNullCharacterCount() const 
    { 
        return VBMath::Subtract(m_buffer.GetCount(), 1); 
    }
    
    size_t GetBufferByteSize() const 
    { 
        return m_buffer.GetByteSize(); 
    }
    
    const T* GetData() const 
    { 
        return m_buffer.GetData(); 
    }

    T* GetData() 
    { 
        return m_buffer.GetData(); 
    }
    
    operator T*() throw() 
    { 
        return m_buffer.GetData(); 
    }

    operator const T*() const throw() 
    { 
        return m_buffer;
    }

    void AllocateNonNullCharacterCount(size_t countNoNullTerminator)
    {
        m_buffer.Allocate(VBMath::Add<size_t>(countNoNullTerminator,1));
    }

    void AllocateCharacterCount(size_t countAllCharacters)
    {
        m_buffer.Allocate(countAllCharacters);
    }

    void AllocateBytes(size_t byteSize)
    {
        m_buffer.AllocateBytes(byteSize);
    }

    void Release()
    {
        m_buffer.Release();
    }

    T* Detach()
    {
        return m_buffer.Detach();
    }

private:

    // Do not define.  
    _RawStringBuffer(const _RawStringBuffer<T> &);
    _RawStringBuffer<T> operator=(const _RawStringBuffer<T> &);

    ArrayBuffer<T> m_buffer;
};

template <typename T>
class _RawStringBufferUtil
{
public:
    static 
    size_t CalculateBufferByteSize(size_t countNoNullTerminator)
    {
        SafeInt<size_t> total = countNoNullTerminator;
        total += 1;
        total *= sizeof(T);
        return total.Value();
    }

    static 
    T* AllocateNonNullCharacterCount(size_t countNoNullTerminator)
    {
        return (T*)VBAllocator::Allocate(CalculateBufferByteSize(countNoNullTerminator));
    }

    static 
    T* AllocateNonNullCharacterCount(size_t countNoNullTerminator, _Out_ size_t *bufferSize)
    {
        ThrowIfNull(bufferSize);

        size_t calc = CalculateBufferByteSize(countNoNullTerminator);
        *bufferSize = calc;
        return (T*)VBAllocator::Allocate(calc);
    }

    static 
    T* Copy(_In_opt_ LPCTSTR source)
    {
        if ( !source )
        {
            return NULL;
        }

        _RawStringBuffer<T> buffer;
        size_t length;
        IfFailThrow(StringCchLength(source, STRSAFE_MAX_CCH, &length));
        buffer.AllocateNonNullCharacterCount(length);
        IfFailThrow(StringCchCopy(buffer, buffer.GetCharacterCount(), source));
        return buffer.Detach();
    }
};




typedef _RawStringBuffer<WCHAR> RawStringBuffer;
typedef _RawStringBufferUtil<WCHAR> RawStringBufferUtil;

typedef _RawStringBuffer<char> RawShortStringBuffer;
typedef _RawStringBufferUtil<char> RawShortStringBufferUtil;
