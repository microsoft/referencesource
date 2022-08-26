#pragma once

#include "..\VBHosted\ITypeScope.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class TypeScope : public ITypeScope
#else
            public ref class TypeScope : public ITypeScope
#endif
            {
            private:
                static TypeScope ^m_empty;

                TypeScope();

            internal:
                static property TypeScope ^Empty
                {
                    TypeScope ^get();
                }

            public:
                virtual bool NamespaceExists(System::String ^ns);
                
                virtual array<System::Type^,1> ^FindTypes(System::String ^typeName,
                                                          System::String ^nsPrefix);
            };
        }
    }
}
