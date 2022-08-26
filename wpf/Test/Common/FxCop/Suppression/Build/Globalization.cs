//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 1038723
// Developer: weibz (SRamani)
// Reason: No need to generate localizable string for the xml comment's tag name. compilation perf fix also requires this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.MarkupCompiler.GenerateXmlComments(System.CodeDom.CodeTypeMember,System.String):System.Void", MessageId="System.CodeDom.CodeCommentStatement.#ctor(System.String,System.Boolean)")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

