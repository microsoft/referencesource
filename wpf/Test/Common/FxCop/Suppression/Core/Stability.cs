//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

// Bug 1394144
// SafeHandle is used to track refcount instead of explicit release
// mruiz - looks like an indirect way of releasing, following up with rajatg
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="System.Windows.Media.Imaging.RenderTargetBitmap.CopyCommon(System.Windows.Media.Imaging.RenderTargetBitmap):System.Void", MessageId="System.Windows.Media.SafeMILHandle")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


// Bug: No Bug
// Reason: Code has been reviewed
// Dev: bkaneva
// mruiz - these are not currently enabled
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1002:PublicVirtualCallerMustBeHardened", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.UnmanagedEffect(System.Boolean):System.Runtime.InteropServices.SafeHandle")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1002:PublicVirtualCallerMustBeHardened", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.Internal_CreateUnmanagedEffect():System.Runtime.InteropServices.SafeHandle")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1002:PublicVirtualCallerMustBeHardened", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.Internal_UpdateUnmanagedPropertyState(System.Runtime.InteropServices.SafeHandle):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1002:PublicVirtualCallerMustBeHardened", Scope="member", Target="System.Windows.Media.Effects.BitmapEffect.UpdateUnmanagedAggregateState():System.Boolean")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="MS.Internal.AppModel.CookieHandler.GetCookie(System.Uri,System.Boolean):System.String")]
