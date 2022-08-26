//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef PID_FIRST_USABLE
#define PID_FIRST_USABLE 2
#endif

#ifndef REFPROPERTYKEY
#ifdef __cplusplus
#define REFPROPERTYKEY const PROPERTYKEY &
#else // !__cplusplus
#define REFPROPERTYKEY const PROPERTYKEY * __MIDL_CONST
#endif // __cplusplus
#endif //REFPROPERTYKEY

#ifdef DEFINE_PROPERTYKEY
#undef DEFINE_PROPERTYKEY
#endif

#ifdef INITGUID
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY name
#endif // INITGUID

#ifndef IsEqualPropertyKey
#define IsEqualPropertyKey(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )
#endif  // IsEqualPropertyKey

#ifndef _PROPERTYKEY_EQUALITY_OPERATORS_
#define _PROPERTYKEY_EQUALITY_OPERATORS_
#ifdef __cplusplus
extern "C++"
{
__inline int operator == (REFPROPERTYKEY pkeyOne, REFPROPERTYKEY pkeyOther) { return IsEqualPropertyKey(pkeyOne, pkeyOther); }
__inline int operator != (REFPROPERTYKEY pkeyOne, REFPROPERTYKEY pkeyOther) { return !(pkeyOne == pkeyOther); }
}
#endif // __cplusplus
#endif // _PROPERTYKEY_EQUALITY_OPERATORS_
