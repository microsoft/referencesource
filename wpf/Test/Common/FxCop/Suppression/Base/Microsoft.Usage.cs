using System.Diagnostics.CodeAnalysis;

//***************************************************************************************************************************
// Warnings related to the No Primary Interop Assembly (NoPIA) are suppressed.  This is not relevant to WPF at the moment.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="MS.Internal.WeakEventTable+EventNameKey.#GetHashCode()")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="MS.Internal.ComponentModel.PropertyKey.#.ctor(System.Type,System.Windows.DependencyProperty)")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="System.ComponentModel.DependencyPropertyDescriptor.#GetHashCode()")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="System.Windows.AttachedPropertyBrowsableForTypeAttribute.#GetHashCode()")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="System.Windows.AttachedPropertyBrowsableWhenAttributePresentAttribute.#GetHashCode()")]
[module: SuppressMessage("Microsoft.Usage","CA2301:EmbeddableTypesInContainersRule", MessageId="DTypeFromCLRType", Scope="member", Target="System.Windows.DependencyObjectType.#FromSystemTypeRecursive(System.Type)")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="System.Windows.DependencyProperty+FromNameKey.#.ctor(System.String,System.Type)")]
[module: SuppressMessage("Microsoft.Usage","CA2303:FlagTypeGetHashCode", Scope="member", Target="System.Windows.DependencyProperty+FromNameKey.#UpdateNameKey(System.Type)")]
[module: SuppressMessage("Microsoft.Usage","CA2302:FlagServiceProviders", Scope="type", Target="System.Windows.Markup.DateTimeValueSerializerContext")]
[module: SuppressMessage("Microsoft.Usage","CA2302:FlagServiceProviders", Scope="type", Target="System.Windows.Markup.ServiceProviders")]
[module: SuppressMessage("Microsoft.Usage","CA2301:EmbeddableTypesInContainersRule", MessageId="_objDict", Scope="member", Target="System.Windows.Markup.ServiceProviders.#AddService(System.Type,System.Object)")]
[module: SuppressMessage("Microsoft.Usage","CA2301:EmbeddableTypesInContainersRule", MessageId="_objDict", Scope="member", Target="System.Windows.Markup.ServiceProviders.#GetService(System.Type)")]
