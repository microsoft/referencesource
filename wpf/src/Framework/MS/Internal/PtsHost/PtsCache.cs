/* 


* 11/17/07 - bartde
*
* NOTICE: Code excluded from Developer Reference Sources.
*         Don't remove the SSS_DROP_BEGIN directive on top of the file.
*
* Reason for exclusion: obscure PTLS interface
*
**************************************************************************/












using System;                                   // IntPtr
using System.Collections.Generic;               // List<T>
using System.Security;                          // SecurityCritical, SecurityTreatAsSafe
using System.Threading;                         // Interlocked
using System.Windows;                           // WrapDirection
using System.Windows.Media.TextFormatting;      // TextFormatter
using System.Windows.Threading;                 // Dispatcher
using System.Windows.Media;
using MS.Internal.Text;                         // TextDpi
using MS.Internal.TextFormatting;               // UnsafeTextPenaltyModule
using MS.Internal.PtsHost.UnsafeNativeMethods;  // PTS

namespace MS.Internal.PtsHost
{















    internal sealed class PtsCache
    {






        #region Internal Methods





        internal static PtsHost AcquireContext(PtsContext ptsContext, TextFormattingMode textFormattingMode)
        {
            PtsCache ptsCache = ptsContext.Dispatcher.PtsCache as PtsCache;
            if (ptsCache == null)
            {
                ptsCache = new PtsCache(ptsContext.Dispatcher);
                ptsContext.Dispatcher.PtsCache = ptsCache;
            }
            return ptsCache.AcquireContextCore(ptsContext, textFormattingMode);
        }





        internal static void ReleaseContext(PtsContext ptsContext)
        {
            PtsCache ptsCache = ptsContext.Dispatcher.PtsCache as PtsCache;
            Invariant.Assert(ptsCache != null, "Cannot retrieve PtsCache from PtsContext object.");
            ptsCache.ReleaseContextCore(ptsContext);
        }











        [SecurityCritical]
        internal static void GetFloaterHandlerInfo(PtsHost ptsHost, IntPtr pobjectinfo)
        {
            PtsCache ptsCache = Dispatcher.CurrentDispatcher.PtsCache as PtsCache;
            Invariant.Assert(ptsCache != null, "Cannot retrieve PtsCache from the current Dispatcher.");
            ptsCache.GetFloaterHandlerInfoCore(ptsHost, pobjectinfo);
        }











        [SecurityCritical]
        internal static void GetTableObjHandlerInfo(PtsHost ptsHost, IntPtr pobjectinfo)
        {
            PtsCache ptsCache = Dispatcher.CurrentDispatcher.PtsCache as PtsCache;
            Invariant.Assert(ptsCache != null, "Cannot retrieve PtsCache from the current Dispatcher.");
            ptsCache.GetTableObjHandlerInfoCore(ptsHost, pobjectinfo);
        }




        internal static bool IsDisposed()
        {
            bool disposed = true;
            Dispatcher dispatcher = Dispatcher.CurrentDispatcher;
            if (dispatcher != null)
            {
                PtsCache ptsCache = Dispatcher.CurrentDispatcher.PtsCache as PtsCache;
                if (ptsCache != null)
                {
                    disposed = (ptsCache._disposed == 1);
                }
            }
            return disposed;
        }

        #endregion Internal Methods







        #region Private Methods









        [SecurityCritical, SecurityTreatAsSafe]
        private PtsCache(Dispatcher dispatcher)
        {



            _contextPool = new List<ContextDesc>(1);










            PtsCacheShutDownListener listener = new PtsCacheShutDownListener(this);
        }




        ~PtsCache()
        {

            if (0 == Interlocked.CompareExchange(ref _disposed, 1, 0))
            {

                DestroyPTSContexts();
            }
        }















        [SecurityCritical, SecurityTreatAsSafe]
        private PtsHost AcquireContextCore(PtsContext ptsContext, TextFormattingMode textFormattingMode)
        {
            int index;


            for (index = 0; index < _contextPool.Count; index++)
            {
                if (!_contextPool[index].InUse &&
                    _contextPool[index].IsOptimalParagraphEnabled == ptsContext.IsOptimalParagraphEnabled)
                {
                    break;
                }
            }


            if (index == _contextPool.Count)
            {
                _contextPool.Add(new ContextDesc());
                _contextPool[index].IsOptimalParagraphEnabled = ptsContext.IsOptimalParagraphEnabled;
                _contextPool[index].PtsHost = new PtsHost();
                _contextPool[index].PtsHost.Context = CreatePTSContext(index, textFormattingMode);
            }



            if (_contextPool[index].IsOptimalParagraphEnabled)
            {
                ptsContext.TextFormatter = _contextPool[index].TextFormatter;
            }


            _contextPool[index].InUse = true;
            _contextPool[index].Owner = new WeakReference(ptsContext);

            return _contextPool[index].PtsHost;
        }





        private void ReleaseContextCore(PtsContext ptsContext)
        {

            lock (_lock)
            {


                if (_disposed == 0)
                {




                    if (_releaseQueue == null)
                    {
                        _releaseQueue = new List<PtsContext>();
                        ptsContext.Dispatcher.BeginInvoke(DispatcherPriority.Background, new DispatcherOperationCallback(OnPtsContextReleased), null);
                    }
                    _releaseQueue.Add(ptsContext);
                }
            }
        }













