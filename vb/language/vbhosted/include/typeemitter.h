#pragma once

#include "SymbolTable.h"

using namespace System::Reflection;
using namespace System::Reflection::Emit;

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
            ref class SymbolMap;

            private ref class TypeEmitter abstract
            {
                static TypeEmitter();
            private:
                ModuleBuilder^ m_moduleBuilder;
                SymbolMap^ m_symbolMap;

            public:
                TypeEmitter(SymbolMap^ symbolMap, ModuleBuilder^ moduleBuilder);
                virtual bool TryGetType(BCSYM_NamedRoot* pSymbol, System::Type^% type) abstract;

            protected:
                property ModuleBuilder^ DynamicModuleBuilder{
                    ModuleBuilder^ get();
                }

                static System::Type^ VoidType;

                virtual TypeBuilder^ DefineType(BCSYM_NamedRoot* pSymbol);
                virtual void DefineGenericTypeParameter(TypeBuilder^ typeBuilder, GenericTypeParameterBuilder^ paramBuilder, BCSYM_GenericParam* pParam);
                virtual FieldBuilder^ DefineField(TypeBuilder^ typeBuilder, BCSYM_Variable* pField);
                virtual PropertyBuilder^ DefineProperty(TypeBuilder^ typeBuilder, BCSYM_Property* pProperty);
                virtual MethodBuilder^ DefineMethod(TypeBuilder^ typeBuilder, BCSYM_Proc* pProc);
                virtual ConstructorBuilder^ DefineConstructor(TypeBuilder^ typeBuilder, BCSYM_Proc* pProc);

                static TypeAttributes GetTypeAttributes(BCSYM_NamedRoot* pType);
                static GenericParameterAttributes GetGenericParameterAttributes(BCSYM_GenericParam* pParam);
                static FieldAttributes GetFieldAttributes(BCSYM_Variable* pField);
                static PropertyAttributes GetPropertyAttributes(BCSYM_Property* pProperty);
                static MethodAttributes GetMethodAttributes(BCSYM_Proc* pProc);

                virtual System::Type^ GetType(TypeBuilder^ typeBuilder, BCSYM * pSymbol);
                array<System::Type^>^ GetParameterTypes(TypeBuilder^ typeBuilder, BCSYM_Proc * pMethodSymbol);
            };
        }
    }
}
