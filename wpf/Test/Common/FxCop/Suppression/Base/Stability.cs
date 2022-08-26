//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//Sherif
//Bug# 1048464 1048462
//See bug for more details.
//FxCop false positive: the methods referred to *are* P/Invoke calls.
[module: SuppressMessage("Microsoft.Interoperability", "CA1404:CallGetLastErrorImmediatelyAfterPInvoke", Scope="member", Target="MS.Win32.HwndSubclass..cctor()")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