        [SecurityCritical]
        private void GetFloaterHandlerInfoCore(PtsHost ptsHost, IntPtr pobjectinfo)
        {
            int index;
            for (index = 0; index < _contextPool.Count; index++)
            {
                if (_contextPool[index].PtsHost == ptsHost)
                {
                    break;
                }
            }
            Invariant.Assert(index < _contextPool.Count, "Cannot find matching PtsHost in the Context pool.");
            PTS.Validate(PTS.GetFloaterHandlerInfo(ref _contextPool[index].FloaterInit, pobjectinfo));
        }













        [SecurityCritical]
        private void GetTableObjHandlerInfoCore(PtsHost ptsHost, IntPtr pobjectinfo)
        {
            int index;
            for (index = 0; index < _contextPool.Count; index++)
            {
                if (_contextPool[index].PtsHost == ptsHost)
                {
                    break;
                }
            }
            Invariant.Assert(index < _contextPool.Count, "Cannot find matching PtsHost in the context pool.");
            PTS.Validate(PTS.GetTableObjHandlerInfo(ref _contextPool[index].TableobjInit, pobjectinfo));
        }




        private void Shutdown()
        {




            GC.WaitForPendingFinalizers();



            if (0 == Interlocked.CompareExchange(ref _disposed, 1, 0))
            {

                OnPtsContextReleased(false);


                DestroyPTSContexts();
            }
        }











        [SecurityCritical, SecurityTreatAsSafe]
        private void DestroyPTSContexts()
        {

            int index = 0;
            while (index < _contextPool.Count)
            {
                PtsContext ptsContext = _contextPool[index].Owner.Target as PtsContext;
                if (ptsContext != null)
                {
                    Invariant.Assert(_contextPool[index].PtsHost.Context == ptsContext.Context, "PTS Context mismatch.");
                    _contextPool[index].Owner = new WeakReference(null);
                    _contextPool[index].InUse = false;

                    Invariant.Assert(!ptsContext.Disposed, "PtsContext has been already disposed.");
                    ptsContext.Dispose();
                }

                if (!_contextPool[index].InUse)
                {



                    Invariant.Assert(_contextPool[index].PtsHost.Context != IntPtr.Zero, "PTS Context handle is not valid.");
                    PTS.IgnoreError(PTS.DestroyDocContext(_contextPool[index].PtsHost.Context));
                    Invariant.Assert(_contextPool[index].InstalledObjects != IntPtr.Zero, "Installed Objects handle is not valid.");
                    PTS.IgnoreError(PTS.DestroyInstalledObjectsInfo(_contextPool[index].InstalledObjects));


                    if (_contextPool[index].TextPenaltyModule != null)
                    {
                        _contextPool[index].TextPenaltyModule.Dispose();
                    }

                    _contextPool.RemoveAt(index);
                }
                else
                {
                    index++;
                }
            }
        }





        private object OnPtsContextReleased(object args)
        {
            OnPtsContextReleased(true);
            return null;
        }














        [SecurityCritical, SecurityTreatAsSafe]
        private void OnPtsContextReleased(bool cleanContextPool)
        {
            int index;


            lock (_lock)
            {

                if (_releaseQueue != null)
                {
                    foreach (PtsContext ptsContext in _releaseQueue)
                    {

                        for (index = 0; index < _contextPool.Count; index++)
                        {
                            if (_contextPool[index].PtsHost.Context == ptsContext.Context)
                            {
                                _contextPool[index].Owner = new WeakReference(null);
                                _contextPool[index].InUse = false;
                                break;
                            }
                        }
                        Invariant.Assert(index < _contextPool.Count, "PtsContext not found in the context pool.");


                        Invariant.Assert(!ptsContext.Disposed, "PtsContext has been already disposed.");
                        ptsContext.Dispose();
                    }
                    _releaseQueue = null;
                }
            }


            if (cleanContextPool && _contextPool.Count > 4)
            {

                index = 4;
                while (index < _contextPool.Count)
                {
                    if (!_contextPool[index].InUse)
                    {
                        Invariant.Assert(_contextPool[index].PtsHost.Context != IntPtr.Zero, "PTS Context handle is not valid.");
                        PTS.Validate(PTS.DestroyDocContext(_contextPool[index].PtsHost.Context));
                        Invariant.Assert(_contextPool[index].InstalledObjects != IntPtr.Zero, "Installed Objects handle is not valid.");
                        PTS.Validate(PTS.DestroyInstalledObjectsInfo(_contextPool[index].InstalledObjects));


                        if (_contextPool[index].TextPenaltyModule != null)
                        {
                            _contextPool[index].TextPenaltyModule.Dispose();
                        }
                        _contextPool.RemoveAt(index);
                        continue;
                    }
                    index++;
                }
            }
        }













