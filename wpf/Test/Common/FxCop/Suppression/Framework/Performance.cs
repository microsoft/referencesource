//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

// Some false positives
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Animation_InvalidBaseValue():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_InvalidPermissionType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionNumberOfElementsMustBeLessOrEqualTo():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CannotRetrievePartsOfWriteOnlyContainer():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SR.get_ResourceManager():System.Resources.ResourceManager")]

// In the shared resources file
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Animation_InvalidTimeKeyTime():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Animation_InvalidAnimationUsingKeyFramesDuration():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Animation_InvalidResolvedKeyTimes():System.Windows.SRID")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1010553
// Developer: drelyea
// Reason: This is an AppDomainManager, and the behavior is by design. PresentationHost.exe sets environment variables to
// this assembly and class name. When the CLR is started, the class specified by those environment variables will be
// instantiated by reflection from the framework and used as the AppDomainManager.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="System.Windows.Interop.PresentationAppDomainManager")]

//**************************************************************************************************************************
// Bug ID: 1074773
// Developer: chandras
// Reason: This SRID is defined in shared\resources\exceptionstringtable.txt and used by other DLLs, so putting a suppression for this in PresentationFramework.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionIsFixedSize():System.Windows.SRID")]


//**************************************************************************************************************************
// Bug ID: 1112578
// Developer: chandras
// Reason: This SRID is defined in shared\resources\exceptionstringtable.txt and used by other DLLs, so putting a suppression for this in PresentationFramework.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionNumberOfElementsMustBeGreaterThanZero():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: 1077956, 1387926
// Developer: Chandras/PeterOst
// Reason: This method is used using reflection by DRT/tools, so putting a suppression for this in PresentationFramework.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlReader.Close():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlReader.get_Prefix():System.String")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlWriter.Close():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlRecordReader.Close():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1099144
// Developer: eveselov
// Reason: need these method for test automation in our DrtEditing.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.CaretElement.get_Debug_CaretElement():System.Windows.Documents.CaretElement")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.CaretElement.get_Debug_RenderScope():System.Windows.FrameworkElement")]

//**************************************************************************************************************************
// Bug ID: 1038841
// Developer: CedricD / JoeL
// Reason: these SRIDs actually are getting called by FrugalMap - the rule has mistakenly tagged these.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalMap_CannotPromoteBeyondHashtable():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalMap_TargetMapCannotHoldAllData():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: 1122576
// Developer: mingliu / kenlai
// Reason: It is used in DocumentSequence.cs, inside an #if block (PAYLOADCODECOVERAGE)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.DocumentReferenceCollectionChangeEventArgs.get_DocumentReferenceList():System.Collections.Generic.IList`1<System.Windows.Documents.DocumentReference>")]

//**************************************************************************************************************************
// Bug ID: 1062635
// Developer: PeterOst
// Reason: XamlReader.Load and the downstream methods it calls (XmlTreeBuildAssembly and ParserHooks) are called
//         by localization tool, so can't be removed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.XamlReader.Load(System.IO.Stream,System.Windows.Markup.ParserHooks,System.Windows.Markup.ParserContext):System.Object")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.XamlReader.XmlTreeBuildAssembly(System.Windows.Markup.ParserContext,System.IO.Stream,System.Windows.Markup.ParserHooks,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.TreeBuilder.set_ParserHooks(System.Windows.Markup.ParserHooks):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1113890
// Developer: CedricD / JoeL
// Reason: these SRIDs actually are getting called by FrugalList - the rule has mistakenly tagged these.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalList_CannotPromoteBeyondArray():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalList_TargetMapCannotHoldAllData():System.Windows.SRID")]


//**************************************************************************************************************************
// Bug ID: 1072898
// Developer: kenlai
// Reason: This is a false positive. The property is used in constructor initializers of the form:
//          : base(ContainerFilter.FilterClsid)
//          One such initilization occurs in ContainerFilterImple.cs.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Interop.ContainerFilter.get_FilterClsid():System.Guid")]

