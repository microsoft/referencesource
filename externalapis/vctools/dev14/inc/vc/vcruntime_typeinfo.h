//
// vcruntime_typeinfo.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// <typeinfo> functionality that is implemented in the VCRuntime.
//
#pragma once

#include <vcruntime.h>
#include <vcruntime_exception.h>

#pragma pack(push, _CRT_PACKING)
#ifndef RC_INVOKED

#if defined _M_CEE_MIXED && !defined _VCRT_BUILD
    // Provide a fake definition of __type_info_node to suppress linker warning
    // LNK4248: unresolved typeref token for '__type_info_node'; image may not run.
    struct __type_info_node { };
#else
    struct __type_info_node;
#endif

#ifdef _M_CEE_PURE
    extern System::IntPtr __type_info_root_node;
#else
    extern __type_info_node __type_info_root_node;
#endif



_CRT_BEGIN_C_HEADER


#pragma warning(push)
#pragma warning(disable: 4510 4512 4610) // This type can never be instantiated
struct __std_type_info_data
{
    char const* _UndecoratedName;
    char const  _DecoratedName[1];
};
#pragma warning(pop)

_VCRTIMP int __cdecl __std_type_info_compare(
    _In_ __std_type_info_data const* _Lhs,
    _In_ __std_type_info_data const* _Rhs
    );

_VCRTIMP size_t __cdecl __std_type_info_hash(
    _In_ __std_type_info_data const* _Data
    );

_VCRTIMP char const* __cdecl __std_type_info_name(
    _Inout_ __std_type_info_data* _Data,
    _Inout_ __type_info_node*     _RootNode
    );

_CRT_END_C_HEADER



class type_info
{
public:

    type_info(type_info const&) = delete;
    type_info& operator=(type_info const&) = delete;

    size_t hash_code() const throw()
    {
        return __std_type_info_hash(&_Data);
    }

    bool operator==(type_info const& _Other) const throw()
    {
        return __std_type_info_compare(&_Data, &_Other._Data) == 0;
    }

    bool operator!=(type_info const& _Other) const throw()
    {
        return __std_type_info_compare(&_Data, &_Other._Data) != 0;
    }

    bool before(type_info const& _Other) const throw()
    {
        return __std_type_info_compare(&_Data, &_Other._Data) < 0;
    }

    char const* name() const throw()
    {
        #ifdef _M_CEE_PURE
        return __std_type_info_name(&_Data, static_cast<__type_info_node*>(__type_info_root_node.ToPointer()));
        #else
        return __std_type_info_name(&_Data, &__type_info_root_node);
        #endif
    }

    char const* raw_name() const throw()
    {
        return _Data._DecoratedName;
    }

    virtual ~type_info() throw();

private:

    mutable __std_type_info_data _Data;
};

namespace std {
	using ::type_info;
}

#if _HAS_EXCEPTIONS

namespace std {

class bad_cast
    : public exception
{
public:

    bad_cast() throw()
        : exception("bad cast", 1)
    {
    }

    static bad_cast __construct_from_string_literal(char const* const _Message) throw()
    {
        return bad_cast(_Message, 1);
    }

private:

    bad_cast(char const* const _Message, int) throw()
        : exception(_Message, 1)
    {
    }
};

class bad_typeid
    : public exception
{
public:

    bad_typeid() throw()
        : exception("bad typeid", 1)
    {
    }

    static bad_typeid __construct_from_string_literal(char const* const _Message) throw()
    {
        return bad_typeid(_Message, 1);
    }

private:

    friend class __non_rtti_object;

    bad_typeid(char const* const _Message, int) throw()
        : exception(_Message, 1)
    {
    }
};

class __non_rtti_object
    : public bad_typeid
{
public:

    static __non_rtti_object __construct_from_string_literal(char const* const _Message) throw()
    {
        return __non_rtti_object(_Message, 1);
    }

private:

    __non_rtti_object(char const* const _Message, int) throw()
        : bad_typeid(_Message, 1)
    {
    }
};

} // namespace std

#endif // _HAS_EXCEPTIONS
#endif // !RC_INVOKED
#pragma pack(pop)
