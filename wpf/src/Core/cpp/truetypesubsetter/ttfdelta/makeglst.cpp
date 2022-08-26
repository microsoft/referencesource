/***************************************************************************
 * module: MakeGLst.C
 *
 * author: Louise Pathe [v-lpathe]
 * date:   Nov 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * fill in an array of chars with 1 and 0 for keep and don't keep specific
 * glyphs. the index into the array corresponds to the glyph index
 * use data in GSUB, JSTF, BASE and composite glyphs to expand beyond list
 * of characters given by client to include all related glyphs that may
 * be needed (automap).
 *
 **************************************************************************/
#if 0
Read this email thread in reverse order to see why the ~Backward Compatibility~ workaround is needed in this code.
Because of the NT workaround, you could create a document on an NT machine which would include ONLY the unicode 0xB7. 
If a document is created on an NT machine, the unicode 0xB7 will be in the list of characters to keep. 
The resulting font, when viewed on an Windows95 machine, will show a missing character for WinAnsi character 0xB7, 
as on that system, the unicode 0x2219 character is required, and the font will not contain it.    
----------
From:   Bodin Dresevic
Sent:   Thursday, April 25, 1996 8:34 AM
To:     K.D. Chang (RhoTech); David Michael Silver; Paul Linnerud; Lori Brownell; Greg Hitch----
Cc:     Louise Pathe (Science Information Associ
Subject:    RE: Middle Dot

I don't like to revisit this issue. 

NT has the following workaround in the tt font driver which I put in to be win 3.1 compatible. 
If a font claims to have both unicode code point b7 and 2219 supported through the cmap 
table we do nothing, that is we report the set of supported glyphs to the engine as it is in the cmap.  
However, if the font does not have unicode code point b7 in its cmap table, but it does 
have 2219, then the font driver lies to the engine that unicode b7 is actually supported by the font. 
For such a font, the request to display unicode point b7 results in displaying 2219. I do not remember
which app was broken because we did not have this behavior before. 
Previously we used to treat all fonts the same, that is we would report to the engine the set of 
supported glyphs exactly as in the cmap table. 
bodin

----------
From:  Greg Hitch----
Sent:  Wednesday, April 24, 1996 6:36 PM
To:  K.D. Chang (RhoTech); David Michael Silver; Paul Linnerud; Lori Brownell
Cc:  Louise Pathe (Science Information Associ; Bodin Dresevic
Subject:  RE: Middle Dot

I guess I can understand keeping the .nls file the same for compatibility reasons, but I'm not exactly clear as to why it is "correct the way it is."
For Win 3.1, Win 95, & Win NT,  the character WinANSI 0xB7 has been remapped to U+2219. To me, this was a redefinition of WinANSI that was comparable 
to the work we did when we added the DTP characters in the 0x80+ range of WinANSI.

GregH

----------
From:  Lori Brownell
Sent:  Tuesday, April 23, 1996 8:41 AM
To:  K.D. Chang (RhoTech); David Michael Silver; Greg Hitch----; Paul Linnerud
Cc:  Louise Pathe (Science Information Associ
Subject:  RE: Middle Dot

The .nls file will not change because it's correct the way that it is.  If you wish to have fonts have the glyph associated with U+00B7 be the bullet instead of the middle dot, that is the typography teams decision.

----------
From:   Paul Linnerud
Sent:   jeudi 18 avril 1996 11:10 
To:     K.D. Chang (RhoTech); Lori Brownell; David Michael Silver; Greg Hitch----
Cc:     Louise Pathe (Science Information Associ; Paul Linnerud
Subject:    RE: Middle Dot

Any resolution with respect to what NT will do about this?

Thanks,
Paul

----------
From:   Greg Hitch----
Sent:   Friday, April 05, 1996 8:11 PM
To:     K.D. Chang (RhoTech); Paul Linnerud; Lori Brownell; David Michael Silver
Cc:     Louise Pathe (Science Information Associ
Subject:    RE: Middle Dot

This goes back actually to February 1992,  right before we shipped Win 3.1
The Middle Dot is actually an accent character, used for example with the L dot. As such, it is positioned to the right of the glyph box. In the previous bitmap fonts that Windows shipped, there was a bullet in that position. Word used (uses) this bullet to display space characters when in full view mode, and with the mid dot, it collided with other glyphs at small sizes.
DavidW, EliK, and myself made the decision to remap in Windows to U+2219 and redefine WinAnsi 0xB7 to the bullet. It has been that way ever since.

GregH

----------
From:  David Michael Silver
Sent:  Friday, April 05, 1996 11:23 AM
To:  K.D. Chang (RhoTech); Greg Hitch----; Paul Linnerud; Lori Brownell
Cc:  Louise Pathe (Science Information Associ
Subject:  RE: Middle Dot

0xB7 is the bullet - changed after bugs reported (by the word team I believe).  JohnWin is the one who instansiated the chagne, you should check with him where the info came from.

----------
From:   Lori Brownell
Sent:   Friday, 05 April, 1996 10:44 AM
To:     K.D. Chang (RhoTech); Greg Hitch----; Paul Linnerud; David Michael Silver
Cc:     Louise Pathe (Science Information Associ
Subject:    RE: Middle Dot

It shouldn't and if it does, then it's a Win95 GDI bug.

----------
From:   Paul Linnerud
Sent:   Friday, April 05, 1996 10:34 AM
To:     K.D. Chang (RhoTech); Greg Hitch----
Cc:     Lori Brownell; Paul Linnerud; Louise Pathe (Science Information Associ
Subject:    FW: Middle Dot

I know that Windows 95 WinANSI defines 0xb7 as U+2219. If you output text with the "A" functions, you get this mapping. 

Thanks,
Paul
----------
From:   K.D. Chang (RhoTech)
Sent:   Friday, April 05, 1996 10:14 AM
To:     Paul Linnerud
Cc:     Lori Brownell
Subject:    RE: Middle Dot

Unicode U+00a0 thru U+00ff are defined the same as ANSI  0xa0 thru 0xff (see Unicode Standard Ver. 1.0 Vol. 1 Page 522 - 524).

Can you point out where WinANSI defines 0xb7 as Unicode U+2219 ?

thanks, kd

----------
From:   Paul Linnerud
Sent:   Thursday, April 04, 1996 3:58 PM
To:     Lori Brownell
Cc:     Greg Hitch----; Louise Pathe (Science Information Associ
Subject:    Middle Dot

For code page 1252, the nls file defines code point 0xb7 as Unicode 0x00b7. WinANSI actually defines 0xb7 as Unicode 0x2219. Could you please look into having the nls file changed. 

Thanks,
PaulLi

/**************************************************************************/

#endif

#include <stdlib.h>  /* for min and max functions */

#include "typedefs.h"
#include "ttfacc.h"
#include "ttfcntrl.h"
#include "ttff.h"
#include "ttftabl1.h"
#include "ttftable.h"
#include "ttmem.h"
#include "makeglst.h"
#include "automap.h"
#include "ttferror.h" /* for error codes */
#include "ttfdelta.h"
#include "sfntoff.h"

#define WIN_ANSI_MIDDLEDOT 0xB7
#define WIN_ANSI_BULLET 0x2219

/* ---------------------------------------------------------------------- */
/* Convert an array of codepoints to user space if this is a symbol font  */
/* ---------------------------------------------------------------------- */
[System::Security::SecurityCritical]
int16 UnicodeToSymbols(
TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* ttfacc info */
CONST CHAR_ID *pulKeepCharCodeList, /* list of chars to keep - from client */
CONST uint16 usCharListCount,       /* count of list of chars to keep */
CHAR_ID **ppulKeepSymbolCodeList /* Out: pulKeepCharCodeList converted to symbols */
                                 /*      or NULL if conversion is not needed */
)
{
uint16 i;
uint32 ulOs2Offset;
USHORT usFirstChar;
USHORT usHighByte;

    *ppulKeepSymbolCodeList = NULL;

    if ((ulOs2Offset = TTTableOffset( pInputBufferInfo, OS2_TAG )) != DIRECTORY_ERROR)
    {
        if (ReadWord( pInputBufferInfo, &usFirstChar, ulOs2Offset + SFNT_OS2_USFIRSTCHAR) == NO_ERROR)
        {
            if (usFirstChar >= 0xf000)
            {
                if (*ppulKeepSymbolCodeList = (CHAR_ID *)Mem_Alloc(usCharListCount * sizeof(CHAR_ID)))
                {
                    /* In user range -> this is a symbol font so go ahead offseting it */
                    usHighByte = (unsigned short)(usFirstChar & 0xff00);
                    for ( i=0; i < usCharListCount; i++ )
                    {
                        if (pulKeepCharCodeList[i] <= 0xff)
                            (*ppulKeepSymbolCodeList)[i] = (CHAR_ID)usHighByte + pulKeepCharCodeList[i];
                        else
                            (*ppulKeepSymbolCodeList)[i] = pulKeepCharCodeList[i];
                    }
                }
                else
                {
                    return ERR_MEM;
                }
            }
        }
        else
        {
            return ERR_READOUTOFBOUNDS;
        }
    }
    else
    {
        return ERR_MISSING_OS2;
    }

    return NO_ERROR;
}

/* ------------------------------------------------------------------- */
/* Check if resulting glyph table would be empty for current keep list */
/* and if it is the case, just add first non-empty glyph to the list.  */
/* ------------------------------------------------------------------- */
[System::Security::SecurityCritical]
int16 EnsureNonEmptyGlyfTable(
                        TTFACC_FILEBUFFERINFO * pInputBufferInfo, 
                        uint8 *puchKeepGlyphList, 
                        uint16 usGlyphCount)
{
    uint16 i,iFirstNonBlankGlyph;
    uint32 * aulLoca;

    /* allocate memory for and read loca table */
    aulLoca = (uint32 *)Mem_Alloc( (usGlyphCount + 1) * sizeof( uint32 ));
    if ( aulLoca == NULL )
        return ERR_MEM;

    if (GetLoca(pInputBufferInfo, aulLoca, usGlyphCount + 1) == 0L)
    {
        Mem_Free(aulLoca);
        return ERR_INVALID_LOCA;
    }
    
    /* Check if all glyphs in the keep list are blank, to avoid empty glyf table */
    iFirstNonBlankGlyph = 0xFFFF;
    for ( i = 0; i < usGlyphCount; i++ )
    {
        if ( aulLoca[ i ] < aulLoca[ i+1 ] )
        {

            if (puchKeepGlyphList[i])
            {
                break;
            }
            
            if (iFirstNonBlankGlyph == 0xFFFF)
            {
                iFirstNonBlankGlyph = i;
            }
        }
    }
    if (i == usGlyphCount)
    {
        /* We haven't found any non-blank glyph in keep list, add first non-blank from the font */
        if (iFirstNonBlankGlyph != 0xFFFF)
        {
            puchKeepGlyphList[iFirstNonBlankGlyph] = 1;
        }
        else
        {
            /* All glyphs in original font are blank. How can this font be valid? */
            Mem_Free(aulLoca);
            return ERR_INVALID_GLYF;
        }
    }

    Mem_Free(aulLoca);
    return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
[System::Security::SecurityCritical]
int16 MakeKeepGlyphList(
TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* ttfacc info */
CONST uint16 usListType, /* 0 = character list, 1 = glyph list */
CONST uint16 usPlatform, /* cmap platform to look for */
CONST uint16 usEncoding, /* cmap encoding to look for */
CONST CHAR_ID *pulKeepCharCodeList, /* list of chars to keep - from client */
CONST uint16 usCharListCount,      /* count of list of chars to keep */
uint8 *puchKeepGlyphList, /* pointer to an array of chars representing glyphs 0-usGlyphListCount. */
CONST uint16 usGlyphListCount, /* count of puchKeepGlyphList array */
uint16 *pusMaxGlyphIndexUsed,
uint16 *pusGlyphKeepCount,
ttBoolean bAddRelatedGlyphs /*whether to add related glyphs from GSUB, GPOS, JSTF and BASE*/
)
{
uint16 i,j;
uint16 usGlyphIdx;
uint32 ulGlyphIdx;      /* need a long for the new cmap formats */
FORMAT4_SEGMENTS * Format4Segments=NULL;    /* pointer to Format4Segments array */
FORMAT12_GROUPS  * Format12Groups=NULL;     /* pointer to Format12Groups array  */
GLYPH_ID * GlyphId=NULL;    /* pointer to GlyphID array - for Format4 subtable */
uint16      usnGlyphs;      /* number of entries in the GlyphID array above */
CMAP_FORMAT6 CmapFormat6;     /* cmap subtable headers */
CMAP_FORMAT4 CmapFormat4;
CMAP_FORMAT0 CmapFormat0;
CMAP_FORMAT12 CmapFormat12;
MAXP Maxp;  /* local copy */
HEAD Head;  /* local copy */
uint16 usnComponents;
uint16 usnMaxComponents;
uint16 *pausComponents = NULL;
uint16 usnComponentDepth = 0;   
uint16 usIdxToLocFmt;
uint32 ulLocaOffset;
uint32 ulGlyfOffset;
uint16 *glyphIndexArray; /* for format 6 cmap subtables */
int16 errCode=NO_ERROR;
uint16 usFoundEncoding;
int16 KeepBullet = FALSE;
int16 FoundBullet = FALSE;
uint16 fKeepFlag; 
uint16 usGlyphKeepCount;
uint16 usMaxGlyphIndexUsed;
uint32 ulCmapOffset;
uint16 usBytesRead;
CMAP_SUBHEADER_GEN CmapSubHeader;


    if ( ! GetHead( pInputBufferInfo, &Head ))
        return( ERR_MISSING_HEAD );
    usIdxToLocFmt = Head.indexToLocFormat;

    if ( ! GetMaxp(pInputBufferInfo, &Maxp))
        return( ERR_MISSING_MAXP );
    
    if ((ulLocaOffset = TTTableOffset( pInputBufferInfo, LOCA_TAG )) == DIRECTORY_ERROR)
        return (ERR_MISSING_LOCA);

    if ((ulGlyfOffset = TTTableOffset( pInputBufferInfo, GLYF_TAG )) == DIRECTORY_ERROR)
        return (ERR_MISSING_GLYF);

    usnMaxComponents = Maxp.maxComponentElements * Maxp.maxComponentDepth; /* maximum total possible */
    pausComponents = (uint16 *)Mem_Alloc(usnMaxComponents * sizeof(uint16));
    if (pausComponents == NULL)
        return(ERR_MEM);

    /* fill in array of glyphs to keep.  Glyph 0 is the missing chr glyph,
        glyph 1 is the NULL glyph. Don't violate the array */
    if( bAddRelatedGlyphs )
    {
        if (usGlyphListCount > 0)
            puchKeepGlyphList[ 0 ] = 1;
        if (usGlyphListCount > 1)
            puchKeepGlyphList[ 1 ] = 1;
        if (usGlyphListCount > 2)
            puchKeepGlyphList[ 2 ] = 1;
    }

    if (usListType == TTFDELTA_GLYPHLIST)
    {
        for ( i = 0; i < usCharListCount; i++ )
            if (pulKeepCharCodeList[ i ] < usGlyphListCount)  /* don't violate array ! */
                puchKeepGlyphList[ pulKeepCharCodeList[ i ] ] = 1;
    }
    else
    {
        CHAR_ID *pulUserCharCodeList = NULL;    /* If we need to convert to user space */

        /* need to convert codepoints for symbol fonts */
        errCode = UnicodeToSymbols( pInputBufferInfo, pulKeepCharCodeList, usCharListCount, &pulUserCharCodeList);
        
        if (errCode == NO_ERROR)
        {
            CONST CHAR_ID *pulCharCodeList;

            if (pulUserCharCodeList)
            {
                pulCharCodeList = pulUserCharCodeList;
            }
            else
            {
                pulCharCodeList = pulKeepCharCodeList;
            }
        
            /* Get the offset to Cmap subtable w required platform and encoding */
            ulCmapOffset = FindCmapSubtable( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding );
            if ( ulCmapOffset > 0 )          
            {
                /* read the header to get format */
                errCode = ReadCmapLength( pInputBufferInfo, &CmapSubHeader, ulCmapOffset, &usBytesRead );
                if (errCode == NO_ERROR)
                {
                    switch(CmapSubHeader.format)
                    {
                    case 0:
                        if ((errCode = ReadCmapFormat0( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat0)) != NO_ERROR)
                            break;
                        for (i = 0; i < usCharListCount; ++i)
                        {
                            if ((uint16)pulCharCodeList[ i ] < CMAP_FORMAT0_ARRAYCOUNT)
                            {
                                usGlyphIdx = CmapFormat0.glyphIndexArray[(uint16)pulCharCodeList[ i ]];
                                if (usGlyphIdx < usGlyphListCount)
                                    puchKeepGlyphList[ usGlyphIdx ] = 1;    
                            }
                        }
                        break;

                    case 4:
                        if ((errCode = ReadAllocCmapFormat4( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat4, &Format4Segments, &GlyphId, &usnGlyphs )) != NO_ERROR)
                            break;

                        for ( i = 0; i < usCharListCount; i++ )
                        {
                            usGlyphIdx = GetGlyphIdx( (uint16)pulCharCodeList[ i ], Format4Segments, (uint16)(CmapFormat4.segCountX2 / 2), GlyphId, usnGlyphs );
                            if (usGlyphIdx != 0 && usGlyphIdx != INVALID_GLYPH_INDEX && usGlyphIdx < usGlyphListCount)
                            {
                                /* If the chr code exists, keep the glyph and its components.  Also
                                account for this in the MinMax chr code global. */
                                puchKeepGlyphList[ usGlyphIdx ] = 1;
                                if ((uint16)pulCharCodeList[ i ] == WIN_ANSI_MIDDLEDOT) /* ~Backward Compatibility~! See comment at top of file */
                                    KeepBullet = TRUE;
                                if ((uint16)pulCharCodeList[ i ] == WIN_ANSI_BULLET) /* ~Backward Compatibility~! See comment at top of file */
                                    FoundBullet = TRUE;
                            }
                        }
                        /* ~Backward Compatibility~! See comment at top of file */
                        if ((usPlatform == TTFSUB_MS_PLATFORMID && usFoundEncoding == TTFSUB_UNICODE_CHAR_SET &&
                            KeepBullet && !FoundBullet))  /* need to add that bullet into the list of CharCodes to keep, and glyphs to keep */
                        {
                            usGlyphIdx = GetGlyphIdx( WIN_ANSI_BULLET, Format4Segments, (uint16)(CmapFormat4.segCountX2 / 2), GlyphId, usnGlyphs );
                            if (usGlyphIdx != 0 && usGlyphIdx != INVALID_GLYPH_INDEX && usGlyphIdx < usGlyphListCount)
                            {
                                puchKeepGlyphList[ usGlyphIdx ] = 1;  /* we are keeping 0xB7, so we must make sure to keep 0x2219 */
                            }
                        }
                        FreeCmapFormat4( Format4Segments, GlyphId );
                        break;


                    case 6:
                        {
                        uint16 firstCode;
                        if ((errCode = ReadAllocCmapFormat6( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat6, &glyphIndexArray)) != NO_ERROR)
                            break;
                        firstCode = CmapFormat6.firstCode;

                        for (i = 0; i < usCharListCount; ++i)
                        {
                            if  (((uint16)pulCharCodeList[ i ] >= firstCode) && 
                                ((uint16)pulCharCodeList[ i ] < firstCode + CmapFormat6.entryCount))
                            {
                                usGlyphIdx = glyphIndexArray[(uint16)pulCharCodeList[ i ] - firstCode];
                                if (usGlyphIdx < usGlyphListCount)
                                    puchKeepGlyphList[ usGlyphIdx ] = 1;    
                            }
                        }
                        FreeCmapFormat6(glyphIndexArray);
                        }
                        break;

                    case 12:
                        if ((errCode = ReadAllocCmapFormat12( pInputBufferInfo, ulCmapOffset, &CmapFormat12, &Format12Groups )) != NO_ERROR)
                            break;

                        for ( i = 0; i < usCharListCount; i++ )
                        {
                            ulGlyphIdx = GetGlyphIdx12( pulCharCodeList[ i ], Format12Groups, CmapFormat12.nGroups );
                            usGlyphIdx = (uint16)ulGlyphIdx; /* truncate for now - only support 16-bit */
                            if (ulGlyphIdx != 0 && ulGlyphIdx != INVALID_GLYPH_INDEX_LONG && ulGlyphIdx < (uint32)usGlyphListCount)
                            {
                                /* If the chr code exists, keep the glyph and its components.  Also
                                account for this in the MinMax chr code global. */
                                puchKeepGlyphList[ usGlyphIdx ] = 1;
                                if (pulCharCodeList[ i ] == WIN_ANSI_MIDDLEDOT) /* ~Backward Compatibility~! See comment at top of file */
                                    KeepBullet = TRUE;
                                if (pulCharCodeList[ i ] == WIN_ANSI_BULLET) /* ~Backward Compatibility~! See comment at top of file */
                                    FoundBullet = TRUE;
                            }
                        }
                        /* ~Backward Compatibility~! See comment at top of file */
                        if ((usPlatform == TTFSUB_MS_PLATFORMID && usFoundEncoding == TTFSUB_UNICODE_CHAR_SET &&
                            KeepBullet && !FoundBullet))  /* need to add that bullet into the list of CharCodes to keep, and glyphs to keep */
                        {
                            ulGlyphIdx = GetGlyphIdx12( (uint32)WIN_ANSI_BULLET, Format12Groups, CmapFormat12.nGroups );
                            usGlyphIdx = (uint16)ulGlyphIdx; /* truncate for now - only support 16-bit */
                            if (ulGlyphIdx != 0 && ulGlyphIdx != INVALID_GLYPH_INDEX_LONG && ulGlyphIdx < (uint32)usGlyphListCount)
                            {
                                puchKeepGlyphList[ usGlyphIdx ] = 1;  /* we are keeping 0xB7, so we must make sure to keep 0x2219 */
                            }
                        }
                        FreeCmapFormat12Groups( Format12Groups );
                        break;
                    }
                }
            }

            if (pulUserCharCodeList)
                Mem_Free(pulUserCharCodeList);
        }
    }

    errCode = EnsureNonEmptyGlyfTable(pInputBufferInfo, puchKeepGlyphList, usGlyphListCount);

    *pusGlyphKeepCount = 0;
    *pusMaxGlyphIndexUsed = 0;
    for (fKeepFlag = 1; errCode == NO_ERROR ; ++fKeepFlag)
    {
        usGlyphKeepCount = 0;
        usMaxGlyphIndexUsed = 0;
        /* Now gather up any components referenced by the list of glyphs to keep and TTO glyphs */
        for (usGlyphIdx = 0; usGlyphIdx < usGlyphListCount; ++usGlyphIdx) /* gather up any components */
        {
            if (puchKeepGlyphList[ usGlyphIdx ] == fKeepFlag)
            {
                usMaxGlyphIndexUsed = usGlyphIdx;
                ++ (usGlyphKeepCount);

                GetComponentGlyphList( pInputBufferInfo, usGlyphIdx, &usnComponents, pausComponents, usnMaxComponents, &usnComponentDepth, 0, usIdxToLocFmt, ulLocaOffset, ulGlyfOffset);
                for ( j = 0; j < usnComponents; j++ )   /* check component value before assignment */
                {
                    if ((pausComponents[ j ] < usGlyphListCount) && ((puchKeepGlyphList)[ pausComponents[ j ] ] == 0))
                        (puchKeepGlyphList)[ pausComponents[ j ] ] = (uint8)(fKeepFlag + 1);  /* so it will be grabbed next time around */
                }
            }
        }
        *pusGlyphKeepCount = (uint16)(*pusGlyphKeepCount + usGlyphKeepCount);
        *pusMaxGlyphIndexUsed = max(usMaxGlyphIndexUsed, *pusMaxGlyphIndexUsed);
        if (!usGlyphKeepCount) /* we didn't find any more */
            break;

        if( bAddRelatedGlyphs )
        {
            /* Now gather up any glyphs referenced by GSUB, GPOS, JSTF or BASE tables */
            if ((errCode = TTOAutoMap(pInputBufferInfo, puchKeepGlyphList, usGlyphListCount, fKeepFlag)) != NO_ERROR)  /* Add to the list of KeepGlyphs based on data from GSUB, BASE and JSTF table */
                break;

            if ((errCode = MortAutoMap(pInputBufferInfo, puchKeepGlyphList, usGlyphListCount, fKeepFlag)) != NO_ERROR)  /* Add to the list of KeepGlyphs based on data from Mort table */
                break;
        }
    }
    Mem_Free(pausComponents);
    
    return errCode;
}
/* ---------------------------------------------------------------------- */
