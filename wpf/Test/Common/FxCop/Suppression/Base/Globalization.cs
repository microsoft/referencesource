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
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope="member", Target="System.Windows.Markup.ReflectionHelper.LoadAssemblyHelper(System.String,System.String):System.Reflection.Assembly", MessageId="System.String.EndsWith(System.String)")]
#endif

//**************************************************************************************************************************
// Bug ID: 1050487
// Developer: ChaiwatP (bchapman)
// Reason: 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Win32.HwndWrapper..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,MS.Win32.HwndWrapperHook[])")]

//**************************************************************************************************************************
// Bug ID: 1625249
// Developer: KenLai
// Reason: format string used for trace messages; will not need to be localized
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.AvTrace.Trace(System.Diagnostics.TraceEventType,System.Int32,System.String,System.String[],System.Object[]):System.Void", MessageId="MS.Internal.AvTraceBuilder.AppendFormat(System.String,System.Object,System.Object)")]

[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.AvTrace.Trace(System.Diagnostics.TraceEventType,System.Int32,System.String,System.String[],System.Object[]):System.Void", MessageId="MS.Internal.AvTraceBuilder.Append(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.AvTrace.Trace(System.Diagnostics.TraceEventType,System.Int32,System.String,System.String[],System.Object[]):System.Void", MessageId="System.Int32.ToString")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************
// **************************************************************************************************************************
// Bug ID: 731486
// Developer: kedecond
// Reason: Issue with fxcop rule, shows error even with Unicode charset. Can be removed when issue is fixed(Bug 735418).
//*************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "LOGFONT.lfFaceName", Scope = "member", Target = "MS.Win32.UnsafeNativeMethods.#SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+ICONMETRICS,System.Int32)")]
[module: SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "LOGFONT.lfFaceName", Scope = "member", Target = "MS.Win32.UnsafeNativeMethods.#SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+NONCLIENTMETRICS,System.Int32)")]


// **************************************************************************************************************************
// Bug ID: 731486
// Developer: pantal
// Reason: Issue with fxcop rule, shows error even with Unicode charset. Can be removed when issue is fixed(Bug 735418).
//*************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpTemplateName", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetOpenFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrDefExt", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetOpenFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrFilter", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetOpenFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrInitialDir", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetOpenFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrTitle", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetOpenFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpTemplateName", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetSaveFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrDefExt", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetSaveFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrFilter", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetSaveFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrInitialDir", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetSaveFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OPENFILENAME_I.lpstrTitle", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetSaveFileName(MS.Win32.NativeMethods+OPENFILENAME_I)")]

[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="OSVERSIONINFOEX.csdVersion", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#GetVersionEx(MS.Win32.NativeMethods+OSVERSIONINFOEX)")]

[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="ShellExecuteInfo.lpClass", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#ShellExecuteEx(MS.Win32.UnsafeNativeMethods+ShellExecuteInfo)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="ShellExecuteInfo.lpDirectory", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#ShellExecuteEx(MS.Win32.UnsafeNativeMethods+ShellExecuteInfo)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="ShellExecuteInfo.lpFile", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#ShellExecuteEx(MS.Win32.UnsafeNativeMethods+ShellExecuteInfo)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="ShellExecuteInfo.lpParameters", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#ShellExecuteEx(MS.Win32.UnsafeNativeMethods+ShellExecuteInfo)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="ShellExecuteInfo.lpVerb", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#ShellExecuteEx(MS.Win32.UnsafeNativeMethods+ShellExecuteInfo)")]

[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="WNDCLASSEX_D.lpszClassName", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#IntRegisterClassEx(MS.Win32.NativeMethods+WNDCLASSEX_D)")]
[module: SuppressMessage("Microsoft.Globalization","CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId="WNDCLASSEX_D.lpszMenuName", Scope="member", Target="MS.Win32.UnsafeNativeMethods.#IntRegisterClassEx(MS.Win32.NativeMethods+WNDCLASSEX_D)")]