        [SecurityCritical]
        private IntPtr CreatePTSContext(int index, TextFormattingMode textFormattingMode)
        {
            PtsHost ptsHost;
            IntPtr installedObjects;
            int installedObjectsCount;
            TextFormatterContext textFormatterContext;
            IntPtr context;

            ptsHost = _contextPool[index].PtsHost;
            Invariant.Assert(ptsHost != null);


            InitInstalledObjectsInfo(ptsHost, ref _contextPool[index].SubtrackParaInfo, ref _contextPool[index].SubpageParaInfo, out installedObjects, out installedObjectsCount);
            _contextPool[index].InstalledObjects = installedObjects;


            InitGenericInfo(ptsHost, (IntPtr)(index + 1), installedObjects, installedObjectsCount, ref _contextPool[index].ContextInfo);


            InitFloaterObjInfo(ptsHost, ref _contextPool[index].FloaterInit);
            InitTableObjInfo(ptsHost, ref _contextPool[index].TableobjInit);


            if (_contextPool[index].IsOptimalParagraphEnabled)
            {
                textFormatterContext = new TextFormatterContext();
                TextPenaltyModule penaltyModule = textFormatterContext.GetTextPenaltyModule();
                IntPtr ptsPenaltyModule = penaltyModule.DangerousGetHandle();

                _contextPool[index].TextPenaltyModule = penaltyModule;
                _contextPool[index].ContextInfo.ptsPenaltyModule = ptsPenaltyModule;
                _contextPool[index].TextFormatter = TextFormatter.CreateFromContext(textFormatterContext, textFormattingMode);





                GC.SuppressFinalize(_contextPool[index].TextPenaltyModule);
            }


            PTS.Validate(PTS.CreateDocContext(ref _contextPool[index].ContextInfo, out context));

            return context;
        }

















