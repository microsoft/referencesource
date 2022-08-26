#include "ItemSpan.h"

namespace MS { namespace Internal
{
    Span::Span(Object^ element, int length)
    {
        this->element = element;
        this->length = length;
    }
}}//MS::Internal
