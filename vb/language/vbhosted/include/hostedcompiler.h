#pragma once

#include "CompilerResults.h"
#include "CompilerContext.h"
#include "CompilerBridge.h"

#ifdef DEBUG
#define TRACE
#endif

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class HostedCompiler sealed
#else
            public ref class HostedCompiler sealed
#endif
	        {
            private:
                CompilerBridge *m_pCompilerBridge;

#ifdef TRACE
				System::Diagnostics::BooleanSwitch ^tracePageHeap;
#endif

            protected:
                !HostedCompiler();

            public:
                HostedCompiler(System::Collections::Generic::IList<System::Reflection::Assembly ^> ^referenceAssemblies);

                ~HostedCompiler();

                void CheckInvalid();
                
                CompilerResults ^CompileExpression(System::String ^expression, 
                                                   CompilerContext ^context);

                CompilerResults ^CompileExpression(System::String ^expression, 
                                                   CompilerContext ^context,
                                                   System::Type ^targetType);
	        };
        }
    }
}
