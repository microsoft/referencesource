#pragma once

#import "mscorlib.tlb" raw_interfaces_only raw_native_types named_guids rename("_Module", "_ReflectionModule") rename("ReportEvent", "_ReflectionReportEvent") rename("value", "_value")

#include "vcclr.h"

//This group of functions assist in converting from Managed types to equivalent mscorlib::* types
HRESULT ConvertSystemReflectionAssemblyToCom(gcroot<System::Reflection::Assembly ^> assembly, mscorlib::_Assembly **ppRetVal);
HRESULT ConvertSystemTypeToCom(gcroot<System::Type ^> type, mscorlib::_Type **ppRetVal);
HRESULT ConvertSystemMethodBaseToCom(gcroot<System::Reflection::MethodBase^> method, IUnknown** ppUnk);

template <typename ManagedType, typename COMType>
HRESULT ConvertManagedObjectToCOM(gcroot<ManagedType ^> object, COMType** ppT)
{

    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyParamCond(object, E_INVALIDARG, "[::ConvertManagedObjectToCOM] 'object' parameter is null");
    VerifyOutPtr(ppT, "[::ConvertManagedObjectToCOM] 'ppT' parameter is null");

    HRESULT hr = S_OK;

    *ppT = NULL;

    // GetIUnknownForObject performs an AddRef().
    System::IntPtr _unknown = System::Runtime::InteropServices::Marshal::GetIUnknownForObject(object);

    IfFalseRet(_unknown.ToPointer(), E_UNEXPECTED);

    hr = reinterpret_cast<LPUNKNOWN>(_unknown.ToPointer())->QueryInterface(__uuidof(COMType), reinterpret_cast<void**>(ppT));

    // Need to Release() the COM object and cleanup the IntPtr.
    System::Runtime::InteropServices::Marshal::Release(_unknown);

    return hr;

}

//This group of functions assist in converting from mscorlib::* types to Managed Types.
HRESULT ConvertCOMTypeToSystemType(IUnknown* pType, gcroot<System::Type^>& type);

template <typename ManagedType>
HRESULT ConvertCOMToManagedObject(IUnknown* pUnk, gcroot<ManagedType^>& managed)
{

    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyParamCond(pUnk, E_INVALIDARG, "[::ConvertManagedObjectToCOM] 'pUnk' parameter is null");

    HRESULT hr = E_UNEXPECTED;

    gcroot<ManagedType^> managedObj;
    gcroot<System::Object^> object = System::Runtime::InteropServices::Marshal::GetObjectForIUnknown(System::IntPtr(pUnk));

    ASSERT(object, "[::ConvertCOMToManagedObject] System::Runtime::InteropServices::Marshal::GetObjectForIUnknown failed");
    if (object)
    {
        managedObj = dynamic_cast<ManagedType^>(static_cast<System::Object^>(object));
        ASSERT(managedObj, "[::ConvertCOMToManagedObject] Cast from System::Object failed");
        if (managedObj)
        {
            managed = managedObj;
            hr = S_OK;
        }
    }

    return hr;
}
