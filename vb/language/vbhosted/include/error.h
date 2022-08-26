#pragma once

#include "SourceLocation.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
            [System::Diagnostics::CodeAnalysis::SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", MessageId="Error", Justification="For Internal Partners Only")]
#if USEPRIVATE
            private ref class Error sealed
#else
            public ref class Error sealed
#endif
            {
            private:
                System::String ^m_description;
                Microsoft::Compiler::VisualBasic::SourceLocation ^m_sourceLocation;
                int m_errorCode;

            internal:
                Error(int errorCode,
                      System::String ^description,
                      Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation);

            public:
                property int ErrorCode {
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