        [SecurityCritical]
        private unsafe void InitGenericInfo(PtsHost ptsHost, IntPtr clientData, IntPtr installedObjects, int installedObjectsCount, ref PTS.FSCONTEXTINFO contextInfo)
        {

            Invariant.Assert(((int)PTS.FSKREF.fskrefPage) == 0);
            Invariant.Assert(((int)PTS.FSKREF.fskrefMargin) == 1);
            Invariant.Assert(((int)PTS.FSKREF.fskrefParagraph) == 2);
            Invariant.Assert(((int)PTS.FSKREF.fskrefChar) == 3);
            Invariant.Assert(((int)PTS.FSKALIGNFIG.fskalfMin) == 0);
            Invariant.Assert(((int)PTS.FSKALIGNFIG.fskalfCenter) == 1);
            Invariant.Assert(((int)PTS.FSKALIGNFIG.fskalfMax) == 2);
            Invariant.Assert(((int)PTS.FSKWRAP.fskwrNone) == ((int)WrapDirection.None));
            Invariant.Assert(((int)PTS.FSKWRAP.fskwrLeft) == ((int)WrapDirection.Left));
            Invariant.Assert(((int)PTS.FSKWRAP.fskwrRight) == ((int)WrapDirection.Right));
            Invariant.Assert(((int)PTS.FSKWRAP.fskwrBoth) == ((int)WrapDirection.Both));
            Invariant.Assert(((int)PTS.FSKWRAP.fskwrLargest) == 4);
            Invariant.Assert(((int)PTS.FSKCLEAR.fskclearNone) == 0);
            Invariant.Assert(((int)PTS.FSKCLEAR.fskclearLeft) == 1);
            Invariant.Assert(((int)PTS.FSKCLEAR.fskclearRight) == 2);
            Invariant.Assert(((int)PTS.FSKCLEAR.fskclearBoth) == 3);


            contextInfo.version = 0;
            contextInfo.fsffi = PTS.fsffiUseTextQuickLoop








                | PTS.fsffiAvalonDisableOptimalInChains;
            contextInfo.drMinColumnBalancingStep = TextDpi.ToTextDpi(10.0);  // Assume 10px as minimal step
            contextInfo.cInstalledObjects = installedObjectsCount;
            contextInfo.pInstalledObjects = installedObjects;
            contextInfo.pfsclient = clientData;
            contextInfo.pfnAssertFailed = new PTS.AssertFailed(ptsHost.AssertFailed);

            contextInfo.fscbk.cbkfig.pfnGetFigureProperties = new PTS.GetFigureProperties(ptsHost.GetFigureProperties);
            contextInfo.fscbk.cbkfig.pfnGetFigurePolygons = new PTS.GetFigurePolygons(ptsHost.GetFigurePolygons);
            contextInfo.fscbk.cbkfig.pfnCalcFigurePosition = new PTS.CalcFigurePosition(ptsHost.CalcFigurePosition);

            contextInfo.fscbk.cbkgen.pfnFSkipPage = new PTS.FSkipPage(ptsHost.FSkipPage);
            contextInfo.fscbk.cbkgen.pfnGetPageDimensions = new PTS.GetPageDimensions(ptsHost.GetPageDimensions);
            contextInfo.fscbk.cbkgen.pfnGetNextSection = new PTS.GetNextSection(ptsHost.GetNextSection);
            contextInfo.fscbk.cbkgen.pfnGetSectionProperties = new PTS.GetSectionProperties(ptsHost.GetSectionProperties);
            contextInfo.fscbk.cbkgen.pfnGetJustificationProperties = new PTS.GetJustificationProperties(ptsHost.GetJustificationProperties);
            contextInfo.fscbk.cbkgen.pfnGetMainTextSegment = new PTS.GetMainTextSegment(ptsHost.GetMainTextSegment);
            contextInfo.fscbk.cbkgen.pfnGetHeaderSegment = new PTS.GetHeaderSegment(ptsHost.GetHeaderSegment);
            contextInfo.fscbk.cbkgen.pfnGetFooterSegment = new PTS.GetFooterSegment(ptsHost.GetFooterSegment);
            contextInfo.fscbk.cbkgen.pfnUpdGetSegmentChange = new PTS.UpdGetSegmentChange(ptsHost.UpdGetSegmentChange);
            contextInfo.fscbk.cbkgen.pfnGetSectionColumnInfo = new PTS.GetSectionColumnInfo(ptsHost.GetSectionColumnInfo);
            contextInfo.fscbk.cbkgen.pfnGetSegmentDefinedColumnSpanAreaInfo = new PTS.GetSegmentDefinedColumnSpanAreaInfo(ptsHost.GetSegmentDefinedColumnSpanAreaInfo);
            contextInfo.fscbk.cbkgen.pfnGetHeightDefinedColumnSpanAreaInfo = new PTS.GetHeightDefinedColumnSpanAreaInfo(ptsHost.GetHeightDefinedColumnSpanAreaInfo);
            contextInfo.fscbk.cbkgen.pfnGetFirstPara = new PTS.GetFirstPara(ptsHost.GetFirstPara);
            contextInfo.fscbk.cbkgen.pfnGetNextPara = new PTS.GetNextPara(ptsHost.GetNextPara);
            contextInfo.fscbk.cbkgen.pfnUpdGetFirstChangeInSegment = new PTS.UpdGetFirstChangeInSegment(ptsHost.UpdGetFirstChangeInSegment);
            contextInfo.fscbk.cbkgen.pfnUpdGetParaChange = new PTS.UpdGetParaChange(ptsHost.UpdGetParaChange);
            contextInfo.fscbk.cbkgen.pfnGetParaProperties = new PTS.GetParaProperties(ptsHost.GetParaProperties);
            contextInfo.fscbk.cbkgen.pfnCreateParaclient = new PTS.CreateParaclient(ptsHost.CreateParaclient);
            contextInfo.fscbk.cbkgen.pfnTransferDisplayInfo = new PTS.TransferDisplayInfo(ptsHost.TransferDisplayInfo);
            contextInfo.fscbk.cbkgen.pfnDestroyParaclient = new PTS.DestroyParaclient(ptsHost.DestroyParaclient);
            contextInfo.fscbk.cbkgen.pfnFInterruptFormattingAfterPara = new PTS.FInterruptFormattingAfterPara(ptsHost.FInterruptFormattingAfterPara);
            contextInfo.fscbk.cbkgen.pfnGetEndnoteSeparators = new PTS.GetEndnoteSeparators(ptsHost.GetEndnoteSeparators);
            contextInfo.fscbk.cbkgen.pfnGetEndnoteSegment = new PTS.GetEndnoteSegment(ptsHost.GetEndnoteSegment);
            contextInfo.fscbk.cbkgen.pfnGetNumberEndnoteColumns = new PTS.GetNumberEndnoteColumns(ptsHost.GetNumberEndnoteColumns);
            contextInfo.fscbk.cbkgen.pfnGetEndnoteColumnInfo = new PTS.GetEndnoteColumnInfo(ptsHost.GetEndnoteColumnInfo);
            contextInfo.fscbk.cbkgen.pfnGetFootnoteSeparators = new PTS.GetFootnoteSeparators(ptsHost.GetFootnoteSeparators);
            contextInfo.fscbk.cbkgen.pfnFFootnoteBeneathText = new PTS.FFootnoteBeneathText(ptsHost.FFootnoteBeneathText);
            contextInfo.fscbk.cbkgen.pfnGetNumberFootnoteColumns = new PTS.GetNumberFootnoteColumns(ptsHost.GetNumberFootnoteColumns);
            contextInfo.fscbk.cbkgen.pfnGetFootnoteColumnInfo = new PTS.GetFootnoteColumnInfo(ptsHost.GetFootnoteColumnInfo);
            contextInfo.fscbk.cbkgen.pfnGetFootnoteSegment = new PTS.GetFootnoteSegment(ptsHost.GetFootnoteSegment);
            contextInfo.fscbk.cbkgen.pfnGetFootnotePresentationAndRejectionOrder = new PTS.GetFootnotePresentationAndRejectionOrder(ptsHost.GetFootnotePresentationAndRejectionOrder);
            contextInfo.fscbk.cbkgen.pfnFAllowFootnoteSeparation = new PTS.FAllowFootnoteSeparation(ptsHost.FAllowFootnoteSeparation);




            contextInfo.fscbk.cbkobj.pfnDuplicateMcsclient = new PTS.DuplicateMcsclient(ptsHost.DuplicateMcsclient);
            contextInfo.fscbk.cbkobj.pfnDestroyMcsclient = new PTS.DestroyMcsclient(ptsHost.DestroyMcsclient);
            contextInfo.fscbk.cbkobj.pfnFEqualMcsclient = new PTS.FEqualMcsclient(ptsHost.FEqualMcsclient);
            contextInfo.fscbk.cbkobj.pfnConvertMcsclient = new PTS.ConvertMcsclient(ptsHost.ConvertMcsclient);
            contextInfo.fscbk.cbkobj.pfnGetObjectHandlerInfo = new PTS.GetObjectHandlerInfo(ptsHost.GetObjectHandlerInfo);

            contextInfo.fscbk.cbktxt.pfnCreateParaBreakingSession = new PTS.CreateParaBreakingSession(ptsHost.CreateParaBreakingSession);
            contextInfo.fscbk.cbktxt.pfnDestroyParaBreakingSession = new PTS.DestroyParaBreakingSession(ptsHost.DestroyParaBreakingSession);
            contextInfo.fscbk.cbktxt.pfnGetTextProperties = new PTS.GetTextProperties(ptsHost.GetTextProperties);
            contextInfo.fscbk.cbktxt.pfnGetNumberFootnotes = new PTS.GetNumberFootnotes(ptsHost.GetNumberFootnotes);
            contextInfo.fscbk.cbktxt.pfnGetFootnotes = new PTS.GetFootnotes(ptsHost.GetFootnotes);
            contextInfo.fscbk.cbktxt.pfnFormatDropCap = new PTS.FormatDropCap(ptsHost.FormatDropCap);
            contextInfo.fscbk.cbktxt.pfnGetDropCapPolygons = new PTS.GetDropCapPolygons(ptsHost.GetDropCapPolygons);
            contextInfo.fscbk.cbktxt.pfnDestroyDropCap = new PTS.DestroyDropCap(ptsHost.DestroyDropCap);
            contextInfo.fscbk.cbktxt.pfnFormatBottomText = new PTS.FormatBottomText(ptsHost.FormatBottomText);
            contextInfo.fscbk.cbktxt.pfnFormatLine = new PTS.FormatLine(ptsHost.FormatLine);
            contextInfo.fscbk.cbktxt.pfnFormatLineForced = new PTS.FormatLineForced(ptsHost.FormatLineForced);
            contextInfo.fscbk.cbktxt.pfnFormatLineVariants = new PTS.FormatLineVariants(ptsHost.FormatLineVariants);
            contextInfo.fscbk.cbktxt.pfnReconstructLineVariant = new PTS.ReconstructLineVariant(ptsHost.ReconstructLineVariant);
            contextInfo.fscbk.cbktxt.pfnDestroyLine = new PTS.DestroyLine(ptsHost.DestroyLine);
            contextInfo.fscbk.cbktxt.pfnDuplicateLineBreakRecord = new PTS.DuplicateLineBreakRecord(ptsHost.DuplicateLineBreakRecord);
            contextInfo.fscbk.cbktxt.pfnDestroyLineBreakRecord = new PTS.DestroyLineBreakRecord(ptsHost.DestroyLineBreakRecord);
            contextInfo.fscbk.cbktxt.pfnSnapGridVertical = new PTS.SnapGridVertical(ptsHost.SnapGridVertical);
            contextInfo.fscbk.cbktxt.pfnGetDvrSuppressibleBottomSpace = new PTS.GetDvrSuppressibleBottomSpace(ptsHost.GetDvrSuppressibleBottomSpace);
            contextInfo.fscbk.cbktxt.pfnGetDvrAdvance = new PTS.GetDvrAdvance(ptsHost.GetDvrAdvance);
            contextInfo.fscbk.cbktxt.pfnUpdGetChangeInText = new PTS.UpdGetChangeInText(ptsHost.UpdGetChangeInText);
            contextInfo.fscbk.cbktxt.pfnUpdGetDropCapChange = new PTS.UpdGetDropCapChange(ptsHost.UpdGetDropCapChange);
            contextInfo.fscbk.cbktxt.pfnFInterruptFormattingText = new PTS.FInterruptFormattingText(ptsHost.FInterruptFormattingText);
            contextInfo.fscbk.cbktxt.pfnGetTextParaCache = new PTS.GetTextParaCache(ptsHost.GetTextParaCache);
            contextInfo.fscbk.cbktxt.pfnSetTextParaCache = new PTS.SetTextParaCache(ptsHost.SetTextParaCache);
            contextInfo.fscbk.cbktxt.pfnGetOptimalLineDcpCache = new PTS.GetOptimalLineDcpCache(ptsHost.GetOptimalLineDcpCache);
            contextInfo.fscbk.cbktxt.pfnGetNumberAttachedObjectsBeforeTextLine = new PTS.GetNumberAttachedObjectsBeforeTextLine(ptsHost.GetNumberAttachedObjectsBeforeTextLine);
            contextInfo.fscbk.cbktxt.pfnGetAttachedObjectsBeforeTextLine = new PTS.GetAttachedObjectsBeforeTextLine(ptsHost.GetAttachedObjectsBeforeTextLine);
            contextInfo.fscbk.cbktxt.pfnGetNumberAttachedObjectsInTextLine = new PTS.GetNumberAttachedObjectsInTextLine(ptsHost.GetNumberAttachedObjectsInTextLine);
            contextInfo.fscbk.cbktxt.pfnGetAttachedObjectsInTextLine = new PTS.GetAttachedObjectsInTextLine(ptsHost.GetAttachedObjectsInTextLine);
            contextInfo.fscbk.cbktxt.pfnUpdGetAttachedObjectChange = new PTS.UpdGetAttachedObjectChange(ptsHost.UpdGetAttachedObjectChange);
            contextInfo.fscbk.cbktxt.pfnGetDurFigureAnchor = new PTS.GetDurFigureAnchor(ptsHost.GetDurFigureAnchor);
        }

















