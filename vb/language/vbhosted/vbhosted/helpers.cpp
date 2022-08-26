#include "stdAfx.h"

HRESULT ConvertSystemReflectionAssemblyToCom(gcroot<System::Reflection::Assembly ^> assembly, mscorlib::_Assembly **ppRetVal)
{
    return ConvertManagedObjectToCOM<System::Reflection::Assembly, mscorlib::_Assembly>(assembly, ppRetVal);
}

HRESULT ConvertSystemTypeToCom(gcroot<System::Type ^> type, mscorlib::_Type **ppRetVal)
{
    return ConvertManagedObjectToCOM<System::Type, mscorlib::_Type>(type, ppRetVal);
}

HRESULT ConvertSystemMethodBaseToCom(gcroot<System::Reflection::MethodBase^> method, IUnknown** ppUnk)
{
    return ConvertManagedObjectToCOM<System::Reflection::MethodBase, IUnknown>(method, ppUnk);
}

HRESULT ConvertCOMTypeToSystemType(IUnknown* pType, gcroot<System::Type^>& type)
{
    return ConvertCOMToManagedObject<System::Type>(pType, type);
}
