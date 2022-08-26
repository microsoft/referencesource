//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************

	
//**************************************************************************************************************************
// Bug ID: 1340942
// Developer: Kiranku
// Reason: False positive.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Utility.TraceProvider.Finalize():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1413286
// Developer: dwaynen
// Reason: We have to pass a handle to an unmanaged component.  The method is marked SecurityCritical.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.Composition.DUCE+CompositionTarget.UpdateWindowSettings(System.Windows.Media.Composition.DUCE+ResourceHandle,System.Threading.AutoResetEvent,MS.Win32.NativeMethods+RECT,System.Windows.Media.Color,System.Single,System.Windows.Media.Composition.MILWindowLayerType,System.Windows.Media.Composition.MILTransparencyFlags,System.Boolean,System.Int32,System.Windows.Media.Composition.DUCE+Channel):System.Void", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug IDs: 199045, 199068
// Developer: brandf
// Reason: A pointer is needed from the SafeHandle's to put in a structure that's sent to unmanaged code.  reviewed by andren. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#OnCommittingBatch(System.Object,System.EventArgs)")]
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#UpdateBitmapSourceResource(System.Windows.Media.Composition.DUCE+Channel,System.Boolean)")]
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Interop.D3DImage.#SendPresent(System.Object,System.EventArgs)")]
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Interop.D3DImage.#UpdateResource(System.Windows.Media.Composition.DUCE+Channel,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 632951
// Developer: brenclar
// Reason: We have to pass a pointer to native code.  The method is marked SecurityCritical.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.#get_DUCECompatiblePtr()")]
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Media.Imaging.BitmapEncoder.#SaveFrame(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle,System.Windows.Media.Imaging.BitmapFrame)")]
[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle", Scope="member", Target="System.Windows.Media.ColorContext.#GetColorContextsHelper(System.Windows.Media.ColorContext+GetColorContextsDelegate)")]


//**************************************************************************************************************************
// Reason: Still using pointers therefore we need to RemoveCallsToGCKeepAlive.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#CreateFontFace(MS.Internal.Text.TextInterface.FontFaceType,MS.Internal.Text.TextInterface.FontFile[],System.UInt32,MS.Internal.Text.TextInterface.FontSimulations)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this).")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#CreateFontFile(System.Uri)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this).")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#CreateFontFile(System.Uri,System.Boolean)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#CreateTextAnalyzer()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this).")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#GetFontCollection(System.Uri)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#GetSystemFontCollection()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#GetSystemFontCollection(System.Boolean)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#AddFontFaceToCache()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#CreateFontFace()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#DisplayMetrics(System.Single,System.Single)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#DisplayMetrics(System.Single,System.Single,System.Boolean)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#GetInformationalStrings(MS.Internal.Text.TextInterface.InformationalStringID,MS.Internal.Text.TextInterface.LocalizedStrings&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#HasCharacter(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_FaceNames()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_Family()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_IsSymbolFont()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_Metrics()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_SimulationFlags()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_Stretch()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_Style()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Font.#get_Weight()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#FindFamilyName(System.String,System.UInt32&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#GetFontFromFontFace(MS.Internal.Text.TextInterface.FontFace)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#get_FamilyCount()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#get_Item(System.String)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontCollection.#get_Item(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#GetArrayOfGlyphIndices({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt32*,System.UInt32,System.UInt16*)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#GetDesignGlyphMetrics({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32,MS.Internal.Text.TextInterface.GlyphMetrics*)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#GetDisplayGlyphMetrics({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32,MS.Internal.Text.TextInterface.GlyphMetrics*,System.Single,System.Boolean,System.Boolean,System.Single)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#GetFileZero()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#ReadFontEmbeddingRights(System.UInt16&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#TryGetFontTable(MS.Internal.Text.TextInterface.OpenTypeTableTag,System.Byte[]&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_GlyphCount()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_Index()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_IsSymbolFont()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_Metrics()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_SimulationFlags()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFace.#get_Type()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#DisplayMetrics(System.Single,System.Single)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#GetFirstMatchingFont(MS.Internal.Text.TextInterface.FontWeight,MS.Internal.Text.TextInterface.FontStretch,MS.Internal.Text.TextInterface.FontStyle)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#GetMatchingFonts(MS.Internal.Text.TextInterface.FontWeight,MS.Internal.Text.TextInterface.FontStretch,MS.Internal.Text.TextInterface.FontStyle)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#get_FamilyNames()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#get_Metrics()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFamily.#get_OrdinalName()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFile.#Analyze(MS.Internal.Text.TextInterface.FontFileType&,MS.Internal.Text.TextInterface.FontFaceType&,System.UInt32&,{modopt(System.Runtime.CompilerServices.IsLong)}System.Int32{modopt(System.Runtime.CompilerServices.IsImplicitlyDereferenced)}*)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFile.#GetUriPath()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontList.#get_Count()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontList.#get_FontsCollection()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontList.#get_Item(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#FindLocaleName(System.String,System.UInt32&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetLocaleName(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetLocaleNameLength(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetString(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#GetStringLength(System.UInt32)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#get_Count()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#get_Keys()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#get_StringsCount()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.LocalizedStrings.#get_Values()", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#GetGlyphPlacements({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,{modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt16*,System.UInt32,{modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32*,System.UInt32,MS.Internal.Text.TextInterface.Font,System.Double,System.Double,System.Boolean,System.Boolean,System.Globalization.CultureInfo,MS.Internal.Text.TextInterface.DWriteFontFeature[][],System.UInt32[],System.Windows.Media.TextFormattingMode,MS.Internal.Text.TextInterface.ItemProps,System.Single,System.Int32*,MS.Internal.Text.TextInterface.GlyphOffset[]&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#GetGlyphs({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32,MS.Internal.Text.TextInterface.Font,System.UInt16,System.Boolean,System.Boolean,System.Globalization.CultureInfo,MS.Internal.Text.TextInterface.DWriteFontFeature[][],System.UInt32[],System.UInt32,System.Windows.Media.TextFormattingMode,MS.Internal.Text.TextInterface.ItemProps,System.UInt16*,System.UInt16*,System.UInt16*,System.UInt32*,System.Int32*,System.UInt32&)", Justification="Still using pointers and hence need to keep GC.KeepAlive(this)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#CreateFontFace(System.Uri,System.UInt32,MS.Internal.Text.TextInterface.FontSimulations)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.FontFile.#Analyze(MS.Internal.Text.TextInterface.Native.DWRITE_FONT_FILE_TYPE&,MS.Internal.Text.TextInterface.Native.DWRITE_FONT_FACE_TYPE&,System.UInt32&,{modopt(System.Runtime.CompilerServices.IsLong)}System.Int32{modopt(System.Runtime.CompilerServices.IsImplicitlyDereferenced)}*)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.ItemProps.#CanShapeTogether(MS.Internal.Text.TextInterface.ItemProps)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#GetBlankGlyphsForControlCharacters({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32,MS.Internal.Text.TextInterface.FontFace,System.UInt16,System.UInt32,System.UInt16*,System.UInt16*,System.Int32*,System.UInt32&)")]
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#GetGlyphPlacementsForControlCharacters({modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.UInt32,MS.Internal.Text.TextInterface.Font,System.Windows.Media.TextFormattingMode,System.Double,System.Double,System.Boolean,System.Single,System.UInt32,{modopt(System.Runtime.CompilerServices.IsConst)}System.UInt16*,System.Int32*,MS.Internal.Text.TextInterface.GlyphOffset[]&)")]


//**************************************************************************************************************************
// Do not care if this methods fails since there is no suitable action if that happens.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="<Module>.RegCloseKey(HKEY__*)", Scope="member", Target="MS.Internal.FontCache.Util2.#GetRegistryKeyLastWriteTimeUtc(System.String,System.Int64&)", Justification="There is no meaningful action if this method fails.")]

//new suppressions since v4 RTM:

//**************************************************************************************************************************
//Design of HwndTarget won't be changing.  Lifetime is properly handled by existing code.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Interop.HwndTarget+NotificationWindowHelper.#_hPowerNotify")]
[module: SuppressMessage("Microsoft.Usage","CA2216:DisposableTypesShouldDeclareFinalizer", Scope="member", Target="System.Windows.Interop.HwndTarget.#Dispose()")]
[module: SuppressMessage("Microsoft.Usage","CA2216:DisposableTypesShouldDeclareFinalizer", Scope="member", Target="System.Windows.Interop.HwndTarget+NotificationWindowHelper.#Dispose()")]