        [SecurityCritical]
        private unsafe void InitInstalledObjectsInfo(PtsHost ptsHost, ref PTS.FSIMETHODS subtrackParaInfo, ref PTS.FSIMETHODS subpageParaInfo, out IntPtr installedObjects, out int installedObjectsCount)
        {

            subtrackParaInfo.pfnCreateContext = new PTS.ObjCreateContext(ptsHost.SubtrackCreateContext);
            subtrackParaInfo.pfnDestroyContext = new PTS.ObjDestroyContext(ptsHost.SubtrackDestroyContext);
            subtrackParaInfo.pfnFormatParaFinite = new PTS.ObjFormatParaFinite(ptsHost.SubtrackFormatParaFinite);
            subtrackParaInfo.pfnFormatParaBottomless = new PTS.ObjFormatParaBottomless(ptsHost.SubtrackFormatParaBottomless);
            subtrackParaInfo.pfnUpdateBottomlessPara = new PTS.ObjUpdateBottomlessPara(ptsHost.SubtrackUpdateBottomlessPara);
            subtrackParaInfo.pfnSynchronizeBottomlessPara = new PTS.ObjSynchronizeBottomlessPara(ptsHost.SubtrackSynchronizeBottomlessPara);
            subtrackParaInfo.pfnComparePara = new PTS.ObjComparePara(ptsHost.SubtrackComparePara);
            subtrackParaInfo.pfnClearUpdateInfoInPara = new PTS.ObjClearUpdateInfoInPara(ptsHost.SubtrackClearUpdateInfoInPara);
            subtrackParaInfo.pfnDestroyPara = new PTS.ObjDestroyPara(ptsHost.SubtrackDestroyPara);
            subtrackParaInfo.pfnDuplicateBreakRecord = new PTS.ObjDuplicateBreakRecord(ptsHost.SubtrackDuplicateBreakRecord);
            subtrackParaInfo.pfnDestroyBreakRecord = new PTS.ObjDestroyBreakRecord(ptsHost.SubtrackDestroyBreakRecord);
            subtrackParaInfo.pfnGetColumnBalancingInfo = new PTS.ObjGetColumnBalancingInfo(ptsHost.SubtrackGetColumnBalancingInfo);
            subtrackParaInfo.pfnGetNumberFootnotes = new PTS.ObjGetNumberFootnotes(ptsHost.SubtrackGetNumberFootnotes);
            subtrackParaInfo.pfnGetFootnoteInfo = new PTS.ObjGetFootnoteInfo(ptsHost.SubtrackGetFootnoteInfo);
            subtrackParaInfo.pfnGetFootnoteInfoWord = IntPtr.Zero;
            subtrackParaInfo.pfnShiftVertical = new PTS.ObjShiftVertical(ptsHost.SubtrackShiftVertical);
            subtrackParaInfo.pfnTransferDisplayInfoPara = new PTS.ObjTransferDisplayInfoPara(ptsHost.SubtrackTransferDisplayInfoPara);


            subpageParaInfo.pfnCreateContext = new PTS.ObjCreateContext(ptsHost.SubpageCreateContext);
            subpageParaInfo.pfnDestroyContext = new PTS.ObjDestroyContext(ptsHost.SubpageDestroyContext);
            subpageParaInfo.pfnFormatParaFinite = new PTS.ObjFormatParaFinite(ptsHost.SubpageFormatParaFinite);
            subpageParaInfo.pfnFormatParaBottomless = new PTS.ObjFormatParaBottomless(ptsHost.SubpageFormatParaBottomless);
            subpageParaInfo.pfnUpdateBottomlessPara = new PTS.ObjUpdateBottomlessPara(ptsHost.SubpageUpdateBottomlessPara);
            subpageParaInfo.pfnSynchronizeBottomlessPara = new PTS.ObjSynchronizeBottomlessPara(ptsHost.SubpageSynchronizeBottomlessPara);
            subpageParaInfo.pfnComparePara = new PTS.ObjComparePara(ptsHost.SubpageComparePara);
            subpageParaInfo.pfnClearUpdateInfoInPara = new PTS.ObjClearUpdateInfoInPara(ptsHost.SubpageClearUpdateInfoInPara);
            subpageParaInfo.pfnDestroyPara = new PTS.ObjDestroyPara(ptsHost.SubpageDestroyPara);
            subpageParaInfo.pfnDuplicateBreakRecord = new PTS.ObjDuplicateBreakRecord(ptsHost.SubpageDuplicateBreakRecord);
            subpageParaInfo.pfnDestroyBreakRecord = new PTS.ObjDestroyBreakRecord(ptsHost.SubpageDestroyBreakRecord);
            subpageParaInfo.pfnGetColumnBalancingInfo = new PTS.ObjGetColumnBalancingInfo(ptsHost.SubpageGetColumnBalancingInfo);
            subpageParaInfo.pfnGetNumberFootnotes = new PTS.ObjGetNumberFootnotes(ptsHost.SubpageGetNumberFootnotes);
            subpageParaInfo.pfnGetFootnoteInfo = new PTS.ObjGetFootnoteInfo(ptsHost.SubpageGetFootnoteInfo);
            subpageParaInfo.pfnShiftVertical = new PTS.ObjShiftVertical(ptsHost.SubpageShiftVertical);
            subpageParaInfo.pfnTransferDisplayInfoPara = new PTS.ObjTransferDisplayInfoPara(ptsHost.SubpageTransferDisplayInfoPara);


            PTS.Validate(PTS.CreateInstalledObjectsInfo(ref subtrackParaInfo, ref subpageParaInfo, out installedObjects, out installedObjectsCount));
        }













