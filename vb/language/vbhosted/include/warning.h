#pragma once

#include "SourceLocation.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class Warning sealed
#else
            public ref class Warning sealed
#endif
            {
            private:
                System::String ^m_description;
                Microsoft::Compiler::VisualBasic::SourceLocation ^m_sourceLocation;
                int m_warningCode;

            internal:
                Warning(int warningCode,
                        System::String ^description,
                        Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation);

            public:
                property int WarningCode {
                    int get();
                }

                property System::String ^Description {
                    System::String ^get();
                }

                property Microsoft::Compiler::VisualBasic::SourceLocation ^SourceLocation {
                    Microsoft::Compiler::VisualBasic::SourceLocation ^get();
                }
            };
        }
    }
}
