#pragma once

#include "Error.h"
#include "Warning.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class CompilerResults sealed
#else
            public ref class CompilerResults sealed
#endif
            {
            private:

                System::Linq::Expressions::LambdaExpression ^m_codeBlock;

                System::Collections::Generic::List<Error ^> ^m_errors;
                System::Collections::Generic::List<Warning ^> ^m_warnings;

            internal:
                void AddError(int errorCode, System::String ^description, SourceLocation ^sourceLocation);
                void AddWarning(int warningCode, System::String ^description, SourceLocation ^sourceLocation);
                void SetCodeBlock(System::Linq::Expressions::LambdaExpression ^value);

            public:
                CompilerResults();

                property System::Linq::Expressions::LambdaExpression ^CodeBlock {
                    System::Linq::Expressions::LambdaExpression ^get();
                }

                property System::Collections::Generic::IList<Error ^> ^Errors {
                    System::Collections::Generic::IList<Error ^> ^get();
                }

                property System::Collections::Generic::IList<Warning ^> ^Warnings {
                    System::Collections::Generic::IList<Warning ^> ^get();
                }
            };
        }
    }
}
