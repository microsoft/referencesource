#pragma once

#include "CompilerContext.h"
#include "CompilerResults.h"
#include "VBHostedCompiler.h"
#include "vcclr.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
            class CompilerBridge
            {
            public:
                CompilerBridge(gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^> ^> referenceAssemblies);
                
                virtual ~CompilerBridge();

                void CompileExpression(gcroot<System::String ^> expression, 
                                       gcroot<CompilerContext ^> context,
                                       gcroot<System::Type ^> targetType,
                                       gcroot<CompilerResults ^> results);

                void CompileStatements(gcroot<System::String ^> statements, 
                                       gcroot<CompilerContext ^> context,
                                       gcroot<CompilerResults ^> results);

            private:
                VbHostedCompiler m_VbHostedCompiler;

                // Do not want to support copy semantics for this class, so 
                // declare as private and don't implement...
                CompilerBridge(const CompilerBridge &source);
                CompilerBridge &operator=(const CompilerBridge &source);

                // Flag is used to mark when an exception has occured in a previous 
                // expression compilation. Indicates that subsequent calls to the 
                // same instance of the compiler should fail without attempting, due 
                // to uncertainty around compiler state.
                bool m_fInvalid;
            };
        }
    }
}
