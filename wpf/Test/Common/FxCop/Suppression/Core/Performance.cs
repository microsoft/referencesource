//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Ink.StrokeFIndices.get_IsValid():System.Boolean")]
#endif

//**************************************************************************************************************************
// Bug ID: 1101266
// Developer: lblanco
// Reason: This method is used by Test to force certain rendering modes, such as software even if hardware is available,
//         or 3D reference rasterizer instead of real hardware.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.Composition.DUCE+CompositionTarget.SetRenderingMode(System.Windows.Media.Composition.DUCE+Resource,MS.Internal.MILRTInitializationFlags,System.Windows.Media.Composition.DUCE+Channel):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1106772
// Developer: RobertWl
// Reason: The Imaging Team plans on use these post Beta 1. It doesn't make sense to remove them now and reintroduce later.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.PresentationCore.SRID.get_FileFormatException():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.PresentationCore.SRID.get_FileFormatExceptionWithFileName():MS.Internal.PresentationCore.SRID")]

//**************************************************************************************************************************
// Bug ID: 1100747
// Developer: cedricd
// Reason: InsertionMap is in the Shared directory and is thus copied in every assembly. Others use it.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.Sort():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.get_Count():System.Int32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.GetKeyValuePair(System.Int32, System.Int32&, System.Object&):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.set_Item(System.Int32, System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.get_Item(System.Int32):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1113889, 1099142
// Developer: CedricD / JoeL
// Reason: This is a generic class, and depending on the usage different methods will or will not used. I am adding the class
// once, instead of every method of the each class seperately.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.SingleItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.ThreeItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.SixItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.ArrayItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.FrugalObjectList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.FrugalStructList`1")]

[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.SingleItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.ThreeItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.SixItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.ArrayItemList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.FrugalObjectList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.FrugalStructList`1")]

//**************************************************************************************************************************
// Developer: bkaneva
// Reason: Unfortunately, the above suppressions don't seem to work even if you fix the generic type issue. So, we need to
// exclude the individual members
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalStructList`1.get_IsFixedSize():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalStructList`1.get_IsReadOnly():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.get_IsFixedSize():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.get_IsReadOnly():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ArrayItemList`1.Promote(MS.Utility.SixItemList`1<T>):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.get_Capacity():System.Int32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.EnsureIndex(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.Insert(System.Int32,T):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.CopyTo(T[],System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalObjectList`1.set_Item(System.Int32,T):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ThreeItemList`1.Promote(MS.Utility.SingleItemList`1<T>):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SixItemList`1.Promote(MS.Utility.ThreeItemList`1<T>):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalStructList`1.Sort():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.FrugalStructList`1.Clone():MS.Utility.FrugalStructList`1<T>")]

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.TextFormatting.CharacterBufferRange.get_IsEmpty():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1038893
// Developer: bkaneva
// Reason: There is a call to this method ColorTransform .ColorTransform (SafeMILHandle bitmapSource, ColorContext srcContext, ColorContext dstContext, System.Windows.Media.PixelFormat pixelFormat)
// It is an internal constructor
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.MILIcmColorTransform.CreateRasterTransform(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeProfileHandle,System.Windows.Media.SafeProfileHandle,MS.Internal.PixelFormatEnum):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1010435
// Developer: garyyang
// Reason:    IsFixedSize is part of the IList interface that must be implemented.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PartialArray`1.get_IsFixedSize():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1010436
// Developer: kenlai
// Reason: API completeness
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.SizeLimitedCache`2.get_MaximumItems():System.Int32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.SizeLimitedCache`2.Remove(K):System.Void")]

//**************************************************************************************************************************
// Developer: kenlai
// Reason: these functions are used in parameter to Debug.Assert()
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.ClassChainingSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.ClassContextSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.CoverageChainingSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.CoverageContextSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.GlyphChainingSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Shaping.GlyphContextSubtable.Format(MS.Internal.Shaping.FontTable):System.UInt16")]
// used in model3denumerator.cs
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Media3D.Matrix3DStack.get_Count():System.Int32")]
// 1010461
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.CachedFontFamily.get_IsComposite():System.Boolean")]
// 1081380
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.RealizationContext+RealizationUpdateSchedule.get_IsEmpty():System.Boolean")]
// 1086896
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Ink.StrokeIntersection.get_IsEmpty():System.Boolean")]
// 1086900
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.TimelineFlags.get_IsInteractivelyReversed():System.Boolean")]

