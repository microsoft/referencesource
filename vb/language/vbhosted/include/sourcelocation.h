#pragma once

namespace Microsoft 
{ 
    namespace Compiler 
    { 
        namespace VisualBasic
        {
#if USEPRIVATE
            private ref class SourceLocation sealed
#else
            public ref class SourceLocation sealed
#endif
            {
            private:
                int m_columnBegin;
                int m_columnEnd;
                int m_lineBegin;
                int m_lineEnd;

            internal:
                SourceLocation(int columnBegin, 
                               int columnEnd, 
                               int lineBegin, 
                               int lineEnd);

            public:
                property int ColumnBegin {
                    int get();
                }

                property int ColumnEnd {
                    int get();
                }

                property int LineBegin {
                    int get();
                }

                property int LineEnd {
                    int get();
                }
            };
        }
    }
}
