//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//**************************************************************************************************************************
// Bug ID: 1131025
// Developer: peterost/varsham
// Reason: This is by-design because this flag is written out to Baml and is meant to be
//         written out as a value smaller than an int.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="MS.Internal.Markup.BamlAttributeUsage")]



//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