        [SecurityCritical]
        private unsafe void InitFloaterObjInfo(PtsHost ptsHost, ref PTS.FSFLOATERINIT floaterInit)
        {
            floaterInit.fsfloatercbk.pfnGetFloaterProperties = new PTS.GetFloaterProperties(ptsHost.GetFloaterProperties);
            floaterInit.fsfloatercbk.pfnFormatFloaterContentFinite = new PTS.FormatFloaterContentFinite(ptsHost.FormatFloaterContentFinite);
            floaterInit.fsfloatercbk.pfnFormatFloaterContentBottomless = new PTS.FormatFloaterContentBottomless(ptsHost.FormatFloaterContentBottomless);
            floaterInit.fsfloatercbk.pfnUpdateBottomlessFloaterContent = new PTS.UpdateBottomlessFloaterContent(ptsHost.UpdateBottomlessFloaterContent);
            floaterInit.fsfloatercbk.pfnGetFloaterPolygons = new PTS.GetFloaterPolygons(ptsHost.GetFloaterPolygons);
            floaterInit.fsfloatercbk.pfnClearUpdateInfoInFloaterContent = new PTS.ClearUpdateInfoInFloaterContent(ptsHost.ClearUpdateInfoInFloaterContent);
            floaterInit.fsfloatercbk.pfnCompareFloaterContents = new PTS.CompareFloaterContents(ptsHost.CompareFloaterContents);
            floaterInit.fsfloatercbk.pfnDestroyFloaterContent = new PTS.DestroyFloaterContent(ptsHost.DestroyFloaterContent);
            floaterInit.fsfloatercbk.pfnDuplicateFloaterContentBreakRecord = new PTS.DuplicateFloaterContentBreakRecord(ptsHost.DuplicateFloaterContentBreakRecord);
            floaterInit.fsfloatercbk.pfnDestroyFloaterContentBreakRecord = new PTS.DestroyFloaterContentBreakRecord(ptsHost.DestroyFloaterContentBreakRecord);
            floaterInit.fsfloatercbk.pfnGetFloaterContentColumnBalancingInfo = new PTS.GetFloaterContentColumnBalancingInfo(ptsHost.GetFloaterContentColumnBalancingInfo);
            floaterInit.fsfloatercbk.pfnGetFloaterContentNumberFootnotes = new PTS.GetFloaterContentNumberFootnotes(ptsHost.GetFloaterContentNumberFootnotes);
            floaterInit.fsfloatercbk.pfnGetFloaterContentFootnoteInfo = new PTS.GetFloaterContentFootnoteInfo(ptsHost.GetFloaterContentFootnoteInfo);
            floaterInit.fsfloatercbk.pfnTransferDisplayInfoInFloaterContent = new PTS.TransferDisplayInfoInFloaterContent(ptsHost.TransferDisplayInfoInFloaterContent);
            floaterInit.fsfloatercbk.pfnGetMCSClientAfterFloater = new PTS.GetMCSClientAfterFloater(ptsHost.GetMCSClientAfterFloater);
            floaterInit.fsfloatercbk.pfnGetDvrUsedForFloater = new PTS.GetDvrUsedForFloater(ptsHost.GetDvrUsedForFloater);
        }