//**************************************************************************************************************************
// Bug ID: 1061971
// Developer: kenlai
// Reason:  this is being used in derived types.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.XamlParser.WriteConnectionId(System.Int32):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010492
// Developer: kenlai
// Reason: This is a debugging helper meant to be called at runtime (under the debugger) or through reflection.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Utility.TraceLog.WriteLog():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1079523
// Developer: kenlai
// Reason: To be used in M11    (DO NOT MOVE TO PERFORMANCE_APPROVED.CS; re-verify in M11)
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Annotations.Component.AnnotationHighlightLayer+HighlightRangeData.get_Modified():System.DateTime")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Annotations.Component.AnnotationHighlightLayer+HighlightRangeData.set_Modified(System.DateTime):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1070245
// Developer: dmitryt
// Reason:LayoutDump is completely internal component that facilitates automated test of layout. It is used during testing via Reflection.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpVisualChildren(System.Xml.XmlTextWriter,System.String,System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpLayoutTree(System.Xml.XmlTextWriter,System.String,System.Windows.UIElement):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpLayoutAndVisualTreeToString(System.String,System.Windows.Media.Visual):System.String")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpUIElement(System.Xml.XmlTextWriter,System.Windows.UIElement,System.Windows.Media.Visual,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpLayoutAndVisualTree(System.Xml.XmlTextWriter,System.String,System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpTableCalculatedMetrics(System.Xml.XmlTextWriter,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpVisual(System.Xml.XmlTextWriter,System.Windows.Media.Visual,System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.GetUIElementsFromVisual(System.Windows.Media.Visual,System.Collections.Generic.List`1<System.Windows.UIElement>):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpUIElementChildren(System.Xml.XmlTextWriter,System.String,System.Windows.Media.Visual):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpLayoutTreeToString(System.String,System.Windows.UIElement):System.String")]


//**************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: Shared resource strings
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CannotModifyReadOnlyContainer():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionElementsCanNotBeNull():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionElementsMustBeOfType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionIsFixedSize():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionNumberOfElementsShouldBeEqualTo():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Default():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Enum_Invalid():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Enumerator_CollectionChanged():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Enumerator_NotStarted():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Enumerator_ReachedEnd():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Enumerator_VerifyContext():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FileFormatException():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FileFormatExceptionWithFileName():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Freezable_CantBeFrozen():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalList_CannotPromoteBeyondArray():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalList_TargetMapCannotHoldAllData():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalMap_CannotPromoteBeyondHashtable():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_FrugalMap_TargetMapCannotHoldAllData():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_General_BadType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_General_ObjectIsReadOnly():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Image_DecoderError():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Image_HeaderError():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_InvalidArgumentType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_InvalidPropertyValueType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterCannotBeGreaterThan():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterCannotBeLessThan():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterMustBeBetween():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterMustBeGreaterThanZero():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterValueCannotBeInfinity():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterValueCannotBeNaN():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterValueCannotBeNegative():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ParameterValueMustBeGreaterThanZero():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_PropertyValueCannotBeNaN():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Rect_Empty():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_TokenizerHelperEmptyToken():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_TokenizerHelperExtraDataEncountered():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_TokenizerHelperMissingEndQuote():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_TokenizerHelperPrematureStringTermination():System.Windows.SRID")]
// 1126477
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CannotConvertStringToType():System.Windows.SRID")]


//**************************************************************************************************************************
// Bug ID: none
// Developer: kenlai
// Reason: These are resource strings used in Debug.Assert
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_NoProcessorForSelectionType():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_EmptyResourceLoaderList():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_SelectionChangeActive():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_SelectionChangeNotActive():System.Windows.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_WrongResourceLoader():System.Windows.SRID")]


//**************************************************************************************************************************
// Bug ID: none
// Developer: kenlai
// Reason: This is used in XamlSerializerUtil.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.NamespaceTable.get_Item(System.Type,System.Boolean):System.Windows.Markup.NamespaceInfo")]

