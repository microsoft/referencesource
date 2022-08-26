//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

// bug 1396442
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Interop.HwndTarget.MilContent_DetachFromHwnd(System.IntPtr):System.Int32")]

// bug 1394338
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.get_WicSourceHandle():System.Windows.Media.Imaging.BitmapSourceSafeMILHandle")]

// bug 1394341
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateBitmapRenderTargetForBitmap(System.IntPtr,System.IntPtr,System.Windows.Media.SafeMILHandle&):System.Int32")]


// bugs 1337500;1337501;1337502;1337503
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilSyncCompositionDevice_Present(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilConnection_CreateChannel(System.IntPtr,System.Boolean,System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.SafeNativeMethods+SafeNativeMethodsPrivate.MilCompositionEngine_InitializePartitionManager(System.Int32,System.Windows.Media.Composition.MIL_SCHEDULE_TYPE):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.SafeNativeMethods+SafeNativeMethodsPrivate.MilTransport_InitializeTransportManager():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilTransport_IsTransportConnected(System.IntPtr,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.EnterMediaSystemLock():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilTransport_CreateDefaultTypeTransport(System.IntPtr,System.IntPtr&):System.Int32")]

// bugs 1341042, 1341045, 1341046
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.PROPVARIANT.pclsidVal")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.PROPVARIANT.pszVal")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.PROPVARIANT.pwszVal")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.PROPVARIANT.punkVal")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.profileStruct.pProfileData")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Imaging.PROPBAG2.pstrName")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.ColorContext+profileStruct.pProfileData")]

// Bug 1341041
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.FactoryMaker.s_pFactory")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.FactoryMaker.s_pImagingFactory")]

//bug 1341040
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="MS.Win32.Recognizer.PACKET_DESCRIPTION.pPacketProperties")]

// bugs 1341044, 1341043
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.Composition.DUCE+Channel._hChannel")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Media.MediaSystem.s_pConnection")]

// bug 1340967 - by design per e-mail from kcorby
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.SetValue(System.Runtime.InteropServices.SafeHandle,System.String,System.Object):System.Void")]

// bugs 1340956 - supression agreed by kcorby and akaza
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.Media.CompositionTarget.set_RootVisual(System.Windows.Media.Visual):System.Void")]

// bug 1344488
// The assert has been reviewed by the Partial Trust team.
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "MS.Internal.FontCache.FontSourceCollection.SetFontSources():System.Void")]

// bug 1344491
// The assert has been reviewed by the Partial Trust team.
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope = "member", Target = "MS.Internal.FontCache.FontSourceCollection.InitializeDirectoryProperties():System.Void")]


//**************************************************************************************************************************
// Bug ID: ?
// Developer: tmulcahy
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetPosition(System.Windows.Media.SafeMediaHandle,System.Int64&):System.Int32")]
//**************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 1248388
// Developer: waynezen
// Reason: This Dispose method asserts UIPermission in order to call BeginInvokeShutdown. The code has been reviewed by both the PartialTrust team and the Tablet Avalon team.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope = "member", Target = "System.Windows.Input.StylusPlugIns.DynamicRendererThreadManager.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1249923
// Developer: ericvan
// Reason: This can't be avoided because we need to marshal the PROPVARIANT structure to unmanaged code, and VT_UNKNOWN
//         expects to contain a raw IUnknown*.  We get this from the safe handle through DangerousGetHandle().  We do an
//         explicit AddRef to maintain the objects life time even if the SafeHandle is eventually released through garbage
//         collection.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Init(System.Object):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1249921, 1249922
// Developer: ericvan
// Reason: GetReaderByIndex, GetWriterByIndex, Next could not remove because they SafeHandles can't be marshalled during unmanaged
//         to managed code transitions, the CLR generatse a MarshalDirectiveException.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.GetWriterByIndex(System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.GetReaderByIndex(System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriterEnumerator.Next(System.UInt32,System.IntPtr&,System.UInt32&):System.Int32")]


//**************************************************************************************************************************
// Bug ID: 1077957, 1080746.
// Developer: mleonov
// Reason: These calls are considered safe. I double checked with MarkA and WChao.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.FontCache.BaseGlyphElement.CreateOtfRasterizer():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.FontFace.TrueTypeFontDriver.ReadCFFMetrics(MS.Internal.FontCache.FontFaceLayoutInfo,System.Boolean,System.UInt16,System.UInt16):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1106796.
// Developer: wchao
// Reason: Reviewed by avptt and considered safe.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_Height():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_DependentLength():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_MarkerBaseline():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_WidthIncludingTrailingWhitespace():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_Width():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_Start():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_Baseline():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_MarkerHeight():System.Double")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_Length():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2119:SealMethodsThatSatisfyPrivateInterfaces", Scope="member", Target="System.Windows.Media.TextFormatting.TextLine.get_NewlineLength():System.Int32")]


//**************************************************************************************************************************
// Bug ID: 1132085
// Developer: cedricd
// Reason: This code was designed to call GC.Collect.  It replaces our use of the CLR's implementation of GC.AddMemoryPressure
//         which internally also calls GC.Collect and had severe performance problems.
//         Without the collect we'd run the risk of severely bloating memory so this code is a win for both reliability and perf.
//         Reviewed by avptt.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.MemoryPressure.Collect():System.Void")]

//*******************************************************************************************************************
// Bug ID: No bug
// Developer: ahodsdon
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MediaTimelineConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MediaTimelineConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MediaTimelineConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MediaTimelineConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.MediaTimelineHelper.CreateMedia(System.Uri,System.IntPtr):System.Void")]

// [bkaneva]
// The calling methods (BlurBitmapEffect.UpdateUnmanagedPropertyState  and DropShadowEffect.UpdateUnmanagedPropertyState)
// have a DemandUIWindowPermission call

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.SetValue(System.Runtime.InteropServices.SafeHandle,System.String,System.Object):System.Void")]


