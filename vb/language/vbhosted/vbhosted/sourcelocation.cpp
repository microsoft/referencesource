#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

SourceLocation::SourceLocation(int columnBegin, int columnEnd, int lineBegin, int lineEnd) :
    m_columnBegin(columnBegin),
    m_columnEnd(columnEnd),
    m_lineBegin(lineBegin),
    m_lineEnd(lineEnd)
{
}

int SourceLocation::ColumnBegin::get()
{
    return m_columnBegin;
}

int SourceLocation::ColumnEnd::get()
{
    return m_columnEnd;
}

int SourceLocation::LineBegin::get()
{
    return m_lineBegin;
}

int SourceLocation::LineEnd::get()
{
    return m_lineEnd;
}