//**************************************************************************************************************************
// Bug ID: none
// Developer: kenlai
// Reason: This is used in Framework\MS\Internal\ptshost\PaginatorParagraph.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Text.TextDpi.ToTextSize(System.Windows.Size):MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSVECTOR")]

//**************************************************************************************************************************
// Bug ID: none
// Developer: kenlai
// Reason: These are used in MimeObjectFactory.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.AppModel.MimeObjectFactoryAsyncResult.get_InnerAsyncResult():System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.AppModel.MimeObjectFactoryAsyncResult.get_InvokingDelegate():MS.Internal.AppModel.StreamToObjectFactoryDelegate")]

//**************************************************************************************************************************
// Bug ID: 1010456, 1417084, 1417085
// Developer: sambent / kenlai
// Reason: These are not called internally but through reflection.  Used by diagnostic tools.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.get_IsCleanupEnabled():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.set_IsCleanupEnabled(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.Cleanup():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.PrintStats():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.get_TraceAccessorTableSize():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Data.BindingOperations.set_TraceAccessorTableSize(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.DataBindEngine.set_CleanupEnabled(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.DataBindEngine.get_CleanupEnabled():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.AccessorTable.PrintStats():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.AccessorTable.set_TraceSize(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.AccessorTable.get_TraceSize():System.Boolean")]


//**************************************************************************************************************************
// Bug ID: 1128110
// Developer: kenlai
// Reason: These are used by NewLoaderSuite.cs through async function calls.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.AppModel.MimeObjectFactory.BeginGetObjectAndCloseStream(System.IO.Stream,System.String,System.Uri,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.AppModel.MimeObjectFactory.EndGetObjectAndCloseStream(System.IAsyncResult):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1048386
// Developer: sambent/kenlai
// Reason:  this method is here just to avoid the compiler error:
//          error CS0649: Warning as Error: Field '..._traceLog' is never assigned to, and will always have its default value null
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.InitializeTraceLog():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038816
// Developer: sujalp
// Reason: This one is called from loader.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Navigation.BindStream.get_Stream():System.IO.Stream")]

//**************************************************************************************************************************
// Bug ID: 1038859
// Developer: peterost
// Reason: #if !PBTCOMPILER added
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlRecord.set_Next(System.Windows.Markup.BamlRecord):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010459
// Developer: zhenbinx
// Reason: DocumentsTrace is a tracing utility for Fixed and Flow documents
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="MS.Internal.Documents.DocumentsTrace")]

//**************************************************************************************************************************
// Bug ID: 1124178
// Developer: RajatG
// Reason: This method is used in Core.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandRegistryPermission():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1073168
// Developer: CedricD / JoeL
// Reason: This is a generic class, and depending on the usage different methods will or will not used. I am adding the class
// once, instead of every method of the each class seperately.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.SingleItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.ThreeItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.SixItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.ArrayItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.FrugalObjectList")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="class", Target="MS.Utility.FrugalStructList")]

[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.SingleItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.ThreeItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.SixItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.ArrayItemList")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.FrugalObjectList")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="class", Target="MS.Utility.FrugalStructList")]

//**************************************************************************************************************************
// Bug ID: 1203385
// Developer: NithinS
// Reason: This is public (e.g. accessible from BamlRecordWriter.WriterProperty) and is called by PresentationBuildTask.dll
// as part of a build task.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.XamlContentPropertyNode.get_PropertyMember():System.Object")]


//**************************************************************************************************************************
// Bug ID: 1182159
// Developer: CedricD
// Reason: This is part of a wrapper to an unmanaged Fusion API.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="MS.Internal.Utility.InstallReference")]


//**************************************************************************************************************************
// Bug ID: 1216176
// Developer: NeilKr
// Reason: This property is used by the compiler and is difficult to factor out for the runtime case.  Suppression Recommended
//         by SRamani and NithinS
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.XamlPropertyNode.get_IsDefinitionName():System.Boolean")]


