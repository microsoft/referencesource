// This is the main DLL file.

#include "stdafx.h"

#ifdef DEBUG
#define TRACE
#endif

using namespace Microsoft::Compiler::VisualBasic;

HostedCompiler::HostedCompiler(System::Collections::Generic::IList<System::Reflection::Assembly ^> ^referenceAssemblies)
{
    if(referenceAssemblies == nullptr)
        referenceAssemblies = gcnew System::Collections::Generic::List<System::Reflection::Assembly ^>();

    m_pCompilerBridge = new CompilerBridge(referenceAssemblies);
    
#ifdef TRACE    
    tracePageHeap = gcnew System::Diagnostics::BooleanSwitch("TracePageHeap", "Trace the VB Page Heap after each compilation");
#endif
}

HostedCompiler::~HostedCompiler()
{
    if(m_pCompilerBridge)
        delete m_pCompilerBridge;

    m_pCompilerBridge = 0;
}
    
HostedCompiler::!HostedCompiler()
{
    if(m_pCompilerBridge)
        delete m_pCompilerBridge;

    m_pCompilerBridge = 0;
}

void HostedCompiler::CheckInvalid()
{
    if(!m_pCompilerBridge)
        throw gcnew System::InvalidOperationException();
}

CompilerResults ^HostedCompiler::CompileExpression(System::String ^expression, 
                                                   CompilerContext ^context)
{
    return CompileExpression(expression, context, nullptr);
}

CompilerResults ^HostedCompiler::CompileExpression(System::String ^expression, 
                                                   CompilerContext ^context,
                                                   System::Type ^targetType)
{
    CheckInvalid();

    // Target type cannot be the "Void" type. Check for this and throw.
    //
    // Note that the message for the ArgumentException is an empty string.
    // This is intentional since we will not be able to localize this message.
    //
    if (targetType != nullptr && System::Void::typeid->Equals(targetType))
        throw gcnew System::ArgumentException("", "targetType");

    CompilerResults ^results = gcnew CompilerResults();
    
    // If expression is null or empty, there is nothing to compile.  In this case,
    // we will simply return an empty CompilerResults to the caller.
    if(!System::String::IsNullOrEmpty(expression))
    {
        if(context == nullptr)
            context = CompilerContext::Empty;
    
        m_pCompilerBridge->CompileExpression(expression, context, targetType, results);

#ifdef TRACE
        if(tracePageHeap->Enabled)
        {
            System::Diagnostics::Trace::WriteLine(
                System::String::Format(
                    System::Globalization::CultureInfo::InvariantCulture,
                    "Expression = \"{0}\" :: CurrentReservedSize = {1} :: CurrentUseSize = {2} :: MaxReservedSize = {3} :: MaxUseSize = {4} :: WorkingSet64 = {5} :: PagedMemorySize64 = {6}", 
                    expression, 
                    g_pvbNorlsManager->GetPageHeap().GetCurrentReserveSize(), 
                    g_pvbNorlsManager->GetPageHeap().GetCurrentUseSize(), 
                    g_pvbNorlsManager->GetPageHeap().GetMaxReserveSize(), 
                    g_pvbNorlsManager->GetPageHeap().GetMaxUseSize(),
                    System::Diagnostics::Process::GetCurrentProcess()->WorkingSet64,
                    System::Diagnostics::Process::GetCurrentProcess()->PagedMemorySize64));
        }
#endif
    }
    
    return results;
}
