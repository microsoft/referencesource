#pragma once

#include "..\VBHosted\IImportScope.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class ImportScope : public Microsoft::Compiler::VisualBasic::IImportScope
#else
            public ref class ImportScope : public Microsoft::Compiler::VisualBasic::IImportScope
#endif
            {
            private:
                static ImportScope ^m_empty;

                ImportScope();

            internal:
                static property ImportScope ^Empty
                {
                    ImportScope ^get();
                }

            public:
                virtual System::Collections::Generic::IList<Import ^> ^GetImports();
            };
        }
    }
}