//**************************************************************************************************************************
// Bug ID: 1340925, 1340931, 1340932, 1388815, 1434241, 1434243, 1494812
// Developer: SamBent
// Reason: False positive.  The calls to GC.KeepAlive are required.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.UpdateSourceValueState(System.Int32,System.ComponentModel.ICollectionView,System.Object,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.AttachOverride(System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.TransferValue(System.Object,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.UpdateValidationError(System.Windows.Controls.ValidationError):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.AttachToContext(System.Windows.Data.BindingExpression+AttachAttempt):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.Activate(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpressionBase.UpdateValidationError(System.Windows.Controls.ValidationError):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Data.XmlBindingWorker.ProcessXmlNodeChanged(System.EventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1340930
// Developer: brianad
// Reason: The KeepAlive is being used to maintain a weak reference.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Documents.DocumentReference.get_CurrentlyLoadedDoc():System.Windows.Documents.FixedDocument")]

//**************************************************************************************************************************
// Bug ID: 1387940
// Developer: sangilj
// Reason: XamlRtfConverter.ForceParagraph require for testing tool.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.XamlRtfConverter.set_ForceParagraph(System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1387909
// Developer: olego
// Reason: THIS IS AN FXCOP BUG - it should really be fixed by FxCop team. Adding exclusion per robertwl's recommendation.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Controls.UIElementCollection.get_VisualParent():System.Windows.UIElement")]

//**************************************************************************************************************************
// Bug ID: 1387910
// Developer: olego
// Reason:LayoutDump is completely internal component that facilitates automated test of layout. It is used during testing via Reflection.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.LayoutDump.DumpLayoutTreeToFile(System.String,System.Windows.UIElement,System.String):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1607360, 1607361
// Developer: sramani
// Reason: Suppressing as per conversation with sramani
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="System.Windows.Markup.XamlFigureLengthSerializer")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="System.Windows.Markup.XamlGridLengthSerializer")]

//**************************************************************************************************************************
// Bug ID: 1500562, 1625252, 1625253, 1625254, 1625255, ...
// Developer: mikehill - This is generated helper code for diagnostic tracing (Refresh exists so that it can be called
// from the debugger).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceResourceDictionary.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceResourceDictionary.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceResourceDictionary.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceResourceDictionary.Refresh():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.Refresh():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.Refresh():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.Refresh():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.Refresh():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.get_IsEnabledOverride():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceMarkup.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceData.get_IsEnabledOverride():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TracePageFormatting.get_IsEnabledOverride():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceNameScope.get_IsEnabledOverride():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1640625
// Developer: bchapman - Function for use by derived classes.   There are no derived classes yet.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Markup.BamlOptimizedStaticResourceRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]

// Unreported violation.
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_DocumentStructureUnexpectedParameterType2():System.Windows.SRID")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

// Not removing resource strings for SP1
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_ExceptionValidationRuleValidateNotSupported():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: kurtb - This is generated helper code for diagnostic tracing (Refresh exists so that it can be called
// from the debugger).  Same as suppresions for above with bug # 1500562, 1625252, 1625253, 1625254, 1625255, ...
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.Trace(System.Diagnostics.TraceEventType,MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.TraceActivityItem(MS.Internal.AvTraceDetails):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.TraceActivityItem(MS.Internal.AvTraceDetails,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.get_IsEnabledOverride():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.TraceHwndHost.Refresh():System.Void")]


//**************************************************************************************************************************
// Bug ID: no bug
// Developer: pantal 
// This method is compiled only in chk builds, but needs to be available for compilation to consumer assert call which is not #if'ed out.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Controls.VirtualizedCellInfoCollection.#IsValidCell(System.Windows.Controls.DataGridCellInfo)")]


//**************************************************************************************************************************
// Only in DEBUG builds, these functions are for access from a debugger to dump useful state.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.TextPointer.#get_DebugId()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Documents.SpellerStatusTable.#Dump()")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Controls.PasswordTextContainer.#DumpPositionList()")]
