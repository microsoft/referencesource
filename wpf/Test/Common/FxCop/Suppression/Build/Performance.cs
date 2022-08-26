//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: These SRID are used when compiling files in Framework\System\Windows\Markup for PBTCompiler
//**************************************************************************************************************************
// 1010496
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserBadEncoding():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserWriterNoSeekEnd():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserBamlVersion():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_InvalidStringFormat():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserFailFindType():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserCannotConvertString():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserWriterUnknownOrigin():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserNoResource():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserOwnerEventMustBePublic():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserNoDigitEnums():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserNoType():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserDefaultConverter():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_Default():MS.Utility.SRID")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserPrefixNSProperty():MS.Utility.SRID")]
// 1130325
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.SRID.get_ParserAbstractType():MS.Utility.SRID")]

//**************************************************************************************************************************
// Bug ID: 1129378
// Developer: kenlai
// Reason: These are used in FileVersion.cs
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.ContainerUtilities.CheckAgainstNull(System.Object,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.ContainerUtilities.ReadByteLengthPrefixedDWordPaddedUnicodeString(System.IO.BinaryReader,System.Int32&):System.String")]

//**************************************************************************************************************************
// Bug ID: 1182107
// Developer: NithinS
// Reason: Accordign to fmunoz this will be called by parser
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.XmlCompatibilityReader.DeclareNamespaceCompatibility(System.String,System.String):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1420767, 1420768
// Developer: Marka
// Reason: This code is shared across various modules in Avalon, so even if it's not called in PBT.dll it is called in one of the others.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.TokenizerHelper..ctor(System.String,System.Char,System.Char)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.TokenizerHelper.get_FoundSeparator():System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.TokenizerHelper.LastTokenRequired():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.TokenizerHelper.NextTokenRequired(System.Boolean):System.String")]

//**************************************************************************************************************************
// Bug ID: 1409927
// Developer: CedricD
// Reason: This is actually referenced in Debug.Assert and inside #defines, so it's likely FxCop isn't seeing it.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "MS.Internal.Markup.BamlVersionHeader.get_BinarySerializationSize():System.Int32")]


//**************************************************************************************************************************
// Bug ID: 1607359
// Developer: marka
// Reason: We're not fixing these types of issues in PBT.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.Size.set_Width(System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.Size.set_Height(System.Double):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1640625
// Developer: bchapman - Function for use by derived classes.   There are no derived classes yet.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.BamlOptimizedStaticResourceRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 832936
// Developer: jezhan - Code file is linked by both PresentationFramework and PresentationBuildTasks, but the private code is 
// 					   only used by PresentationFramework.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.XamlParseException.#.ctor(System.String,System.Int32,System.Int32,System.Uri,System.Exception)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.XamlParseException.#set_LineNumber(System.Int32)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Markup.XamlParseException.#set_LinePosition(System.Int32)")]
