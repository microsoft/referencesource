#pragma once

#include "SymbolTable.h"
#import "mscorlib.tlb" raw_interfaces_only raw_native_types named_guids rename("_Module", "_ReflectionModule") rename("ReportEvent", "_ReflectionReportEvent") rename("value", "_value")
#include "vcclr.h"
#include "AnonymousTypeEmitter.h"
#include "AnonymousDelegateEmitter.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
            ref class AnonymousTypeEmitter;

            private ref class SymbolMap sealed 
            {
            private:
                AnonymousTypeEmitter^ m_anonymousTypeEmitter;
                AnonymousDelegateEmitter^ m_anonymousDelegateEmitter;
                AssemblyBuilder^ m_assemblyBuilder;
                ModuleBuilder^ m_moduleBuilder;

            public:
                ~SymbolMap();
                System::Type^ GetType(BCSYM* pSymbol);
                System::Reflection::MethodInfo^ GetMethod(BCSYM_Proc * pMethodSymbol, GenericBinding* pBindingContext);
                System::Reflection::ConstructorInfo^ GetConstructor(BCSYM_Proc * pConstructorSymbol, GenericTypeBinding* pBindingContext);
                System::Reflection::FieldInfo^ GetField(BCSYM_Variable * pFieldSymbol, GenericTypeBinding* pBindingContext);

            private:
                property AssemblyBuilder^ DynamicAssemblyBuilder{
                    AssemblyBuilder^ get();
                }
                property ModuleBuilder^ DynamicModuleBuilder{
                    ModuleBuilder^ get();
                }
                property AnonymousTypeEmitter^ DynamicAnonymousTypeEmitter{
                    AnonymousTypeEmitter^ get();
                }
                property AnonymousDelegateEmitter^ DynamicAnonymousDelegateEmitter{
                    AnonymousDelegateEmitter^ get();
                }

                System::Type^ GetType(BCSYM* pSymbol, GenericBinding* pBindingContext);
                System::Type^ GetAnonymousType(BCSYM_GenericTypeBinding* pSymbol);
                System::Type^ GetAnonymousDelegate(BCSYM* pSymbol);
                System::Type^ GetBasicType(BCSYM_NamedRoot* pSymbol);
                System::Type^ GetBoundGenericType(BCSYM_GenericTypeBinding* pSymbol, GenericBinding* pBindingContext);
                System::Type^ MakeConcrete(System::Type^ typeDef, array<System::Type^>^ parentArguments, BCSYM_GenericTypeBinding* pSymbol, GenericBinding* pBindingContext);
                System::Type^ GetGenericParamType(BCSYM_GenericParam* pSymbol, GenericBinding* pBindingContext);
                System::Type^ GetArrayType(BCSYM_ArrayType* pSymbol, GenericBinding* pBindingContext);

                System::Reflection::Assembly^ GetAssembly(BCSYM_NamedRoot* pSymbol);
                System::Reflection::MethodBase^ SymbolMap::FindMethod(BCSYM_Proc * pProcSymbol, System::String^ name, array<System::Reflection::MethodBase^>^ methods, System::Type^ type);
                array<System::Type^>^ GetParameterTypes(BCSYM_Proc * pMethodSymbol, GenericBinding* pBindingContext);

                bool Equals(BCSYM* pSymbol, System::Type^ type);
                bool EqualsBasicType(BCSYM_NamedRoot* pSymbol, System::Type^ type);
                bool EqualsBoundGenericType(BCSYM_GenericTypeBinding* pSymbol, System::Type^ type);
                bool EqualsGenericParamType(BCSYM_GenericParam* pSymbol, System::Type^ type);
                bool EqualsArrayType(BCSYM_ArrayType* pSymbol, System::Type^ type);
                    
            };
        }
    }
}
