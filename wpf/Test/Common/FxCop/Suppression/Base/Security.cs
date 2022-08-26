//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//**************************************************************************************************************************
// Bug ID: 1395027
// Developer: igorbel
// Reason: Al public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
// All the entry points listed beow has been reviewed by the PArtial Trust Team.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetNameAndDescription(System.UInt32,System.UInt32,System.UInt32&,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMSetNameAndDescription(System.UInt32,System.Boolean,System.UInt32,System.String,System.String):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1395026
// Developer: igorbel
// Reason: Al public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.Initialize(System.DateTime,System.DateTime,System.String,System.Uri,System.Security.RightsManagement.ContentUser,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Guid,System.Collections.Generic.ICollection`1<System.Security.RightsManagement.ContentGrant>,System.Collections.Generic.IDictionary`2<System.Int32,System.Security.RightsManagement.LocalizedNameDescriptionPair>):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1366735
// Developer: igorbel
// Reason: Al public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.CryptoProvider..ctor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Security.RightsManagement.ContentUser)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.CryptoProvider.get_CanDecrypt():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.CryptoProvider.get_CanEncrypt():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.CryptoProvider.Dispose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.UnsignedPublishLicense..ctor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String)")]



//**************************************************************************************************************************
// Bug ID: 1366737 1368519
// Developer: igorbel
// Reason: Al public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.Initialize(System.DateTime,System.DateTime,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.String>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Uri>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Security.RightsManagement.ContentUser>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.String>,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Guid,System.Collections.Generic.ICollection`1<MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Security.RightsManagement.ContentGrant>>):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.Initialize(System.DateTime,System.DateTime,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.String>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Uri>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Security.RightsManagement.ContentUser>,MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.String>,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Guid,System.Collections.Generic.ICollection`1<MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.Security.RightsManagement.ContentGrant>>):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1366737 1368520
// Developer: igorbel
// Reason: Al public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetGrantsFromBoundUseLicense(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Security.RightsManagement.ContentUser):System.Collections.Generic.List`1<System.Security.RightsManagement.ContentGrant>")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.BindUseLicense(MS.Internal.Security.RightsManagement.UnmanagedCriticalDataForSet`1<System.String>):System.Security.RightsManagement.CryptoProvider")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.Dispose(System.Boolean):System.Void")]



