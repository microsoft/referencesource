#pragma once

#include "..\VBHosted\IScriptScope.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class ScriptScope : public Microsoft::Compiler::VisualBasic::IScriptScope
#else
            public ref class ScriptScope : public Microsoft::Compiler::VisualBasic::IScriptScope
#endif
            {
            private:
                static ScriptScope ^m_empty;

                ScriptScope();

            internal:
                static property ScriptScope ^Empty
                {
                    ScriptScope ^get();
                }

            public:
                virtual System::Type ^FindVariable(System::String ^name);
            };
        }
    }
}
