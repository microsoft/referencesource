#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

CompilerOptions::CompilerOptions() : 
    m_optionCompare(OptionCompareSetting::Binary),
    m_optionStrict(OptionStrictSetting::On),
    m_optionInfer(false),
    m_removeIntChecks(false),
    m_warningLevel(OptionWarningLevelSetting::Regular)
{
    m_empty = m_treatWarningsAsErrors = m_ignoreWarnings = gcnew array<int>(0);
}

OptionCompareSetting CompilerOptions::OptionCompare::get()
{
    return m_optionCompare;
} 

void CompilerOptions::OptionCompare::set(OptionCompareSetting value)
{
    m_optionCompare = value;
}

OptionStrictSetting CompilerOptions::OptionStrict::get()
{
    return m_optionStrict;
}

void CompilerOptions::OptionStrict::set(OptionStrictSetting value)
{
    m_optionStrict = value;
}
        
bool CompilerOptions::OptionInfer::get()
{
    return m_optionInfer;
}

void CompilerOptions::OptionInfer::set(bool value)
{
    m_optionInfer = value;
}

bool CompilerOptions::RemoveIntChecks::get()
{
    return m_removeIntChecks;
}

void CompilerOptions::RemoveIntChecks::set(bool value)
{
    m_removeIntChecks = value;
}

OptionWarningLevelSetting CompilerOptions::WarningLevel::get()
{
    return m_warningLevel;
}

void CompilerOptions::WarningLevel::set(OptionWarningLevelSetting value)
{
    m_warningLevel = value;
}
        
array<int> ^CompilerOptions::IgnoreWarnings::get()
{
    return m_ignoreWarnings;
}

void CompilerOptions::IgnoreWarnings::set(array<int> ^value)
{
    if(value == nullptr || value->Length == 0)
    {
        m_ignoreWarnings = m_empty;
        return;
    }
    m_ignoreWarnings = gcnew array<int>(value->Length);
    value->CopyTo(m_ignoreWarnings, 0);
}

array<int> ^CompilerOptions::TreatWarningsAsErrors::get()
{
    return m_treatWarningsAsErrors;
}

void CompilerOptions::TreatWarningsAsErrors::set(array<int> ^value)
{
    if(value == nullptr || value->Length == 0)
    {
        m_treatWarningsAsErrors = m_empty;
        return;
    }
    m_treatWarningsAsErrors = gcnew array<int>(value->Length);
    value->CopyTo(m_treatWarningsAsErrors, 0);
}