//**************************************************************************************************************************
// Bug: 1010560
// Developer: kenlai
// Reason: this is used in timeline.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.TimelineCollection.InternalGetItem(System.Int32):System.Windows.Media.Animation.Timeline")]

//**************************************************************************************************************************
// Developer: kenlai
// Reason: this is used for debugging purposes
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.TimingEventList+TimingEvent.get_IsInteractive():System.Boolean")]

//**************************************************************************************************************************
// Developer: kenlai
// Reason: this is called through reflection in DRT
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.MediaContext.CompleteRender():System.Void")]

//**************************************************************************************************************************
// Task: 21308
// Developer: NiklasB
// Reason: this will be used in M11
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.FontFaceLayoutInfo.get_DesignRecommendedLeading():System.UInt16")]

//**************************************************************************************************************************
// Reason:
// Analyze() is created to serve the testing only. it is protected by security attribute to insure
// that BidiTest application only can call this method. the public key in the security attribute is
// generated from the BidiTest assembly by the command "sn -Tp biditest.exe"
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TextFormatting.Bidi.Analyze(System.Char[],System.Int32,System.Int32,MS.Internal.TextFormatting.Bidi+Flags,MS.Internal.TextFormatting.Bidi+State,System.Byte[]&,System.Int32&):System.Boolean")]

//**************************************************************************************************************************
// All of the following bugs are related (i.e. dead code bugs) but have different PS bug ids (but same reason for suppression)
// Developer: patricsw
// Reason: According to JordanPa (as cited in the bug) this is dead code but the owner of the dead code is planning
//         on using this code in M11. Thus, for now supressing the error seems to correct thing to do.
//***************************************************************************************************************************
//
// Bug 1010557
//
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.DrawingContextWalker.set_ShouldStopWalking(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.RenderDataDrawingContext.UseAnimations(System.Windows.Size,System.Windows.Media.Animation.AnimationClock):System.UInt32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.DrawingVisual.WalkContent(System.Windows.Media.DrawingContextWalker):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.MediaSystem.set_ForceMemoryRDP(System.Boolean):System.Void")]
//
// Bug 1010560
//
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.TimelineData.get_SubtreeFinalizer():System.Object")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.DoubleAnimationClockResource.get_BaseValue():System.Double")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.PointAnimationClockResource.get_BaseValue():System.Windows.Point")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.RectAnimationClockResource.get_BaseValue():System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Animation.SizeAnimationClockResource.get_BaseValue():System.Windows.Size")]
//
// Bug 1010567
//
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.TextFormatting.CharacterBufferRange.get_IsEmpty():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: ahodsdon
// Reason: These methods are code-gen'ed. It's possible that other implementers of these methods would use the parameters.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1801:AvoidUnusedParameters", Scope="member", Target="System.Windows.Media.MediaTimeline.CtorPostscript(System.Windows.Media.MediaTimeline,System.Windows.Media.Animation.Animatable+CloneType):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1801:AvoidUnusedParameters", Scope="member", Target="System.Windows.Media.MediaTimeline.CtorPrequel(System.Windows.Media.MediaTimeline,System.Windows.Media.Animation.Animatable+CloneType):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1801:AvoidUnusedParameters", Scope="member", Target="System.Windows.Media.MediaTimeline.CtorPrequel(System.Windows.Media.MediaTimeline,System.Windows.Media.Animation.Animatable+CloneType):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1801:AvoidUnusedParameters", Scope="member", Target="System.Windows.Media.MediaTimeline.CtorPrequel(System.Windows.Media.MediaTimeline,System.Windows.Media.Animation.Animatable+CloneType):System.Void")]


//**************************************************************************************************************************
// Developer: bkaneva
// Reason: There is a call to this method from the constructor of CompNodeVisual
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_CompNodeVisual_InvalidResourceHandle():MS.Internal.PresentationCore.SRID")]


//***************************************************************************************************************************
// Developer: bkaneva
// Reason: Shared string resource
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_TokenizerHelperExtraDataEncountered():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_FrugalMap_TargetMapCannotHoldAllData():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_CannotConvertStringToType():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_FrugalMap_CannotPromoteBeyondHashtable():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_TokenizerHelperEmptyToken():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_MoreThanOneStartingParts():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_TokenizerHelperPrematureStringTermination():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_Freezable_CantBeFrozen():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_Default():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_CannotModifyReadOnlyContainer():MS.Internal.PresentationCore.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_TokenizerHelperMissingEndQuote():MS.Internal.PresentationCore.SRID")]

