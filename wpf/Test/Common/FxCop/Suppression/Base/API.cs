//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;


//********** SherifM
//********** AvoidExcessiveComplexity
//********** Multiple bugs.

[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.KeyInterop.VirtualKeyFromKey(System.Windows.Input.Key):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.KeyInterop.KeyFromVirtualKey(System.Int32):System.Windows.Input.Key")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.KeyConverter.GetKey(System.String,System.Globalization.CultureInfo):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Win32.NativeMethods+VARIANT.ToObject():System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Utility.TraceProvider.TraceEvent(System.Guid,System.UInt32,System.Object,System.Object,System.Object,System.Object,System.Object,System.Object,System.Object,System.Object,System.Object):System.UInt32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Utility.TraceProvider.EncodeObject(System.Object,MS.Utility.TraceProvider+MofField*,System.Char*,System.UInt32&,System.Byte*):System.String")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Utility.SixObjectMap.RemoveEntry(System.Int32):System.Void")]

//* SherifM
//* Bug# 1061813
//* Approved by actprog
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="Microsoft.Win32.ThreadMessageEventHandler.BeginInvoke(Microsoft.Win32.MSG&,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="Microsoft.Win32.ThreadMessageEventHandler.EndInvoke(Microsoft.Win32.MSG&,System.Boolean&,System.IAsyncResult):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="Microsoft.Win32.ThreadMessageEventHandler.Invoke(Microsoft.Win32.MSG&,System.Boolean&):System.Void")]


//* SherifM
//* Bug# 1061830
//* Approved by actprog
[module: SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Scope="member", Target="System.IO.Packaging.PackageDigitalSignature.SignatureValue")]

//**************************************************************************************************************************
// Bug ID: 1048326
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Input.IKeyboardInputSink.OnMnemonic(Microsoft.Win32.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1048326
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Input.IKeyboardInputSink.TranslateAccelerator(Microsoft.Win32.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1075314
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Input.IKeyboardInputSink.TranslateChar(Microsoft.Win32.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]


//**************************************************************************************************************************
// Bug ID: 1061850
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way for better readability.
//         There is a bug in FxCop SuppressMessage Copy code. When SuppressMessage strings are copied for these violations
//         it copied them without mentioning parameter name or number as below. This way we cannot distinguish the parameters.
//         I am putting the copied text here for all the  violations listed in the bug as they all need to be suppressed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1061832
// Developer: SherifM
// Reason: see bug for details
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1054:UriParametersShouldNotBeStrings", Scope="member", Target="System.IO.Packaging.PackUriHelper.Create(System.String,System.String):System.Uri")]
[module: SuppressMessage("Microsoft.Design", "CA1054:UriParametersShouldNotBeStrings", Scope="member", Target="System.IO.Packaging.PackUriHelper.Create(System.String,System.String):System.Uri")]


//**************************************************************************************************************************
// Bug ID: 1061832
// Developer: SherifM
// Reason: see bug for details
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1054:UriParametersShouldNotBeStrings", Scope="member", Target="System.IO.Packaging.PackUriHelper.Create(System.String,System.String):System.Uri")]
[module: SuppressMessage("Microsoft.Design", "CA1054:UriParametersShouldNotBeStrings", Scope="member", Target="System.IO.Packaging.PackUriHelper.Create(System.String,System.String):System.Uri")]

//**************************************************************************************************************************
// Bug ID: 1038642
// Developer: Chandras/DwayneN/YutakaS
// Reason: see bug for details
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumLock")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad0")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad1")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad2")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad3")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad4")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad5")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad6")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad7")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad8")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.NumPad9")]


//**************************************************************************************************************************
// Bug ID: 1097192
// Developer: MLeonov
// Reason: This is by design per rule documentation:
// "It is safe to exclude a message from this rule if the string parameter does not represent a URI."
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1057:StringUriOverloadsCallSystemUriOverloads", Scope = "member", Target = "System.IO.FileFormatException..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Design", "CA1057:StringUriOverloadsCallSystemUriOverloads", Scope = "member", Target = "System.IO.FileFormatException..ctor(System.String,System.Exception)")]

