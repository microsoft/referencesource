#pragma once

#include "TypeEmitter.h"

using namespace System::Reflection;
using namespace System::Reflection::Emit;

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
            private ref class AnonymousTypeEmitter : TypeEmitter 
            {
                static AnonymousTypeEmitter();
            private:
                ref class PropertyAndField
                {
                private:
                    PropertyBuilder^ _Property;
                    FieldBuilder^ _Field;

                public:
                    property PropertyBuilder^ Property
                    {
                        PropertyBuilder^ get()
                        {
                            return _Property;
                        }
                        void set(PropertyBuilder^ value)
                        {
                            _Property = value;
                        }
                    }
                    property FieldBuilder^ Field
                    {
                        FieldBuilder^ get()
                        {
                            return _Field;
                        }
                        void set(FieldBuilder^ value)
                        {
                            _Field = value;
                        }
                    }
                };

                static initonly System::Type^ ObjectType;
                static initonly System::Type^ StringType;
                static initonly System::Type^ StringBuilderType;
                System::Collections::Generic::Dictionary<System::String^, System::Type^>^ m_types;

            public:
                AnonymousTypeEmitter(SymbolMap^ symbolMap, ModuleBuilder^ moduleBuilder);

                virtual bool TryGetType(BCSYM_NamedRoot* pSymbol, System::Type^% type) override;

                System::Type^ EmitType(BCSYM_NamedRoot* pSymbol);
                PropertyAndField^ FindPropertyAndField(BCSYM_NamedRoot* pAnonymousType, GenericParameter* pParam, TypeBuilder^ typeBuilder, bool &hasKey);
                void FindMethods(BCSYM_NamedRoot* pAnonymousType, TypeBuilder^ typeBuilder, ConstructorBuilder^% ctorBuilder, MethodBuilder^% toStringBuilder, MethodBuilder^% equalsObjBuilder, MethodBuilder^% equalsTypeBuilder, MethodBuilder^% getHashBuilder);

            private:
                void EmitProperty(PropertyAndField^ prop);
                void EmitCtor(array<PropertyAndField^>^ properties, ConstructorBuilder^ method);
                void EmitToString(array<PropertyAndField^>^ properties, MethodBuilder^ method);
                void EmitEqualsObj(TypeBuilder^ typeBuilder, MethodBuilder^ method, MethodBuilder^ typedEqualsMethoed);
                void EmitEqualsTyped(array<PropertyAndField^>^ properties, MethodBuilder^ method);
                void EmitGetHashCode(array<PropertyAndField^>^ properties, MethodBuilder^ method);
            };
        }
    }
}