//**************************************************************************************************************************
// Bug ID:  1366740 1368521
// Developer: igorbel
// Reason: All of the SuppressUnmanagedCodeSecurity attributes below have been reviewed by the Partial Trust Team
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateIssuanceLicense(MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.String,System.String,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateBoundLicense(System.UInt32,MS.Internal.Security.RightsManagement.BoundLicenseParams,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMInitEnvironment(System.UInt32,System.UInt32,System.String,System.String,System.String,System.UInt32&,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateEnablingBitsDecryptor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetBoundLicenseObject(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetBoundLicenseAttribute(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.UInt32&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateEnablingBitsEncryptor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMDecrypt(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.UInt32,System.UInt32,System.Byte[],System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateLicenseStorageSession(System.UInt32,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.UInt32,System.UInt32,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetInfo(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMEncrypt(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.UInt32,System.UInt32,System.Byte[],System.UInt32&,System.Byte[]):System.Int32")]


//**************************************************************************************************************************
// Bug ID: 1341052
// Developer: LGolding
// Reason: By Design, approved by andren, for reasons explained in the bug and its attached e-mail thread.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="MS.Internal.Interop.PROPSPECunion.name")]

//**************************************************************************************************************************
// Bug ID: 1344581
// Developer: FrankGor
// Reason: By Design, code was Reviewed by PT, and already suppressed in Core (method was moved)
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetUrlCacheConfigInfo(MS.Win32.NativeMethods+InternetCacheConfigInfo&,System.UInt32&,System.UInt32):System.Boolean")]


//**************************************************************************************************************************
// Bug ID: 1133992
// Developer: DavidJen
// Reason: It is safe to exclude a message from this rule if the field type is immutable.
//         The field's value EmptySortDescriptionCollection is an immutable collection.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.ComponentModel.SortDescriptionCollection.Empty")]


//**************************************************************************************************************************
// Bug ID: Private run
// Developer: andren
// Reason: partial trust security team has reviewed all of these
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.MSInternal", "CA900:AptcaAssembliesShouldBeReviewed")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.IO.FileFormatException.get_SourceUri():System.Uri")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.DependencyObject.GetValueCommon(System.Windows.DependencyProperty,System.Windows.PropertyMetadata):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.DependencyObject.InvalidateProperty(System.Windows.DependencyProperty,System.Windows.PropertyMetadata):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Win32.HwndSubclass.UnhookWindowProc(System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntDestroyWindow(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetProcAddress(System.Runtime.InteropServices.HandleRef,System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.DispatchMessage(System.Windows.Interop.MSG&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntPostMessage(System.Runtime.InteropServices.HandleRef,System.Int32,System.IntPtr,System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetModuleHandle(System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MsgWaitForMultipleObjectsEx(System.Int32,System.IntPtr[],System.Int32,System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TranslateMessage(System.Windows.Interop.MSG&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetMessageW(Microsoft.Win32.MSG&,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+EtwTrace.TraceEvent(System.UInt64,System.Char*):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Utility.EventTrace.IsEnabled(MS.Utility.EventTrace+Flags,MS.Utility.EventTrace+Level):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Threading.Dispatcher..ctor()")]

// ***** added 2/22/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Threading.Dispatcher.DisableAppExceptionDefaultHandler():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Threading.Dispatcher.get_IsJitDebuggingEnabled():System.Boolean")]

// ***** added 2/23/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.TextServicesLoader.TIPsWantToRun():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateThreadMgr(MS.Win32.UnsafeNativeMethods+ITfThreadMgr&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.PostThreadMessage(System.Int32,System.Int32,System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Win32.HandleCollector+HandleType.Add():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntCreateWindowEx(System.Int32,System.String,System.String,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Object):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetClassInfoEx(System.IntPtr,System.String,MS.Win32.NativeMethods+WNDCLASSEX_I):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.UnregisterClass(System.Int32,System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.RegisterClassEx(MS.Win32.NativeMethods+WNDCLASSEX_D):System.UInt16")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Threading.Dispatcher.IsDebugExceptionHandlerEnvironmentVariableSet():System.Boolean")]

// *** added 3/2/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CreateFileMapping(Microsoft.Win32.SafeHandles.SafeFileHandle,MS.Win32.NativeMethods+SECURITY_ATTRIBUTES,System.Int32,System.UInt32,System.UInt32,System.String):MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MapViewOfFileEx(MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle,System.Int32,System.Int32,System.Int32,System.Int32,System.IntPtr):MS.Win32.UnsafeNativeMethods+SafeViewOfFileHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetFocus():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.NotifyWinEvent(System.Int32,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmSetOpenStatus(System.Runtime.InteropServices.HandleRef,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntOleInitialize(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleSetClipboard(MS.Win32.UnsafeNativeMethods+IOleDataObject):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetMessageExtraInfo():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetKeyboardState(System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetOpenStatus(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntReleaseDC(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OpenFileMapping(System.Int32,System.Boolean,System.String):MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ClientToScreen(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+POINT):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetDC(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleGetClipboard(MS.Win32.UnsafeNativeMethods+IOleDataObject&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetConversionStatus(System.Runtime.InteropServices.HandleRef,System.Int32&,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.VirtualAlloc(System.IntPtr,System.UIntPtr,System.Int32,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetSystemMetrics(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetDeviceCaps(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmAssociateContext(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetContext(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetAncestor(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetFileSizeEx(Microsoft.Win32.SafeHandles.SafeFileHandle,MS.Win32.UnsafeNativeMethods+LARGE_INTEGER&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IsWindow(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateInputProcessorProfiles(MS.Win32.UnsafeNativeMethods+ITfInputProcessorProfiles&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleFlushClipboard():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLongPtr(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmReleaseContext(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CreateFile(System.String,System.UInt32,System.UInt32,MS.Win32.NativeMethods+SECURITY_ATTRIBUTES,System.Int32,System.Int32,System.IntPtr):Microsoft.Win32.SafeHandles.SafeFileHandle")]

// *** added 3/7/05:
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.ModifierKeysConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.ModifierKeysConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.ModifierKeysConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.ModifierKeysConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.KeyConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ExpressionConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ExpressionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ExpressionConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ExpressionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.SizeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.SizeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.SizeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.SizeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.RectConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.RectConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.RectConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.RectConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.VectorConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.VectorConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.VectorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.VectorConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PointConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PointConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PointConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PointConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MatrixConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MatrixConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MatrixConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.MatrixConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]

// *** added 3/11/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,System.Boolean&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+ANIMATIONINFO,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ShellExecute(System.Runtime.InteropServices.HandleRef,System.String,System.String,System.String,System.String,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+HIGHCONTRAST_I&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,System.Int32&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CoInternetIsFeatureEnabled(System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetIconInfoImpl(System.Runtime.InteropServices.HandleRef,MS.Win32.UnsafeNativeMethods+ICONINFO_IMPL):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+RECT&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+NONCLIENTMETRICS,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CoInternetIsFeatureZoneElevationEnabled(System.String,System.String,MS.Win32.NativeMethods+IInternetSecurityManager,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetKeyState(System.Int32):System.Int16")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetFocus(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetLayeredWindowAttributes(System.Runtime.InteropServices.HandleRef,System.Int32,System.Byte,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmNotifyIME(System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+ICONMETRICS,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetCursorPos(MS.Win32.NativeMethods+POINT):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.WindowsBase.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]

// *** added 3/15/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CoInternetIsFeatureZoneElevationEnabled(System.String,System.String,MS.Win32.UnsafeNativeMethods+IInternetSecurityManager,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MapViewOfFileEx(MS.Win32.UnsafeNativeMethods+SafeFileMappingHandle,System.Int32,System.Int32,System.Int32,System.IntPtr,System.IntPtr):MS.Win32.UnsafeNativeMethods+SafeViewOfFileHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.UnregisterClass(System.IntPtr,System.IntPtr):System.Boolean")]

// *** added 3/17/05:
//bug 1127615
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetKeyboardState(System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntClientToScreen(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+POINT):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntPostThreadMessage(System.Int32,System.Int32,System.IntPtr,System.IntPtr):System.Int32")]

// *** added 3/21/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Threading.ExceptionWrapper.LoadFilterMode():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetLastError(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.WaitForMultipleObjectsEx(System.Int32,System.IntPtr[],System.Boolean,System.Int32,System.Boolean):System.Int32")]

// *** added 3/24/05:
//dwaynen:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntMsgWaitForMultipleObjectsEx(System.Int32,System.IntPtr[],System.Int32,System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetMessageW(System.Windows.Interop.MSG&,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntSetFocus(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntRegisterClassEx(MS.Win32.NativeMethods+WNDCLASSEX_D):System.UInt16")]
//prasadt:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.UnhookWinEvent(System.IntPtr):System.Boolean")]

// *** added 3/25/05:
//akaza:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntReleaseCapture():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.MonitorFromRect(MS.Win32.NativeMethods+RECT&,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetCurrentThreadId():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IsWindowUnicode(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntGetMonitorInfo(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+MONITORINFOEX):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.LoadImage(System.IntPtr,System.String,System.Int32,System.Int32,System.Int32,System.Int32):MS.Win32.NativeMethods+IconHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.EnableWindow(System.Runtime.InteropServices.HandleRef,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetCursor():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetCurrentThemeName(System.Text.StringBuilder,System.Int32,System.Text.StringBuilder,System.Int32,System.Text.StringBuilder,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.ShowWindowAsync(System.Runtime.InteropServices.HandleRef,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.KillTimer(System.Runtime.InteropServices.HandleRef,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.RegisterClipboardFormat(System.String):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.MessageBox(System.Runtime.InteropServices.HandleRef,System.String,System.String,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetClipboardFormatName(System.Int32,System.Text.StringBuilder,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.DoDragDrop(MS.Win32.UnsafeNativeMethods+IOleDataObject,MS.Win32.UnsafeNativeMethods+IOleDropSource,System.Int32,System.Int32[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetCapture():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetKeyboardLayout(System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetBitmapBits(System.Runtime.InteropServices.HandleRef,System.Int32,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetDoubleClickTime():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntAdjustWindowRectEx(MS.Win32.NativeMethods+RECT&,System.Int32,System.Boolean,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetCurrentProcessId():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntGetWindowRect(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+RECT&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntScreenToClient(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+POINT):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.SetTimer(System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32,MS.Win32.NativeMethods+TimerProc):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetKeyboardLayoutList(System.Int32,System.IntPtr[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.LoadCursor(System.Runtime.InteropServices.HandleRef,System.IntPtr):MS.Win32.NativeMethods+CursorHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.MonitorFromWindow(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetSysColor(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntSetWindowPos(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.IntGetClientRect(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+RECT&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.MapVirtualKey(System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntUnregisterClass(System.IntPtr,System.IntPtr):System.Int32")]

// *** added 4/10/05:
// bug 1141697
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetCurrentThemeName(System.Text.StringBuilder,System.Int32,System.Text.StringBuilder,System.Int32,System.Text.StringBuilder,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetBitmapBits(System.Runtime.InteropServices.HandleRef,System.Int32,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.MessageBox(System.Runtime.InteropServices.HandleRef,System.String,System.String,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.UnsafeSendMessage(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntEnableWindowNoThrow(System.Runtime.InteropServices.HandleRef,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetWindowPos(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ShowWindowAsync(System.Runtime.InteropServices.HandleRef,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetProcAddressNoThrow(System.Runtime.InteropServices.HandleRef,System.String):System.IntPtr")]

// *** added 4/25/05:
// bug 1154718
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetMessagePos():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate._TrackMouseEvent(MS.Win32.NativeMethods+TRACKMOUSEEVENT):System.Boolean")]
// bugs 1155509, 11
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ComponentDispatcher.RaiseThreadMessage(System.Windows.Interop.MSG&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ComponentDispatcher.add_ThreadPreprocessMessage(System.Windows.Interop.ThreadMessageEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Invariant.FailFast(System.String,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Invariant.get_IsDialogOverrideEnabled():System.Boolean")]

// *** added 5/9/05:
// bug 1162824
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntCriticalSetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetParent(System.Runtime.InteropServices.HandleRef):System.IntPtr")]

// *** added 6/15/05:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Invariant.FailFast(System.String,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.LoadCursor(System.Runtime.InteropServices.HandleRef,System.IntPtr):MS.Win32.NativeMethods+IconHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.TrackMouseEvent(MS.Win32.NativeMethods+TRACKMOUSEEVENT):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.GetKeyboardLayoutList(System.Int32,System.IntPtr[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.HwndWrapper..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,MS.Win32.HwndWrapperHook[])")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+IconHandle..ctor(System.IntPtr,System.Boolean)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+IconHandle.SetHandleInternal(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+IconHandle..ctor()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+VARIANT.ToObject():System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.HwndSubclass.UnhookWindowProc(System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.HwndSubclass..ctor(MS.Win32.HwndWrapperHook)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.PtrToStructure(System.IntPtr,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetParent(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetIconInfo(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+ICONINFO&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.PushFrameImpl(System.Windows.Threading.DispatcherFrame):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.TranslateAndDispatchMessage(System.Windows.Interop.MSG&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.InvokeImpl(System.Windows.Threading.DispatcherPriority,System.TimeSpan,System.Delegate,System.Object,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.ShutdownImpl():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.GetMessage(System.Windows.Interop.MSG&,System.IntPtr,System.Int32,System.Int32):System.Boolean")]

[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Security.Permissions.MediaPermission.Copy():System.Security.IPermission")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Security.Permissions.MediaPermissionAttribute.CreatePermission():System.Security.IPermission")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Security.Permissions.WebBrowserPermissionAttribute.CreatePermission():System.Security.IPermission")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Security.Permissions.WebBrowserPermission.Copy():System.Security.IPermission")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.PlaySound(System.String,System.IntPtr,MS.Win32.SafeNativeMethods+PlaySoundFlags):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetActiveWindow():System.IntPtr")]

// *** added 6/22/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleGetClipboard(System.Runtime.InteropServices.ComTypes.IDataObject&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CopyMemoryW(System.IntPtr,System.Char[],System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleSetClipboard(System.Runtime.InteropServices.ComTypes.IDataObject):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CopyMemoryW(System.IntPtr,System.String,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalReAlloc(System.Runtime.InteropServices.HandleRef,System.IntPtr,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalAlloc(System.Int32,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalFree(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntDeleteObject(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalLock(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalUnlock(System.Runtime.InteropServices.HandleRef):System.Boolean")]

// *** added 6/27/05:
//bug 1203396
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_Converter():System.ComponentModel.TypeConverter")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.CanResetValue(System.Object):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.AddValueChanged(System.Object,System.EventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.GetValue(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_SupportsChangeEvents():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_IsLocalizable():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.GetHashCode():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.ResetValue(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_IsReadOnly():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.GetChildProperties(System.Object,System.Attribute[]):System.ComponentModel.PropertyDescriptorCollection")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.Equals(System.Object):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.ShouldSerializeValue(System.Object):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.SetValue(System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.GetEditor(System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.RemoveValueChanged(System.Object,System.EventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_ComponentType():System.Type")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_PropertyType():System.Type")]
//bug 1203395
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyObjectProvider.GetTypeDescriptor(System.Type,System.Object):System.ComponentModel.ICustomTypeDescriptor")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyObjectProvider.GetCache(System.Object):System.Collections.IDictionary")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.ComponentModel.DependencyObjectProvider.GetExtendedTypeDescriptor(System.Object):System.ComponentModel.ICustomTypeDescriptor")]
//bug 1203393,4
[module: SuppressMessage("Microsoft.Security", "CA2113:SecureLateBindingMethods", Scope="member", Target="System.Windows.Markup.ValueSerializer.GetSerializerFor(System.ComponentModel.PropertyDescriptor):System.Windows.Markup.ValueSerializer")]
[module: SuppressMessage("Microsoft.Security", "CA2113:SecureLateBindingMethods", Scope="member", Target="System.Windows.Markup.ValueSerializer.GetSerializerFor(System.Type):System.Windows.Markup.ValueSerializer")]
[module: SuppressMessage("Microsoft.Security", "CA2113:SecureLateBindingMethods", Scope="member", Target="System.Windows.Markup.ValueSerializerAttribute.get_ValueSerializerType():System.Type")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Invariant.KillIfPresentationHost():System.Diagnostics.Process")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmSetConversionStatus(System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32):System.Boolean")]

// *** added 7/7/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntDestroyCursor(System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods._WindowFromPoint(MS.Win32.UnsafeNativeMethods+POINTSTRUCT):System.Int32")]

// *** added 7/11/05:
//bug 1218585
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Int32RectConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Int32RectConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Int32RectConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Int32RectConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.PushFrame(System.Windows.Threading.DispatcherFrame):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.Dispatcher.Run():System.Void")]

// *** added 7/13/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.GetStringTypeEx(System.UInt32,System.UInt32,System.Char[],System.Int32,System.UInt16[]):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.OleIsCurrentClipboard(System.Runtime.InteropServices.ComTypes.IDataObject):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.WideCharToMultiByte(System.Int32,System.Int32,System.String,System.Int32,System.Byte[],System.Int32,System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.IO.Packaging.XmlDigitalSignatureProcessor.Sign(System.IO.Packaging.PackageDigitalSignatureManager,System.Collections.Generic.IEnumerable`1<System.Uri>,System.Collections.Generic.IEnumerable`1<System.IO.Packaging.PackageRelationship>,System.Security.Cryptography.X509Certificates.X509Certificate2,System.String,System.Boolean,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.DataObject>,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.Reference>):System.IO.Packaging.PackageDigitalSignature")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.IO.Packaging.XmlDigitalSignatureProcessor.TransformXml(System.Security.Cryptography.Xml.Transform,System.Object):System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.IO.Packaging.XmlDigitalSignatureProcessor.Verify(System.Security.Cryptography.X509Certificates.X509Certificate2):System.Boolean")]

// *** added 7/25/05:
//bug 1235324,5
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.IO.Packaging.CompoundFile.StreamInfo.SafeReleaseComObject(System.Object):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.IO.Packaging.CompoundFile.StorageInfo.SafeReleaseComObject(System.Object):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.IO.Packaging.StorageRoot.CreateOnStream(System.IO.Stream,System.IO.FileMode):System.IO.Packaging.StorageRoot")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Security.RightsManagement.Errors.ThrowOnErrorCode(System.Int32):System.Void")]


// *** added 8/1/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntWindowFromPoint(MS.Win32.UnsafeNativeMethods+POINTSTRUCT):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CoInternetCreateSecurityManager(System.Object,System.Object&,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntCriticalSetWindowLongPtr(System.Runtime.InteropServices.HandleRef,System.Int32,System.IntPtr):System.IntPtr")]

// *** added 8/4/05:
//bugs 1245353, 1246832
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateThreadMgr(MS.Win32.UnsafeNativeMethods+ITfThreadMgr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.RegisterClipboardFormat(System.String):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.TF_CreateInputProcessorProfiles(MS.Win32.UnsafeNativeMethods+ITfInputProcessorProfiles&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GlobalSize(System.Runtime.InteropServices.HandleRef):System.IntPtr")]

// *** added 8/8/05:
// bug 1250345
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.NaturalLanguageDllGetClassObject(System.Guid&,System.Guid&,System.Object&):System.Void")]
//bug 1254196
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.IO.Packaging.XmlDigitalSignatureProcessor.Sign(System.IO.Packaging.PackageDigitalSignatureManager,System.Collections.Generic.IEnumerable`1<System.Uri>,System.Collections.Generic.IEnumerable`1<System.IO.Packaging.PackageRelationshipSelector>,System.Security.Cryptography.X509Certificates.X509Certificate2,System.String,System.Boolean,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.DataObject>,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.Reference>):System.IO.Packaging.PackageDigitalSignature")]

// *** added 8/29/05:
//bug 1278665
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.InternetGetCookie(System.String,System.String,System.Text.StringBuilder,System.UInt32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.InternetSetCookie(System.String,System.String,System.String):System.Boolean")]
//bug 1278666
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.UnsafeNativeMethods.DRMDeleteLicense(System.UInt32,System.String):System.Int32")]

// *** added 9/12/05:
//bug 1290708
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SafeReleaseComObject(System.Object):System.Int32")]

// *** added 9/19/05:
//bug
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMEnumerateLicense(System.UInt32,System.UInt32,System.UInt32,System.Boolean&,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCloseSession(System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetUserInfo(System.UInt32,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetBoundLicenseAttribute(System.UInt32,System.String,System.UInt32,System.UInt32&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMParseUnboundLicense(System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCloseHandle(System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetUnboundLicenseAttribute(System.UInt32,System.String,System.UInt32,System.UInt32&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetSignedIssuanceLicense(System.UInt32,System.UInt32,System.UInt32,System.Byte[],System.UInt32,System.String,System.String,MS.Internal.Security.RightsManagement.CallbackDelegate,System.String,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateBoundLicense(System.UInt32,MS.Internal.Security.RightsManagement.BoundLicenseParams,System.String,System.UInt32&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetSecurityProvider(System.UInt32,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateUser(System.String,System.String,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCloseQueryHandle(System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMEncrypt(System.UInt32,System.UInt32,System.UInt32,System.Byte[],System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMAcquireLicense(System.UInt32,System.UInt32,System.String,System.String,System.String,System.String,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMDecrypt(System.UInt32,System.UInt32,System.UInt32,System.Byte[],System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetIssuanceLicenseInfo(System.UInt32,MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.UInt32,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateClientSession(MS.Internal.Security.RightsManagement.CallbackDelegate,System.UInt32,System.String,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMDeleteLicense(System.UInt32,System.String):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetOwnerLicense(System.UInt32,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetUnboundLicenseObject(System.UInt32,System.String,System.UInt32,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetUserRights(System.UInt32,System.UInt32,System.UInt32,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateEnablingBitsEncryptor(System.UInt32,System.String,System.UInt32,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetBoundLicenseObject(System.UInt32,System.String,System.UInt32,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCloseEnvironmentHandle(System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMSetMetaData(System.UInt32,System.String,System.String,System.String,System.String,System.String,System.String):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMDeconstructCertificateChain(System.String,System.UInt32,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMIsActivated(System.UInt32,System.UInt32,MS.Internal.Security.RightsManagement.ActivationServerInfo):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMAddRightWithUser(System.UInt32,System.UInt32,System.UInt32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetRightInfo(System.UInt32,System.UInt32&,System.Text.StringBuilder,MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetServiceLocation(System.UInt32,System.UInt32,System.UInt32,System.String,System.UInt32&,System.Text.StringBuilder):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateIssuanceLicense(MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.String,System.String,System.UInt32,System.String,System.UInt32,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetUsers(System.UInt32,System.UInt32,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMGetInfo(System.UInt32,System.String,System.UInt32&,System.UInt32&,System.Byte[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateLicenseStorageSession(System.UInt32,System.UInt32,System.UInt32,System.UInt32,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateEnablingBitsDecryptor(System.UInt32,System.String,System.UInt32,System.String,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMInitEnvironment(System.UInt32,System.UInt32,System.String,System.String,System.String,System.UInt32&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMCreateRight(System.String,MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.UInt32,System.String[],System.String[],System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Security.RightsManagement.SafeNativeMethods+UnsafeNativeMethods.DRMActivate(System.UInt32,System.UInt32,System.UInt32,MS.Internal.Security.RightsManagement.ActivationServerInfo,System.IntPtr,System.IntPtr):System.Int32")]

// *** added 9/22/05
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CopyMemory(System.IntPtr,System.Byte[],System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ObtainUserAgentString(System.Int32,System.Text.StringBuilder,System.Int32&):System.Int32")]

// *** added 9/27/05
//bug 1321141
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetRegistryPassportCertificationUrl():System.String")]

// *** added 10/12/05
//several bugs
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Threading.DispatcherOperation.Wait(System.TimeSpan):System.Windows.Threading.DispatcherOperationStatus")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.LoadCursor(System.Runtime.InteropServices.HandleRef,System.IntPtr):MS.Win32.NativeMethods+CursorHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.CriticalSetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SafeReleaseComObject(System.Object):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.Errors.ThrowOnErrorCode(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.GetStringTypeEx(System.UInt32,System.UInt32,System.Char[],System.Int32,System.UInt16[]):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetWindowTextLength(System.Runtime.InteropServices.HandleRef):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetWindowText(System.Runtime.InteropServices.HandleRef,System.Text.StringBuilder,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetFocus(System.Runtime.InteropServices.HandleRef):System.IntPtr")]

// *** added 10/17/05
//bug 1341053
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="MS.Win32.NativeMethods+SECURITY_ATTRIBUTES.lpSecurityDescriptor")]
//bug 1341054
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="MS.Win32.NativeMethods+VARIANT.data1")]
//bug 1348028
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.WindowsBase.SecurityHelper.SizeOf(System.Type):System.Int32")]

// *** added 10/31/05
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.PackageDigitalSignature.Verify(System.Security.Cryptography.X509Certificates.X509Certificate):System.IO.Packaging.VerifyResult")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.PackageDigitalSignatureManager.Sign(System.Collections.Generic.IEnumerable`1<System.Uri>,System.Security.Cryptography.X509Certificates.X509Certificate,System.Collections.Generic.IEnumerable`1<System.IO.Packaging.PackageRelationshipSelector>,System.String,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.DataObject>,System.Collections.Generic.IEnumerable`1<System.Security.Cryptography.Xml.Reference>):System.IO.Packaging.PackageDigitalSignature")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.PackageDigitalSignatureManager.PromptForSigningCertificate(System.IntPtr):System.Security.Cryptography.X509Certificates.X509Certificate")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.PackageDigitalSignatureManager.VerifyCertificate(System.Security.Cryptography.X509Certificates.X509Certificate):System.Security.Cryptography.X509Certificates.X509ChainStatusFlags")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.StorageInfo.StorageIsEmpty():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.IO.Packaging.StorageInfo.DestroyElement(System.String):System.Void")]

// *** added 11/10/05
//bug 1380383
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowTextLength(System.Runtime.InteropServices.HandleRef):System.Int32")]

// *** added 11/28/05
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies")]

// *** added 12/6/05:
//bugs 1419287, 1419291
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.SafeNativeMethods.QueryPerformanceCounter(System.Int64&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.SafeNativeMethods+SafeNativeMethodsPrivate.QueryPerformanceCounter(System.Int64&):System.Boolean")]

// *** added 2/16/06:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.get_Property():System.ComponentModel.PropertyDescriptor")]

//bug 1191914, 1398213
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Windows.Markup.ReflectionHelper.LoadAssemblyHelper(System.String,System.String):System.Reflection.Assembly", MessageId = "System.Reflection.Assembly.LoadFrom")]

[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.SplashScreen.#_hInstance")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.SplashScreen.#_hBitmap")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


// Developer: brucemac
// Description: Bogus violations.
[module: SuppressMessage("Microsoft.Security", "CA2101:SpecifyMarshalingForPInvokeStringArguments", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZLib.ums_inflate_init(MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZStream&,System.String,System.Int32):MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZLib+ErrorCode")]
[module: SuppressMessage("Microsoft.Security", "CA2101:SpecifyMarshalingForPInvokeStringArguments", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZLib.ums_deflate_init(MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZStream&,System.Int32,System.String,System.Int32):MS.Internal.IO.Packaging.CompoundFile.UnsafeNativeMethods+ZLib+ErrorCode")]


//**************************************************************************************************************************
// Bug ID: 1442262
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.BindUseLicense(System.String,System.Collections.Generic.List`1<System.String>,MS.Internal.Security.RightsManagement.BoundLicenseParams,System.Int32&):System.Security.RightsManagement.CryptoProvider")]


//**************************************************************************************************************************
// Bug ID: 1471896
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Security.RightsManagement.CryptoProvider.DecryptPublishLicense(System.String):System.Security.RightsManagement.UnsignedPublishLicense")]


//**************************************************************************************************************************
// Bug ID: 1442262
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.BindUseLicense(System.String,System.Collections.Generic.List`1<MS.Internal.Security.RightsManagement.RightNameExpirationInfoPair>,MS.Internal.Security.RightsManagement.BoundLicenseParams,System.Int32&):System.Security.RightsManagement.CryptoProvider")]


//**************************************************************************************************************************
// Bug ID: 1533647
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.Initialize(System.DateTime,System.DateTime,System.String,System.Uri,System.Security.RightsManagement.ContentUser,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Guid,System.Collections.Generic.ICollection`1<System.Security.RightsManagement.ContentGrant>,System.Collections.Generic.IDictionary`2<System.Int32,System.Security.RightsManagement.LocalizedNameDescriptionPair>,System.Collections.Generic.IDictionary`2<System.String,System.String>,System.Int32):System.Void")]

//**************************************************************************************************************************
// Developer: lblanco
// Bug: 1539515
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.NativeMethods+BitmapHandle..ctor(System.IntPtr,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 1533647
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.Initialize(System.DateTime,System.DateTime,System.String,System.Uri,System.Security.RightsManagement.ContentUser,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Guid,System.Collections.Generic.ICollection`1<System.Security.RightsManagement.ContentGrant>,System.Collections.Generic.IDictionary`2<System.Int32,System.Security.RightsManagement.LocalizedNameDescriptionPair>,System.Collections.Generic.IDictionary`2<System.String,System.String>,System.Int32,MS.Internal.Security.RightsManagement.RevocationPoint):System.Void")]

//*********************************************************************************************************************************
// Bug ID: 1571473
// Developer: benwest
// Reason: All of these callers are SecurityCritical -- it is acceptable for them to call link demanded methods.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Win32.NativeMethods+VARIANT.Clear():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Win32.NativeMethods+VARIANT.GetRefInt(System.IntPtr):System.IntPtr")]


//*********************************************************************************************************************************
// Bug ID: 1573217
// Developer: rukeh
// Reason: The method that triggers link demand fxcop failure is ThrowExceptionForHR(). Throwing an exception is deemed as a safe
// operation (throwing exceptions is allowed in Partial Trust). And we need this private method to be visible to the caller that is
// security transparent.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.IO.Packaging.StorageBasedPackageProperties.OpenPropertyStorage(System.Guid&,MS.Internal.IO.Packaging.CompoundFile.IPropertyStorage&):System.Void")]

//*********************************************************************************************************************************
// Bug ID: 1587862
// Developer: rukeh
// Reason: The method that triggers link demand fxcop failure is Marshal.ThrowExceptionForHR(). Throwing an exception is deemed as a safe
// operation (throwing exceptions is allowed in Partial Trust).
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.WindowsBase.SecurityHelper.ThrowExceptionForHR(System.Int32):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1611900
// Developer: igorbel
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession..ctor(System.Security.RightsManagement.ContentUser,System.Security.RightsManagement.UserActivationMode)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.AcquireUseLicense(System.String,System.Boolean):System.Security.RightsManagement.UseLicense")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.CheckDisposed():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetOwnerLicense(MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.SignIssuanceLicense(MS.Internal.Security.RightsManagement.IssuanceLicense,System.Security.RightsManagement.UseLicense&):System.Security.RightsManagement.PublishLicense")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.CheckDisposed():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.GetIssuanceLicenseInfo(System.DateTime&,System.DateTime&,MS.Internal.Security.RightsManagement.DistributionPointInfo,System.String&,System.String&,System.Security.RightsManagement.ContentUser&,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Security.RightsManagement.IssuanceLicense.UpdateUnsignedPublishLicense(System.Security.RightsManagement.UnsignedPublishLicense):System.Void")]

// **************************************************************************************************************************
// Bug ID: 1641130, 1641131
// Developer: benwest
// Reason: All of these callers are SecurityCritical -- it is acceptable for them to call link demanded methods.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Win32.SafeNativeMethods.DestroyCaret():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Win32.SafeNativeMethods.GetCaretBlinkTime():System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Win32.UnsafeNativeMethods.CreateBitmap(System.Int32,System.Int32,System.Int32,System.Int32,System.Byte[]):MS.Win32.NativeMethods+BitmapHandle")]

// **************************************************************************************************************************
// Bug ID: 1653882
// Developer: SamBent
// Reason: This call is SecurityCritical -- it is acceptable to call link demanded methods.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetWindowLongWndProc(System.Runtime.InteropServices.HandleRef):MS.Win32.NativeMethods+WndProc")]

//*********************************************************************************************************************************
// Bug ID:    1533743
// Developer: bchapman
// Reason: In the designer senario we need to load the user's code behind DLL over and over
//         is the user makes changes.  We can't unload the old assemblies so we need a way to
//         load the new ones even though we have an assemblies already  loaded with exactly that idenity.
//
//         LoadFile() gives us that feature.  Fusion would rather we loaded the user assembly into
//         a seperate AppDomain so it can be unloaded but Avalon doesn't support that model.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Windows.Markup.ReflectionHelper.LoadAssemblyHelper(System.String,System.String):System.Reflection.Assembly", MessageId = "System.Reflection.Assembly.LoadFile")]

//**************************************************************************************************************************
// Bug ID: 1683828
// Developer: rukeh
// Reason: All public RM APIs entry points have explicit demand for RightsManagementPermission
// public entry point is marked as SecurityCritical , TreatAsSafe
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateUser(System.String,System.String,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMInitEnvironment(System.UInt32,System.UInt32,System.String,System.String,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementEnvironmentHandle&,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateIssuanceLicense(MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.String,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMParseUnboundLicense(System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementQueryHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateClientSession(MS.Internal.Security.RightsManagement.CallbackDelegate,System.UInt32,System.String,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementSessionHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateBoundLicense(MS.Internal.Security.RightsManagement.SafeRightsManagementEnvironmentHandle,MS.Internal.Security.RightsManagement.BoundLicenseParams,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&,System.UInt32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateEnablingBitsDecryptor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateLicenseStorageSession(MS.Internal.Security.RightsManagement.SafeRightsManagementEnvironmentHandle,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,MS.Internal.Security.RightsManagement.SafeRightsManagementSessionHandle,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementSessionHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMGetUnboundLicenseObject(MS.Internal.Security.RightsManagement.SafeRightsManagementQueryHandle,System.String,System.UInt32,MS.Internal.Security.RightsManagement.SafeRightsManagementQueryHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMGetUsers(MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle,System.UInt32,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMGetUserRights(MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle,System.UInt32,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateEnablingBitsEncryptor(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.String,MS.Internal.Security.RightsManagement.SafeRightsManagementHandle&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMGetIssuanceLicenseInfo(MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle,MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.UInt32,System.UInt32&,System.Text.StringBuilder,System.UInt32&,System.Text.StringBuilder,MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&,System.Boolean&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Security.RightsManagement.SafeNativeMethods.DRMCreateRight(System.String,MS.Internal.Security.RightsManagement.SystemTime,MS.Internal.Security.RightsManagement.SystemTime,System.UInt32,System.String[],System.String[],MS.Internal.Security.RightsManagement.SafeRightsManagementPubHandle&):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1705249 (1635435)
// Developer: kenlai
// Reason: This call is SecurityCritical -- it is acceptable to call link demanded methods.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Diagnostics.PresentationTraceSources.CreateTraceSource(System.String):System.Diagnostics.TraceSource")]



//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

// **************************************************************************************************************************
// Bug ID: 140708
// Developer: SamBent
// Reason: This call is SecurityCritical -- it is acceptable to call link demanded methods.
//*************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.ShutDownListener.#StopListening()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.ShutDownListener.#.ctor(System.Object,MS.Internal.ShutDownEvents)")]

//***************************************************************************************************************************
// New Suppressions since 4.0 RTM
//***************************************************************************************************************************

//***************************************************************************************************************************
// Appropriate full demands are in place here.  LinkDemands are no longer used.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.IO.FileFormatException.#GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Security.RightsManagement.RightsManagementException.#GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]