//**************************************************************************************************************************
// Bug ID: Private run
// Developer: andren
// Reason: partial trust security team has reviewed all of these
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.MSInternal", "CA900:AptcaAssembliesShouldBeReviewed")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.CrossProcess.ReceiveSyncMessage(MS.Win32.SafePortHandle,MS.Win32.PortMessage*,System.Void*,System.IntPtr*):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.CrossProcess.SendAsyncMessage(MS.Win32.SafePortHandle,MS.Win32.PortMessage*,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.CrossProcess.SendReceiveSyncMessage(MS.Win32.SafePortHandle,NT._LARGE_INTEGER*,MS.Win32.PortMessage*):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Win32.HandleCollector+HandleType.Add(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.FontCache.FontSource.GetLastWriteTime():System.Int64")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.MediaDataHelper.CreateMedia(System.Uri,System.IO.Stream,System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.Mem_Alloc(System.UInt32):System.Void*")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.Mem_Free(System.Void*):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.Mem_ReAlloc(System.Void*,System.UInt32):System.Void*")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.CrossProcess.ConnectToPort(System.String,System.Int32,MS.Win32.SafePortHandle&,System.String&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.CrossProcess.SendAsyncMessage(MS.Win32.SafePortHandle,MS.Win32.PortMessage*,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.CrossProcess.SendReceiveSyncMessage(MS.Win32.SafePortHandle,NT._LARGE_INTEGER*,MS.Win32.PortMessage*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.CrossProcess.StartServiceWithTimeout(System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.FontSource.GetLastWriteTime():System.Int64")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Win32.HwndSubclass.UnhookWindowProc(System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.Int32,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.ContextLayoutManager.fireLayoutUpdateEvent():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.EnableOrDisableInputMethod(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.get_DefaultImc():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.KeyboardDevice.Focus(System.Windows.DependencyObject,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.MouseDevice.Capture(System.Windows.IInputElement,System.Windows.Input.CaptureMode):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.FactoryMaker.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Input.InputManager.ProcessInput(System.Windows.Input.InputEventArgs):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Input.ProcessInputEventArgs.PushInput(System.Windows.Input.InputEventArgs,System.Windows.Input.StagingAreaInputItem):System.Windows.Input.StagingAreaInputItem")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Input.InputManager.ProcessInput(System.Windows.Input.InputEventArgs):System.Boolean", MessageId="0#")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Input.ProcessInputEventArgs.PushInput(System.Windows.Input.InputEventArgs,System.Windows.Input.StagingAreaInputItem):System.Windows.Input.StagingAreaInputItem", MessageId="0#")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.NotifyAsyncContentLoaded(System.Windows.UIElement,System.Windows.Automation.AsyncContentLoadedState,System.Int64,System.Int64):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.NotifyEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement,System.Windows.Automation.AutomationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.NotifyPropertyChanged(System.Windows.UIElement,System.Windows.Automation.AutomationProperty,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.NotifyPropertyChanged(System.Windows.UIElement,System.Windows.DependencyProperty):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.NotifyStructureChangedWorker(System.Windows.UIElement,System.Windows.UIElement,System.Windows.Automation.StructureChangeType):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.ElementProxy.InContextHostRawElementProvider(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.ElementUtil.CheckConnected(System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.ElementUtil.CheckEnabled(System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.ElementUtil.Invoke(System.Windows.Media.Visual,System.Windows.Threading.DispatcherOperationCallback,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.FontCache.CacheManager.Lookup(MS.Internal.FontCache.IFontCacheElement):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.FontCache.GlyphBitmapElement.AddGlyph(System.UInt16):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.RenewableCacheManager.Lookup(MS.Internal.CacheReachable):MS.Internal.CacheReachable")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.AutomationElementFromUIElement(System.Windows.UIElement):System.Windows.Automation.AutomationElement")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.Int32,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement,System.Windows.Automation.AutomationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.ContextLayoutManager.UpdateLayout():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Media.Animation.TimeManager.Tick():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.UIElement.Measure(System.Windows.Size):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CreateFile(System.String,System.UInt32,System.UInt32,MS.Win32.NativeMethods+SECURITY_ATTRIBUTES,System.Int32,System.Int32,System.IntPtr):Microsoft.Win32.SafeHandles.SafeFileHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CreateFileMapping(Microsoft.Win32.SafeHandles.SafeFileHandle,MS.Win32.NativeMethods+SECURITY_ATTRIBUTES,System.Int32,System.UInt32,System.UInt32,System.String):MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.EnterCompositionEngineLock():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ExitCompositionEngineLock():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetDeviceCaps(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetFileSizeEx(Microsoft.Win32.SafeHandles.SafeFileHandle,MS.Win32.UnsafeNativeMethods+LARGE_INTEGER&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetKeyboardState(System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetMessageExtraInfo():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetModuleHandle(System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetProcAddress(System.Runtime.InteropServices.HandleRef,System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetSystemMetrics(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmAssociateContext(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntDestroyWindow(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetDC(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetModuleHandle(System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLongPtr(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntOleInitialize(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntPostMessage(System.Runtime.InteropServices.HandleRef,System.Int32,System.IntPtr,System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntReleaseDC(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MapViewOfFileEx(MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle,System.Int32,System.Int32,System.Int32,System.Int32,System.IntPtr):MS.Win32.UnsafeNativeMethods+SafeViewOfFileHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MILCreateStreamFromStreamDescriptor(System.Windows.Media.StreamDescriptor&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleFlushClipboard():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleSetClipboard(MS.Win32.UnsafeNativeMethods+IOleDataObject):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OpenFileMapping(System.Int32,System.Boolean,System.String):MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.VirtualAlloc(System.IntPtr,System.UIntPtr,System.Int32,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+EtwTrace.TraceEvent(System.UInt64,System.Char*):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILFactory2.CreateBitmapRenderTarget(System.IntPtr,System.UInt32,System.UInt32,MS.Internal.PixelFormatEnum,System.Single,System.Single,MS.Internal.MILRTInitializationFlags,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILFactory2.CreateFactory(System.IntPtr&,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILFactory2.CreateSWRenderTargetForBitmap(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILUnknown.AddRef(System.IntPtr):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILUnknown.QueryInterface(System.IntPtr,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilConnection_CommitChannel(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilConnection_CreateChannel(System.Boolean,System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilResource_CreateOrAddRefOnChannel(System.IntPtr,System.Windows.Media.Composition.DUCE+ResourceType,System.Windows.Media.Composition.DUCE+ResourceHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilResource_ReleaseOnChannel(System.IntPtr,System.Windows.Media.Composition.DUCE+ResourceHandle,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilResource_SendCommand(System.Byte*,System.UInt32,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilResource_SendCommandBitmapSource(System.Windows.Media.Composition.DUCE+ResourceHandle,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle,System.Boolean,System.Boolean,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.add_PostProcessInput(System.Windows.Input.ProcessInputEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.get_Current():System.Windows.Input.InputManager")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.ProcessInput(System.Windows.Input.InputEventArgs):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputProviderSite.ReportInput(System.Windows.Input.InputReport):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputReport.get_InputSource():System.Windows.PresentationSource")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.PresentationSource.AddSourceChangedHandler(System.Windows.IInputElement,System.Windows.SourceChangedEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.PresentationSource.OnAncestorChanged(System.Windows.ContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="System.Windows.Media.BitmapImage.CreateFromStream(System.IO.Stream,System.Windows.Media.BitmapCodecInfo,System.String,System.Boolean,System.Windows.Media.IntegerRect,System.Windows.Media.BitmapSizeOptions,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.Int32,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DataObject.PastingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DataObject.CopyingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DataObject.SettingDataEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.IsVisiblePropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewDragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.DragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewDropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewDragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewDragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.GiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.QueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.DragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.DragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewQueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.PreviewGiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.DragDrop.DropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.PreviewTextInputUpdateEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.TextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.PreviewTextInputStartEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.TextInputUpdateEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.PreviewTextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.TextCompositionManager.TextInputStartEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.GotStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.LostStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.InputManager.InputReportEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.InputManager.PreviewInputReportEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.AccessKeyManager.AccessKeyPressedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseUpOutsideCapturedElementEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.GotMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.LostMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.QueryCursorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseDownOutsideCapturedElementEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.PreviewMouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Mouse.MouseUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.PreviewGotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.PreviewKeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.KeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.PreviewLostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.GotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.PreviewKeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.LostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Keyboard.KeyUpEvent")]

// *** added 1/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PointUtil.ClientToScreen(System.Windows.Point,System.Windows.PresentationSource):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PointUtil.ScreenToClient(System.Windows.Point,System.Windows.PresentationSource):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.NotifyWinEvent(System.Int32,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ClientToScreen(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+POINT):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleGetClipboard(MS.Win32.UnsafeNativeMethods+IOleDataObject&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetAncestor(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]

// *** added 2/22/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PolygonHitTest(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Point*,System.Byte*,System.UInt32,System.UInt32,System.Double,System.Boolean,System.Windows.Point*,System.Boolean*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PathGeometryHitTest(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Windows.Point*,System.Boolean*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilChannel_BeginCommand(System.IntPtr,System.Byte*,System.UInt32,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilChannel_EndCommand(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilChannel_AppendCommandData(System.IntPtr,System.Byte*,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryCombine(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.GeometryCombineMode,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryFlatten(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryOutline(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryWiden(System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.GlyphTypeface..ctor(System.Uri,System.Windows.Media.StyleSimulations,System.Boolean)")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.GlyphTypeface..ctor(System.Uri,System.Int32,System.Windows.Media.StyleSimulations)")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.FontCache.FamilyCollection.GetFiles():System.String[]")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapDecoder.Initialize(System.Windows.Media.SafeMILHandle,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapDecoder.GetFrame(System.IntPtr,System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapDecoder.GetThumbnail(System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapDecoder.GetFrameCount(System.Windows.Media.SafeMILHandle,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICCodec.WICConvertBitmapSource(System.Guid&,System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICCodec.CreateImagingFactory(System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapSource.GetSize(System.Windows.Media.SafeMILHandle,System.UInt32&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapSource.GetResolution(System.Windows.Media.SafeMILHandle,System.Double&,System.Double&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICBitmapSource.GetPixelFormat(System.Windows.Media.SafeMILHandle,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICImagingFactory.CreateEncoder(System.IntPtr,System.Guid&,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromSource(System.IntPtr,System.IntPtr,MS.Internal.WICBitmapCreateCacheOptions,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICImagingFactory.CreateStream(System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICImagingFactory.CreateDecoder(System.IntPtr,System.Guid&,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICStream.InitializeFromMemory(System.IntPtr,System.IntPtr,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WICStream.InitializeFromIStream(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.FontSource+PinnedByteArrayStream..ctor(System.Byte[])")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.FileMapping.Create(System.String,System.Int64):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.FileMapping.OpenFile(System.String,System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.FileMapping.OpenSection(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputElement.TranslatePoint(System.Windows.Point,System.Windows.DependencyObject,System.Windows.DependencyObject):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetFocus():System.IntPtr")]

// *** added 2/23/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.TextServicesLoader.TIPsWantToRun():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WindowsCodecApi.CreateBitmapFromSection(System.UInt32,System.UInt32,System.Guid&,System.IntPtr,System.UInt32,System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateThreadMgr(MS.Win32.UnsafeNativeMethods+ITfThreadMgr&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.TextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewGotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewGiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewQueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.QueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewLostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.KeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewKeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.KeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewKeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewTextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.QueryCursorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.TextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.KeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewLostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewGiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewKeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewQueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.KeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewKeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.QueryCursorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewGotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewTextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.QueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmSetOpenStatus(System.Runtime.InteropServices.HandleRef,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetOpenStatus(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetConversionStatus(System.Runtime.InteropServices.HandleRef,System.Int32&,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetContext(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmReleaseContext(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateInputProcessorProfiles(MS.Win32.UnsafeNativeMethods+ITfInputProcessorProfiles&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IsWindow(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntCreateWindowEx(System.Int32,System.String,System.String,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Object):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetClassInfoEx(System.IntPtr,System.String,MS.Win32.NativeMethods+WNDCLASSEX_I):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.UnregisterClass(System.Int32,System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.RegisterClassEx(MS.Win32.NativeMethods+WNDCLASSEX_D):System.UInt16")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Interop.HwndTarget.VisualTarget_AttachToHwnd(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Interop.HwndTarget.VisualTarget_DetachFromHwnd(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.FontFace.CompositeFontParser.CreateXmlReader(System.String,System.Boolean):System.Xml.XmlReader")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.CommandBinding..ctor(System.Windows.Input.RoutedCommand,System.Windows.Input.ExecuteEventHandler,System.Windows.Input.QueryEnabledEventHandler)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.CommandBinding..ctor(System.Windows.Input.RoutedCommand)")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndKeyboardInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndAppCommandInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndStylusInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.set_ImeConversionMode(System.Windows.Input.ImeConversionMode):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.set_ImeState(System.Windows.Input.InputMethodState):System.Void")]

// *** added 2/28/05:

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.add_PreNotifyInput(System.Windows.Input.NotifyInputEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.add_PostNotifyInput(System.Windows.Input.NotifyInputEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputManager.add_PreProcessInput(System.Windows.Input.PreProcessInputEventHandler):System.Void")]

// *** added 3/2/05:

[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.set_ImeConversionMode(System.Windows.Input.ImeConversionModeValues):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.CrossProcess.SendReceiveSyncMessage(MS.Win32.SafePortHandle,System.Int32,MS.Win32.PortMessage*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryCombine(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.GeometryCombineMode,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryFlatten(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryOutline(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryWiden(System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Delegate,System.Windows.Media.FillRule&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapSource.GetSize(System.Windows.Media.SafeMILHandle,System.UInt32&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapSource.GetResolution(System.Windows.Media.SafeMILHandle,System.Double&,System.Double&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapSource.GetPixelFormat(System.Windows.Media.SafeMILHandle,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateEncoder(System.IntPtr,System.Guid&,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromSource(System.IntPtr,System.IntPtr,MS.Internal.WICBitmapCreateCacheOptions,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateStream(System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateDecoder(System.IntPtr,System.Guid&,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WindowsCodecApi.CreateBitmapFromSection(System.UInt32,System.UInt32,System.Guid&,System.IntPtr,System.UInt32,System.UInt32,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICStream.InitializeFromMemory(System.IntPtr,System.IntPtr,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICStream.InitializeFromIStream(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.Initialize(System.Windows.Media.SafeMILHandle,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetFrame(System.Windows.Media.SafeMILHandle,System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetThumbnail(System.IntPtr,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetFrameCount(System.Windows.Media.SafeMILHandle,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.WICConvertBitmapSource(System.Guid&,System.Windows.Media.SafeMILHandle,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.CreateImagingFactory(System.UInt32,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.QueryInterface(System.Windows.Media.SafeMILHandle,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.QueryInterface(System.IntPtr,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateBitmapRenderTarget(System.IntPtr,System.UInt32,System.UInt32,MS.Internal.PixelFormatEnum,System.Single,System.Single,MS.Internal.MILRTInitializationFlags,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateSWRenderTargetForBitmap(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateFactory(System.IntPtr&,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods.EnterCompositionEngineLock():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods.MILCreateStreamFromStreamDescriptor(System.Windows.Media.StreamDescriptor&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.CrossProcess.SendReceiveSyncMessage(MS.Win32.SafePortHandle,System.Int32,MS.Win32.PortMessage*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]

// *** added 3/7/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputProcessorProfiles.Uninitialize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputProcessorProfiles.UnadviseNotifySink():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PolygonHitTest(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Point*,System.Byte*,System.UInt32,System.UInt32,System.Double,System.Boolean,System.Windows.Point*,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PathGeometryHitTest(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Windows.Point*,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.InputScopeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.InputScopeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.InputScopeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.InputScopeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseActionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseActionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseActionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseActionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyGestureConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyGestureConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyGestureConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyGestureConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseGestureConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseGestureConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseGestureConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.MouseGestureConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CursorConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CursorConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CursorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CursorConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VideoDataConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VideoDataConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VideoDataConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VideoDataConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VectorCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VectorCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VectorCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.VectorCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.DoubleCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.DoubleCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.DoubleCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.DoubleCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.AudioDataConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.AudioDataConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.AudioDataConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.AudioDataConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathSegmentConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathSegmentConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PixelFormatConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PixelFormatConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PixelFormatConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PixelFormatConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ColorCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PointCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PointCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PointCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PointCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.GeometryConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.GeometryConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.GeometryConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.GeometryConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BitmapSourceConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BitmapSourceConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BitmapSourceConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BitmapSourceConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.TransformConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.TransformConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.TransformConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.TransformConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BrushConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BrushConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BrushConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.BrushConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.FontFamilyConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.FontFamilyConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.FontFamilyConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.FontFamilyConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.IntegerCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.IntegerCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.IntegerCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.IntegerCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDoubleTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDoubleTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDoubleTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDoubleTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt64TypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt64TypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt64TypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt64TypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt16TypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt16TypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt16TypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt16TypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVectorTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVectorTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVectorTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVectorTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.DurationConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.DurationConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.DurationConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.DurationConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableTimeSpanConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableTimeSpanConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableTimeSpanConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableTimeSpanConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRect3DTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRect3DTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRect3DTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRect3DTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDecimalTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDecimalTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDecimalTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableDecimalTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePoint3DTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePoint3DTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePoint3DTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePoint3DTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRectTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRectTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRectTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableRectTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVector3DTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVector3DTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVector3DTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableVector3DTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSize3DTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSize3DTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSize3DTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSize3DTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableColorTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableColorTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableColorTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableColorTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableByteTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableByteTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableByteTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableByteTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt32TypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt32TypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt32TypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableInt32TypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.RepeatBehaviorConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.RepeatBehaviorConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.RepeatBehaviorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.RepeatBehaviorConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSizeTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSizeTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSizeTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSizeTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSingleTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSingleTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSingleTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableSingleTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePointTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePointTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePointTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullablePointTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Ink.StrokeCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Ink.StrokeCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Ink.StrokeCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Ink.StrokeCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Ink.StrokeCollectionConverter.GetStandardValuesSupported(System.ComponentModel.ITypeDescriptorContext):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point4DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point4DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point4DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point4DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Rect3DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Rect3DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Rect3DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Rect3DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Matrix3DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Matrix3DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Matrix3DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Matrix3DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.QuaternionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.QuaternionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.QuaternionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.QuaternionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Size3DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Size3DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Size3DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Size3DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Vector3DConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Media3D.Point3DCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeyTimeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeyTimeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeyTimeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeyTimeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TextDecorationCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TextDecorationCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TextDecorationCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TextDecorationCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontWeightConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontWeightConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontWeightConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontWeightConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStyleConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStyleConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStyleConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStyleConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeySplineConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeySplineConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeySplineConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.KeySplineConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CultureInfoConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CultureInfoConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CultureInfoConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CultureInfoConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStretchConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStretchConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStretchConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontStretchConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]

// *** added 3/11/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.MediaSystem.ReadAnimationSmoothingSetting():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.FontFace.CompositeFontParser.CreateXmlReader(System.String):System.Xml.XmlReader")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ImageSourceConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ImageSourceConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ImageSourceConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.ImageSourceConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]

// *** added 3/21/05
// bug 1124183
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmap.SetPalette(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmap.SetResolution(System.Windows.Media.SafeMILHandle,System.Double,System.Double):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.DoesSupportAnimation(System.Windows.Media.SafeMILHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.DoesSupportChromakey(System.Windows.Media.SafeMILHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.DoesSupportLossless(System.Windows.Media.SafeMILHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.DoesSupportMultiframe(System.Windows.Media.SafeMILHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.GetContainerFormat(System.Windows.Media.SafeMILHandle,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.GetDeviceManufacturer(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.GetDeviceModels(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.GetMimeTypes(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.CopyPalette(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetColorContextByIndex(System.Windows.Media.SafeMILHandle,System.UInt32,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetColorContextCount(System.Windows.Media.SafeMILHandle,System.Windows.Media..IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetDecoderInfo(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetPreview(System.Windows.Media.SafeMILHandle,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapSource.CopyPalette(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapSource.CopyPixels(System.Windows.Media.SafeMILHandle,System.Windows.Int32Rect&,System.UInt32,System.UInt32,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.CreateIcmColorContext(System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.CreateIcmColorTransform(System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.WICSetEncoderFormat(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetAuthor(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetCLSID(System.Windows.Media.SafeMILHandle,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetDescription(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetFriendlyName(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetSpecVersion(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentInfo.GetVersion(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICFormatConverter.Initialize(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle,System.Guid&,MS.Internal.DitherType,System.Windows.Media.SafeMILHandle,System.Double,MS.Internal.WICPaletteType):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromHBITMAP(System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.WICBitmapAlphaChannelOption,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromHICON(System.IntPtr,System.IntPtr,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromMemory(System.IntPtr,System.UInt32,System.UInt32,System.Guid&,System.UInt32,System.UInt32,System.IntPtr,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateDecoderFromStream(System.IntPtr,System.IntPtr,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateFormatConverter(System.IntPtr,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreatePalette(System.IntPtr,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetThumbnail(System.Windows.Media.SafeMILHandle,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.Initialize(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateEncoder(System.IntPtr,System.Guid&,System.Guid&,System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateBitmapFromSource(System.IntPtr,System.Windows.Media.SafeMILHandle,MS.Internal.WICBitmapCreateCacheOptions,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle&):System.Int32")]

// *** added 3/25/05:
//akaza:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.SafeNativeMethods+SafeNativeMethodsPrivate.MilTransport_InitializeTransport(System.Int32,System.Windows.Media.Composition.MIL_SCHEDULE_TYPE):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndTarget..ctor(System.IntPtr)")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Automation.AutomationEventManager.UnsafeNotifyAutomationInteropProvider(System.Windows.UIElement,System.Windows.Automation.AutomationProperty,System.Object,System.Object):System.Void")]

// *** added 4/10/05:
//bug 1136392
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndSource.SetProcessDPIAware():System.Void")]

// *** added 4/25/05:
// bugs 1154717,19,20
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider.PossiblyDeactivate(System.IntPtr,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Windows.Interop.HwndSourceParameters)")]
// bugs 1155510-13
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.CriticalHandleWMGetobject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.CriticalHandleWMGetobject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PolygonBounds(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Point*,System.Byte*,System.UInt32,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Double,System.Boolean,System.Boolean,System.Windows.Rect*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryBounds(System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Double,System.Boolean,System.Boolean,System.Windows.Media.Composition.MIL_RECTD_RB*")]

// *** added 5/2/05:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.CompositionTarget.set_RootVisual(System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateMediaPlayer(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Guid,System.Windows.Media.SafeMediaHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryBounds(System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Double,System.Boolean,System.Boolean,System.Windows.Rect*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2113:SecureLateBindingMethods", Scope="member", Target="System.Windows.Ink.Stroke.Clone():System.Windows.Ink.Stroke")]
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.PresentationSource.set_RootVisual(System.Windows.Media.Visual):System.Void")]

// *** added 5/6/05:
// bug 1160476
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#DemandPathDiscovery(System.String)")]

// *** added 5/9/05:
// bugs 1162823, others
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.EnterCompositionEngineLock():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MILCreateStreamFromStreamDescriptor(System.Windows.Media.StreamDescriptor&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.PresentationSource.OnAncestorChanged(System.Windows.ContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.PresentationSource.AddSourceChangedHandler(System.Windows.IInputElement,System.Windows.SourceChangedEventHandler):System.Void")]

// *** added 5/16/05:
// bug 1169116
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#CreateUriDiscoveryPermission(System.Uri)")]
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#CreateUriReadPermission(System.Uri)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextCompositionManager.CompleteComposition(System.Windows.Input.TextComposition):System.Boolean")]
//bug 1170265
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.CreateWmpFactory(System.Windows.Media.SafeMILHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILFactory2.RegisterWmpFactory(System.Windows.Media.SafeMILHandle):System.Int32")]

// *** added 5/27/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.EventProxyWrapper.MILCreateEventProxy(System.Windows.Media.EventProxyDescriptor&,System.Windows.Media.SafeMILHandle&):System.Int32")]

// *** added 6/2/05:
// bugs 1184113,4,5
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.MediaPlayerState.OpenMedia(System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.MediaPlayerState.OpenMedia(System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationCore.BindUriHelper.GetAppBase():System.Uri")]
// bug 1182167
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider.ReportInput(System.IntPtr,System.Windows.Input.InputMode,System.Int32,System.Windows.Input.RawMouseActions,System.Int32,System.Int32,System.Int32):System.Boolean")]
// bug 1184116
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.HasVideo(System.Windows.Media.SafeMediaHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetMediaLength(System.Windows.Media.SafeMediaHandle,System.Int64&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.Open(System.Windows.Media.SafeMediaHandle,System.String):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.CanPause(System.Windows.Media.SafeMediaHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetDownloadProgress(System.Windows.Media.SafeMediaHandle,System.Double&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetNaturalHeight(System.Windows.Media.SafeMediaHandle,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetNaturalWidth(System.Windows.Media.SafeMediaHandle,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.IsBuffering(System.Windows.Media.SafeMediaHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.HasAudio(System.Windows.Media.SafeMediaHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.GetBufferingProgress(System.Windows.Media.SafeMediaHandle,System.Double&):System.Int32")]

// *** added 6/6/05:
//bug 1186040
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Compile.UnsafeNativeMethods.FindMimeFromData(System.Runtime.InteropServices.ComTypes.IBindCtx,System.String,System.IntPtr,System.Int32,System.String,System.Int32,System.String&,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.TextFormatting.UnsafeNativeMethods.LoDisposeBreakRecord(System.IntPtr,System.Boolean):MS.Internal.TextFormatting.LsErr")]

// *** added 6/15/05:

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice.Synchronize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice.UpdateCursorPrivate():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice.ChangeMouseCapture(System.Windows.IInputElement,System.Windows.Input.IMouseInputProvider,System.Windows.Input.CaptureMode,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextCompositionManager..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextComposition..ctor(System.Windows.Input.InputManager,System.Windows.IInputElement,System.String,System.Windows.Input.TextCompositionAutoComplete)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesCompartment.get_Value():System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesCompartment.set_Value(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesCompartment.UnadviseNotifySink():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesCompartment.AdviseNotifySink(MS.Win32.UnsafeNativeMethods+ITfCompartmentEventSink):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.AccessKeyManager..ctor()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.RoutedQueryEnabledEventArgs..ctor(System.Windows.Input.RoutedCommand)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesManager..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.Cursor.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.Cursor..ctor(System.Runtime.InteropServices.SafeHandle)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputMethod._ShowConfigureUI(System.Windows.UIElement,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputMethod._ShowRegisterWordUI(System.Windows.UIElement,System.Boolean,System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputMethod.GetCurrentKeybordTipProfile():MS.Win32.UnsafeNativeMethods+TF_LANGUAGEPROFILE")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusLogic..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.KeyboardDevice.TryChangeFocus(System.Windows.DependencyObject,System.Windows.Input.IKeyboardInputProvider,System.Boolean,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.KeyboardDevice..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.KeyboardDevice.ChangeFocus(System.Windows.DependencyObject,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesContext.StartTransitoryExtension(System.Windows.Input.DefaultTextStore):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesContext.RegisterTextStore(System.Windows.Input.DefaultTextStore):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.CommandDevice..ctor(System.Windows.Input.InputManager)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputProcessorProfiles.Uninitialize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.MediaPlayer.CreateMedia(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.MediaPlayerState.VerifyAPI():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.EventProxyWrapper.CreateEventProxyWrapper(System.Windows.Media.IInvokable):System.Windows.Media.SafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Geometry.GetBoundsHelper(System.Windows.Media.Pen,System.Windows.Media.Matrix*,System.Windows.Point*,System.Byte*,System.UInt32,System.UInt32,System.Windows.Media.Matrix*,System.Double,System.Windows.Media.ToleranceType,System.Boolean):System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.GetStandardColorSpaceProfile():System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Media.ColorContext.GetStreamFromProfileName(System.String):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Media.ColorContext.FromRawBytes(System.Byte[]):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Media.ColorContext.GetSystemColorProfileDirectory():System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.VisualTarget.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.PathGeometry.GetPathBounds(System.Windows.Media.Pen,System.Windows.Media.Matrix,System.Double,System.Windows.Media.ToleranceType,System.Boolean):System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Geometry.GetWidenedPathGeometry(System.Windows.Media.Pen,System.Double,System.Windows.Media.ToleranceType):System.Windows.Media.PathGeometry")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorTransform.InitializeMILIcmColorTransformInterface():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.StreamAsIStream.IStreamFrom(System.IO.Stream):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.SafeMILHandle..ctor(System.IntPtr,System.Int64)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.SafeMILHandle..ctor()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontFace.TrueTypeFontDriver.ComputeFontSubset(System.Collections.Generic.ICollection`1<System.UInt16>):System.Byte[]")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.TrueTypeSubsetter.ComputeSubset(System.Void*,System.Int32,System.Uri,System.Int32,System.UInt16[]):System.Byte[]")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.HRESULT.Check(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.CrossProcess.SendAsyncMessage(MS.Win32.SafePortHandle,MS.Win32.PortMessage*,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.CrossProcess.SendReceiveSyncMessage(MS.Win32.SafePortHandle,System.Int32,MS.Win32.PortMessage*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Composition.DUCE+Channel.SendCommandBitmapSource(System.Windows.Media.Composition.DUCE+ResourceHandle,System.Windows.Media.Imaging.BitmapSourceSafeMILHandle,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap..ctor(System.IntPtr,System.Int32,System.Int32,System.Windows.Media.PixelFormat,System.Int32,System.Int32)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap..ctor(System.IntPtr,System.IntPtr,System.Windows.Media.IntegerRect,System.Windows.Media.Imaging.BitmapSizeOptions)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap..ctor(System.IntPtr,System.Windows.Media.IntegerRect,System.Windows.Media.Imaging.BitmapSizeOptions)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapSourceSafeMILHandle..ctor(System.IntPtr)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.get_DUCECompatibleMILPtr():System.Windows.Media.Imaging.BitmapSourceSafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapPalette.CreateFromBitmapSource(System.Windows.Media.Imaging.BitmapSource):System.Windows.Media.Imaging.BitmapPalette")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapPalette.CreateInternalPalette():System.Windows.Media.SafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.ElementCacher..ctor(MS.Internal.FontCache.FileMapping,System.Boolean)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.FontSource+PinnedByteArrayStream..ctor(System.Byte[])")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.CheckedPointer..ctor(System.IO.UnmanagedMemoryStream)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.FileMapping.Create(System.String,System.Int64):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.FileMapping.OpenFile(System.String,System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.FontCache.FileMapping.OpenSection(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafePortHandle..ctor(System.Void*,System.Boolean)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndKeyboardInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndKeyboardInputProvider.ReportInput(System.IntPtr,System.Windows.Input.InputMode,System.Int32,System.Windows.Input.RawKeyboardActions,System.Int32,System.Boolean,System.Boolean,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider.ReportInput(System.IntPtr,System.Windows.Input.InputMode,System.Int32,System.Windows.Input.RawMouseActions,System.Int32,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.SetProcessDPIAware():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.set_RootVisualInternal(System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndStylusInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource+WeakEventPreprocessMessage..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndAppCommandInputProvider..ctor(System.Windows.Interop.HwndSource)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndTarget.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.TextFormatting.UnsafeNativeMethods.LoCloneBreakRecord(System.IntPtr,System.IntPtr&):MS.Internal.TextFormatting.LsErr")]

// *** added 6/20/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.FocusManager.LostFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.FocusManager.GotFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseLeftButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseLeftButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseLeftButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseLeftButtonUpEvent")]

// *** added 6/22/05:
//bugs 1199506, 1200450
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.PathFigureCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.StrokeCollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.StrokeCollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.StrokeCollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.StrokeCollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.StrokeCollectionConverter.GetStandardValuesSupported(System.ComponentModel.ITypeDescriptorContext):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TipButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Height")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.YTiltOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Width")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.PitchRotation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.NormalPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.BarrelButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.X")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Y")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Z")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.RollRotation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TimerTick")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.AzimuthOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TangentPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.SecondaryTipButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.PacketStatus")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.XTiltOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.AltitudeOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TwistOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.ButtonPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.SerialNumber")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.YawRotation")]

// *** added 6/27/05:
//bug 1206768
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.FontCache.ServiceOps.StartServiceWithTimeout(System.String):System.Boolean")]
//bug 1206765
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationCore.SafeSecurityHelper.IsFeatureDisabled(MS.Internal.PresentationCore.SafeSecurityHelper+KeyToRead):System.Boolean")]
//bug 1206766
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.SetVolume(System.Windows.Media.SafeMediaHandle,System.Double):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.SetBalance(System.Windows.Media.SafeMediaHandle,System.Double):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilChannel_SetNotificationWindow(System.IntPtr,System.IntPtr,System.Int32):System.Int32")]
//bug 1206763
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.FontCache.LPCServices.TryWaitForClient(MS.Win32.SafePortHandle,System.ValueType,System.Int32&):MS.Internal.FontCache.LPCMessage")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.FontCache.LPCServices.TrySendRequest(MS.Internal.FontCache.LPCClientConnection,MS.Internal.FontCache.LPCMessage,System.Int32&):MS.Internal.FontCache.LPCMessage")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.FontCache.LPCServices.TrySendReply(MS.Win32.SafePortHandle,MS.Internal.FontCache.LPCMessage,System.Int32&):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.FontCache.LPCServices.TrySendDatagram(MS.Internal.FontCache.LPCClientConnection,MS.Internal.FontCache.LPCMessage,System.Int32&):System.Void")]

// *** added 7/7/05:
//bug 1212741
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.TextFormatting.UnsafeNativeMethods.LoSetDoc(System.IntPtr,System.Int32,System.Int32,MS.Internal.TextFormatting.LsDevRes&):MS.Internal.TextFormatting.LsErr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.TextFormatting.UnsafeNativeMethods.LoSetBreaking(System.IntPtr,System.Int32):MS.Internal.TextFormatting.LsErr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.TextFormatting.UnsafeNativeMethods.LoCreateContext(MS.Internal.TextFormatting.LsContextInfo&,MS.Internal.TextFormatting.LscbkRedefined&,System.IntPtr&,System.IntPtr&):MS.Internal.TextFormatting.LsErr")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CursorConverter.GetStandardValues(System.ComponentModel.ITypeDescriptorContext):System.ComponentModel.TypeConverter+StandardValuesCollection")]

// *** added 7/11/05:
//bug 1218586
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.InputMethod.HwndFromInputElement(System.Windows.IInputElement):System.IntPtr")]
//bug 1218581
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Int32CollectionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Int32CollectionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Int32CollectionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Int32CollectionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]

// *** added 7/13/05:
//bug 1219913
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.Stop(System.Windows.Media.SafeMediaHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.SetPosition(System.Windows.Media.SafeMediaHandle,System.Int64):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.Start(System.Windows.Media.SafeMediaHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.Resume(System.Windows.Media.SafeMediaHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.SetRate(System.Windows.Media.SafeMediaHandle,System.Double):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.MILMedia.Pause(System.Windows.Media.SafeMediaHandle):System.Int32")]
//bug 1219914
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilComposition_PeekNextMessage(System.IntPtr,System.Windows.Media.Composition.DUCE+MilMessage+Message&,System.IntPtr,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.SafeNativeMethods+SafeNativeMethodsPrivate.MilTransport_InitializeTransport(System.Int32,System.Windows.Media.Composition.MIL_SCHEDULE_TYPE,System.IntPtr):System.Int32")]

// *** added 7/15/05:
//bug 1224356
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilComposition_SyncFlush(System.IntPtr):System.Int32")]

// *** added 8/1/05:
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.PerformSchemeCheckToDisallowCrossSchemes(System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.DemandMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessUncFiles(System.Uri):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessUncFiles(System.Uri):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessHttpFiles(System.Uri):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessHttpFiles(System.Uri):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.Imaging.BitmapDownload.BeginDownload(System.Windows.Media.Imaging.BitmapDecoder,System.Uri,System.Net.Cache.RequestCachePolicy,System.IO.Stream):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.Imaging.BitmapDownload.BeginDownload(System.Windows.Media.Imaging.BitmapDecoder,System.Uri,System.Net.Cache.RequestCachePolicy,System.IO.Stream):System.Void")]

// *** added 8/4/05:
// bugs 1246826,7,8,9
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.StylusLogic.ReadSystemConfig():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.CreateRecognizer(System.Guid&,MS.Win32.Recognizer.RecognizerSafeHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.AddStroke(MS.Win32.Recognizer.ContextSafeHandle,MS.Win32.Recognizer.PACKET_DESCRIPTION&,System.UInt32,System.IntPtr,MS.Win32.NativeMethods+XFORM):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.EndInkInput(MS.Win32.Recognizer.ContextSafeHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.GetLatticePtr(MS.Win32.Recognizer.ContextSafeHandle,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.CreateContext(MS.Win32.Recognizer.RecognizerSafeHandle,MS.Win32.Recognizer.ContextSafeHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.DestroyAlternate(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.GetAlternateList(MS.Win32.Recognizer.ContextSafeHandle,MS.Win32.Recognizer.RECO_RANGE&,System.UInt32&,System.IntPtr[],MS.Win32.Recognizer.ALT_BREAKS):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.ResetContext(MS.Win32.Recognizer.ContextSafeHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.GetConfidenceLevel(System.IntPtr,MS.Win32.Recognizer.RECO_RANGE&,System.Windows.Ink.RecognitionConfidence&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.SetEnabledUnicodeRanges(MS.Win32.Recognizer.ContextSafeHandle,System.UInt32,MS.Win32.Recognizer.CHARACTER_RANGE[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.Process(MS.Win32.Recognizer.ContextSafeHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Recognizer.UnsafeNativeMethods.GetString(System.IntPtr,MS.Win32.Recognizer.RECO_RANGE&,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.StylusPlugIns.DynamicRendererThreadManager.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.CreateResetEvent(System.IntPtr&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.RaiseResetEvent(System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.IsfCompressPacketData(MS.Win32.Penimc.CompressorSafeHandle,System.Int32[],System.UInt32,System.Byte&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.IsfDecompressPropertyData(System.Byte[],System.UInt32,System.UInt32&,System.Byte[],System.Byte&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.IsfDecompressPacketData(MS.Win32.Penimc.CompressorSafeHandle,System.Byte[],System.UInt32&,System.UInt32,System.Int32[],System.Byte&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.IsfCompressPropertyData(System.Byte[],System.UInt32,System.Byte&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Penimc.UnsafeNativeMethods.IsfLoadCompressor(System.Byte[],System.UInt32&):MS.Win32.Penimc.CompressorSafeHandle")]

// *** added 8/8/05:
//bug 1249929
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.WICMapShortNameToGuid(System.String,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.WICMapGuidToShortName(System.Guid&,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
//bug 1249931
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetMetadataQueryReader(System.Windows.Media.SafeMILHandle,System.IntPtr&):System.Int32")]
//bug 1249933
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryWriter.SetMetadataByName(System.Windows.Media.SafeMILHandle,System.String,System.Windows.Media.Imaging.PROPVARIANT&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryWriter.RemoveMetadataByName(System.Windows.Media.SafeMILHandle,System.String):System.Int32")]
//bug 1249935
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICFastMetadataEncoder.Commit(System.Windows.Media.SafeMILHandle):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICFastMetadataEncoder.GetMetadataQueryWriter(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
//bug 1249937
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle):System.UInt32")]
//bug 1249939
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentFactory.CreateMetadataWriterFromReader(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICComponentFactory.CreateQueryWriterFromBlockWriter(System.IntPtr,System.IntPtr,System.IntPtr&):System.Int32")]
//bug 1249941
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryReader.GetLocation(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryReader.GetMetadataByName(System.Windows.Media.SafeMILHandle,System.String,System.Windows.Media.Imaging.PROPVARIANT&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryReader.GetContainerFormat(System.Windows.Media.SafeMILHandle,System.Guid&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryReader.ContainsMetadataByName(System.Windows.Media.SafeMILHandle,System.String,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICMetadataQueryReader.GetEnumerator(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
//bug 1249942
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateQueryWriter(System.IntPtr,System.Guid&,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateQueryWriterFromReader(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateFastMetadataEncoderFromDecoder(System.IntPtr,System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
//bug 1250429
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilSyncCompositionDevice_Present():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_PathGeometryHitTestPathGeometry(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Boolean,System.Windows.Media.IntersectionDetail*):System.Int32")]

// *** added 8/18/05:
//bug 1261986
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.MS.Internal.TtfDelta.Mem_ReAlloc(System.Void*,System.UInt32):System.Void*")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.MS.Internal.TtfDelta.Mem_Alloc(System.UInt32):System.Void*")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="<Module>.MS.Internal.TtfDelta.Mem_Free(System.Void*):System.Void")]
//bug 1264106
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.MilCoreApi.MilUtility_GetPointAtLengthFraction(System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Double,System.Windows.Point&,System.Windows.Point&):System.Int32")]
//bug 1261864
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapEncoder.GetMetadataQueryWriter(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeMILHandle&):System.Int32")]
//bug 1269152
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.DemandMediaAccessPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.CreateMediaAccessPermission(System.String):System.Security.CodeAccessPermission")]

// *** added 8/21/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseUpEvent")]

// *** added 8/29/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.Composition.DUCE+UnsafeNativeMethods.MilConnection_DestroyChannel(System.IntPtr):System.Int32")]

// *** added 9/09/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.ColorContext_InitializeFromMemory(System.Windows.Media.SafeMILHandle,System.IntPtr,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+Wininet.GetUrlCacheConfigInfo(MS.Win32.NativeMethods+InternetCacheConfigInfo&,System.UInt32&,System.UInt32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICPixelFormatInfo.GetBitsPerPixel(System.IntPtr,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICImagingFactory.CreateComponentInfo(System.IntPtr,System.Guid&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.PixelFormat.get_BitsPerPixel():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICCodec.CreateColorContext(System.IntPtr,System.Windows.Media.SafeMILHandle&):System.Int32")]

// *** added 9/19/05:
//bugs 1295560,1
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorContext.GetColorProfileFromHandle(System.Windows.Media.SafeMILHandle,System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorContext.OpenColorProfileW(System.Windows.Media.SafeMILHandle,System.IntPtr,System.Windows.Media.SafeProfileHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorContext.GetStandardColorSpaceProfileW(System.Windows.Media.SafeMILHandle,System.IntPtr,System.UInt32,System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorContext.GetColorProfileHeader(System.Windows.Media.SafeMILHandle,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorTransform.TranslateColors(System.Windows.Media.SafeMILHandle,System.IntPtr,System.UInt32,System.UInt32,System.IntPtr,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILIcmColorTransform.CreateVectorTransform(System.Windows.Media.SafeMILHandle,System.Windows.Media.SafeProfileHandle,System.Windows.Media.SafeProfileHandle):System.Int32")]
//bugs 1302468,9
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessHttpFiles(System.Uri,System.IO.Stream):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessHttpFiles(System.Uri,System.IO.Stream):System.IO.Stream")]

// *** added 9/22/05:
//bug 1309469
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapEncoder.Save(System.IO.Stream):System.Void")]
//bug 1309470
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DurationConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DurationConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DurationConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DurationConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
//bug 1309482
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICPalette.GetColors(System.Windows.Media.SafeMILHandle,System.Int32,System.IntPtr,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICPalette.GetType(System.Windows.Media.SafeMILHandle,MS.Internal.WICPaletteType&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICPalette.GetColorCount(System.Windows.Media.SafeMILHandle,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICPalette.HasAlpha(System.Windows.Media.SafeMILHandle,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.ContentPosition.Missing")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.DocumentPage.Missing")]

// ** added 9/26/05:
//bug 1319673
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Clipboard.IsDataObjectFromLessPriviligedApplicationDomain(System.Windows.IDataObject):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.ExtractAppDomainPermissionSet():System.Security.PermissionSet")]
// bug 1332209
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapDecoder.GetColorContexts(System.Windows.Media.SafeMILHandle,System.UInt32,System.IntPtr,System.UInt32&):System.Int32")]

// *** added 10/10/05:
//bug 1337368
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#DemandMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage)")]
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#CallerHasMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage)")]

// *** added 10/12/05:
//several bugs
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusLogic.UpdateRectangles():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusLogic.InputManagerProcessInput(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusLogic.InputManagerProcessInputEventArgs(System.Windows.Input.InputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.GestureRecognition.NativeRecognizer.GetPacketData(System.Windows.Ink.Stroke,MS.Win32.Recognizer.PACKET_DESCRIPTION&,System.Int32&,System.IntPtr&,MS.Win32.NativeMethods+XFORM&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.GestureRecognition.NativeRecognizer.AddStrokes(MS.Win32.Recognizer.ContextSafeHandle,System.Windows.Ink.StrokeCollection):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.GestureRecognition.NativeRecognizer.SetEnabledGestures(MS.Win32.Recognizer.ContextSafeHandle,System.Windows.Ink.ApplicationGesture[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.GestureRecognition.NativeRecognizer.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.GestureRecognition.NativeRecognizer.ReleaseResourcesinPacketDescription(MS.Win32.Recognizer.PACKET_DESCRIPTION,System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.Compressor..ctor(System.Byte[],System.UInt32&)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.Compressor.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.MediaPlayerState.CreateMedia(System.Windows.Media.MediaPlayer):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.get_ColorSpaceFamily():System.Windows.Media.ColorContext+StandardColorSpace")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.GetColorContextHandlerFromProfileBytes(System.Byte[]):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.OpenProfileStream():System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.get_NumChannels():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContext.InitializeMILIcmColorContextInterface():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorTransform..ctor(System.Windows.Media.ColorContext,System.Windows.Media.ColorContext)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorTransform.Translate(System.Single[],System.Single[]):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusPlugIns.StylusPlugInCollection.EnsureEventsHooked():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.InkOutlinesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.DryBrushBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.SmudgeStickBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.RippleBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.FilmGrainBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.ColoredPencilBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PlasterBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PaintDaubsBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.WatercolorBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.HalftoneScreenBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.DropShadowBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.ReticulationBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.SpatterBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.StainedGlassBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.SumieBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.DarkStrokesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PatchworkBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.BlurBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.OuterGlowBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PosterEdgesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.CrosshatchBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.SprayedStrokesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.DiffuseGlowBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.TornEdgesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.CraquelureBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.EmbossBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.AccentedEdgesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.FrescoBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.CutoutBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.CharcoalBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.GlowingEdgesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.SpongeBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PlasticWrapBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.AngledStrokesBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PhotocopyBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.ChalkAndCharcoalBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.NotepaperBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.GrainBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.StampBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.GraphicPenBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.PaletteKnifeBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.NeonGlowBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.BasReliefBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.MosaicBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.ChromeBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.WaterPaperBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.Penimc.CompressorSafeHandle..ctor(System.Boolean)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap..ctor(System.IntPtr,System.IntPtr,System.Windows.Int32Rect,System.Windows.Media.Imaging.BitmapSizeOptions)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap..ctor(System.IntPtr,System.Windows.Int32Rect,System.Windows.Media.Imaging.BitmapSizeOptions)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadataEnumerator..ctor(System.Windows.Media.SafeMILHandle)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata.InitializeFromBlockWriter(System.Guid,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata.InitializeFromBlockWriter(System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.FormatConvertedBitmap.EndInit():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.get_IWICBitmapSourceHandle():System.Windows.Media.Imaging.BitmapSourceSafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapPalette.UpdateManaged():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.ToObject(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Clear():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Init(System.String[],System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Init(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Init(System.Array,System.Type,System.Runtime.InteropServices.VarEnum):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapDownload.BeginDownload(System.Windows.Media.Imaging.BitmapDecoder,System.Uri,System.Net.Cache.RequestCachePolicy,System.IO.Stream):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Clipboard.ThrowIfFailed(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalAlloc(System.Int32,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalUnlock(System.Runtime.InteropServices.HandleRef):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.System.Runtime.InteropServices.ComTypes.IDataObject.GetDataHere(System.Runtime.InteropServices.ComTypes.FORMATETC&,System.Runtime.InteropServices.ComTypes.STGMEDIUM&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.System.Runtime.InteropServices.ComTypes.IDataObject.SetData(System.Runtime.InteropServices.ComTypes.FORMATETC&,System.Runtime.InteropServices.ComTypes.STGMEDIUM&,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.System.Runtime.InteropServices.ComTypes.IDataObject.GetData(System.Runtime.InteropServices.ComTypes.FORMATETC&,System.Runtime.InteropServices.ComTypes.STGMEDIUM&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveFileListToHandle(System.IntPtr,System.String[],System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveStreamToHandle(System.IntPtr,System.IO.Stream,System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalLock(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveStringToHandle(System.IntPtr,System.String,System.Boolean,System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalSize(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32WideCharToMultiByte(System.String,System.Int32,System.Byte[],System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalFree(System.Runtime.InteropServices.HandleRef):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.Win32GlobalReAlloc(System.Runtime.InteropServices.HandleRef,System.IntPtr,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveSystemBitmapSourceToHandle(System.IntPtr,System.Object,System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveStringToHandleAsUtf8(System.IntPtr,System.String,System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.SaveSystemDrawingBitmapToHandle(System.IntPtr,System.Object,System.Boolean):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.System.Runtime.InteropServices.ComTypes.IDataObject.DUnadvise(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.GetCompatibleBitmap(System.Object):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.TextFormatting.TextFormatterContext.Init():System.Void")]

// *** added 10/17/05:
//bug 1344579
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.WinInet.get_InternetCacheFolder():System.Uri")]
//bug 1344490
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MilCoreApi.MilUtility_PathGeometryBounds(System.Windows.Media.Composition.MIL_PEN_DATA*,System.Double*,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Windows.Media.FillRule,System.Byte*,System.UInt32,System.Windows.Media.Composition.MIL_MATRIX3X2D*,System.Double,System.Boolean,System.Boolean,System.Windows.Media.Composition.MIL_RECTD_RB*):System.Int32")]

// *** added 10/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.StylusPlugIns.RawStylusInput.GetStylusPoints(System.Windows.Media.GeneralTransform):System.Windows.Input.StylusPointCollection")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.Cursor.LoadFromFile(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.Cursor.LoadFromStream(System.IO.Stream):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusPlugIns.RawStylusInput.GetStylusPoints(System.Windows.Media.GeneralTransform):System.Windows.Input.StylusPointCollection")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.RenderTargetBitmap.RenderTargetContentsChanged():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapPalette.get_InternalPalette():System.Windows.Media.SafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.DataObject.GetEnhancedMetafileHandle(System.String,System.Object):System.IntPtr")]

// *** added 11/10/05:
//bug 1380380
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Input.MouseDevice.GlobalHitTest(System.Windows.Point,System.Windows.PresentationSource,System.Windows.IInputElement&,System.Windows.IInputElement&):System.Void")]

// *** added 11/28/05
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies")]

// *** added 12/13/05
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TabletDeviceCollection.HasTabletDevices():System.Boolean")]

// *** added 2/16/06:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.SafeHandleIsInvalid(System.Runtime.InteropServices.SafeHandle):System.Boolean")]

//*** added 3/14/06
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.SetupDecoderFromUriOrStream(System.Uri,System.IO.Stream,System.Windows.Media.Imaging.BitmapCacheOption,System.Guid&,System.Boolean&,System.IO.Stream&,System.IO.UnmanagedMemoryStream&,Microsoft.Win32.SafeHandles.SafeFileHandle&):System.Windows.Media.SafeMILHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.ProcessHttpsFiles(System.Uri,System.IO.Stream):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.GetIStreamFromStream(System.IO.Stream&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1587786
// Fixer: arielki
// Reason: ok to suppress - Uri.LocalPath values are not leaked
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#BlockCrossDomainForHttpsApps(System.Uri)")]
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.#EnforceUncContentAccessRules(System.Uri)")]

//**************************************************************************************************************************
// Bug ID: 1577605
// Alias: ArielKi
// Reason: Methods are SecurityCritical, LinkDemand method exposure got reviewed and appropriately constrained.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.InputElement.TranslatePoint(System.Windows.Point,System.Windows.DependencyObject,System.Windows.DependencyObject,System.Boolean&):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice.GlobalHitTest(System.Windows.Point,System.Windows.PresentationSource,System.Windows.IInputElement&,System.Windows.IInputElement&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusLogic.RegisterHwndForInput(System.Windows.Input.InputManager,System.Windows.PresentationSource):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.StylusDevice.Synchronize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PointUtil.RootToClient(System.Windows.Rect,System.Windows.PresentationSource):System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PointUtil.RootToClient(System.Windows.Point,System.Windows.PresentationSource):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PointUtil.TryClientToRoot(System.Windows.Point,System.Windows.PresentationSource,System.Boolean,System.Boolean&):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.GetCompositionTargetCore():System.Windows.Media.CompositionTarget")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.get_UsesPerPixelOpacity():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1577605
// Alias: ArielKi
// Reason: This is showing up because of a FXCop 1.32 bug. PresentationSource.get_CompositionTarget() is neither abstract nor
// virtual, so doesn't require an InheritanceDemand.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2114:MethodSecurityShouldBeASupersetOfType", Scope="member", Target="System.Windows.PresentationSource.get_CompositionTarget():System.Windows.Media.CompositionTarget")]

//**************************************************************************************************************************
// Bug ID: 1594384
// Fixer: akaza
// Reason: ok to suppress - internal static PermissionSet ExtractAppDomainPermissionSet() is critical
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.PresentationCore.SecurityHelper.ExtractAppDomainPermissionSet:PermissionSet")]

//**************************************************************************************************************************
// Bug ID: 1607363
// Alias: ArielKi
// Reason: LinkDemand method exposure got reviewed and appropriately constrained.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PresentationCore.SecurityHelper.ExtractAppDomainPermissionSet():System.Security.PermissionSet")]

//**************************************************************************************************************************
// Bug ID: 1742261
// Alias: bedej
// Reason: s_pLocalTransport is a transport connection object, the same as s_pConnection for which there is already an
// exception
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.MediaSystem.s_pLocalTransport")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.InteropBitmap.InitializeFromHBitmap(System.IntPtr,System.IntPtr,System.Windows.Int32Rect,System.Windows.Media.Imaging.BitmapSizeOptions,MS.Internal.WICBitmapAlphaChannelOption):System.Void")]
#endif

// 1420523
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+WICBitmapCodecInfo.GetFileExtensions(System.Windows.Media.SafeMILHandle,System.UInt32,System.Text.StringBuilder,System.UInt32&):System.Int32")]

// bug 1396542
// This is the same issue as the approved suppressions in CL: 119787
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Effects.BevelBitmapEffect.UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]

// bug 1381899
// Developer: huwang
// Reason: This function is already marked as SecurityCritical TreatAsSafe. AKaza agrees this should be excluded.
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.AppModel.SiteOfOriginContainer.GetDeploymentUri():System.Uri")]


// Nonreported violation or previously suppressed violation.
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.CriticalCopyPixels(System.Windows.Int32Rect,System.IntPtr,System.Int32,System.Int32):System.Void")]

// bug 1413285
// Developer: dwaynen
// Reason: This function is marked as SecurityCritical.
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Composition.DUCE+CompositionTarget.UpdateWindowSettings(System.Windows.Media.Composition.DUCE+ResourceHandle,System.Threading.AutoResetEvent,MS.Win32.NativeMethods+RECT,System.Windows.Media.Color,System.Single,System.Windows.Media.Composition.MILWindowLayerType,System.Windows.Media.Composition.MILTransparencyFlags,System.Boolean,System.Int32,System.Windows.Media.Composition.DUCE+Channel):System.Void")]

// 1428994, 1474342
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ByteStreamGeometryContext.DisposeCore():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ByteStreamGeometryContext.FinishFigure():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ByteStreamGeometryContext.WriteData(System.Byte*,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ByteStreamGeometryContext.FinishSegment():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ByteStreamGeometryContext.ReadWriteData(System.Boolean,System.Byte*,System.Int32,System.Int32,System.Int32&):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1425556
// Developer: mleonov
// Reason: The assert has been reviewed by the Partial Trust team.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "MS.Internal.FontCache.FontSource.GetLastWriteTimeUtc():System.DateTime")]


//**************************************************************************************************************************
// Bug ID: 1425555
// Developer: mleonov
// Reason: This particular method (Marshal.GetLastWin32Error()) is safe to expose, so there is no security vulnerability here.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.FontCache.FileMapping.OpenFile(System.String):System.Void")]

// jordanpa - this method only does matrix math
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Media.MILUtilities.MIL3DCalcProjected2DBounds(System.Windows.Media.Composition.D3DMATRIX,System.Windows.Media.MILRect3D,System.Windows.Media.MILRectF_RB):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1468165
// Developer: rajatg/robertwl
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ImageSourceConverter.GetBitmapStream(System.Byte[]):System.IO.Stream")]


//**************************************************************************************************************************
// Bug ID: 1496404
// Developer: neilkr
// Reason: Match() is not an event handler even though it has a similar signature
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Input.InputGesture.Matches(System.Object,System.Windows.Input.InputEventArgs):System.Boolean")]

//**************************************************************************************************************************
// Developer: mleonov.
// Similar to 1206763, these are OK to call, as there is no other way to get to the raw handle.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "MS.Internal.FontCache.ALPCServices.TrySendReply(MS.Win32.SafePortHandle,MS.Internal.FontCache.LPCMessage,System.Int32&):System.Void", MessageId = "System.Runtime.InteropServices.SafeHandle.DangerousGetHandle")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "MS.Internal.FontCache.ALPCServices.TryProcessConnectionRequest(MS.Win32.SafePortHandle,System.Int32,System.IntPtr,MS.Internal.FontCache.LPCMessage,System.Boolean,System.Int32&):MS.Win32.SafePortHandle", MessageId = "System.Runtime.InteropServices.SafeHandle.DangerousGetHandle")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "MS.Internal.FontCache.ALPCServices.TryWaitForClient(MS.Win32.SafePortHandle,MS.Internal.FontCache.LPCMessage,System.Int32,System.Int32&):MS.Internal.FontCache.LPCMessage", MessageId = "System.Runtime.InteropServices.SafeHandle.DangerousGetHandle")]

//**************************************************************************************************************************
// Developer: mleonov.
// These new violations are a result of refactoring and are equivalent to other approved issues in FontCache code.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.FontCache.ElementCacher.GetVersionPointer():MS.Internal.FontCache.CheckedPointer")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.FontCache.ElementCacher.GetCacheMemoryRemaining(System.Byte*):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.FontCache.ElementCacher.get_Mapping():MS.Internal.FontCache.CheckedPointer")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.FontCache.ElementCacher.UnsafeGetCacheHeader():MS.Internal.FontCache.ElementCacher+CacheHeader*")]


//**************************************************************************************************************************
// Developer: rajatg
// These new violations are a result of refactoring and are equivalent to other approved issues in Imaging code.
// The DangerousGetHandle issue has no workaround and the code is as per what the CLR security folks recommend
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContextHelper.get_IsInvalid():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContextHelper.GetColorProfileFromHandle(System.IntPtr,System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContextHelper.GetColorProfileHeader(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorContextHelper.OpenColorProfile(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Media.ColorTransformHelper.CreateTransform(System.Windows.Media.SafeProfileHandle,System.Windows.Media.SafeProfileHandle):System.Void", MessageId="System.Runtime.InteropServices.SafeHandle.DangerousGetHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorTransformHelper.CreateTransform(System.Windows.Media.SafeProfileHandle,System.Windows.Media.SafeProfileHandle):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.ColorTransformHelper.TranslateColors(System.IntPtr,System.UInt32,System.UInt32,System.IntPtr,System.UInt32):System.Void")]


//**************************************************************************************************************************
// Developer: lblanco
// Bug: 1539515
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+BitmapHandle..ctor(System.IntPtr,System.Boolean)")]

//**************************************************************************************************************************
// Developer: tmulcahy
// Bug: 1562817
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.HRESULT.ConvertHRToException(System.Int32):System.Exception")]

//**************************************************************************************************************************
// Developer: waynezen
// The indirect caller of those two methods at the TAS boundary is checking the unmanaged code permission.
// And the all immediate callers which are non-public methods are marked as SecurityCritical.
// Bug: 1585860
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.DataObject.Win32CreateStreamOnHGlobal(System.IntPtr,System.Boolean,System.Runtime.InteropServices.ComTypes.IStream&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.DataObject.GetDataIntoOleStructsByTypeMedimIStream(System.String,System.Object,System.Runtime.InteropServices.ComTypes.STGMEDIUM&):System.Int32")]

//**************************************************************************************************************************
// Developer: tmulcahy
// Bug: 1613551
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.MediaPlayerState.ClearBitmapSource():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.MediaPlayerState.CloseRenderTarget():System.Void")]

//**************************************************************************************************************************
// Developer: rajatg
// Bug: 1632176
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.WritePixels(System.Windows.Int32Rect,System.IntPtr,System.Int32,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.WritePixels(System.Windows.Int32Rect,System.Array,System.Int32,System.Int32):System.Void")]

//**************************************************************************************************************************
// Developer: dorinung
// Bug: 1667289se
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.MediaSystem.s_pTransportManager")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.AppModel.CookieHandler.GetCookie(System.Uri,System.Boolean):System.String")]


//**************************************************************************************************************************
// Bug ID: DevDiv 107460
// Developer: cedricd
// Reason: FxCop is confused here. We're implementing an interface, not overriding a virtual method.
// We've added LinkDemands to the interface methods on IKeyboardInputSink.  All classes implementing this interface (HwndSource in this case)
// use an explicit interface implementation.  The only way to call the method is through the interface, so the link demand will be enforced.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.TranslateAccelerator(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.OnMnemonic(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.OnMnemonic(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.TranslateChar(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.TranslateChar(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndSource.System.Windows.Interop.IKeyboardInputSink.set_KeyboardInputSite(System.Windows.Interop.IKeyboardInputSite):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.Dispose(System.Boolean):System.Void")]


//**************************************************************************************************************************
// Bug ID: 109437
// Developer: benwest
// Reason: Methods are SecurityCritical -- it's ok to call LinkDemanded code.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TextServicesContext.StartTransitoryExtension():System.Void")]


//**************************************************************************************************************************
// Developer: dwaynen
// Reason: Methods are SecurityCritical -- it's ok to call LinkDemanded code.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#WritePixels(System.Windows.Int32Rect,System.Array,System.Int32,System.Int32,System.Int32)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#Unlock()")]

//**************************************************************************************************************************
// Bug IDs: 199045
// Developer: brandf
// Reason: Method does not expose data from LinkDemanded code (Marshal.SizeOf)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#ValidateArrayAndGetInfo(System.Array,System.Boolean,System.Int32&,System.Int32&,System.Type&)")]

//**************************************************************************************************************************
// Bug IDs: 198727
// Developer: gschneid
// Reason: This method hooks up a reverse-p-invoke callback for native code to call back into managed code to produce glyph 
// bitmaps. This is done independent of any arguments or user data (i.e. the user cannot manipulate which method is being used
// for the reverse p-invoke). Note also that the method is marked as SecurityCritical and hence has been verified to not be called
// in an untrusted context.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Media.GlyphCache.#SendCallbackEntryPoint()")]


//**************************************************************************************************************************
// Developer: bencar
// Reason: ManipulationDevice is initializing the same way as MouseDevice, which also has a suppression.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.ManipulationDevice.#.ctor(System.Windows.Input.InputManager)")]

//**************************************************************************************************************************
// Developer: kedecond
// Reason: All the callers are SecurityCritical - it is ok for them to call link demand methods
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Input.ManipulationDevice+ManipulationInputProvider.#PushManipulationInput(System.Windows.PresentationSource,System.Windows.Input.InputEventArgs)")]

//**************************************************************************************************************************
// Developer: arathira
// Reason: Callers are marked SecurityCritical and TreatAsSafe because CompositionTarget.TransformToDevice
// and CompositionTarget.TransformToDevice are treated as safe.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.UIElement.#RoundRect(System.Windows.Rect,System.Windows.UIElement)")]

//**************************************************************************************************************************
// Developer: bencar
// Reason: These methods are only meant to be called by our code (hence the link demand), but it is OK for them to be
//         indirectly called from public API (what the rule checks for).
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.ManipulationDevice.#.ctor(System.Windows.UIElement)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.ManipulationDevice.#ProcessManipulationInput(System.Windows.Input.InputEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.MouseDevice.#GlobalHitTest(System.Boolean,System.Windows.Point,System.Windows.PresentationSource,System.Windows.IInputElement&,System.Windows.IInputElement&)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#AttachTouchDevice()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#DetachTouchDevice()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseGotCapture(System.Windows.IInputElement)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseLostCapture(System.Windows.IInputElement)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseTouchDown()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseTouchEnter(System.Windows.IInputElement)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseTouchLeave(System.Windows.IInputElement)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseTouchMove()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#RaiseTouchUp()")]


//**************************************************************************************************************************
// Bug IDs: 692903
// Developer: BrenClar
// Reason: This is existing code. The use of IntPtr has previously been security reviewed and marked as SecurityCritical.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.MediaContext+ChannelManager.#_pSyncConnection")]


//**************************************************************************************************************************
// Bug ID: Dev10 692903
// Developer: bartde
// Reason: The IKIS methods on HwndSource are marked with a LinkDemand to encourage overrides to do that too;
//         see HwndSource.cs for more details.
//         The WeakEventPreprocessMessage methods are SecurityCritical and are considered safe to be exposed publicly;
//         notice these methods are part of a private inner class, so they're not directly exposed either.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.#System.Windows.Interop.IKeyboardInputSink.set_KeyboardInputSite(System.Windows.Interop.IKeyboardInputSite)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource.#System.Windows.Interop.IKeyboardInputSink.TranslateAccelerator(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource+WeakEventPreprocessMessage.#.ctor(System.Windows.Interop.HwndSource,System.Boolean)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndSource+WeakEventPreprocessMessage.#Dispose()")]


//**************************************************************************************************************************
// Bug ID: Dev10 798737
// Developer: pantal
// Reason: System.Windows.Input.Manipulations.dll operates as a Security Transparent Assembly with the new security model, 
//         which this FxCop rule is not presently interpreting properly.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.InertiaExpansionBehavior.#ApplyParameters(System.Windows.Input.InertiaExpansionBehavior,System.Windows.Input.Manipulations.InertiaProcessor2D,System.Windows.Vector)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.InertiaRotationBehavior.#ApplyParameters(System.Windows.Input.InertiaRotationBehavior,System.Windows.Input.Manipulations.InertiaProcessor2D,System.Double)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.InertiaTranslationBehavior.#ApplyParameters(System.Windows.Input.InertiaTranslationBehavior,System.Windows.Input.Manipulations.InertiaProcessor2D,System.Windows.Vector)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationInertiaStartingEventArgs.#ApplyParameters(System.Windows.Input.Manipulations.InertiaProcessor2D)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#BeginInertia(System.Windows.Input.ManipulationInertiaStartingEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#Complete(System.Boolean)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#ConvertCompletedArguments(System.Windows.Input.Manipulations.Manipulation2DCompletedEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#ConvertDelta(System.Windows.Input.Manipulations.ManipulationDelta2D,System.Windows.Input.ManipulationDelta)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#ConvertPivot(System.Windows.Input.ManipulationPivot)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#ConvertVelocities(System.Windows.Input.Manipulations.ManipulationVelocities2D)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#set_ManipulationMode(System.Windows.Input.ManipulationModes)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#set_ManipulationPivot(System.Windows.Input.ManipulationPivot)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#OnInertiaCompleted(System.Object,System.Windows.Input.Manipulations.Manipulation2DCompletedEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#OnInertiaTick(System.Object,System.EventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#OnManipulationCompleted(System.Object,System.Windows.Input.Manipulations.Manipulation2DCompletedEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#OnManipulationDelta(System.Object,System.Windows.Input.Manipulations.Manipulation2DDeltaEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#OnManipulationStarted(System.Object,System.Windows.Input.Manipulations.Manipulation2DStartedEventArgs)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#ReportFrame(System.Collections.Generic.ICollection`1<System.Windows.Input.IManipulator>)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#SetManipulationParameters(System.Windows.Input.Manipulations.ManipulationParameters2D)")]
[module: SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Input.ManipulationLogic.#UpdateManipulators(System.Collections.Generic.ICollection`1<System.Windows.Input.IManipulator>)")]

//**************************************************************************************************************************
// Bug ID: Dev10 806220
// Developer: vamsp
// Reason: Method does not expose data from LinkDemanded code (PresentationSource.CompositionTarget)
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.TouchDevice.#Synchronize()")]