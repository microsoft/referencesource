#pragma once

#include "OptionCompareSettings.h"
#include "OptionStrictSettings.h"
#include "OptionWarningLevelSettings.h"

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class CompilerOptions sealed
#else
            public ref class CompilerOptions sealed
#endif
            {
            private:
                OptionCompareSetting m_optionCompare;
                OptionStrictSetting m_optionStrict;
                bool m_optionInfer;
                bool m_removeIntChecks;
                OptionWarningLevelSetting m_warningLevel;
                array<int> ^m_ignoreWarnings;
                array<int> ^m_treatWarningsAsErrors;
                array<int> ^m_empty;

            public: 
                CompilerOptions();

                property OptionCompareSetting OptionCompare { 
                    OptionCompareSetting get(); 
                    void set(OptionCompareSetting value); 
                }

                property OptionStrictSetting OptionStrict { 
                    OptionStrictSetting get(); 
                    void set(OptionStrictSetting value); 
                }
                
                property bool OptionInfer { 
                    bool get(); 
                    void set(bool value); 
                }

                property bool RemoveIntChecks { 
                    bool get(); 
                    void set(bool value); 
                }

                property OptionWarningLevelSetting WarningLevel { 
                    OptionWarningLevelSetting get(); 
                    void set(OptionWarningLevelSetting value); 
                }

                [System::Diagnostics::CodeAnalysis::SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Justification="For Internal Partners Only")]
                property array<int> ^IgnoreWarnings {
                    array<int> ^get();
                    void set(array<int> ^value);
                }

                [System::Diagnostics::CodeAnalysis::SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Justification="For Internal Partners Only")]
                property array<int> ^TreatWarningsAsErrors {
                    array<int> ^get();
                    void set(array<int> ^value);
                }
            };
        }
    }
}