//**************************************************************************************************************************
// Bug ID: 1086743
// Developer: cedricd
// Reason: This structure goes straight into unmanaged code and cannot be changed.  I've verfied it is always 64bit aligned
//         so this rule appears bogus.  I have not had the time to debug the rule to verify why it flagged this as a problem
//         so I've assigned M11 bug 1132271 to myself.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="MS.Utility.TraceProvider+BaseEvent")]


//**************************************************************************************************************************
// Bug ID: 1048300
// Developer: a-shsolk
// Reason: Resolved as Duplicate of Bug 938461(wont fix) by actdata (sambent)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.ComponentModel.ICollectionView")]

//**************************************************************************************************************************
// Developer: rrelyea
// Reason: Bulk exclude of this rule.  We'll let dev leads know about them...so in case they are worried...
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader.Read():System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.IO.Zip.ZipArchive.ValidateModeAccessShareStreaming(System.IO.Stream,System.IO.FileMode,System.IO.FileAccess,System.IO.FileShare,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Security.RightsManagement.Errors.DecodeBindingFailureCode(System.Int32):System.Security.RightsManagement.BindingFailure")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.IO.Packaging.PackageCoreProperties.ParseCorePropertyPart(System.IO.Packaging.PackagePart):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1756224
// Developer: sramani
// Reason: see bug for details
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Markup.RootNamespaceAttribute..ctor(System.String)", MessageId = "nameSpace")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


// [dbrown/avarch] The following request had no comment. It looks like the code has changed in any case.
// I'm commenting the request out but leaving the comment in here as history just in case it still is in the code.
//[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased",
//              Scope="member", Target="Microsoft.Win32.ComponentDispatcher.CurrentKeyboardMSG")]

//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.DependencyObject.OnPropertyChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void")]
#endif

//**************************************************************************************************************************
// Bug ID: 1010939
// Developer: Chandras/DwayneN/YutakaS
// Reason: see bug for details
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Input.Key.LineFeed")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.AbntC1")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.AbntC2")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.DbeSbcsChar")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.DbeEnterDlgConversionMode")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.JunjaMode")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.OemEnlw")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.DbeDbcsChar")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.EraseEof")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Input.Key.CrSel")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.CrSel")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Input.Key.ExSel")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.ExSel")]

//**************************************************************************************************************************
// Bug ID: 1159442
// Developer: jdmack
// Reason: These are wrappers around native the Win32 MSG struct's members, and as such should preserve the Win32 casing style
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.pt_x")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.pt_y")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.message")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.hwnd")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.time")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.lParam")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "Microsoft.Win32.MSG.wParam")]

