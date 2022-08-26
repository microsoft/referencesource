//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// initial CRC should be ~DWORD(0) = 0xffff ffff ffff ffff
class CRC32
{
public:
    CRC32(DWORD in) : 
        m_CRC32(in) 
    {
    }

    CRC32() : 
        m_CRC32(~DWORD(0))
    {
    }

    CRC32(const CRC32 &in) : 
        m_CRC32(in.m_CRC32)
    {
    }

    CRC32(
        const void * pPtr,
        size_t dwNumBytes);

    template <class T>
    CRC32(const T * pPtr) :
        m_CRC32(~DWORD(0))
    {
        Update( pPtr );
    }

    DWORD Update(BYTE bNextByte);

    DWORD Update(
        const void * pPtr,
        size_t dwNumBytes);

    // template version for adding a structure, data type, or class
    // do not add classes with virtual functions
    template <class T>
    DWORD Update(const T * pPtr)
    {
        return Update( pPtr, sizeof(*pPtr));
    }

    // a conversion operator is used instead of a 'Get' method so
    // that you can use declarations like " DWORD myCRC = CRC32(ptr,len)"
    //
    //  This class can be used like a function or as an object
    //
    //  For example:
    //      CRC32 myCRC32;
    //      myCRC32.Update( ptr, len );
    //      DWORD myValue = myCRC32;
    //
    //      CRC32 myCRC32(ptr1, len1 );
    //      myCRC32.Update(ptr2, len2 );
    //
    //      DWORD myValue = CRC32(ptr, len )
    //
    operator DWORD() const  
    {
         return m_CRC32; 
    };

protected:
    DWORD      m_CRC32;
};
