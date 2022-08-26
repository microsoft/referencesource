using System.Diagnostics.CodeAnalysis;

//***************************************************************************************************************************
// Warnings related to the No Primary Interop Assembly (NoPIA) are suppressed.  This is not relevant to WPF at the moment.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA2302:FlagServiceProviders", Scope="type", Target="MS.Internal.Markup.TypeConvertContext")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="MS.Internal.Markup.XamlPropertyFullName.#GetHashCode()")]
