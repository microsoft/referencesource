#ifndef _ITEM_SPAN_H
#define _ITEM_SPAN_H

#include "Common.h"
namespace MS { namespace Internal
{
    private ref struct Span sealed
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="element">Span element</param>
        /// <param name="length">Span length</param>
        Span(Object^ element, int length);

        /// <summary>
        /// Span element
        /// </summary>
        Object^  element;

        /// <summary>
        /// Span length
        /// </summary>
        int      length;
    };
}}//MS::Internal
#endif //_ITEM_SPAN_H