//**************************************************************************************************************************
// Bug ID: 1182058
// Developer: jdmack
// Reason: These are wrappers around native the Win32 MSG struct's members, and as such should preserve the Win32 casing style
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.pt_x")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.pt_y")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.message")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.hwnd")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.time")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.lParam")]
[module: SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Interop.MSG.wParam")]

//**************************************************************************************************************************
// Bug ID: 1182091
// Developer: mikehill
// Reason: Method contains a large switch statement.
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader.Read():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1218565
// Developer: davidjen
// Reason: the 2 flagged members are events, but the [NonSerialized] attibute can only be applied to fields
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2235:MarkAllNonSerializableFields", Scope = "member", Target = "System.Collections.ObjectModel.ObservableCollection'1.CollectionChanged")]
[module: SuppressMessage("Microsoft.Usage", "CA2235:MarkAllNonSerializableFields", Scope = "member", Target = "System.Collections.ObjectModel.ObservableCollection'1.PropertyChanged")]

//**************************************************************************************************************************
// Bug ID: 1233100
// Developer: dwaynen
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1003:UseGenericEventHandlerInstances", Scope="type", Target="System.Windows.DependencyPropertyChangedEventHandler")]
[module: SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix", Scope="type", Target="System.Windows.DependencyPropertyChangedEventArgs")]

//**************************************************************************************************************************
// Bug ID: 1340943
// Developer: dwaynen
// Reason: We have not moved to SafeHandles, so we still need this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Win32.HwndWrapper..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,MS.Win32.HwndWrapperHook[])")]

//**************************************************************************************************************************
// Bug ID: 1319515
// Developer: jordanpa
// Reason: CloneCore(Freezable) is called by Clone() so that subclasses of Freezable can override it to clone their data.
//         CloneCore(DependencyObject) would work but it's not correct.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Freezable.CloneCore(System.Windows.Freezable):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1307226
// Developer: tjhsiang
// Reason: complexity happens
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader.ReadStartElement(System.Boolean&):System.Boolean")]

//***************************************************************************************************************************
// Bug ID: 1340726
// Developer: kenlai
// Reason: Keys are supposed to be single letter.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.A", MessageId="A")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.B", MessageId="B")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.C", MessageId="C")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.D", MessageId="D")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.E", MessageId="E")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.F", MessageId="F")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.G", MessageId="G")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.H", MessageId="H")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.I", MessageId="I")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.J", MessageId="J")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.K", MessageId="K")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.L", MessageId="L")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.M", MessageId="M")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.N", MessageId="N")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.O", MessageId="O")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.P", MessageId="P")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.Q", MessageId="Q")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.R", MessageId="R")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.S", MessageId="S")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.T", MessageId="T")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.U", MessageId="U")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.V", MessageId="V")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.W", MessageId="W")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.Key.Z", MessageId="Z")]

//***************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: These are coordinates X and Y
//***************************************************************************************************************************
//1340727
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Point..ctor(System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Point..ctor(System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Point.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Point.Y", MessageId="Y")]
//1340728
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect.Contains(System.Double,System.Double):System.Boolean", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect.Contains(System.Double,System.Double):System.Boolean", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Rect.Y", MessageId="Y")]
//1340731
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Vector..ctor(System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Vector..ctor(System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Vector.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Vector.Y", MessageId="Y")]
//1340733
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Int32Rect..ctor(System.Int32,System.Int32,System.Int32,System.Int32)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Int32Rect..ctor(System.Int32,System.Int32,System.Int32,System.Int32)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Int32Rect.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Int32Rect.Y", MessageId="Y")]

//***************************************************************************************************************************
// Bug ID: 1340729
// Developer: kenlai
// Reason: this is a bug in FxCop.  EventArgs *should* be named 'e'
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.DependencyObject.OnPropertyChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId="0#e")]

//**************************************************************************************************************************
// Bug ID: 1365558
// Developer: SamBent
// Reason: This method is not an event handler.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.WeakEventManager.DeliverEvent(System.Object,System.EventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1365561
// Developer: SamBent
// Reason: This type is designed for use only by classes derived from WeakEventManager.  Hence it is protected.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1034:NestedTypesShouldNotBeVisible", Scope = "type", Target = "System.Windows.WeakEventManager+ListenerList")]

//**************************************************************************************************************************
// Bug ID: 1365564
// Developer: SamBent
// Reason: The cure is worse than the disease.  Resulting code would be less readable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.WeakEventManager+ListenerList.PrepareForWriting(System.Windows.WeakEventManager+ListenerList&):System.Boolean", MessageId="0#")]

//**************************************************************************************************************************
// Bug ID: 1340776
// Developer: GillesK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="multi")]

//**************************************************************************************************************************
// Bug ID: 1386318
// Developer: jordanpa
// Reason: Technically this could take DOs instead of Freezables, but it's only supposed to be called with Freezables...
//         hence the name.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Freezable.OnFreezablePropertyChanged(System.Windows.Freezable,System.Windows.Freezable,System.Windows.DependencyProperty):System.Void")]

//**************************************************************************************************************************
// Bug ID: (not yet filed - I went ahead and ran FXCop to check if some of my changes would cause violations)
// Developer: tjhsiang
// Reason: I use a generic Dictionary<string, bool> and evidently this is not an "approved" generic type.  I talked to
// robertwl, and he said for now to just suppress these messages.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader..ctor(System.Xml.XmlReader)")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader..ctor(System.Xml.XmlReader,System.Windows.Markup.IsXmlNamespaceSupportedCallback,System.Collections.Generic.IEnumerable`1<System.String>)")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader._namespaces")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader.IsNamespaceKnown(System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope._ignorables")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope.CanIgnore(System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope.Ignorable(System.String):System.Void")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope.IsIgnorableAtCurrentScope(System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope.MustUnderstand(System.String):System.Void")]
[module: SuppressMessage("Microsoft.MSInternal", "CA908:UseApprovedGenericsForPrecompiledAssemblies", Scope="member", Target="System.Windows.Markup.XmlCompatibilityReader+CompatibilityScope.Verify():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1431117
// Developer: RobertWl/AlikK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="multidisk")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="nonreadable")]


//**************************************************************************************************************************
// Bug ID: 1581903
// Developer: mikehill
// The objects are not modified during the assert.  But more fundamentally, the object in question (string)
// is immutable anyway.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.WindowsBase.SecurityHelper.ReadRegistryValue(Microsoft.Win32.RegistryKey,System.String,System.String):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1680865
// Developer: AlikK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Multidisk")]

//***************************************************************************************************************************
// Bug ID: 1762494
// Developer: SarjanaS 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope = "resource", Target = "ExceptionStringTable.resources", MessageId = "canonicalization")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//***************************************************************************************************************************
// Bug ID: No bug
// Developer: kurtb 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Diagnostics.PresentationTraceSources.HwndHostSource", MessageId="Hwnd")]

//***************************************************************************************************************************
// Bug ID: 202854
// Developer: bchapman 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Uid", Scope="type", Target="System.Windows.Markup.UidPropertyAttribute")]


//***************************************************************************************************************************
// Bug ID: 731489
// Developer: pantal
// Dev 10 - Bulk Suppressions of legacy breaking change issues
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.IKeyboardInputSink.#OnMnemonic(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.IKeyboardInputSink.#TranslateAccelerator(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.IKeyboardInputSink.#TranslateChar(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]

[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="d", Scope="type", Target="System.Windows.CoerceValueCallback")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="e", Scope="member", Target="System.Windows.DependencyObject.#OnPropertyChanged(System.Windows.DependencyPropertyChangedEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="e", Scope="type", Target="System.Windows.DependencyPropertyChangedEventHandler")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Int32Rect.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Int32Rect.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Point.#.ctor(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Point.#.ctor(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="d", Scope="type", Target="System.Windows.PropertyChangedCallback")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="e", Scope="type", Target="System.Windows.PropertyChangedCallback")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Rect.#.ctor(System.Double,System.Double,System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Rect.#.ctor(System.Double,System.Double,System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Rect.#Contains(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Rect.#Contains(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Vector.#.ctor(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Vector.#.ctor(System.Double,System.Double)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="hwnd", Scope="member", Target="System.Windows.Interop.MSG.#hwnd")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="m", Scope="member", Target="System.Windows.Media.Matrix.#.ctor(System.Double,System.Double,System.Double,System.Double,System.Double,System.Double)")]

[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.NameScope")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="topMost", Scope="member", Target="System.Windows.SplashScreen.#Show(System.Boolean,System.Boolean)")]

[module: SuppressMessage("Microsoft.Naming","CA1716:IdentifiersShouldNotMatchKeywords", MessageId="Error", Scope="member", Target="System.Windows.Data.DataSourceProvider.#OnQueryFinished(System.Object,System.Exception,System.Windows.Threading.DispatcherOperationCallback,System.Object)")]

[module: SuppressMessage("Microsoft.Naming","CA1709:IdentifiersShouldBeCasedCorrectly", MessageId="Pa", Scope="member", Target="System.Windows.Input.Key.#Pa1")]

[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Threading.DispatcherSynchronizationContext.#Wait(System.IntPtr[],System.Boolean,System.Int32)")]


//***************************************************************************************************************************
// Bug ID: No Bug
// Developer: pantal
// Ignoring the return is intentional in all of the cases below.  Successful ETW registration isn’t critical for the app to continue running since it’s just for diagnostic purposes.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.ClassicEtw.UnregisterTraceGuids(System.UInt64)", Scope="member", Target="MS.Utility.ClassicTraceProvider.#Finalize()")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.ClassicEtw.RegisterTraceGuidsW(MS.Win32.ClassicEtw+ControlCallback,System.IntPtr,System.Guid@,System.Int32,MS.Win32.ClassicEtw+TRACE_GUID_REGISTRATION@,System.String,System.String,System.UInt64@)", Scope="member", Target="MS.Utility.ClassicTraceProvider.#Register(System.Guid)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.ManifestEtw.EventUnregister(System.UInt64)", Scope="member", Target="MS.Utility.ManifestTraceProvider.#Finalize()")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.ManifestEtw.EventRegister(System.Guid@,MS.Win32.ManifestEtw+EtwEnableCallback,System.Void*,System.UInt64@)", Scope="member", Target="MS.Utility.ManifestTraceProvider.#Register(System.Guid)")]


//***************************************************************************************************************************
// Bug ID: No Bug
// Developer: pantal
// PackagingUtilities employs branching GC logic out of Dispose() which confuses FxCop. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.IO.Packaging.PackagingUtilities+SafeIsolatedStorageFileStream.#Dispose(System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.IO.Packaging.PackagingUtilities+ReliableIsolatedStorageFileFolder.#Dispose(System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.IO.Packaging.PackagingUtilities+ReliableIsolatedStorageFileFolder.#Dispose()")]


//***************************************************************************************************************************
// Bug ID: No Bug
// Developer: pantal
// Irregular GC.SuppressFinalize() calls in existing implementation.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Win32.NativeMethods+VARIANT.#SuppressFinalize()")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Collections.ObjectModel.WeakReadOnlyCollection`1+WeakEnumerator.#Dispose()")]



//**************************************************************************************************************************
// Bug ID: No Bug
// Developer: pantal
// Reason: Won't fix legacy items, clearing up FxCop scans.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.Interop.PROPVARIANT+NativeMethods.PropVariantClear(MS.Internal.Interop.PROPVARIANT)", Scope="member", Target="MS.Internal.Interop.PROPVARIANT.#Clear()")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Win32.HwndWrapper.#WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&)", Justification="FxCop cannot assess GC calls outside of dispose.")]
[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="resource", Target="ExceptionStringTable.resources", Justification="Hwnd is a valid name, but we can't add it to dictionary.")]

//**************************************************************************************************************************
// New since v4 RTM:
//**************************************************************************************************************************

//**************************************************************************************************************************
// The code involved has a comment describing why it creates the object but doesn't use it.  By design.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.ObservableCollectionDefaultValueFactory`1+ObservableCollectionDefaultPromoter", Scope="member", Target="MS.Internal.ObservableCollectionDefaultValueFactory`1.#CreateDefaultValue(System.Windows.DependencyObject,System.Windows.DependencyProperty)")]

//**************************************************************************************************************************
// Bug ID: No Bug
// Developer: dwaynen
// Reason: Name for GetAwaiter is required by the compiler.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Awaiter", Scope="member", Target="System.Windows.Threading.DispatcherOperation.#GetAwaiter()")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Awaiter", Scope="member", Target="System.Windows.Threading.DispatcherOperation`1.#GetAwaiter()")]
