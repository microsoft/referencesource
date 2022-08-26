/* 


* 11/17/07 - bartde
*
* NOTICE: Code excluded from Developer Reference Sources.
*         Don't remove the SSS_DROP_BEGIN directive on top of the file.
*
* Reason for exclusion: obscure PTLS interface
*
**************************************************************************/















using System;
using System.Diagnostics;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using System.Windows;
using System.Globalization;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.TextFormatting;
using MS.Internal;
using MS.Internal.Shaping;
using MS.Internal.FontCache;

using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

using MS.Internal.Text.TextInterface;





#pragma warning disable 1634, 1691








#pragma warning disable 6500

namespace MS.Internal.TextFormatting
{



    internal sealed class LineServicesCallbacks
    {







































        [SecurityCritical]
        internal unsafe LsErr FetchRunRedefined(
            IntPtr              pols,               // Line Layout context
            int                 lscpFetch,          // position to fetch
            int                 fIsStyle,           // flag indicates if pstyle is given
            IntPtr              pstyle,             // current demanded style
            char*               pwchTextBuffer,     // [in/out] fixed-size character buffer
            int                 cchTextBuffer,      // buffer length in characters
            ref int             fIsBufferUsed,      // [out] Boolean flag indicating the fixed-size buffer is used
            out char*           pwchText,           // [out] pointer to run's character string
            ref int             cchText,            // [out] length of string
            ref int             fIsHidden,          // [out] Is this run hidden?
            ref LsChp           lschp,              // [out] run's character properties
            ref IntPtr          lsplsrun            // [out] fetched run
            )
        {
            LsErr lserr = LsErr.None;
            pwchText = null;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                FullTextState fullTextState = FullText;
                TextStore store = fullTextState.StoreFrom(lscpFetch);

                int lsrunOffset;

                lsrun = store.FetchLSRun(
                    lscpFetch,
                    fullTextState.TextFormattingMode,
                    fullTextState.IsSideways,
                    out plsrun,
                    out lsrunOffset,
                    out cchText
                    );

                fIsBufferUsed = 0;
                pwchText = lsrun.CharacterBuffer.GetCharacterPointer();

                if (pwchText == null)
                {












                    if (cchText <= cchTextBuffer)
                    {
                        Invariant.Assert(pwchTextBuffer != null);

                        int j = lsrun.OffsetToFirstChar + lsrunOffset;
                        for (int i = 0; i < cchText; i++, j++)
                        {
                            pwchTextBuffer[i] = lsrun.CharacterBuffer[j];
                        }

                        fIsBufferUsed = 1;
                    }
                    else
                    {
                        return LsErr.None;
                    }
                }
                else
                {
                    pwchText += lsrun.OffsetToFirstChar + lsrunOffset;
                }


                lschp = new LsChp();
                fIsHidden = 0;

                switch (lsrun.Type)
                {
                    case Plsrun.Reverse:
                        lschp.idObj = (ushort)TextStore.ObjectId.Reverse;
                        break;

                    case Plsrun.FormatAnchor:
                    case Plsrun.CloseAnchor:
                        lschp.idObj = (ushort)TextStore.ObjectId.Text_chp;
                        break;

                    case Plsrun.InlineObject:
                        lschp.idObj = (ushort)TextStore.ObjectId.InlineObject;
                        SetChpFormat(lsrun.RunProp, ref lschp);
                        break;

                    case Plsrun.Hidden:
                        lschp.idObj = (ushort)TextStore.ObjectId.Text_chp;
                        fIsHidden = 1;
                        break;

                    case Plsrun.Text:
                        {
                            Debug.Assert(TextStore.IsContent(plsrun), "Unrecognizable run!");
                            Debug.Assert(lsrun.RunProp != null, "invalid lsrun!");

                            lschp.idObj = (ushort)TextStore.ObjectId.Text_chp;

                            if (    lsrun.Shapeable != null
                                &&  lsrun.Shapeable.IsShapingRequired)
                            {
                                lschp.flags |= LsChp.Flags.fGlyphBased;

                                if (lsrun.Shapeable.NeedsMaxClusterSize)
                                {









                                    lschp.dcpMaxContent = lsrun.Shapeable.MaxClusterSize;
                                }
                            }

                            SetChpFormat(lsrun.RunProp, ref lschp);


                            Invariant.Assert(!TextStore.IsNewline(lsrun.CharacterAttributeFlags));
                            break;
                        }

                    default :

                        lschp.idObj = (ushort)TextStore.ObjectId.Text_chp;
                        store.CchEol = lsrun.Length;
                        break;                                            
                }


                if (    lsrun.Type == Plsrun.Text
                    ||  lsrun.Type == Plsrun.InlineObject)
                {

                    Debug.Assert(lsrun.RunProp != null);

                    if (    lsrun.RunProp != null
                        &&  lsrun.RunProp.BaselineAlignment != BaselineAlignment.Baseline)
                    {
                        FullText.VerticalAdjust = true;
                    }
                }


                lsplsrun = (IntPtr)plsrun;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("FetchRunRedefined", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }





        [SecurityCritical,SecurityTreatAsSafe]
        private void SetChpFormat(
            TextRunProperties   runProp,
            ref LsChp           lschp
            )
        {
            SetChpFormat(runProp.TextDecorations, ref lschp);
            SetChpFormat(FullText.TextStore.Pap.TextDecorations, ref lschp);
        }

        private void SetChpFormat(
            TextDecorationCollection    textDecorations,
            ref LsChp                   lschp
            )
        {

            if (textDecorations != null)
            {

                for (int i = 0; i < textDecorations.Count; i++)
                {
                    switch (textDecorations[i].Location)
                    {
                        case TextDecorationLocation.Underline:
                            lschp.flags |= LsChp.Flags.fUnderline;
                            break;

                        case TextDecorationLocation.OverLine:
                        case TextDecorationLocation.Strikethrough:
                        case TextDecorationLocation.Baseline:
                            lschp.flags |= LsChp.Flags.fStrike;
                            break;
                    }
                }
            }
        }





        [SecurityCritical]
        internal LsErr FetchPap(
            IntPtr      pols,           // Line Layout context
            int         lscpFetch,      // position to fetch
            ref LsPap   lspap           // [out] paragraph properties
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                lspap = new LsPap();

                TextStore store = FullText.StoreFrom(lscpFetch);

                lspap.cpFirst = lspap.cpFirstContent = lscpFetch;   // note: LS doesnt really care
                lspap.lskeop = LsKEOP.lskeopEndPara1;







                lspap.grpf |= LsPap.Flags.fFmiTreatHyphenAsRegular;

                ParaProp pap = store.Pap;

                if (FullText.ForceWrap)
                {
                    lspap.grpf |= LsPap.Flags.fFmiApplyBreakingRules;
                }
                else if (pap.Wrap)
                {
                    lspap.grpf |= LsPap.Flags.fFmiApplyBreakingRules;

                    if (!pap.EmergencyWrap)
                    {
                        lspap.grpf |= LsPap.Flags.fFmiForceBreakAsNext;
                    }

                    if (pap.Hyphenator != null)
                    {
                        lspap.grpf |= LsPap.Flags.fFmiAllowHyphenation;
                    }
                }

                if (pap.FirstLineInParagraph)
                {
                    lspap.cpFirstContent = store.CpFirst;
                    lspap.cpFirst = lspap.cpFirstContent;

                    if (FullText.TextMarkerStore != null)
                    {
                        lspap.grpf |= LsPap.Flags.fFmiAnm;
                    }
                }

                lspap.fJustify = (pap.Justify ? 1 : 0);

                if (pap.Wrap && pap.OptimalBreak)
                {
                    lspap.lsbrj = LsBreakJust.lsbrjBreakOptimal;
                    lspap.lskj = LsKJust.lskjFullMixed;
                }
                else
                {
                    lspap.lsbrj = LsBreakJust.lsbrjBreakJustify;
                    if (pap.Justify)
                    {
                        lspap.lskj = LsKJust.lskjFullInterWord;
                    }
                }

                lspap.lstflow = pap.RightToLeft ? LsTFlow.lstflowWS : LsTFlow.lstflowES;
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("FetchPap", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }




        [SecurityCritical]
        internal LsErr FetchLineProps(
            IntPtr              pols,               // Line Layout context
            int                 lscpFetch,          // character position to fetch
            int                 firstLineInPara,    // (bool) whether this the first line in paragraph
            ref LsLineProps     lsLineProps         // [out] line properties
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                TextStore store = FullText.TextStore;
                TextStore markerStore = FullText.TextMarkerStore;
                ParaProp pap = store.Pap;
                FormatSettings settings = store.Settings;

                lsLineProps = new LsLineProps();

                if (FullText.GetMainTextToMarkerIdealDistance() != 0)
                    lsLineProps.durLeft = TextFormatterImp.RealToIdeal(markerStore.Pap.TextMarkerProperties.Offset);
                else
                    lsLineProps.durLeft = settings.TextIndent;

                if (    pap.Wrap 
                    &&  pap.OptimalBreak
                    &&  settings.MaxLineWidth < FullText.FormatWidth)
                {

                    lsLineProps.durRightBreak = lsLineProps.durRightJustify = (FullText.FormatWidth -  settings.MaxLineWidth);
                }
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("FetchLineProps", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }





        [SecurityCritical]
        internal LsErr GetRunTextMetrics(
            System.IntPtr       pols,           // Line Layout context
            Plsrun              plsrun,         // plsrun
            LsDevice            lsDevice,       // kind of device
            LsTFlow             lstFlow,        // text flow
            ref LsTxM           lstTextMetrics  // [out] returning metrics
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                FullTextState fullText = FullText;
                TextStore store = fullText.StoreFrom(plsrun);
                lsrun = store.GetRun(plsrun);

                if (lsrun.Height > 0)
                {
                    lstTextMetrics.dvAscent = lsrun.BaselineOffset;
                    lstTextMetrics.dvMultiLineHeight = lsrun.Height;
                }
                else
                {
                    Typeface typeface = store.Pap.DefaultTypeface;
                    lstTextMetrics.dvAscent = (int)Math.Round(typeface.Baseline(store.Pap.EmSize, Constants.DefaultIdealToReal, store.Settings.TextSource.PixelsPerDip, fullText.TextFormattingMode));
                    lstTextMetrics.dvMultiLineHeight = (int)Math.Round(typeface.LineSpacing(store.Pap.EmSize, Constants.DefaultIdealToReal, store.Settings.TextSource.PixelsPerDip, fullText.TextFormattingMode));
                }

                lstTextMetrics.dvDescent = lstTextMetrics.dvMultiLineHeight - lstTextMetrics.dvAscent;
                lstTextMetrics.fMonospaced = 0;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetRunTextMetrics", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }







        [SecurityCritical]
        internal unsafe LsErr GetRunCharWidths(
            IntPtr          pols,               // Line Layout context
            Plsrun          plsrun,             // plsrun
            LsDevice        device,             // kind of device
            char*           charString,         // character string
            int             stringLength,       // string length
            int             maxWidth,           // max width allowance
            LsTFlow         textFlow,           // text flow
            int*            charWidths,         // [out] returning char widths up to given upperbound
            ref int         totalWidth,         // [out] total run width
            ref int         stringLengthFitted  // [out] number of char fitted
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                TextFormatterImp formatter;

                if (FullText != null)
                {
                    lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);
                    formatter = FullText.Formatter;
                }
                else
                {








                    #if DEBUG



                    Debug.Assert(Draw.CurrentLine.FullTextState != null);
                    #endif
                    lsrun = Draw.CurrentLine.GetRun(plsrun);
                    formatter = Draw.CurrentLine.Formatter;
                }

                if (lsrun.Type == Plsrun.Text)
                {
                    Debug.Assert(lsrun.Shapeable != null && stringLength > 0);
                    lsrun.Shapeable.GetAdvanceWidthsUnshaped(charString, stringLength, TextFormatterImp.ToIdeal, charWidths);

                    totalWidth = 0;
                    stringLengthFitted = 0;

                    do
                    {
                        totalWidth += charWidths[stringLengthFitted];

                    } while (
                            ++stringLengthFitted < stringLength
                        && totalWidth <= maxWidth
                        );

                    if (totalWidth <= maxWidth && FullText != null)
                    {
                        int cpLimit = lsrun.OffsetToFirstCp + stringLengthFitted;
                        if (cpLimit > FullText.CpMeasured)
                        {
                            FullText.CpMeasured = cpLimit;
                        }
                    }
                }
                else
                {

                    charWidths[0] = 0;
                    totalWidth = 0;
                    stringLengthFitted = stringLength;
                }
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetRunCharWidths", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }




        [SecurityCritical]
        internal LsErr GetDurMaxExpandRagged(
            IntPtr      pols,               // Line Layout context
            Plsrun      plsrun,             // plsrun
            LsTFlow     lstFlow,            // text flow
            ref int     maxExpandRagged     // [out] em width
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {



                lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);
                maxExpandRagged = lsrun.EmSize;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetDurMaxExpandRagged", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }





        [SecurityCritical]
        internal LsErr GetAutoNumberInfo(
            IntPtr          pols,               // Line Layout context
            ref LsKAlign    alignment,          // [out] Marker alignment
            ref LsChp       lschp,              // [out] Marker properties
            ref IntPtr      lsplsrun,           // [out] Marker run
            ref ushort      addedChar,          // [out] Character to add after marker
            ref LsChp       lschpAddedChar,     // [out] Added character properties
            ref IntPtr      lsplsrunAddedChar,  // [out] Added character run
            ref int         fWord95Model,        // [out] true iff follow Word95 autonumbering model
            ref int         offset,             // [out] Offset from marker to start of main text (relevant iff word95Model is true)
            ref int         width               // [out] Offset from margin to start of main text (relevant iff word95Model is true)
            )
        {
            LsErr lserr = LsErr.None;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                FullTextState fullTextState = FullText;
                TextStore markerStore = fullTextState.TextMarkerStore;
                TextStore store = fullTextState.TextStore;
                Debug.Assert(markerStore != null, "No marker store, yet autonumbering is specified!");

                int lscp = TextStore.LscpFirstMarker;
                int lsrunLength;

                do
                {
                    int lsrunOffset;

                    lsrun = markerStore.FetchLSRun(
                        lscp, 
                        fullTextState.TextFormattingMode,
                        fullTextState.IsSideways,
                        out plsrun, 
                        out lsrunOffset,
                        out lsrunLength
                        );

                    lscp += lsrunLength;

                } while (!TextStore.IsContent(plsrun));

                alignment = LsKAlign.lskalRight;

                lschp = new LsChp();
                lschp.idObj = (ushort)TextStore.ObjectId.Text_chp;

                SetChpFormat(lsrun.RunProp, ref lschp);

                addedChar = FullText.GetMainTextToMarkerIdealDistance() != 0 ? (ushort)'\t' : (ushort)0;

                lschpAddedChar = lschp;

                fWord95Model = 0;   // Word95 model requires precise marker width in which we never have
                offset = 0;         // marker offset is controlled by tab stop
                width = 0;

                lsplsrun = (IntPtr)plsrun;
                lsplsrunAddedChar = lsplsrun;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetAutoNumberInfo", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }




        [SecurityCritical]
        internal LsErr GetRunUnderlineInfo(
            IntPtr          pols,           // Line Layout context
            Plsrun          plsrun,         // plsrun
            ref LsHeights   lsHeights,      // run height
            LsTFlow         textFlow,       // text flow direction
            ref LsULInfo    ulInfo          // [out] result underline info
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                lsrun = Draw.CurrentLine.GetRun(plsrun);

                Debug.Assert(
                    !TextStore.IsContent(plsrun) || lsrun.Type == Plsrun.Text || lsrun.Type == Plsrun.InlineObject,
                    "Invalid run"
                    );

                ulInfo = new LsULInfo();

                double underlinePositionInEm;
                double underlineThicknessInEm;

                if (lsrun.Shapeable != null)
                {
                    underlinePositionInEm = lsrun.Shapeable.UnderlinePosition;
                    underlineThicknessInEm = lsrun.Shapeable.UnderlineThickness;
                }
                else
                {

                    underlinePositionInEm = lsrun.RunProp.Typeface.UnderlinePosition;
                    underlineThicknessInEm = lsrun.RunProp.Typeface.UnderlineThickness;
                }

                ulInfo.cNumberOfLines = 1;
                ulInfo.dvpFirstUnderlineOffset = (int)Math.Round(lsrun.EmSize * -underlinePositionInEm);
                ulInfo.dvpFirstUnderlineSize = (int)Math.Round(lsrun.EmSize * underlineThicknessInEm);






                Debug.Assert(ulInfo.dvpFirstUnderlineSize >= 0);

                if (ulInfo.dvpFirstUnderlineSize <= 0)
                    ulInfo.dvpFirstUnderlineSize = 1;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetAutoNumberInfo", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }




        [SecurityCritical]
        internal LsErr GetRunStrikethroughInfo(
            IntPtr          pols,           // Line Layout context
            Plsrun          plsrun,         // plsrun
            ref LsHeights   lsHeights,      // run height
            LsTFlow         textFlow,       // text flow direction
            ref LsStInfo    stInfo          // [out] result strikethrough info
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                lsrun = Draw.CurrentLine.GetRun(plsrun);

                Debug.Assert(
                    !TextStore.IsContent(plsrun) || lsrun.Type == Plsrun.Text || lsrun.Type == Plsrun.InlineObject,
                    "Invalid run"
                    );

                stInfo = new LsStInfo();

                double strikeThroughPositionInEm;
                double strikeThroughThicknessInEm;

                GetLSRunStrikethroughMetrics(lsrun, out strikeThroughPositionInEm, out strikeThroughThicknessInEm);

                stInfo.cNumberOfLines = 1;
                stInfo.dvpLowerStrikethroughOffset = (int)Math.Round(lsrun.EmSize * strikeThroughPositionInEm);
                stInfo.dvpLowerStrikethroughSize = (int)Math.Round(lsrun.EmSize * strikeThroughThicknessInEm);

                Debug.Assert(stInfo.dvpLowerStrikethroughSize >= 0);





                if (stInfo.dvpLowerStrikethroughSize <= 0)
                    stInfo.dvpLowerStrikethroughSize = 1;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetRunStrikethroughInfo", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }


        private void GetLSRunStrikethroughMetrics(
            LSRun       lsrun,
            out double  strikeThroughPositionInEm,
            out double  strikeThroughThicknessInEm
            )
        {
            if (lsrun.Shapeable != null)
            {
                strikeThroughPositionInEm = lsrun.Shapeable.StrikethroughPosition;
                strikeThroughThicknessInEm = lsrun.Shapeable.StrikethroughThickness;
            }
            else
            {

                strikeThroughPositionInEm = lsrun.RunProp.Typeface.StrikethroughPosition;
                strikeThroughThicknessInEm = lsrun.RunProp.Typeface.StrikethroughThickness;
            }
        }





        [SecurityCritical]
        internal LsErr Hyphenate(
            IntPtr          pols,                   // Line Layout context
            int             fLastHyphenFound,       // whether last hyphen found?
            int             lscpLastHyphen,         // cp of the last found hyphen
            ref LsHyph      lastHyph,               // [in] last found hyphenation
            int             lscpWordStart,          // first character of word
            int             lscpExceed,             // first character in this word that exceeds column
            ref int         fHyphenFound,           // [out] hyphenation opportunity found?
            ref int         lscpHyphen,             // [out] cp of the character before hyphen
            ref LsHyph      lsHyph                  // [out] hyphen info
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                fHyphenFound = FullText.FindNextHyphenBreak(
                    lscpWordStart,
                    lscpExceed - lscpWordStart,
                    true,   // isCurrentAtWordStart
                    ref lscpHyphen,
                    ref lsHyph
                    ) ? 1 : 0;

                Invariant.Assert(fHyphenFound == 0 || (lscpHyphen >= lscpWordStart && lscpHyphen < lscpExceed));
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("Hyphenate", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }

        



        [SecurityCritical]
        internal LsErr GetNextHyphenOpp(
            IntPtr              pols,                   // Line Layout context
            int                 lscpStartSearch,        // LSCP to start search for hyphen opportunity
            int                 lsdcpSearch,            // number of LSCP to look for the hyphen opportunity
            ref int             fHyphenFound,           // [out] hyphen found
            ref int             lscpHyphen,             // [out] LSCP of character before hyphen
            ref LsHyph          lsHyph                  // [out] hyphen info
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                fHyphenFound = FullText.FindNextHyphenBreak(
                    lscpStartSearch,
                    lsdcpSearch,
                    false,  // !isCurrentAtWordStart
                    ref lscpHyphen,
                    ref lsHyph
                    ) ? 1 : 0;

                Invariant.Assert(fHyphenFound == 0 || (lscpHyphen >= lscpStartSearch && lscpHyphen < lscpStartSearch + lsdcpSearch));
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetNextHyphenOpp", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }

        



        [SecurityCritical]
        internal LsErr GetPrevHyphenOpp(
            IntPtr              pols,                   // Line Layout context
            int                 lscpStartSearch,        // LSCP to start search for hyphen opportunity
            int                 lsdcpSearch,            // number of LSCP to look for the hyphen opportunity
            ref int             fHyphenFound,           // [out] hyphen found
            ref int             lscpHyphen,             // [out] LSCP of character before hyphen
            ref LsHyph          lsHyph                  // [out] hyphen info
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                fHyphenFound = FullText.FindNextHyphenBreak(





                    lscpStartSearch + 1,
                    -lsdcpSearch,
                    false,  // !isCurrentAtWordStart
                    ref lscpHyphen,
                    ref lsHyph
                    ) ? 1 : 0;

                Invariant.Assert(fHyphenFound == 0 || (lscpHyphen > lscpStartSearch - lsdcpSearch && lscpHyphen <= lscpStartSearch));
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetPrevHyphenOpp", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }






        [SecurityCritical]
        internal LsErr DrawStrikethrough(
            IntPtr          pols,           // Line Layout context
            Plsrun          plsrun,         // plsrun
            uint            stType,         // kind of strike
            ref LSPOINT     ptOrigin,       // [in] drawing origin
            int             stLength,       // strike length
            int             stThickness,    // strike thickness
            LsTFlow         textFlow,       // text flow direction
            uint            displayMode,    // display mode
            ref LSRECT      clipRect        // [in] clipping rectangle
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;
            try
            {
                if (!TextStore.IsContent(plsrun))
                {

                    return LsErr.None;
                }

                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);

                double strikeThroughPositionInEm;
                double strikeThroughThicknessInEm;

                GetLSRunStrikethroughMetrics(lsrun, out strikeThroughPositionInEm, out strikeThroughThicknessInEm);

                int baselineTop = ptOrigin.y + (int)Math.Round(lsrun.EmSize * strikeThroughPositionInEm);
                int overlineTop = baselineTop - (lsrun.BaselineOffset - (int)Math.Round(lsrun.EmSize * strikeThroughThicknessInEm));

                const uint locationMask = 
                    (1U << (int)TextDecorationLocation.OverLine) |
                    (1U << (int)TextDecorationLocation.Strikethrough) |
                    (1U << (int)TextDecorationLocation.Baseline);

                DrawTextDecorations(
                    lsrun,
                    locationMask,
                    ptOrigin.x,   // left
                    0,            // underline top; not used
                    overlineTop,
                    ptOrigin.y,   // strikethrough top from LS
                    baselineTop,
                    stLength,
                    stThickness,
                    textFlow
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("DrawStrikethrough", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }






        [SecurityCritical]
        internal LsErr DrawUnderline(
            IntPtr pols,                // Line Layout context
            Plsrun      plsrun,         // plsrun
            uint        ulType,         // kind of underline
            ref LSPOINT ptOrigin,       // [in] drawing origin
            int         ulLength,       // underline length
            int         ulThickness,    // underline thickness
            LsTFlow     textFlow,       // text flow direction
            uint        displayMode,    // display mode
            ref LSRECT  clipRect        // [in] clipping rectangle
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;
            try
            {
                if (!TextStore.IsContent(plsrun))
                {

                    return LsErr.None;
                }

                lsrun = Draw.CurrentLine.GetRun(plsrun);

                const uint locationMask = (1U << (int)TextDecorationLocation.Underline);

                DrawTextDecorations(
                    lsrun,
                    locationMask,
                    ptOrigin.x,   // left
                    ptOrigin.y,   // underline top from LS
                    0,            // overline top; not used
                    0,            // strikethrough top; not used
                    0,            // baseline top; not used
                    ulLength,
                    ulThickness,
                    textFlow
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("DrawUnderline", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }





        [SecurityCritical]
        private void DrawTextDecorations(
            LSRun    lsrun,
            uint     locationMask,
            int      left,
            int      underlineTop,
            int      overlineTop,
            int      strikethroughTop,
            int      baselineTop,
            int      length,
            int      thickness,
            LsTFlow  textFlow
            )
        {
            TextMetrics.FullTextLine currentLine = Draw.CurrentLine;


            TextDecorationCollection textDecorations = currentLine.TextDecorations;
            if (textDecorations != null)
            {
                DrawTextDecorationCollection(
                    lsrun,
                    locationMask,
                    textDecorations,
                    currentLine.DefaultTextDecorationsBrush,
                    left,
                    underlineTop,
                    overlineTop,
                    strikethroughTop,
                    baselineTop,
                    length,
                    thickness,
                    textFlow
                    );
            }


            textDecorations = lsrun.RunProp.TextDecorations;
            if (textDecorations != null)
            {
                DrawTextDecorationCollection(
                    lsrun,
                    locationMask,
                    textDecorations,
                    lsrun.RunProp.ForegroundBrush,
                    left,
                    underlineTop,
                    overlineTop,
                    strikethroughTop,
                    baselineTop,
                    length,
                    thickness,
                    textFlow
                    );
            }
        }





        [SecurityCritical]
        private void DrawTextDecorationCollection(
            LSRun                     lsrun,
            uint                      locationMask,
            TextDecorationCollection  textDecorations,
            Brush                     foregroundBrush,
            int                       left,
            int                       underlineTop,
            int                       overlineTop,
            int                       strikethroughTop,
            int                       baselineTop,
            int                       length,
            int                       thickness,
            LsTFlow                   textFlow
            )
        {
            Invariant.Assert(textDecorations != null);

            foreach (TextDecoration td in textDecorations)
            {
                if (((1U << (int)td.Location) & locationMask) != 0)
                {
                    switch (td.Location)
                    {
                        case TextDecorationLocation.Underline:
                            _boundingBox.Union(
                                DrawTextDecoration(
                                    lsrun,
                                    foregroundBrush,
                                    new LSPOINT(left, underlineTop),
                                    length,
                                    thickness,
                                    textFlow,
                                    td
                                    )
                                );
                            break;

                        case TextDecorationLocation.OverLine:
                            _boundingBox.Union(
                                DrawTextDecoration(
                                    lsrun,
                                    foregroundBrush,
                                    new LSPOINT(left, overlineTop),
                                    length,
                                    thickness,
                                    textFlow,
                                    td
                                    )
                                );
                            break;

                        case TextDecorationLocation.Strikethrough:
                            _boundingBox.Union(
                                DrawTextDecoration(
                                    lsrun,
                                    foregroundBrush,
                                    new LSPOINT(left, strikethroughTop),
                                    length,
                                    thickness,
                                    textFlow,
                                    td
                                    )
                                );
                            break;

                        case TextDecorationLocation.Baseline:
                            _boundingBox.Union(
                                DrawTextDecoration(
                                    lsrun,
                                    foregroundBrush,
                                    new LSPOINT(left, baselineTop),
                                    length,
                                    thickness,
                                    textFlow,
                                    td
                                    )
                                );
                            break;
                    }
                }
            }
        }








        [SecurityCritical]
        private Rect DrawTextDecoration(
            LSRun           lsrun,           // lsrun
            Brush           foregroundBrush, // default brush if text decoration has no pen
            LSPOINT         ptOrigin,        // drawing origin
            int             ulLength,        // underline length
            int             ulThickness,     // underline thickness
            LsTFlow         textFlow,        // text flow direction
            TextDecoration  textDecoration   //TextDecoration to be draw (add to sublinecollection
            )
        {
            switch (textFlow)
            {
                case LsTFlow.lstflowWS:
                case LsTFlow.lstflowNE:
                case LsTFlow.lstflowNW:

                    ptOrigin.x -= ulLength;
                    break;
            }

            TextMetrics.FullTextLine currentLine = Draw.CurrentLine;

            if (currentLine.RightToLeft)
            {
                ptOrigin.x = -ptOrigin.x;
            }

            int u = currentLine.LSLineUToParagraphU(ptOrigin.x);

            Point baselineOrigin = LSRun.UVToXY(
                Draw.LineOrigin,
                Draw.VectorToLineOrigin,
                u,
                currentLine.BaselineOffset,
                currentLine
                );

            Point lineOrigin = LSRun.UVToXY(
                Draw.LineOrigin,
                Draw.VectorToLineOrigin,
                u,
                ptOrigin.y + lsrun.BaselineMoveOffset,
                currentLine
                );



            double penThickness = 1.0;
            if (textDecoration.Pen != null)
            {
                penThickness = textDecoration.Pen.Thickness;
            }


            switch (textDecoration.PenThicknessUnit)
            {
                case TextDecorationUnit.FontRecommended:

                    penThickness = currentLine.Formatter.IdealToReal(ulThickness * penThickness, currentLine.PixelsPerDip);
                    break;

                case TextDecorationUnit.FontRenderingEmSize:
                    penThickness = currentLine.Formatter.IdealToReal(penThickness * lsrun.EmSize, currentLine.PixelsPerDip);
                    break;

                case TextDecorationUnit.Pixel:

                    break;

                default:
                    Debug.Assert(false, "Not supported TextDecorationUnit");
                    break;
            }


            penThickness = Math.Abs(penThickness);            




            double unitValue = 1.0;
            switch (textDecoration.PenOffsetUnit)
            {
                case TextDecorationUnit.FontRecommended:

                    unitValue = (lineOrigin.Y - baselineOrigin.Y);
                    break;

                case TextDecorationUnit.FontRenderingEmSize:
                    unitValue = currentLine.Formatter.IdealToReal(lsrun.EmSize, currentLine.PixelsPerDip);
                    break;

                case TextDecorationUnit.Pixel:
                    unitValue = 1.0;
                    break;

                default:
                    Debug.Assert(false, "Not supported TextDecorationUnit");
                    break;
            }


            double lineLength = currentLine.Formatter.IdealToReal(ulLength, currentLine.PixelsPerDip);

            DrawingContext drawingContext = Draw.DrawingContext;

            if (drawingContext != null)
            {      
                
                


                double drawingPenThickness = penThickness;



                Point  drawingLineOrigin   = lineOrigin;

                bool animated = !textDecoration.CanFreeze && (unitValue != 0);

                int pushCount = 0; // counter for the number of explicit DrawingContext.Push()
                

                Draw.SetGuidelineY(baselineOrigin.Y);                
                
                try 
                {
                    if (animated)
                    {                    









                        ScaleTransform scaleTransform = new ScaleTransform(
                            1.0,        // X scale
                            unitValue,  // y scale 
                            drawingLineOrigin.X,  // reference point of scaling
                            drawingLineOrigin.Y  // reference point of scaling
                            );
                           
                        TranslateTransform yTranslate = new TranslateTransform(
                            0,                       // x translate
                            textDecoration.PenOffset // y translate
                            );


                        drawingPenThickness = drawingPenThickness / Math.Abs(unitValue);
                                                    

                        drawingContext.PushTransform(scaleTransform);
                        pushCount++;
                        drawingContext.PushTransform(yTranslate);
                        pushCount++;

                    }
                    else
                    {

                        drawingLineOrigin.Y += unitValue * textDecoration.PenOffset;
                    }                    







                    drawingContext.PushGuidelineY2(baselineOrigin.Y, drawingLineOrigin.Y - drawingPenThickness * 0.5 - baselineOrigin.Y);
                    pushCount++;









                    if (textDecoration.Pen == null)
                    {

                        drawingContext.DrawRectangle(
                            foregroundBrush,               // fill using foreground
                            null,                          // null pen for rectangle stroke
                            new Rect(
                                drawingLineOrigin.X,
                                drawingLineOrigin.Y - drawingPenThickness * 0.5,
                                lineLength,
                                drawingPenThickness
                                )                    
                            );                    
                    }
                    else                    
                    {





                        Pen textDecorationPen = textDecoration.Pen.CloneCurrentValue();
                        if (Object.ReferenceEquals(textDecoration.Pen, textDecorationPen))
                        {

                            textDecorationPen = textDecoration.Pen.Clone();
                        }
                        
                        textDecorationPen.Thickness = drawingPenThickness;                  
                        

                        drawingContext.DrawLine(
                            textDecorationPen,
                            drawingLineOrigin,
                            new Point(drawingLineOrigin.X + lineLength, drawingLineOrigin.Y)
                            );
                    }
                }
                finally 
                {               
                    for (int i = 0; i < pushCount; i++)
                    {
                        drawingContext.Pop(); 
                    }

                    Draw.UnsetGuidelineY();
                }
            }
            
            return new Rect(
                lineOrigin.X,
                lineOrigin.Y + unitValue * textDecoration.PenOffset - penThickness * 0.5,
                lineLength,
                penThickness
                );
        }






        [SecurityCritical]
        internal unsafe LsErr DrawTextRun(
            IntPtr          pols,               // Line Layout context
            Plsrun          plsrun,             // plsrun
            ref LSPOINT     ptText,             // [in] text origin
            char*           pwchText,           // character string
            int*            piCharAdvances,     // char advance widths
            int             cchText,            // text length
            LsTFlow         textFlow,           // text flow
            uint            displayMode,        // draw in transparent or opaque
            ref LSPOINT     ptRun,              // [in] run origin
            ref LsHeights   lsHeights,          // [in] run height
            int             dupRun,             // run length
            ref LSRECT      clipRect            // [in] from DisplayLine's clip rectangle param
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);

                GlyphRun glyphRun = ComputeUnshapedGlyphRun(
                    lsrun, 
                    textFlow, 
                    currentLine.Formatter,
                    true,       // origin of the glyph run provided at drawing time                    
                    ptText, 
                    dupRun, 
                    cchText, 
                    pwchText, 
                    piCharAdvances,
                    currentLine.IsJustified
                    );

                if (glyphRun != null)
                {
                    DrawingContext drawingContext = Draw.DrawingContext;

                    Draw.SetGuidelineY(glyphRun.BaselineOrigin.Y);                    

                    try 
                    {
                        _boundingBox.Union(
                            lsrun.DrawGlyphRun(
                                drawingContext, 
                                null,   // draw with the run's foreground brush
                                glyphRun
                                )
                            );
                    }
                    finally
                    {
                        Draw.UnsetGuidelineY();
                    }
                }
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("DrawTextRun", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }





        [SecurityCritical]
        internal LsErr FInterruptShaping(
            IntPtr          pols,               // Line Layout context
            LsTFlow         textFlow,           // text flow
            Plsrun          plsrunFirst,        // first run
            Plsrun          plsrunSecond,       // second run
            ref int         fIsInterruptOk      // [out] disconnect glyphs between runs?
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                TextStore store = FullText.StoreFrom(plsrunFirst);

                if (    !TextStore.IsContent(plsrunFirst)
                    ||  !TextStore.IsContent(plsrunSecond))
                {
                    fIsInterruptOk = 1;
                    return LsErr.None;
                }


                LSRun lsrunFirst = store.GetRun(plsrunFirst);
                LSRun lsrunSecond = store.GetRun(plsrunSecond);


                fIsInterruptOk = !(

                    lsrunFirst.BidiLevel == lsrunSecond.BidiLevel

                    && lsrunFirst.Shapeable != null
                    && lsrunSecond.Shapeable != null
                    && lsrunFirst.Shapeable.CanShapeTogether(lsrunSecond.Shapeable)
                    ) ? 1 : 0;

            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("FInterruptShaping", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }

        internal static CultureInfo GetNumberCulture(TextRunProperties properties, out NumberSubstitutionMethod method)
        {
            NumberSubstitution sub = properties.NumberSubstitution;
            if (sub == null)
            {
                method = NumberSubstitutionMethod.AsCulture;
                return CultureMapper.GetSpecificCulture(properties.CultureInfo);
            }

            method = sub.Substitution;

            switch (sub.CultureSource)
            {
                case NumberCultureSource.Text:
                    return CultureMapper.GetSpecificCulture(properties.CultureInfo);

                case NumberCultureSource.User:
                    return CultureInfo.CurrentCulture;

                case NumberCultureSource.Override:
                    return sub.CultureOverride;
            }

            return null;
        }
























        [SecurityCritical]
        internal unsafe LsErr GetGlyphsRedefined(
            IntPtr                      pols,                   // Line Layout context
            IntPtr*                     plsplsruns,             // array of plsruns
            int*                        pcchPlsrun,             // array of character count per run
            int                         plsrunCount,            // number of runs
            char*                       pwchText,               // character string
            int                         cchText,                // character count
            LsTFlow                     textFlow,               // text flow direction
            ushort*                     puGlyphsBuffer,         // [in/out] fixed-size buffer for glyph indices
            uint*                       piGlyphPropsBuffer,     // [in/out] fixed-size buffer for glyph properties list
            int                         cgiGlyphBuffers,        // glyph buffers length in glyphs
            ref int                     fIsGlyphBuffersUsed,    // [out] Boolean flag indicates glyph buffers being used
            ushort*                     puClusterMap,           // [out] character-to-glyph cluster map
            ushort*                     puCharProperties,       // [out] character properties
            int*                        pfCanGlyphAlone,        // [out] parallel to character codes: glyphing does not depend on neighbor?
            ref int                     glyphCount              // [out] glyph buffer length and returning actual glyph count
            )
        {
            Invariant.Assert(puGlyphsBuffer != null && piGlyphPropsBuffer != null);

            LsErr lserr = LsErr.None;
            LSRun lsrunFirst = null;

            try
            {
                LSRun[] lsruns = RemapLSRuns(plsplsruns, plsrunCount);
                lsrunFirst = lsruns[0];

                Debug.Assert(lsrunFirst.Shapeable != null);
                Debug.Assert(cchText > 0); // LineServices should not pass in zero character count;

                bool isRightToLeft = ((lsrunFirst.BidiLevel & 1) != 0);

                DWriteFontFeature[][] fontFeatures;
                uint[]                fontFeatureRanges;
                uint                  actualGlyphCount;
                checked 
                {
                    uint uCchText = (uint)cchText;
                    LSRun.CompileFeatureSet(lsruns, pcchPlsrun, uCchText, out fontFeatures, out fontFeatureRanges);
                    
                    GlyphTypeface glyphTypeface = lsrunFirst.Shapeable.GlyphTypeFace;
                    
                    FullText.Formatter.TextAnalyzer.GetGlyphs(
                        (ushort*)pwchText,
                        uCchText,
                        glyphTypeface.FontDWrite,
                        glyphTypeface.BlankGlyphIndex,
                        false,
                        isRightToLeft,
                        lsrunFirst.RunProp.CultureInfo,
                        fontFeatures,
                        fontFeatureRanges,
                        (uint)cgiGlyphBuffers,
                        FullText.TextFormattingMode,
                        lsrunFirst.Shapeable.ItemProps,
                        puClusterMap,
                        puCharProperties,
                        puGlyphsBuffer,
                        piGlyphPropsBuffer,
                        pfCanGlyphAlone,
                        out actualGlyphCount
                        );

                    glyphCount = (int)actualGlyphCount;
                   
                    if (glyphCount <= cgiGlyphBuffers)
                    {
                        fIsGlyphBuffersUsed = 1;
                    }
                    else
                    {
                        fIsGlyphBuffersUsed = 0;
                    }
                }
            }
            catch (Exception e)
            {
                SaveException(e, (Plsrun)(plsplsruns[0]), lsrunFirst);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetGlyphsRedefined", (Plsrun)(plsplsruns[0]), lsrunFirst);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
            
        }





        [SecurityCritical]
        internal unsafe LsErr GetGlyphPositions(
            IntPtr                      pols,               // Line Layout context
            IntPtr*                     plsplsruns,         // array of plsruns
            int*                        pcchPlsrun,         // array of character count per run
            int                         plsrunCount,        // number of runs
            LsDevice                    device,             // on reference or presentation device
            char*                       pwchText,           // character string
            ushort*                     puClusterMap,       // character-to-glyph cluster map
            ushort*                     puCharProperties,   // character properties
            int                         cchText,            // character count
            ushort*                     puGlyphs,           // glyph indices
            uint*                       piGlyphProperties,  // glyph properties
            int                         glyphCount,         // glyph count
            LsTFlow                     textFlow,           // text flow direction
            int*                        piGlyphAdvances,    // [out] glyph advances
            GlyphOffset*                piiGlyphOffsets     // [out] glyph offsets
            )
        {            
            LsErr lserr = LsErr.None;
            LSRun lsrunFirst = null;

            try
            {
                LSRun[] lsruns = RemapLSRuns(plsplsruns, plsrunCount);
                lsrunFirst = lsruns[0];

                bool isRightToLeft = ((lsrunFirst.BidiLevel & 1) != 0);

                GlyphOffset[] glyphOffset;

                GlyphTypeface glyphTypeface = lsrunFirst.Shapeable.GlyphTypeFace;

                DWriteFontFeature[][] fontFeatures;
                uint[] fontFeatureRanges;
                LSRun.CompileFeatureSet(lsruns, pcchPlsrun, checked((uint)cchText), out fontFeatures, out fontFeatureRanges);

                
                FullText.Formatter.TextAnalyzer.GetGlyphPlacements(
                    (ushort*)pwchText,
                    puClusterMap,
                    (ushort*)puCharProperties,
                    (uint)cchText,
                    puGlyphs,
                    piGlyphProperties,
                    (uint)glyphCount,
                    glyphTypeface.FontDWrite,
                    lsrunFirst.Shapeable.EmSize,
                    TextFormatterImp.ToIdeal,
                    false,
                    isRightToLeft,
                    lsrunFirst.RunProp.CultureInfo,
                    fontFeatures,
                    fontFeatureRanges,
                    FullText.TextFormattingMode,
                    lsrunFirst.Shapeable.ItemProps,
                    (float)FullText.StoreFrom(lsrunFirst.Type).Settings.TextSource.PixelsPerDip,
                    piGlyphAdvances,
                    out glyphOffset
                    );

                for (int i = 0; i < glyphCount; ++i)
                {
                    piiGlyphOffsets[i].du = glyphOffset[i].du;
                    piiGlyphOffsets[i].dv = glyphOffset[i].dv;
                }                
                 
            }
            catch (Exception e)
            {
                SaveException(e, (Plsrun)(plsplsruns[0]), lsrunFirst);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetGlyphPositions", (Plsrun)(plsplsruns[0]), lsrunFirst);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
            
        }








        [SecurityCritical]
        private unsafe LSRun[] RemapLSRuns(
            IntPtr*         plsplsruns,
            int             plsrunCount
            )
        {
            LSRun[] lsruns = new LSRun[plsrunCount];
            TextStore store = FullText.StoreFrom((Plsrun)(*plsplsruns));

            for (int i = 0; i < lsruns.Length; i++)
            {
                Plsrun plsrun = (Plsrun)plsplsruns[i];
                lsruns[i] = store.GetRun(plsrun);
                Debug.Assert(TextStore.IsContent(plsrun) && lsruns[i] != null);
            }
            return lsruns;
        }






        [SecurityCritical]
        internal unsafe LsErr DrawGlyphs(
            IntPtr                      pols,                       // Line Layout context
            Plsrun                      plsrun,                     // plsrun
            char*                       pwchText,                   // character string
            ushort*                     puClusterMap,               // character-to-cluster map
            ushort*                     puCharProperties,           // character properties
            int                         charCount,                  // character count
            ushort*                     puGlyphs,                   // glyph indices
            int*                        piJustifiedGlyphAdvances,   // justified glyph advances
            int*                        piGlyphAdvances,            // original ideal glyph advances
            GlyphOffset*                piiGlyphOffsets,            // glyph offsets
            uint*                       piGlyphProperties,          // glyph properties
            LsExpType*                  plsExpType,                 // glyph expansion types
            int                         glyphCount,                 // glyph count
            LsTFlow                     textFlow,                   // text flow
            uint                        displayMode,                // draw transparent or opaque
            ref LSPOINT                 ptRun,                      // [in] display position (at baseline)
            ref LsHeights               lsHeights,                  // [in] run height metrics
            int                         runWidth,                   // run overall advance width
            ref LSRECT                  clippingRect                // [in] clipping rectangle if any applied
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);

                Debug.Assert(TextStore.IsContent(plsrun) && lsrun.Shapeable != null);

                GlyphRun glyphRun = ComputeShapedGlyphRun(
                    lsrun,
                    currentLine.Formatter,
                    true,           // origin of the glyph run provided at drawing time
                    ptRun,
                    charCount,
                    pwchText,
                    puClusterMap,
                    glyphCount,
                    puGlyphs,
                    piJustifiedGlyphAdvances,
                    piiGlyphOffsets,
                    currentLine.IsJustified
                    );

                if (glyphRun != null)
                {
                    DrawingContext drawingContext = Draw.DrawingContext;

                    Draw.SetGuidelineY(glyphRun.BaselineOrigin.Y);

                    try 
                    {
                        _boundingBox.Union(
                            lsrun.DrawGlyphRun(
                                drawingContext, 
                                null,     // draw with the run's foreground
                                glyphRun
                                )
                            );
                    }
                    finally 
                    {
                        Draw.UnsetGuidelineY();
                    }

                }
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("DrawGlyphs", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }











        [SecurityCritical]
        internal unsafe LsErr GetCharCompressionInfoFullMixed(
            IntPtr              pols,                   // Line Layout context
            LsDevice            device,                 // kind of device
            LsTFlow             textFlow,               // text flow
            LsCharRunInfo       *plscharrunInfo,        // char-based run info
            LsNeighborInfo      *plsneighborInfoLeft,   // left neighbor info
            LsNeighborInfo      *plsneighborInfoRight,  // right neigbor info
            int                 maxPriorityLevel,       // maximum priority level
            int**               pplscompressionLeft,    // [in/out] fill in left compression amount per priority level on the way out
            int**               pplscompressionRight    // [in/out] fill in right compression amount per priority level on the way out
            )
        {
            LsErr lserr = LsErr.None;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                Invariant.Assert(maxPriorityLevel == 3);

                plsrun = plscharrunInfo->plsrun;
                lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);

                return AdjustChars(
                    plscharrunInfo,
                    false,  // compressing
                    (int)(lsrun.EmSize * Constants.MinInterWordCompressionPerEm),
                    pplscompressionLeft,
                    pplscompressionRight
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetCharCompressionInfoFullMixed", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }











        [SecurityCritical]
        internal unsafe LsErr GetCharExpansionInfoFullMixed(
            IntPtr              pols,                   // Line Layout context
            LsDevice            device,                 // kind of device
            LsTFlow             textFlow,               // text flow
            LsCharRunInfo       *plscharrunInfo,        // char-based run info
            LsNeighborInfo      *plsneighborInfoLeft,   // left neighbor info
            LsNeighborInfo      *plsneighborInfoRight,  // right neigbor info
            int                 maxPriorityLevel,       // maximum priority level
            int**               pplsexpansionLeft,      // [in/out] fill in left expansion amount per priority level on the way out
            int**               pplsexpansionRight      // [in/out] fill in right expansion amount per priority level on the way out
            )
        {
            LsErr lserr = LsErr.None;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                Invariant.Assert(maxPriorityLevel == 3);

                plsrun = plscharrunInfo->plsrun;
                lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);

                return AdjustChars(
                    plscharrunInfo,
                    true,   // expanding
                    (int)(lsrun.EmSize * Constants.MaxInterWordExpansionPerEm),
                    pplsexpansionLeft,
                    pplsexpansionRight
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetCharExpansionInfoFullMixed", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }








        [SecurityCritical]
        private unsafe LsErr AdjustChars(
            LsCharRunInfo       *plscharrunInfo,
            bool                expanding,
            int                 interWordAdjustTo,
            int**               pplsAdjustLeft,
            int**               pplsAdjustRight
            )
        {
            char* pwch = plscharrunInfo->pwch;
            int cchRun = plscharrunInfo->cwch;


            for (int i = 0; i < cchRun; i++)
            {
                int adjustedCharWidth = plscharrunInfo->rgduNominalWidth[i] + plscharrunInfo->rgduChangeLeft[i] + plscharrunInfo->rgduChangeRight[i];


                pplsAdjustLeft[0][i] = 0;
                pplsAdjustLeft[1][i] = 0;
                pplsAdjustLeft[2][i] = 0;


                pplsAdjustRight[0][i] = 0;
                pplsAdjustRight[1][i] = 0;
                pplsAdjustRight[2][i] = 0;

                ushort flags = (ushort)(Classification.CharAttributeOf((int)Classification.GetUnicodeClassUTF16(pwch[i]))).Flags;
                if ((flags & ((ushort)CharacterAttributeFlags.CharacterSpace)) != 0)
                {
                    if (expanding)
                    {
                        int expandedBy = Math.Max(0, interWordAdjustTo - adjustedCharWidth);
                        pplsAdjustRight[0][i] = expandedBy;
                        pplsAdjustRight[1][i] = expandedBy * Constants.AcceptableLineStretchability;
                        pplsAdjustRight[2][i] = FullText.FormatWidth;
                    }
                    else
                    {
                        pplsAdjustRight[0][i] = Math.Max(0, adjustedCharWidth - interWordAdjustTo);
                    }
                }
                else if (expanding)
                {

                    pplsAdjustRight[2][i] = FullText.FormatWidth; 
                }
            }
            return LsErr.None;
        }











        [SecurityCritical]
        internal unsafe LsErr GetGlyphCompressionInfoFullMixed(
            IntPtr              pols,                   // Line Layout context
            LsDevice            device,                 // kind of device
            LsTFlow             textFlow,               // text flow
            LsGlyphRunInfo      *plsglyphrunInfo,       // glyph-based run info
            LsNeighborInfo      *plsneighborInfoLeft,   // left neighbor info
            LsNeighborInfo      *plsneighborInfoRight,  // right neigbor info
            int                 maxPriorityLevel,       // maximum priority level
            int                 **pplscompressionLeft,  // [in/out] fill in left compression amount per priority level on the way out
            int                 **pplscompressionRight  // [in/out] fill in right compression amount per priority level on the way out
            )
        {
            LsErr lserr = LsErr.None;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                Invariant.Assert(maxPriorityLevel == 3);

                plsrun = plsglyphrunInfo->plsrun;
                lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);
                int em = lsrun.EmSize;

                return CompressGlyphs(
                    plsglyphrunInfo,
                    (int)(em * Constants.MinInterWordCompressionPerEm),
                    pplscompressionLeft,
                    pplscompressionRight
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetGlyphCompressionInfoFullMixed", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }










        [SecurityCritical]
        private unsafe LsErr CompressGlyphs(
            LsGlyphRunInfo      *plsglyphrunInfo,
            int                 interWordCompressTo,
            int                 **pplsCompressionLeft,
            int                 **pplsCompressionRight
            )
        {
            char* pwch = plsglyphrunInfo->pwch;
            ushort* pgmap = plsglyphrunInfo->rggmap;
            int cchRun = plsglyphrunInfo->cwch;
            int cgiRun = plsglyphrunInfo->cgindex;

            int ich = 0;
            int igi = pgmap[ich];
            int cgi = 0;


            while (ich < cchRun)
            {

                int cch = 1;
                while (ich + cch < cchRun && pgmap[ich + cch] == igi)
                    cch++;


                cgi = (ich + cch == cchRun) ? cgiRun - igi : pgmap[ich + cch] - igi;

                int i, j;


                for (j = 0; j < cch; j++)
                {
                    ushort flags = (ushort)(Classification.CharAttributeOf((int)Classification.GetUnicodeClassUTF16(pwch[ich + j]))).Flags;
                    if ((flags & ((ushort)CharacterAttributeFlags.CharacterSpace)) != 0)
                        break;
                }

                int glyphAdvance = 0;
                for (i = 0; i < cgi; i++)
                {
                    glyphAdvance += plsglyphrunInfo->rgduWidth[igi + i];


                    pplsCompressionLeft[0][igi + i] = 0;
                    pplsCompressionLeft[1][igi + i] = 0;
                    pplsCompressionLeft[2][igi + i] = 0;


                    pplsCompressionRight[0][igi + i] = 0;
                    pplsCompressionRight[1][igi + i] = 0;
                    pplsCompressionRight[2][igi + i] = 0;

                    if (    i == cgi - 1
                        &&  cch == 1
                        &&  j < cch
                        )
                    {

                        pplsCompressionRight[0][igi + i] = Math.Max(0, glyphAdvance - interWordCompressTo);
                    }
                }

                ich += cch;
                igi += cgi;
            }

            Invariant.Assert(igi == cgiRun);
            return LsErr.None;
        }










        [SecurityCritical]
        internal unsafe LsErr GetGlyphExpansionInfoFullMixed(
            IntPtr              pols,                   // Line Layout context
            LsDevice            device,                 // kind of device
            LsTFlow             textFlow,               // text flow
            LsGlyphRunInfo      *plsglyphrunInfo,       // glyph-based run info
            LsNeighborInfo      *plsneighborInfoLeft,   // left neighbor info
            LsNeighborInfo      *plsneighborInfoRight,  // right neigbor info
            int                 maxPriorityLevel,       // maximum priority level
            int                 **pplsexpansionLeft,    // [in/out] fill in left expansion amount per priority level on the way out
            int                 **pplsexpansionRight,   // [in/out] fill in right expansion amount per priority level on the way out
            LsExpType           *plsexptype,            // [in/out] fill in glyph expansion type for each glyph
            int                 *pduMinInk              // [in/out] fill in glyph minimum expansion for exptAddInkContinuous
            )
        {
            LsErr lserr = LsErr.None;
            Plsrun plsrun = Plsrun.Undefined;
            LSRun lsrun = null;

            try
            {
                Invariant.Assert(maxPriorityLevel == 3);

                plsrun = plsglyphrunInfo->plsrun;
                lsrun = FullText.StoreFrom(plsrun).GetRun(plsrun);
                int em = lsrun.EmSize;

                return ExpandGlyphs(
                    plsglyphrunInfo,
                    (int)(em * Constants.MaxInterWordExpansionPerEm),
                    pplsexpansionLeft,
                    pplsexpansionRight,
                    plsexptype,
                    LsExpType.AddWhiteSpace,    // inter-word expansion type


                    ((lsrun.BidiLevel & 1) == 0 ? LsExpType.AddWhiteSpace : LsExpType.None)
                    );
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetGlyphExpansionInfoFullMixed", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }









        [SecurityCritical]
        private unsafe LsErr ExpandGlyphs(
            LsGlyphRunInfo      *plsglyphrunInfo,
            int                 interWordExpandTo,
            int                 **pplsExpansionLeft,
            int                 **pplsExpansionRight,
            LsExpType           *plsexptype,
            LsExpType           interWordExpansionType,
            LsExpType           interLetterExpansionType
            )
        {
            char* pwch = plsglyphrunInfo->pwch;
            ushort* pgmap = plsglyphrunInfo->rggmap;
            int cchRun = plsglyphrunInfo->cwch;
            int cgiRun = plsglyphrunInfo->cgindex;

            int ich = 0;
            int igi = pgmap[ich];
            int cgi = 0;


            while (ich < cchRun)
            {

                int cch = 1;
                while (ich + cch < cchRun && pgmap[ich + cch] == igi)
                    cch++;


                cgi = (ich + cch == cchRun) ? cgiRun - igi : pgmap[ich + cch] - igi;

                int i, j;


                for (j = 0; j < cch; j++)
                {
                    ushort flags = (ushort)(Classification.CharAttributeOf((int)Classification.GetUnicodeClassUTF16(pwch[ich + j]))).Flags;
                    if ((flags & ((ushort)CharacterAttributeFlags.CharacterSpace)) != 0)
                        break;
                }

                int glyphAdvance = 0;
                for (i = 0; i < cgi; i++)
                {
                    glyphAdvance += plsglyphrunInfo->rgduWidth[igi + i];


                    pplsExpansionLeft[0][igi + i] = 0;
                    pplsExpansionLeft[1][igi + i] = 0;
                    pplsExpansionLeft[2][igi + i] = 0;


                    pplsExpansionRight[0][igi + i] = 0;
                    pplsExpansionRight[1][igi + i] = 0;
                    pplsExpansionRight[2][igi + i] = 0;

                    if (i == cgi - 1)
                    {
                        if (cch == 1 && j < cch)
                        {

                            int expandedBy = Math.Max(0, interWordExpandTo - glyphAdvance);
                            pplsExpansionRight[0][igi + i] = expandedBy;
                            pplsExpansionRight[1][igi + i] = expandedBy * Constants.AcceptableLineStretchability;
                            pplsExpansionRight[2][igi + i] = FullText.FormatWidth;
                            plsexptype[igi + i] = interWordExpansionType;
                        }
                        else
                        {

                            pplsExpansionRight[2][igi + i] = FullText.FormatWidth;
                            plsexptype[igi + i] = interLetterExpansionType;
                        }
                    }
                }

                ich += cch;
                igi += cgi;
            }

            Invariant.Assert(igi == cgiRun);
            return LsErr.None;
        }













        [SecurityCritical]
        internal unsafe LsErr GetObjectHandlerInfo(
            System.IntPtr   pols,               // Line Layout context
            uint            objectId,           // installed object id
            void*           objectInfo          // [out] object handler info
            )
        {
            LsErr lserr = LsErr.None;

            try
            {
                if (objectId < (uint)TextStore.ObjectId.MaxNative)
                {

                    return UnsafeNativeMethods.LocbkGetObjectHandlerInfo(
                        pols,
                        objectId,
                        objectInfo
                        );
                }



                switch (objectId)
                {
                    case (uint)TextStore.ObjectId.InlineObject:
                        InlineInit inlineInit = new InlineInit();
                        inlineInit.pfnFormat = this.InlineFormatDelegate;
                        inlineInit.pfnDraw = this.InlineDrawDelegate;
                        Marshal.StructureToPtr(inlineInit, (System.IntPtr)objectInfo, false);
                        break;

                    default:
                        Debug.Assert(false, "Unsupported installed object!");
                        break;
                }
            }
            catch (Exception e)
            {
                SaveException(e, Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("GetObjectHandlerInfo", Plsrun.Undefined, null);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }




        [SecurityCritical]
        internal LsErr InlineFormat(
            System.IntPtr           pols,               // Line Layout context
            Plsrun                  plsrun,             // plsrun
            int                     lscpInline,         // first cp of the run
            int                     currentPosition,    // inline's current pen location in text direction
            int                     rightMargin,        // right margin
            ref ObjDim              pobjDim,            // [out] object dimension
            out int                 fFirstRealOnLine,   // [out] is this run the first in line
            out int                 fPenPositionUsed,   // [out] is pen position used to format object
            out LsBrkCond           breakBefore,        // [out] break condition before this object
            out LsBrkCond           breakAfter          // [out] break condition after this object
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            fFirstRealOnLine = 0;
            fPenPositionUsed = 0;
            breakBefore = LsBrkCond.Please;
            breakAfter = LsBrkCond.Please;

            try
            {
                TextFormatterImp formatter = FullText.Formatter;

                TextStore store = FullText.StoreFrom(plsrun);
                lsrun = store.GetRun(plsrun);
                TextEmbeddedObject textObject = lsrun.TextRun as TextEmbeddedObject;

                Debug.Assert(textObject != null);

                int cpInline = store.GetExternalCp(lscpInline);

                fFirstRealOnLine = (cpInline == store.CpFirst) ? 1 : 0;

                TextEmbeddedObjectMetrics metrics = store.FormatTextObject(
                    textObject,
                    cpInline,
                    currentPosition,
                    rightMargin
                    );

                pobjDim = new ObjDim();
                pobjDim.dur = TextFormatterImp.RealToIdeal(metrics.Width);
                pobjDim.heightsRef.dvMultiLineHeight = TextFormatterImp.RealToIdeal(metrics.Height);
                pobjDim.heightsRef.dvAscent = TextFormatterImp.RealToIdeal(metrics.Baseline);
                pobjDim.heightsRef.dvDescent = pobjDim.heightsRef.dvMultiLineHeight - pobjDim.heightsRef.dvAscent;
                pobjDim.heightsPres = pobjDim.heightsRef;

                breakBefore = BreakConditionToLsBrkCond(textObject.BreakBefore);
                breakAfter = BreakConditionToLsBrkCond(textObject.BreakAfter);
                fPenPositionUsed = (!textObject.HasFixedSize) ? 1 : 0;


                lsrun.BaselineOffset = pobjDim.heightsRef.dvAscent;
                lsrun.Height = pobjDim.heightsRef.dvMultiLineHeight;
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("InlineFormat", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }


        private LsBrkCond BreakConditionToLsBrkCond(LineBreakCondition breakCondition)
        {
            switch (breakCondition)
            {
                case LineBreakCondition.BreakDesired:
                    return LsBrkCond.Please;

                case LineBreakCondition.BreakPossible:
                    return LsBrkCond.Can;

                case LineBreakCondition.BreakRestrained:
                    return LsBrkCond.Never;

                case LineBreakCondition.BreakAlways:
                    return LsBrkCond.Must;
            }
            Debug.Assert(false);
            return LsBrkCond.Please;
        }





        [SecurityCritical]
        internal LsErr InlineDraw(
            System.IntPtr   pols,           // Line Layout context
            Plsrun          plsrun,         // plsrun
            ref LSPOINT     ptRun,          // [in] pen position at which to render the object
            LsTFlow         textFlow,       // text flow direction
            int             runWidth        // object width
            )
        {
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try
            {
                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);

                LSPOINT lsrunOrigin = ptRun;

                Debug.Assert(lsrun.Type == Plsrun.InlineObject);

                int baseDirection = currentLine.RightToLeft ? 1 : 0;
                int runDirection = (int)(lsrun.BidiLevel & 1);

                if (baseDirection != 0)
                {
                    lsrunOrigin.x = -lsrunOrigin.x;
                }

                TextEmbeddedObject textObject = lsrun.TextRun as TextEmbeddedObject;

                Debug.Assert(textObject != null);
                Debug.Assert(textFlow != LsTFlow.lstflowWS || runDirection != 0);

                if ((baseDirection ^ runDirection) != 0)
                {

                    lsrunOrigin.x -= runWidth;
                }


                Point baselineOrigin = new Point(
                    currentLine.Formatter.IdealToReal(currentLine.LSLineUToParagraphU(lsrunOrigin.x), currentLine.PixelsPerDip)+ Draw.VectorToLineOrigin.X,
                    currentLine.Formatter.IdealToReal((lsrunOrigin.y + lsrun.BaselineMoveOffset), currentLine.PixelsPerDip) + Draw.VectorToLineOrigin.Y
                    );


                Rect objectBounds = textObject.ComputeBoundingBox(
                     baseDirection != 0, // rightToLeft
                    false  // no sideway support yet
                    );

                if (!objectBounds.IsEmpty)
                {


                    objectBounds.X += baselineOrigin.X;
                    objectBounds.Y += baselineOrigin.Y;
                }


                _boundingBox.Union(
                    new Rect(

                    LSRun.UVToXY(
                            Draw.LineOrigin,
                            new Point(),
                            objectBounds.Location.X,
                            objectBounds.Location.Y,
                            currentLine
                            ),

                    LSRun.UVToXY(
                            Draw.LineOrigin,
                            new Point(),
                            objectBounds.Location.X + objectBounds.Size.Width,
                            objectBounds.Location.Y + objectBounds.Size.Height,
                            currentLine
                            )
                        )
                    );

                DrawingContext drawingContext = Draw.DrawingContext;                
                
                if (drawingContext != null)
                {

                    Draw.SetGuidelineY(baselineOrigin.Y);

                    try 
                    {                    
                        if (Draw.AntiInversion == null)
                        {

                            textObject.Draw(
                                drawingContext,
                                LSRun.UVToXY(
                                    Draw.LineOrigin,
                                    new Point(),
                                    baselineOrigin.X,
                                    baselineOrigin.Y,
                                    currentLine
                                    ),
                                baseDirection != 0,
                                false
                                );
                        }
                        else
                        {




                            drawingContext.PushTransform(Draw.AntiInversion);
                            try 
                            {
                                textObject.Draw(drawingContext, baselineOrigin, baseDirection != 0, false);
                            } 
                            finally
                            {
                                drawingContext.Pop();
                            }
                        }
                    }
                    finally 
                    {
                        Draw.UnsetGuidelineY();
                    }
                }
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("InlineDraw", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            return lserr;
        }


















        [SecurityCritical]
        internal unsafe LsErr EnumText(        
            IntPtr                      pols,                           // ls context
            Plsrun                      plsrun,                         // plsrun
            int                         cpFirst,                        // first cp of the ls dnode
            int                         dcp,                            // dcp of the dnode
            char                        *pwchText,                      // characters for glyph run
            int                         cchText,                        // length of characters 
            LsTFlow                     lstFlow,                        // flow direction
            int                         fReverseOrder,                  // flag for reverse order enumeration
            int                         fGeometryProvided,              // flag for providing geometry 
            ref LSPOINT                 pptStart,                       // [in] logical start of the run
            ref LsHeights               pheights,                       // [in] height (iff geometryProvided)
            int                         dupRun,                         // width of the run
            int                         glyphBaseRun,                   // flag for glyph based run
            int                         *piCharAdvances,                // character advance widths (iff !glyphBaseRun)
            ushort                      *puClusterMap,                  // cluster map (iff glyphBaseRun)
            ushort                      *characterProperties,           // character properties (iff glyphBaseRun)
            ushort                      *puGlyphs,                      // glyph indices (iff glyphBaseRun)
            int                         *piJustifiedGlyphAdvances,      // glyph advances (iff glyphBaseRun)
            GlyphOffset                 *piiGlyphOffsets,               // glyph offsets (iff glyphBaseRun)
            uint                        *piGlyphProperties,             // glyph properties (iff glyphProperties)
            int                         glyphCount                      // glyph count
            )
        {
            Debug.Assert(fGeometryProvided == 0, "Line enumeration doesn't need geometry information");

            if (cpFirst < 0)
            {

                return LsErr.None;
            }
            
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try                 
            {   
                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);
                GlyphRun glyphRun = null;
                if (glyphBaseRun != 0)
                {

                    if (glyphCount > 0)
                    {

                        glyphRun = ComputeShapedGlyphRun(
                            lsrun, 
                            currentLine.Formatter,
                            false,      // glyph run origin not provided
                            pptStart, 
                            cchText, 
                            pwchText, 
                            puClusterMap, 
                            glyphCount, 
                            puGlyphs,
                            piJustifiedGlyphAdvances, 
                            piiGlyphOffsets,
                            currentLine.IsJustified
                            );
                    }
                }
                else if (cchText > 0)
                {

                    dupRun = 0;
                    for (int i = 0; i < cchText; i++)
                    {
                        dupRun += piCharAdvances[i];
                    }

                    

                    glyphRun = ComputeUnshapedGlyphRun(
                        lsrun, 
                        lstFlow, 
                        currentLine.Formatter,
                        false,      // glyph run origin not provided at enumeration
                        pptStart, 
                        dupRun, 
                        cchText, 
                        pwchText, 
                        piCharAdvances,
                        currentLine.IsJustified
                        );
                }
                
                if (glyphRun != null)
                {





                    IndexedGlyphRuns.Add(
                        new IndexedGlyphRun(
                           currentLine.GetExternalCp(cpFirst),
                           dcp,
                           glyphRun
                           )
                    );
                }                   
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("EnumText", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            
            return lserr;
        }






        [SecurityCritical]
        internal unsafe LsErr EnumTab(
            IntPtr              pols,               // pointer to context
            Plsrun              plsrun,             // plsrun
            int                 cpFirst,            // first cp of the dnode run
            char                *pwchText,          // a single tab character
            char                tabLeader,          // a single tab leader character
            LsTFlow             lstFlow,            // flow direction
            int                 fReverseOrder,      // flag for reverse order enumeration
            int                 fGeometryProvided,  // flag for providing geometry information
            ref LSPOINT         pptStart,           // [in] logical start of the run (iff geometryProvided)
            ref LsHeights       heights,            // [in] height (iff geometryProvided)
            int                 dupRun              // width of the run
            )
        {       
            if (cpFirst < 0)
            {

                return LsErr.None;
            }
        
            LsErr lserr = LsErr.None;
            LSRun lsrun = null;

            try 
            {
                TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                lsrun = currentLine.GetRun(plsrun);
                GlyphRun glyphRun = null;                
                
                if (lsrun.Type == Plsrun.Text)
                {         


                    int charWidth = 0;
                    lsrun.Shapeable.GetAdvanceWidthsUnshaped(
                        &tabLeader, 
                        1,
                        TextFormatterImp.ToIdeal, 
                        &charWidth
                        );                

                    glyphRun = ComputeUnshapedGlyphRun(
                        lsrun, 
                        lstFlow,
                        currentLine.Formatter,
                        false,      // glyph run origin not provided at enumeration time
                        pptStart, 
                        charWidth, 
                        1, 
                        &tabLeader, 
                        &charWidth,
                        currentLine.IsJustified
                        );

                }

                if (glyphRun != null)
                {                    
                    IndexedGlyphRuns.Add(
                        new IndexedGlyphRun(
                           currentLine.GetExternalCp(cpFirst),
                           1,       // dcp is 1 for a Tab character
                           glyphRun
                           )
                    );                    
                }                
            }
            catch (Exception e)
            {
                SaveException(e, plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }
            catch
            {
                SaveNonCLSException("EnumTab", plsrun, lsrun);
                lserr = LsErr.ClientAbort;
            }

            return lserr;
        }





        private bool IsSpace(char ch)
        {
            if (   ch == '\u0009' // tab
                || ch == '\u0020' // Space
                )
            {
                return true;
            }

            return false;
        }




        private static int RealToIdeal(double i)
        {
            return TextFormatterImp.RealToIdeal(i);
        }














        private static double RoundDipForDisplayModeJustifiedText(double value, double pixelsPerDip)
        {
            return TextFormatterImp.RoundDipForDisplayModeJustifiedText(value, pixelsPerDip);
        }




        private static double IdealToRealWithNoRounding(double i)
        {
            return TextFormatterImp.IdealToRealWithNoRounding(i);
        }



















        [SecurityCritical]
        private unsafe void AdjustMetricsForDisplayModeJustifiedText(
            char              *pwchText,
            int               *piGlyphAdvances,
            int               glyphCount,
            bool              isRightToLeft,
            int               idealBaselineOriginX,
            int               idealBaselineOriginY,
            double            pixelsPerDip,
            out Point         baselineOrigin,
            out IList<double> adjustedAdvanceWidths
            )
        {
            adjustedAdvanceWidths = new double[glyphCount];

            baselineOrigin = new Point(RoundDipForDisplayModeJustifiedText(IdealToRealWithNoRounding(idealBaselineOriginX), pixelsPerDip),
                                       RoundDipForDisplayModeJustifiedText(IdealToRealWithNoRounding(idealBaselineOriginY), pixelsPerDip));

            int idealRoundedBaselineOriginX = RealToIdeal(baselineOrigin.X);
            




            int idealStartingError = idealBaselineOriginX - idealRoundedBaselineOriginX;

            if (isRightToLeft)
            {
                idealStartingError *= -1; 
            }

            if (glyphCount > 0)
            {



                double realAccumulatedRoundedAdvanceWidth = 0;
                double realAccumulatedAdvanceWidth        = 0;
                int    idealAccumulatedAdvanceWidth       = idealStartingError;
                
                double error                   = 0;
                double realAdvanceWidth        = 0;
                int    indexOfLastKnownSpace   = -1;
                double realRoundedAdvanceWidth = 0;

                for (int i = 0; i < glyphCount; ++i)
                {
                    if (IsSpace(pwchText[i]))
                    {
                        indexOfLastKnownSpace = i;
                    }

                    idealAccumulatedAdvanceWidth       += piGlyphAdvances[i];
                    realAccumulatedAdvanceWidth         = IdealToRealWithNoRounding(idealAccumulatedAdvanceWidth);

                    realAdvanceWidth                    = IdealToRealWithNoRounding(piGlyphAdvances[i]);
                    realRoundedAdvanceWidth             = RoundDipForDisplayModeJustifiedText(realAdvanceWidth, pixelsPerDip);
                    realAccumulatedRoundedAdvanceWidth += realRoundedAdvanceWidth;

































                    error += RoundDipForDisplayModeJustifiedText(
                                        realAccumulatedRoundedAdvanceWidth 
                                        - RoundDipForDisplayModeJustifiedText(realAccumulatedAdvanceWidth, pixelsPerDip),
                                        pixelsPerDip
                                        );

                    adjustedAdvanceWidths[i] = realRoundedAdvanceWidth;

                    if (indexOfLastKnownSpace >= 0)
                    {
                        adjustedAdvanceWidths[indexOfLastKnownSpace] -= error;
                        realAccumulatedRoundedAdvanceWidth           -= error;
                        error = 0;
                    }
                }





                if (indexOfLastKnownSpace < 0)
                {
                    realAccumulatedRoundedAdvanceWidth = 0;
                    realAccumulatedAdvanceWidth        = 0;
                    idealAccumulatedAdvanceWidth       = idealStartingError;
                    realAdvanceWidth                   = 0;
                    realRoundedAdvanceWidth            = 0;
                    error                              = 0;

                    for (int i = 0; i < glyphCount; ++i)
                    {
                        idealAccumulatedAdvanceWidth       += piGlyphAdvances[i];
                        realAccumulatedAdvanceWidth         = IdealToRealWithNoRounding(idealAccumulatedAdvanceWidth);

                        realAdvanceWidth                    = IdealToRealWithNoRounding(piGlyphAdvances[i]);
                        realRoundedAdvanceWidth             = RoundDipForDisplayModeJustifiedText(realAdvanceWidth, pixelsPerDip);
                        realAccumulatedRoundedAdvanceWidth += realRoundedAdvanceWidth;



                        error = RoundDipForDisplayModeJustifiedText(
                                        realAccumulatedRoundedAdvanceWidth
                                        - RoundDipForDisplayModeJustifiedText(realAccumulatedAdvanceWidth, pixelsPerDip),
                                        pixelsPerDip
                                        );
                        adjustedAdvanceWidths[i]            = realRoundedAdvanceWidth - error;
                        realAccumulatedRoundedAdvanceWidth -= error;
                    }
                }
            }
        }





        [SecurityCritical]
        private unsafe GlyphRun ComputeShapedGlyphRun(
            LSRun                   lsrun,                      // ls run
            TextFormatterImp        textFormatterImp,           // The TextFormatter Implementation
            bool                    originProvided,             // flag indicate whether the origin of the run is provided                        
            LSPOINT                 lsrunOrigin,                // physical start of the run
            int                     charCount,                  // characters count
            char                    *pwchText,                  // characters for the GlyphRun
            ushort                  *puClusterMap,              // cluster map
            int                     glyphCount,                 // glyph count
            ushort                  *puGlyphs,                  // glyph indices
            int                     *piJustifiedGlyphAdvances,  // glyph advances
            GlyphOffset             *piiGlyphOffsets,           // glyph offsets
            bool                    justify
            )
        {
            TextMetrics.FullTextLine currentLine = Draw.CurrentLine;

            Point runOrigin = new Point();
            int nominalX = 0;
            int nominalY = 0;

            if (originProvided)
            {   
                if (currentLine.RightToLeft)
                {





                    lsrunOrigin.x = -lsrunOrigin.x;
                }

                if (textFormatterImp.TextFormattingMode == TextFormattingMode.Display && justify)
                {
                    LSRun.UVToNominalXY(
                        Draw.LineOrigin,
                        Draw.VectorToLineOrigin,
                        currentLine.LSLineUToParagraphU(lsrunOrigin.x),
                        lsrunOrigin.y + lsrun.BaselineMoveOffset,
                        currentLine,
                        out nominalX,
                        out nominalY
                        );
                }
                else
                {
                    runOrigin = LSRun.UVToXY(
                        Draw.LineOrigin,
                        Draw.VectorToLineOrigin,
                        currentLine.LSLineUToParagraphU(lsrunOrigin.x),
                        lsrunOrigin.y + lsrun.BaselineMoveOffset,
                        currentLine
                        );
                }
            }





            char[] charString = new char[charCount];
            ushort[] clusterMap = new ushort[charCount];

            for (int i = 0; i < charCount; i++)
            {
                charString[i] = pwchText[i];
                clusterMap[i] = puClusterMap[i];
            }

            ushort[] glyphIndices = new ushort[glyphCount];            
            IList<double> glyphAdvances;
            IList<Point> glyphOffsets;

            bool isRightToLeft = (lsrun.BidiLevel & 1) != 0;

            if (textFormatterImp.TextFormattingMode == TextFormattingMode.Ideal)
            {
                glyphAdvances = new ThousandthOfEmRealDoubles(textFormatterImp.IdealToReal(lsrun.EmSize, currentLine.PixelsPerDip), glyphCount);
                glyphOffsets = new ThousandthOfEmRealPoints(textFormatterImp.IdealToReal(lsrun.EmSize, currentLine.PixelsPerDip), glyphCount);

                for (int i = 0; i < glyphCount; i++)
                {
                    glyphIndices[i] = puGlyphs[i];
                    glyphAdvances[i] = textFormatterImp.IdealToReal(piJustifiedGlyphAdvances[i], currentLine.PixelsPerDip);
                    glyphOffsets[i] = new Point(
                        textFormatterImp.IdealToReal(piiGlyphOffsets[i].du, currentLine.PixelsPerDip),
                        textFormatterImp.IdealToReal(piiGlyphOffsets[i].dv, currentLine.PixelsPerDip)
                        );
                }
            }
            else
            {
                if (justify)
                {
                    AdjustMetricsForDisplayModeJustifiedText(
                        pwchText,
                        piJustifiedGlyphAdvances,
                        glyphCount,
                        isRightToLeft,
                        nominalX,
                        nominalY,
                        currentLine.PixelsPerDip,
                        out runOrigin,
                        out glyphAdvances
                        );
                }
                else
                {
                    glyphAdvances = new List<double>(glyphCount);
                    for (int i = 0; i < glyphCount; i++)
                    {
                        glyphAdvances.Add(textFormatterImp.IdealToReal(piJustifiedGlyphAdvances[i], currentLine.PixelsPerDip));
                    }
                }
                glyphOffsets  = new List<Point>(glyphCount);
                for (int i = 0; i < glyphCount; i++)
                {
                    glyphIndices[i] = puGlyphs[i];
                    glyphOffsets.Add(new Point(
                            textFormatterImp.IdealToReal(piiGlyphOffsets[i].du, currentLine.PixelsPerDip),
                            textFormatterImp.IdealToReal(piiGlyphOffsets[i].dv, currentLine.PixelsPerDip)
                            ));
                }
            }

#if CHECK_GLYPHS
            if (   lsrun._glyphs != null
                && glyphCount <= lsrun._glyphs.Length)
            {
                for (int i = 0; i < glyphCount; i++)
                {
                    Debug.Assert(glyphIndices[i] == lsrun._glyphs[i], "Corrupted glyphs");
                }
            }
#endif            

            GlyphRun glyphRun = lsrun.Shapeable.ComputeShapedGlyphRun(
                runOrigin,
                charString, 
                clusterMap, 
                glyphIndices, 
                glyphAdvances, 
                glyphOffsets,
                isRightToLeft, 
                false   // no sideway support yet
                );

            return glyphRun;
        }






        [SecurityCritical]
        private unsafe GlyphRun ComputeUnshapedGlyphRun(
            LSRun               lsrun,              // LSrun used to shape the GlyphRun            
            LsTFlow             textFlow,           // flow direction
            TextFormatterImp    textFormatterImp,   // The TextFormatter Implementation
            bool                originProvided,     // flag indicate whether the origin of the run is provided                        
            LSPOINT             lsrunOrigin,        // physical start of the run
            int                 dupRun,             // width of the run
            int                 cchText,            // character count
            char                *pwchText,          // characters for display 
            int                 *piCharAdvances,    // character advance widths,
            bool                justify
            )
        {
            GlyphRun glyphRun = null;
            if (lsrun.Type == Plsrun.Text)
            {
                Debug.Assert(lsrun.Shapeable != null);
                Point runOrigin    = new Point();
                int nominalX = 0;
                int nominalY = 0;

                if (originProvided)
                {                   
                    TextMetrics.FullTextLine currentLine = Draw.CurrentLine;
                    
                    if (textFlow == LsTFlow.lstflowWS)
                    {
                        lsrunOrigin.x -= dupRun;
                    }

                    if (currentLine.RightToLeft)
                    {
                        lsrunOrigin.x = -lsrunOrigin.x;
                    }

                    if (textFormatterImp.TextFormattingMode == TextFormattingMode.Display && justify)
                    {
                        LSRun.UVToNominalXY(
                            Draw.LineOrigin,
                            Draw.VectorToLineOrigin,
                            currentLine.LSLineUToParagraphU(lsrunOrigin.x),
                            lsrunOrigin.y + lsrun.BaselineMoveOffset,
                            currentLine,
                            out nominalX,
                            out nominalY
                            );
                    }
                    else
                    {
                        runOrigin = LSRun.UVToXY(
                            Draw.LineOrigin,
                            Draw.VectorToLineOrigin,
                            currentLine.LSLineUToParagraphU(lsrunOrigin.x),
                            lsrunOrigin.y + lsrun.BaselineMoveOffset,
                            currentLine
                            );
                    }
                }



                
                char[] charString = new char[cchText];
                IList<double> charWidths;

                bool isRightToLeft = (lsrun.BidiLevel & 1) != 0;

                if (textFormatterImp.TextFormattingMode == TextFormattingMode.Ideal)
                {
                    charWidths = new ThousandthOfEmRealDoubles(textFormatterImp.IdealToReal(lsrun.EmSize, Draw.CurrentLine.PixelsPerDip), cchText);
                    for (int i = 0; i < cchText; i++)
                    {
                        charString[i] = pwchText[i];
                        charWidths[i] = textFormatterImp.IdealToReal(piCharAdvances[i], Draw.CurrentLine.PixelsPerDip);
                    }
                }
                else
                {
                    if (justify)
                    {
                        AdjustMetricsForDisplayModeJustifiedText(
                            pwchText,
                            piCharAdvances,
                            cchText,
                            isRightToLeft,
                            nominalX,
                            nominalY,
                            Draw.CurrentLine.PixelsPerDip,
                            out runOrigin,
                            out charWidths
                            );
                    }
                    else
                    {
                        charWidths = new List<double>(cchText);
                        for (int i = 0; i < cchText; i++)
                        {
                            charWidths.Add(textFormatterImp.IdealToReal(piCharAdvances[i], Draw.CurrentLine.PixelsPerDip));
                        }
                    }
                    for (int i = 0; i < cchText; i++)
                    {
                        charString[i] = pwchText[i];
                    }
                }

                

                glyphRun = lsrun.Shapeable.ComputeUnshapedGlyphRun(
                    runOrigin,
                    charString,
                    charWidths
                    );
            }            

            return glyphRun;
        } 















        [SecurityCritical, SecurityTreatAsSafe]
        internal unsafe LineServicesCallbacks()
        {
            _pfnFetchRunRedefined                   = new FetchRunRedefined(this.FetchRunRedefined);
            _pfnFetchLineProps                      = new FetchLineProps(this.FetchLineProps);
            _pfnFetchPap                            = new FetchPap(this.FetchPap);
            _pfnGetRunTextMetrics                   = new GetRunTextMetrics(this.GetRunTextMetrics);
            _pfnGetRunCharWidths                    = new GetRunCharWidths(this.GetRunCharWidths);
            _pfnGetDurMaxExpandRagged               = new GetDurMaxExpandRagged(this.GetDurMaxExpandRagged);
            _pfnDrawTextRun                         = new DrawTextRun(this.DrawTextRun);
            _pfnGetGlyphsRedefined                  = new GetGlyphsRedefined(this.GetGlyphsRedefined);
            _pfnGetGlyphPositions                   = new GetGlyphPositions(this.GetGlyphPositions);
            _pfnGetAutoNumberInfo                   = new GetAutoNumberInfo(this.GetAutoNumberInfo);
            _pfnDrawGlyphs                          = new DrawGlyphs(this.DrawGlyphs);
            _pfnGetObjectHandlerInfo                = new GetObjectHandlerInfo(this.GetObjectHandlerInfo);
            _pfnGetRunUnderlineInfo                 = new GetRunUnderlineInfo(this.GetRunUnderlineInfo);
            _pfnGetRunStrikethroughInfo             = new GetRunStrikethroughInfo(this.GetRunStrikethroughInfo);
            _pfnHyphenate                           = new Hyphenate(this.Hyphenate);
            _pfnGetNextHyphenOpp                    = new GetNextHyphenOpp(this.GetNextHyphenOpp);
            _pfnGetPrevHyphenOpp                    = new GetPrevHyphenOpp(this.GetPrevHyphenOpp);
            _pfnDrawUnderline                       = new DrawUnderline(this.DrawUnderline);
            _pfnDrawStrikethrough                   = new DrawStrikethrough(this.DrawStrikethrough);
            _pfnFInterruptShaping                   = new FInterruptShaping(this.FInterruptShaping);
            _pfnGetCharCompressionInfoFullMixed     = new GetCharCompressionInfoFullMixed(this.GetCharCompressionInfoFullMixed);
            _pfnGetCharExpansionInfoFullMixed       = new GetCharExpansionInfoFullMixed(this.GetCharExpansionInfoFullMixed);
            _pfnGetGlyphCompressionInfoFullMixed    = new GetGlyphCompressionInfoFullMixed(this.GetGlyphCompressionInfoFullMixed);
            _pfnGetGlyphExpansionInfoFullMixed      = new GetGlyphExpansionInfoFullMixed(this.GetGlyphExpansionInfoFullMixed);
            _pfnEnumText                            = new EnumText(this.EnumText);
            _pfnEnumTab                             = new EnumTab(this.EnumTab);
        }





        [SecurityCritical]
        internal void PopulateContextInfo(ref LsContextInfo contextInfo, ref LscbkRedefined lscbkRedef)
        {
            lscbkRedef.pfnFetchRunRedefined                 = _pfnFetchRunRedefined;
            lscbkRedef.pfnGetGlyphsRedefined                = _pfnGetGlyphsRedefined;
            lscbkRedef.pfnFetchLineProps                    = _pfnFetchLineProps;
            contextInfo.pfnFetchLineProps                   = _pfnFetchLineProps;
            contextInfo.pfnFetchPap                         = _pfnFetchPap;
            contextInfo.pfnGetRunTextMetrics                = _pfnGetRunTextMetrics;
            contextInfo.pfnGetRunCharWidths                 = _pfnGetRunCharWidths;
            contextInfo.pfnGetDurMaxExpandRagged            = _pfnGetDurMaxExpandRagged;
            contextInfo.pfnDrawTextRun                      = _pfnDrawTextRun;
            contextInfo.pfnGetGlyphPositions                = _pfnGetGlyphPositions;
            contextInfo.pfnGetAutoNumberInfo                = _pfnGetAutoNumberInfo;
            contextInfo.pfnDrawGlyphs                       = _pfnDrawGlyphs;
            contextInfo.pfnGetObjectHandlerInfo             = _pfnGetObjectHandlerInfo;
            contextInfo.pfnGetRunUnderlineInfo              = _pfnGetRunUnderlineInfo;
            contextInfo.pfnGetRunStrikethroughInfo          = _pfnGetRunStrikethroughInfo;
            contextInfo.pfnHyphenate                        = _pfnHyphenate;
            contextInfo.pfnGetNextHyphenOpp                 = _pfnGetNextHyphenOpp;
            contextInfo.pfnGetPrevHyphenOpp                 = _pfnGetPrevHyphenOpp;
            contextInfo.pfnDrawUnderline                    = _pfnDrawUnderline;
            contextInfo.pfnDrawStrikethrough                = _pfnDrawStrikethrough;
            contextInfo.pfnFInterruptShaping                = _pfnFInterruptShaping;
            contextInfo.pfnGetCharCompressionInfoFullMixed  = _pfnGetCharCompressionInfoFullMixed;
            contextInfo.pfnGetCharExpansionInfoFullMixed    = _pfnGetCharExpansionInfoFullMixed;
            contextInfo.pfnGetGlyphCompressionInfoFullMixed = _pfnGetGlyphCompressionInfoFullMixed;
            contextInfo.pfnGetGlyphExpansionInfoFullMixed   = _pfnGetGlyphExpansionInfoFullMixed;
            contextInfo.pfnEnumText                         = _pfnEnumText;
            contextInfo.pfnEnumTab                          = _pfnEnumTab;
        }







        [SecurityCritical]
        private FetchRunRedefined                   _pfnFetchRunRedefined;
        [SecurityCritical]
        private FetchLineProps                      _pfnFetchLineProps;
        [SecurityCritical]
        private FetchPap                            _pfnFetchPap;
        [SecurityCritical]
        private GetRunTextMetrics                   _pfnGetRunTextMetrics;
        [SecurityCritical]
        private GetRunCharWidths                    _pfnGetRunCharWidths;
        [SecurityCritical]
        private GetDurMaxExpandRagged               _pfnGetDurMaxExpandRagged;
        [SecurityCritical]
        private GetAutoNumberInfo                   _pfnGetAutoNumberInfo;
        [SecurityCritical]
        private DrawTextRun                         _pfnDrawTextRun;
        [SecurityCritical]
        private GetGlyphsRedefined                  _pfnGetGlyphsRedefined;
        [SecurityCritical]
        private GetGlyphPositions                   _pfnGetGlyphPositions;
        [SecurityCritical]
        private DrawGlyphs                          _pfnDrawGlyphs;
        [SecurityCritical]
        private GetObjectHandlerInfo                _pfnGetObjectHandlerInfo;
        [SecurityCritical]
        private GetRunUnderlineInfo                 _pfnGetRunUnderlineInfo;
        [SecurityCritical]
        private GetRunStrikethroughInfo             _pfnGetRunStrikethroughInfo;
        [SecurityCritical]
        private Hyphenate                           _pfnHyphenate;
        [SecurityCritical]
        private GetNextHyphenOpp                    _pfnGetNextHyphenOpp;
        [SecurityCritical]
        private GetPrevHyphenOpp                    _pfnGetPrevHyphenOpp;
        [SecurityCritical]
        private DrawUnderline                       _pfnDrawUnderline;
        [SecurityCritical]
        private DrawStrikethrough                   _pfnDrawStrikethrough;
        [SecurityCritical]
        private FInterruptShaping                   _pfnFInterruptShaping;
        [SecurityCritical]
        private GetCharCompressionInfoFullMixed     _pfnGetCharCompressionInfoFullMixed;
        [SecurityCritical]
        private GetCharExpansionInfoFullMixed       _pfnGetCharExpansionInfoFullMixed;
        [SecurityCritical]
        private GetGlyphCompressionInfoFullMixed    _pfnGetGlyphCompressionInfoFullMixed;
        [SecurityCritical]
        private GetGlyphExpansionInfoFullMixed      _pfnGetGlyphExpansionInfoFullMixed;
        [SecurityCritical]
        private EnumText                            _pfnEnumText;
        [SecurityCritical]
        private EnumTab                             _pfnEnumTab;





        [SecurityCritical]
        private InlineFormat _pfnInlineFormat;



        internal InlineFormat InlineFormatDelegate
        {
            [SecurityCritical]
            get
            {
                unsafe
                {
                    if (_pfnInlineFormat == null)
                        _pfnInlineFormat = new InlineFormat(this.InlineFormat);
                    return _pfnInlineFormat;
                }
            }
        }




        [SecurityCritical]
        private InlineDraw _pfnInlineDraw;



        internal InlineDraw InlineDrawDelegate
        {
            [SecurityCritical]
            get
            {
                if (_pfnInlineDraw == null)
                    _pfnInlineDraw = new InlineDraw(this.InlineDraw);
                return _pfnInlineDraw;
            }
        }








        [SecurityCritical]
        private void SaveException(Exception e, Plsrun plsrun, LSRun lsrun)
        {
            e.Data[ExceptionContext.Key] = new ExceptionContext(e.Data[ExceptionContext.Key], e.StackTrace, plsrun, lsrun);
            _exception = e;
        }




        [SecurityCritical]
        private void SaveNonCLSException(string methodName, Plsrun plsrun, LSRun lsrun)
        {
            Exception e = new System.Exception(SR.Get(SRID.NonCLSException));
            e.Data[ExceptionContext.Key] = new ExceptionContext(null, methodName, plsrun, lsrun);
            _exception = e;
        }

        [Serializable()]
        private class ExceptionContext
        {
            public ExceptionContext(object innerContext, string stackTraceOrMethodName, Plsrun plsrun, LSRun lsrun)
            {
                _stackTraceOrMethodName = stackTraceOrMethodName;
                _plsrun = (uint)plsrun;
                _lsrun = lsrun;
                _innerContext = innerContext;
            }

            public override string ToString()
            {
                return _stackTraceOrMethodName;
            }

            public const string Key = "ExceptionContext";

            private object _innerContext;
            private string _stackTraceOrMethodName;
            private uint _plsrun;

            [NonSerialized()]
            private LSRun _lsrun;
        }





        [SecurityCritical]
        private Exception _exception;

        internal Exception Exception
        {
            [SecurityCritical]
            get { return _exception; }
            [SecurityCritical]
            set { _exception = value; }
        }










        [SecurityCritical]
        private object _owner;




        internal object Owner
        {
            [SecurityCritical]
            get { return _owner; }
            [SecurityCritical]
            set { _owner = value; }
        }




        private FullTextState FullText
        {
            [SecurityCritical]
            get { return _owner as FullTextState; }
        }




        private DrawingState Draw
        {
            [SecurityCritical]
            get { return _owner as DrawingState; }
        }


        private Rect _boundingBox;




        internal void EmptyBoundingBox()
        {
            _boundingBox = Rect.Empty;
        }




        internal Rect BoundingBox
        {
            get { return _boundingBox; }
        }



        private ICollection<IndexedGlyphRun> _indexedGlyphRuns;

        internal void ClearIndexedGlyphRuns()
        {

            _indexedGlyphRuns = null;
        }




        internal ICollection<IndexedGlyphRun> IndexedGlyphRuns
        {
            get 
            {
                if (_indexedGlyphRuns == null)
                {
                    _indexedGlyphRuns = new List<IndexedGlyphRun>(8);                    
                }
                
                return _indexedGlyphRuns;
            }            
        }
    }
}
