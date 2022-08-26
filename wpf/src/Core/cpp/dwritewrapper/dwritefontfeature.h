#ifndef __FONTFEATURE_H
#define __FONTFEATURE_H

#include "DWriteFontFeatureTag.h"
namespace MS { namespace Internal { namespace Text { namespace TextInterface
{
    
    /// <summary>
    /// Specifies properties used to identify and execute typographic feature in the font.
    ///
    /// COMPATIBILITY WARNING: The layout of this struct must match exactly the layout of DWRITE_FONT_FEATURE from DWrite.h
    /// We will perform an unsafe cast from this type to DWRITE_FONT_FEATURE when obtaining Font Features. We do this to improve perf.
    ///
    /// </summary>
    [StructLayout(LayoutKind::Sequential)]
    private value struct DWriteFontFeature
    {        
        /// <summary>
        /// The feature OpenType name identifier.
        /// </summary>
        DWriteFontFeatureTag nameTag;

        /// <summary>
        /// Execution parameter of the feature.
        /// </summary>
        /// <remarks>
        /// The parameter should be non-zero to enable the feature.  Once enabled, a feature can't be disabled again within
        /// the same range.  Features requiring a selector use this value to indicate the selector index. 
        /// </remarks>
        UINT32 parameter;

        DWriteFontFeature(DWriteFontFeatureTag dwriteNameTag, UINT32 dwriteParameter)
        {
            nameTag = dwriteNameTag;
            parameter = dwriteParameter;
        }
    };
}}}}

#endif //__FONTFEATURE_H