//**************************************************************************************************************************
// Developer: bkaneva
// Reason: These methods are used by Test
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Interop.HwndTarget.SetRenderingMode(System.Windows.Interop.RenderingMode):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Interop.HwndTarget.get_CompositionTarget():System.Windows.Media.Composition.DUCE+ResourceHandle")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.MediaSystem.set_ForceRecord(System.Boolean):System.Void")]

//**************************************************************************************************************************
// Developer: bkaneva
// Reason: These methods are part of collection class
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.BitmapEffectPrimitiveCollection.Internal_GetItem(System.Int32):System.Windows.Media.BitmapEffectPrimitive")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.BitmapEffectPrimitiveCollection.get_Empty():System.Windows.Media.BitmapEffectPrimitiveCollection")]

//**************************************************************************************************************************
// Bug ID: 1187117, 1187109, 1187116
// Developer: wchao
// Reason: These methods are either marked FriendAccessAllowed or called by one. They are for Framework use on optimal paragraph. We decided not to expose that feature in Core for V1.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextFormatter.CreateFromContext(System.Windows.Media.TextFormatting.TextFormatterContext):System.Windows.Media.TextFormatting.TextFormatter")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.TextFormatting.UnsafeNativeMethods.LoCreateBreaks(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.TextFormatting.LsBreaks&):MS.Internal.TextFormatting.LsErr")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.TextFormatting.FullTextState.GetBreakpointInternalCp(System.Int32):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1143312
// Developer: dorinung
// Reason: These methods are used by Test
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilComposition_SyncFlush(System.IntPtr):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1162820
// Developer: cedricd
// Reason: FrugalList is a helper data structure that is compiled into PC, PF, and WB.  Developers
// use some methods in one assembly and some in others, so these violations are usually spurious.  
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.FrugalStructList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.FrugalObjectList`1")]

//**************************************************************************************************************************
// Bug ID: 1187120, 1187107, 1187105, 1187110, 1187115, 
// Developer: Wchao
// Reason: These methods are used by Core API test. We cant make this API public in V1 due to logistic reason. But our test team 
//         still needs to test them thru Reflection as the feature is exposed at Framework level.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope = "type", Target = "MS.Internal.TextFormatting.FullTextBreakpoint")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextParagraphCache.get_Ploparabreak():MS.Internal.SecurityCriticalDataForSet`1<System.IntPtr>")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextParagraphCache.get_FullText():MS.Internal.TextFormatting.FullTextState")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextParagraphCache.get_FiniteFormatWidth():System.Int32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextLineBreak.get_BreakRecord():MS.Internal.SecurityCriticalDataForSet`1<System.IntPtr>")]

//**************************************************************************************************************************
// Bug ID: 1340907, 1340908, 1380364, 1409928, 1409929
// Developer: samgeo
// Reason: We implement IDisposable and will not move to SafeHandle because our classes don't wrap simple unmanaged 
// resources but rather setup / tear down ResetEvents, or clean up communication with our COM server. 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Input.PenContext.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Input.StylusPlugIns.DynamicRendererThreadManager.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Input.PenThread.DisposeHelper():System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Input.PenThreadWorker.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Input.PenThreadWorker.ThreadProc():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1505715
// Developer: mleonov
// Reason: These calls are needed to preserve the lifetime of memory sections referred to by the font cache code.
// SafeHandle doesn’t help us here, as it prevents garbage collector to run during the interop calls to encapsulated objects,
// and makes sure critical finalization occurs. In our case we have all managed code dealing with unmanaged memory pointers,
// and we rely on managed object lifetime to guarantee that pointers are valid.
// Converting FontCache code from pointers to SafeHandle-like objects that keep references to the memory section
// would be very expensive performance and resource wise.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "System.Windows.Media.GlyphCache+Manager.AddRealization(MS.Internal.SecurityCriticalData`1<System.String>,System.Int32,System.UInt16,System.Double,System.Double,System.Double,System.Collections.Generic.IList`1<System.UInt16>):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1340914
// Developer: mleonov
// Reason: These calls are needed to preserve the lifetime of memory sections referred to by the font cache code.
// SafeHandle doesn’t help us here, as it prevents garbage collector to run during the interop calls to encapsulated objects,
// and makes sure critical finalization occurs. In our case we have all managed code dealing with unmanaged memory pointers,
// and we rely on managed object lifetime to guarantee that pointers are valid.
// Converting FontCache code from pointers to SafeHandle-like objects that keep references to the memory section
// would be very expensive performance and resource wise.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive",Scope="member",Target="System.Windows.Media.GlyphTypeface.#ComputeGlyphOutline(System.UInt16,System.Boolean,System.Double)")]

//**************************************************************************************************************************
// Bug ID: 1340921
// Developer: mleonov
// Reason: Under the covers, Managed C++ compiler expands ~LPCClientConnection destructor to implement IDisposable pattern.
// Unfortunately, it still keeps the original destructor method intact despite the fact it’s never called directly.
// This is an external issue I'm discussing with the C++/CLI team.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.FontCache.LPCClientConnection.~LPCClientConnection():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1340911, 1340912
// Developer: niklasb
// Reason: We need GC.KeepAlive here to prevent possible race condition between DisposeInternal and the finalizer
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Media.TextFormatting.TextLineBreak.DisposeInternal(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Media.TextFormatting.TextParagraphCache..ctor(MS.Internal.TextFormatting.FormatSettings,System.Int32,System.Int32)")]

//**************************************************************************************************************************
// Bug ID: 1340910
// Developer: niklasb
// Reason: We need GC.KeepAlive here to ensure the lifetime of contextInfo
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Media.TextFormatting.TextFormatterContext.Init():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1340918, 1340920, 1344600, 1344601, 1341048
// Developer: wchao
// Reason: These warnings concern several unmanaged objects which we keep as IntPtr inside the correspondent managed wrapper classes. 
//         These unmanaged objects are not exposed to untrusted code. They are disposed and become invalid once the managed object 
//         is disposed. So, they are not subject to handle-recylcing attack.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextParagraphCache.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.FullTextBreakpoint.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.FullTextBreakpoint.CreateMultiple(System.Windows.Media.TextFormatting.TextParagraphCache,System.Int32,System.Int32,System.Windows.Media.TextFormatting.TextLineBreak,System.IntPtr,System.Int32&):System.Collections.Generic.IList`1<System.Windows.Media.TextFormatting.TextBreakpoint>")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.TextMetrics+FullTextLine.DisposeInternal(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.TextMetrics+FullTextLine.DrawTextLine(System.Windows.Media.DrawingContext,System.Windows.Point,System.Windows.Media.MatrixTransform):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.TextMetrics+FullTextLine.FormatLine(MS.Internal.TextFormatting.FullTextState,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,MS.Internal.TextFormatting.LineFlags,MS.Internal.TextFormatting.FormattedTextSymbols):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1513778
// Developer: Wchao
// Reason: From local run
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.TextPenaltyModule.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope = "member", Target = "MS.Internal.TextFormatting.TextPenaltyModule.DangerousGetHandle():System.IntPtr")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 202861
// Developer: grzegorz
// Reason: False positive. Used internally by Managed C++ code (Cpp\PresentationCoreCpp.nativeproj).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TrueTypeRasterizer.#get_SourceUri()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.AbortableTask.#IncRunning()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.ClientManager.#IncIntPtr(System.IntPtr)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.ExplicitAccessList.#GetACLSize(System.Int32)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.ExplicitAccessList.#GetAceOffset(System.Int32)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.ExplicitAccessList.#GetAceSize()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.ExplicitAccessList.#GetMaxSidSize()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.LPCMessage.#InternalDispose(System.Boolean)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.FontCache.LPCServices.#AreWaitablePortsSupported()")]


//***************************************************************************************************************************
// False positives. These managed C++ methods are being called. 
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetStringLength(System.UInt32)", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetLocaleNameLength(System.UInt32)", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.ItemProps.#!ItemProps()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.FontList.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.FontFile.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#get_FontFamilyObject()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#CreateFontFace()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#CleanUp()", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#Initialize(MS.Internal.Text.TextInterface.FactoryType)", Justification="Tool is confused about managed C++ code.")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#!Factory()", Justification="Tool is confused about managed C++ code.")]


//***************************************************************************************************************************
// New Suppressions since 4.0 RTM
//***************************************************************************************************************************

//***************************************************************************************************************************
// This was Dev11 bug 132184 but got punted to Post-Dev11.  This is the last issue keeping us from being FxCop clean, so suppress.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Media.Composition.DUCE+Channel.#IsSecurityCriticalCommand(System.Byte*)")]
