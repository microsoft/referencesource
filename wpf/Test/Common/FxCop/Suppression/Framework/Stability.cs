//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//SherifM
//Bug# 1104448
//See bug description for more details (External for VS Whidbey bug).
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.Documents.CaretElement.Win32CreateCaret():System.Void")]
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.Documents.CaretElement.Win32GetCaretBlinkTime():System.Int32")]
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.Documents.CaretElement.Win32DestroyCaret():System.Void")]
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.Documents.CaretElement.Win32SetCaretPos():System.Void")]

//andren
//bug 1380386
//FxCop detection issue (see bug, basically FxCop isn't noticing the line after a return can't be hit)
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.Application.GetCookie(System.Uri):System.String")]

//SherifM
//Bug# 1038780
//See bug description for more details
// mruiz - we do not build this type anymore, but it could be supressed if it is brought back in its current form
//[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="System.Windows.IconData.CreateIconFromFile(System.String):MS.Win32.NativeMethods+IconHandle")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

