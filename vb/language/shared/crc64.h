//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// initial CRC should be ~uint64(0) = 0xffff ffff ffff ffff
typedef UINT64 uint64;

class CRC64
{
public:
    CRC64( uint64 in) : m_crc64( in ) {};
    CRC64() : m_crc64( ~uint64(0) ) {};
    CRC64( const CRC64& in ) : m_crc64( in.m_crc64 ) {};
    CRC64(const void* pPtr, unsigned dwNumBytes);
    template <class T>
    CRC64(const T* pPtr) : m_crc64(  ~uint64(0) )
        { Update( pPtr ); }

    uint64      Update( BYTE bNextByte );
    uint64      Update( const void* pPtr, unsigned dwNumBytes );

    // template version for adding a structure, data type, or class
    // do not add classes with virtual functions
    template <class T>
    uint64      Update( const T* pPtr )
                { return Update( pPtr, sizeof(*pPtr));}

    // a conversion operator is used instead of a 'Get' method so
    // that you can use declarations like " uint64 myCRC = CRC64(ptr,len)"
    //
    //  This class can be used like a function or as an object
    //
    //  For example:
    //      CRC64 myCRC64;
    //      myCRC64.Update( ptr, len );
    //      uint64 myValue = myCRC64;
    //
    //      CRC64 myCRC64(ptr1, len1 );
    //      myCRC64.Update(ptr2, len2 );
    //
    //      uint64 myValue = CRC64(ptr, len )
    //
    operator    uint64() const  { return m_crc64; };

protected:
    uint64      m_crc64;
};