        [SecurityCritical]
        private unsafe void InitTableObjInfo(PtsHost ptsHost, ref PTS.FSTABLEOBJINIT tableobjInit)
        {

            tableobjInit.tableobjcbk.pfnGetTableProperties = new PTS.GetTableProperties(ptsHost.GetTableProperties);
            tableobjInit.tableobjcbk.pfnAutofitTable = new PTS.AutofitTable(ptsHost.AutofitTable);
            tableobjInit.tableobjcbk.pfnUpdAutofitTable = new PTS.UpdAutofitTable(ptsHost.UpdAutofitTable);
            tableobjInit.tableobjcbk.pfnGetMCSClientAfterTable = new PTS.GetMCSClientAfterTable(ptsHost.GetMCSClientAfterTable);
            tableobjInit.tableobjcbk.pfnGetDvrUsedForFloatTable = IntPtr.Zero;


            tableobjInit.tablecbkfetch.pfnGetFirstHeaderRow = new PTS.GetFirstHeaderRow(ptsHost.GetFirstHeaderRow);
            tableobjInit.tablecbkfetch.pfnGetNextHeaderRow = new PTS.GetNextHeaderRow(ptsHost.GetNextHeaderRow);
            tableobjInit.tablecbkfetch.pfnGetFirstFooterRow = new PTS.GetFirstFooterRow(ptsHost.GetFirstFooterRow);
            tableobjInit.tablecbkfetch.pfnGetNextFooterRow = new PTS.GetNextFooterRow(ptsHost.GetNextFooterRow);
            tableobjInit.tablecbkfetch.pfnGetFirstRow = new PTS.GetFirstRow(ptsHost.GetFirstRow);
            tableobjInit.tablecbkfetch.pfnGetNextRow = new PTS.GetNextRow(ptsHost.GetNextRow);
            tableobjInit.tablecbkfetch.pfnUpdFChangeInHeaderFooter = new PTS.UpdFChangeInHeaderFooter(ptsHost.UpdFChangeInHeaderFooter);
            tableobjInit.tablecbkfetch.pfnUpdGetFirstChangeInTable = new PTS.UpdGetFirstChangeInTable(ptsHost.UpdGetFirstChangeInTable);
            tableobjInit.tablecbkfetch.pfnUpdGetRowChange = new PTS.UpdGetRowChange(ptsHost.UpdGetRowChange);
            tableobjInit.tablecbkfetch.pfnUpdGetCellChange = new PTS.UpdGetCellChange(ptsHost.UpdGetCellChange);
            tableobjInit.tablecbkfetch.pfnGetDistributionKind = new PTS.GetDistributionKind(ptsHost.GetDistributionKind);
            tableobjInit.tablecbkfetch.pfnGetRowProperties = new PTS.GetRowProperties(ptsHost.GetRowProperties);
            tableobjInit.tablecbkfetch.pfnGetCells = new PTS.GetCells(ptsHost.GetCells);
            tableobjInit.tablecbkfetch.pfnFInterruptFormattingTable = new PTS.FInterruptFormattingTable(ptsHost.FInterruptFormattingTable);
            tableobjInit.tablecbkfetch.pfnCalcHorizontalBBoxOfRow = new PTS.CalcHorizontalBBoxOfRow(ptsHost.CalcHorizontalBBoxOfRow);


            tableobjInit.tablecbkcell.pfnFormatCellFinite = new PTS.FormatCellFinite(ptsHost.FormatCellFinite);
            tableobjInit.tablecbkcell.pfnFormatCellBottomless = new PTS.FormatCellBottomless(ptsHost.FormatCellBottomless);
            tableobjInit.tablecbkcell.pfnUpdateBottomlessCell = new PTS.UpdateBottomlessCell(ptsHost.UpdateBottomlessCell);
            tableobjInit.tablecbkcell.pfnCompareCells = new PTS.CompareCells(ptsHost.CompareCells);
            tableobjInit.tablecbkcell.pfnClearUpdateInfoInCell = new PTS.ClearUpdateInfoInCell(ptsHost.ClearUpdateInfoInCell);
            tableobjInit.tablecbkcell.pfnSetCellHeight = new PTS.SetCellHeight(ptsHost.SetCellHeight);
            tableobjInit.tablecbkcell.pfnDestroyCell = new PTS.DestroyCell(ptsHost.DestroyCell);
            tableobjInit.tablecbkcell.pfnDuplicateCellBreakRecord = new PTS.DuplicateCellBreakRecord(ptsHost.DuplicateCellBreakRecord);
            tableobjInit.tablecbkcell.pfnDestroyCellBreakRecord = new PTS.DestroyCellBreakRecord(ptsHost.DestroyCellBreakRecord);
            tableobjInit.tablecbkcell.pfnGetCellNumberFootnotes = new PTS.GetCellNumberFootnotes(ptsHost.GetCellNumberFootnotes);
            tableobjInit.tablecbkcell.pfnGetCellFootnoteInfo = IntPtr.Zero;
            tableobjInit.tablecbkcell.pfnGetCellFootnoteInfoWord = IntPtr.Zero;
            tableobjInit.tablecbkcell.pfnGetCellMinColumnBalancingStep = new PTS.GetCellMinColumnBalancingStep(ptsHost.GetCellMinColumnBalancingStep);
            tableobjInit.tablecbkcell.pfnTransferDisplayInfoCell = new PTS.TransferDisplayInfoCell(ptsHost.TransferDisplayInfoCell);



            tableobjInit.tablecbkfetchword.pfnGetTablePropertiesWord  = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnGetRowPropertiesWord    = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnGetNumberFiguresForTableRow = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnGetRowWidthWord = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnGetFiguresForTableRow   = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnFStopBeforeTableRowLr   = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnFIgnoreCollisionForTableRow = IntPtr.Zero;
            tableobjInit.tablecbkfetchword.pfnChangeRowHeightRestriction = IntPtr.Zero;
            */
        }

