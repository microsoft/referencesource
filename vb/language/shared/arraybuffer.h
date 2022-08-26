//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Thin wrapper that allows for easy dealings with arrays of items
//
//-------------------------------------------------------------------------------------------------

#pragma once

template<typename T>
class ArrayBuffer
{
    class ArrayBufferIterator : public virtual IConstIterator<T>
    {
    public:
        ArrayBufferIterator(ArrayBuffer* pArray) :
          m_pArray(pArray),
          m_indexPlusOne(0)
        {
        }

        virtual __override
        bool MoveNext()
        {
            if( m_indexPlusOne >= m_pArray->GetCount() )
            {
                return false;
            }

            ++m_indexPlusOne;
            return true;
        }

        virtual __override
        T Current()
        {
            return (*m_pArray)[m_indexPlusOne-1];
        }

    private:
        size_t m_indexPlusOne;
        ArrayBuffer* m_pArray;
    };

public:
    ArrayBuffer(): m_elementCount(0)
    {
    }

    explicit ArrayBuffer(size_t elementCount)
    {
        Allocate(elementCount);
    }

    size_t GetCount() const 
    { 
        return m_elementCount; 
    }

    size_t GetByteSize() const 
    { 
        return VBMath::Multiply(sizeof(T), m_elementCount); 
    }

    const T* GetData() const 
    { 
        return m_spData; 
    }

    T* GetData() 
    { 
        return m_spData; 
    }

    void Allocate(size_t elementCount)
    {
        size_t bufferSize = VBMath::Multiply(sizeof(T), elementCount);
        m_spData.AllocateBytes(bufferSize);
        m_elementCount = elementCount;
    }

    void AllocateBytes(size_t byteSize)
    {
        m_spData.AllocateBytes(byteSize);
        m_elementCount = VBMath::Divide(byteSize, sizeof(T));
    }

    void Release()
    {
        m_spData.Free();
        m_elementCount = 0;
    }

    T* Detach() throw()
    {
        m_elementCount = 0;
        return m_spData.Detach();
    }

    T & operator[](const size_t index) 
    {
        VSASSERT(index < m_elementCount, "Invalid ArrayBuffer index");
        return m_spData[index];
    }

    const T & operator[](const size_t index) const
    {
        VSASSERT(index < m_elementCount, "Invalid ArrayBuffer index");
        return m_spData[index];
    }

    ConstIterator<T> GetConstIterator()
    {
        return ArrayBufferIterator(this);
    }

    void ZeroBuffer()
    {
        if ( m_spData )
        {
            memset(m_spData, 0, GetByteSize());
        }
    }

private:

    // Do not define.  
    ArrayBuffer(const ArrayBuffer<T> &);
    ArrayBuffer<T> operator=(const ArrayBuffer<T> &);

    size_t m_elementCount;
    VBHeapPtr<T> m_spData;
};


