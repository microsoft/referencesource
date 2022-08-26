/***
*gcroot.h
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:  Template class that wraps GCHandle from mscorlib.dll.
*
*       Use this class to declare gc "pointers" that live in the C++ heap.
*
*       Example:
*               struct StringList {
*                       msclr::gcroot<String^> str;
*                       StringList *next;
*                       StringList(); // should have ctors and dtors
*                       ~StringList();
*               };
*
*       By convention, we maintain a 1-to-1 relationship between C++ objects
*       and the handle slots they "point" to.  Thus, two distinct C++ objects
*       always refer to two distinct handles, even if they "point" to the same
*       object.  Therefore, when the C++ object is destroyed, its handle can
*       be freed without error.
*
*       Note that we cannot currently embed a GCHandle directly in an unmanaged C++
*       class.  We therefore store a void*, and use the conversion methods of
*       GCHandle to reconstitute a GCHandle from the void* on demand.
*
*   [Public]
*
****/

#pragma once

#if !defined (_INC_MSCLR_GCROOT)
#define _INC_MSCLR_GCROOT
#ifndef RC_INVOKED

#include <stddef.h>


#ifdef __cplusplus_cli
 #define __GCHANDLE_TO_VOIDPTR(x) ((GCHandle::operator System::IntPtr(x)).ToPointer())
 #define __VOIDPTR_TO_GCHANDLE(x) (GCHandle::operator GCHandle(System::IntPtr(x)))
 #define __NULLPTR nullptr
#else  /* __cplusplus_cli */
 #define __GCHANDLE_TO_VOIDPTR(x) ((GCHandle::op_Explicit(x)).ToPointer())
 #define __VOIDPTR_TO_GCHANDLE(x) (GCHandle::op_Explicit(x))
 #define __NULLPTR 0
#endif  /* __cplusplus_cli */

#ifndef __DEFINE_GCROOT_IN_GLOBAL_NAMESPACE
namespace msclr
{
#endif  /* __DEFINE_GCROOT_IN_GLOBAL_NAMESPACE */

template <class T> struct gcroot {

        typedef System::Runtime::InteropServices::GCHandle GCHandle;

        // always allocate a new handle during construction (see above)
        //
        // Initializes to a NULL handle, which is always safe
        [System::Diagnostics::DebuggerStepThroughAttribute]
        [System::Security::SecuritySafeCritical]
        gcroot() {
                _handle = __GCHANDLE_TO_VOIDPTR(GCHandle::Alloc(__NULLPTR));
        }

        // this can't be T& here because & does not yet work on managed types
        // (T should be a pointer anyway).
        //
        gcroot(T t) {
                _handle = __GCHANDLE_TO_VOIDPTR(GCHandle::Alloc(t));
        }

        gcroot(const gcroot& r) {
                // don't copy a handle, copy what it points to (see above)
                _handle = __GCHANDLE_TO_VOIDPTR(
                                                GCHandle::Alloc(
                                                        __VOIDPTR_TO_GCHANDLE(r._handle).Target ));
        }

        // Since C++ objects and handles are allocated 1-to-1, we can
        // free the handle when the object is destroyed
        //
        [System::Diagnostics::DebuggerStepThroughAttribute]
        [System::Security::SecurityCritical]
        ~gcroot() {
                GCHandle g = __VOIDPTR_TO_GCHANDLE(_handle);
                g.Free();
                _handle = 0; // should fail if reconstituted
        }

        [System::Diagnostics::DebuggerStepThroughAttribute]
        [System::Security::SecurityCritical]
        gcroot& operator=(T t) {
                // no need to check for valid handle; was allocated in ctor
                __VOIDPTR_TO_GCHANDLE(_handle).Target = t;
                return *this;
        }

        gcroot& operator=(const gcroot &r) {
                // no need to check for valid handle; was allocated in ctor
                T t = (T)r;
                __VOIDPTR_TO_GCHANDLE(_handle).Target = t;
                return *this;
        }

        void swap( gcroot<T> & _right )
        {
                using std::swap;
                swap( _handle, _right._handle );
        }

        // The managed object is not a secret or protected resource, so it's okay to expose to anyone who has access to the gcroot object
        [System::Security::SecuritySafeCritical]
        operator T () const {
                // gcroot is typesafe, so use static_cast
                return static_cast<T>( __VOIDPTR_TO_GCHANDLE(_handle).Target );
        }

        // don't return T& here because & to gc pointer not yet implemented
        // (T should be a pointer anyway).
        [System::Security::SecuritySafeCritical]
        T operator->() const {
                // gcroot is typesafe, so use static_cast
                return static_cast<T>(__VOIDPTR_TO_GCHANDLE(_handle).Target);
        }

private:
        // Don't let anyone copy the handle value directly, or make a copy
        // by taking the address of this object and pointing to it from
        // somewhere else.  The root will be freed when the dtor of this
        // object gets called, and anyone pointing to it still will
        // cause serious harm to the Garbage Collector.
        //
        void* _handle;
        T* operator& ();
};

template<typename T>
void swap( gcroot<T> & _left,
        gcroot<T> & _right )
{
        _left.swap( _right );
}

#ifndef __DEFINE_GCROOT_IN_GLOBAL_NAMESPACE
} // namespace msclr
#endif  /* __DEFINE_GCROOT_IN_GLOBAL_NAMESPACE */

#undef __GCHANDLE_TO_VOIDPTR
#undef __VOIDPTR_TO_GCHANDLE
#undef __NULLPTR

#endif  /* RC_INVOKED */
#endif  /* !defined (_INC_MSCLR_GCROOT) */
