//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


// Windows OS Bug: 1086750
// VSWhidbey Bug:  469677
// Contact:        ChrisEck
//
// The FxCop violation states:
// As it is declared in your code, parameter 'bAlpha' of PInvoke
// UnsafeNativeMethods.SetLayeredWindowAttributes(HandleRef, Int32, Byte, Int32):Boolean
// will be 1 bytes wide on 32-bit platforms. This is not correct, as the actual native
// declaration of this API indicates it should be 4 bytes wide on 32-bit platforms.
//
// This appears to be a spurious FxCop complaint.  The signature should be fine, as a BYTE
// should be 1-byte on 32-bit systems.
//
[module: SuppressMessage("Microsoft.Portability", "CA1901:PInvokeDeclarationsShouldBePortable", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetLayeredWindowAttributes(System.Runtime.InteropServices.HandleRef,System.Int32,System.Byte,System.Int32):System.Boolean")]

// Windows OS Bug: 1086750
// Contact:        ChrisEck
//
// The FxCop violation states:
// As it is declared in your code, parameter 'pt' of PInvoke
// UnsafeNativeMethods.IntWindowFromPoint(POINTSTRUCT):IntPtr will be 8 bytes wide on
// 32-bit platforms. This is not correct, as the actual native declaration of this
// API indicates it should be 4 bytes wide on 32-bit platforms.
//
// This appears to be a spurious FxCop complaint.  The signature should be fine, as the POINTSTRUCT
// is passed by value, and it contains two 32-bit integers.  It should be 8-bytes wide, and the Win32
// definition of the POINT struct also contains to LONG values.
//
[module: SuppressMessage("Microsoft.Portability", "CA1901:PInvokeDeclarationsShouldBePortable", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntWindowFromPoint(MS.Win32.UnsafeNativeMethods+POINTSTRUCT):System.IntPtr")]

// Windows OS Bug: 1086750
//
// This violation is technically correct.  But we only call through this PInvoke definition on 32-bit
// code, which we check at runtime.  The signature is fine for 32-bit, and is the best tradeoff since
// we need to pass a delegate.  For 64-bit code, we use SetWindowLongPtr.
//
[module: SuppressMessage("Microsoft.Portability", "CA1901:PInvokeDeclarationsShouldBePortable", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntCriticalSetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32,MS.Win32.NativeMethods+WndProc):System.Int32")]

// Windows OS Bug: 1086750
//
// This violation is technically correct.  But we only call through this PInvoke definition on 32-bit
// code, which we check at runtime.  The signature is fine for 32-bit, and is the best tradeoff since
// we need to return a delegate.  For 64-bit code, we use GetWindowLongPtr.
//
[module: SuppressMessage("Microsoft.Portability", "CA1901:PInvokeDeclarationsShouldBePortable", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLongWndProc(System.Runtime.InteropServices.HandleRef,System.Int32):MS.Win32.NativeMethods+WndProc")]

//-----------------------------------------------------------------------------
//-------------   *** WARNING ***
//-------------    This file (ZipIoLocalFileBlock.cs) is part of a legally monitored development project.
//-------------    Do not check in changes to this project.  Do not raid bugs on this
//-------------    code in the main PS database.  Do not contact the owner(s) of this
//-------------    code directly.  Contact the legal team at ‘ZSLegal’ for assistance. In case you have
//-------------    any feedback or comments about this code. Please carry along this message to the "APPROVED" copy too.
//-------------   *** WARNING ***
//-----------------------------------------------------------------------------
// Bug ID: 1117665   (corresponds to Bug Id 37 in WinZeus Database)
// Developer: WinZeus (contact this alias instead of direct contact with developer)
// Reason: In this case the class ZipIOLocalFileBlock and ZipIoBlockManager have mutual/circular references.
// ZipIoBlockManager is the main holder/owner of the resources including ZipIOLocalFileBlock. So the method
// ZipIoBlockManager.Dispose is calling ZipIOLocalFileBlock.Dispose, adding another reverse call
// (from ZipIOLocalFileBlock.Dispose to ZipIoBlockManager.Dispose) doesn't make any sense in this specific case.
//-----------------------------------------------------------------------------
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="MS.Internal.IO.Zip.ZipIOLocalFileBlock.Dispose():System.Void")]


//SherifM
//Bug# 1048359
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope="member", Target="System.Windows.Media.Matrix.Rotate(System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope="member", Target="System.Windows.Media.Matrix.RotatePrepend(System.Double):System.Void")]


//*******************************************************************************************************************
// Bug ID: 1169102
// Developer: CedricD
// Reason: This usage of a virtual class is fine.  The class is internal and all derived classes
// are resiliant to this.  I've added a comment to the code for anyone else subclassing it.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Win32.NativeMethods+IconHandle..ctor(System.IntPtr,System.Boolean)")]

//*******************************************************************************************************************
// Bug ID: 1533782
// Bug ID: 1539509
// Developer: LBlanco
// Reason: This code is the same as the IconHandle code, so the same reasoning applies. To reiterate,
// this usage of a virtual class is fine.  The class is internal and all derived classes
// are resiliant to this.  There is a comment to the code for anyone else subclassing it.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Win32.NativeMethods+BitmapHandle..ctor(System.IntPtr,System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Win32.NativeMethods+BitmapHandle..ctor(System.IntPtr,System.Boolean)")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="MS.Win32.UnsafeNativeMethods+WIC.#ReleaseInterface(System.IntPtr&)", MessageId="MS.Win32.UnsafeNativeMethods+WIC.Release(System.IntPtr)")]


//**************************************************************************************************************************
// Bug IDs: 140708
// Developer: sambent
// Reason: A new ShutDownListener is "used" because it listens to events like AppDomain.DomainUnload.
//      The reference implicit in the event's delegate list keeps the object alive.  FxCop doesn't understand this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="MS.Internal.WeakEventTable.#.ctor()", MessageId="MS.Internal.WeakEventTable+WeakEventTableShutDownListener")]

//**************************************************************************************************************************
// Bug IDs: 202870
// Developer: dwaynen
// Reason: A new ShutDownListener is "used" because it listens to events like AppDomain.DomainUnload.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="MS.Win32.ManagedWndProcTracker.#.cctor()", MessageId="MS.Win32.ManagedWndProcTracker+ManagedWndProcTrackerShutDownListener")]

//**************************************************************************************************************************
// Bug ID: 744160
// Developer: brandf
// Reason: In Justification field
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.ObservableCollectionDefaultValueFactory`1<type parameter.T>+ObservableCollectionDefaultPromoter", Scope="member", Target="MS.Internal.ObservableCollectionDefaultValueFactory`1.#CreateDefaultValue(System.Windows.DependencyObject,System.Windows.DependencyProperty)", Justification="Creating this object causes an event to be hooked.")]

//**************************************************************************************************************************
// Bug ID: 784238
// Developer: bchapman
// Reason: The GCNotificationToken object is created only for the executing code to be notified of a GC and is intended to be released immediately.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "MS.Internal.WindowsBase.GCNotificationToken", Scope = "member", Target = "MS.Internal.WindowsBase.GCNotificationToken.#RegisterCallback(System.Threading.WaitCallback,System.Object)", Justification = "The GCNotificationToken object is created only for the executing code to be notified of a GC and is intended to be released immediately")]