        #endregion Private Methods







        #region Private Fields





















        [SecurityCritical]
        private List<ContextDesc> _contextPool;




        private List<PtsContext> _releaseQueue;




        private object _lock = new object();




        private int _disposed;

        #endregion Private Fields







        #region Private Types









        private class ContextDesc
        {
            internal PtsHost PtsHost;
            internal PTS.FSCONTEXTINFO ContextInfo;
            internal PTS.FSIMETHODS SubtrackParaInfo;
            internal PTS.FSIMETHODS SubpageParaInfo;
            internal PTS.FSFLOATERINIT FloaterInit;
            internal PTS.FSTABLEOBJINIT TableobjInit;
            internal IntPtr InstalledObjects;
            internal TextFormatter TextFormatter;
            internal TextPenaltyModule TextPenaltyModule;
            internal bool IsOptimalParagraphEnabled;
            internal WeakReference Owner;
            internal bool InUse;
        }

        private sealed class PtsCacheShutDownListener : ShutDownListener
        {





            [SecurityCritical,SecurityTreatAsSafe]
            public PtsCacheShutDownListener(PtsCache target) : base(target)
            {
            }

            internal override void OnShutDown(object target, object sender, EventArgs e)
            {
                PtsCache ptsCache = (PtsCache)target;
                ptsCache.Shutdown();
            }
        }

        #endregion Private Types
    }
}
