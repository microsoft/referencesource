#pragma once

#include "IScriptScope.h"
#include "ITypeScope.h"
#include "IImportScope.h"
#include "CompilerOptions.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class CompilerContext sealed
#else
            public ref class CompilerContext sealed
#endif
            {
            private:
                static CompilerContext ^m_empty;

                IScriptScope ^m_scriptScope;
                ITypeScope ^m_typeScope;
                IImportScope ^m_importScope;
                CompilerOptions ^m_options;

            internal:
                static property CompilerContext ^Empty
                {
                    CompilerContext ^get();
                }

            public: 
                CompilerContext(IScriptScope ^scriptScope, 
                                ITypeScope ^typeScope, 
                                IImportScope ^importScope, 
                                CompilerOptions ^options);
 
                property IScriptScope ^ScriptScope {
                    IScriptScope ^get();
                }

                property ITypeScope ^TypeScope {
                    ITypeScope ^get();
                }

                property IImportScope ^ImportScope {
                    IImportScope ^get();
                }

                property CompilerOptions ^Options {
                    CompilerOptions ^get();
                }
            };
        }
    }
}
