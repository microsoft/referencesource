//
// This file specifies various assembly level attributes.
//

using System;
using MS.Internal.WindowsBase;
using System.Resources;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using System.Security;
using System.Windows.Markup;


[assembly:DependencyAttribute("System,", LoadHint.Always)]
[assembly:DependencyAttribute("System.Xaml,", LoadHint.Sometimes)]

[assembly:InternalsVisibleTo(BuildInfo.PresentationCore)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFramework)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationUI)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkRoyale)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkLuna)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkAero)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkAero2)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkAeroLite)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkClassic)]
[assembly:InternalsVisibleTo(BuildInfo.ReachFramework)]
[assembly:InternalsVisibleTo(BuildInfo.SystemWindowsPresentation)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemCore)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemData)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemDrawing)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemXml)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemXmlLinq)]

[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ValueSerializer))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ArrayExtension))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.DateTimeValueSerializer))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.IComponentConnector))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.INameScope))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.IProvideValueTarget))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.IUriContext))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.IValueSerializerContext))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.IXamlTypeResolver))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.MarkupExtension))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.NullExtension))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.StaticExtension))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.TypeExtension))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.AmbientAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.UsableDuringInitializationAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ConstructorArgumentAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ContentPropertyAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ContentWrapperAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.DependsOnAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.DictionaryKeyPropertyAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.MarkupExtensionReturnTypeAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.NameScopePropertyAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.RootNamespaceAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.TrimSurroundingWhitespaceAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.UidPropertyAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.ValueSerializerAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.WhitespaceSignificantCollectionAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.XmlLangPropertyAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.XmlnsCompatibleWithAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.XmlnsDefinitionAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.XmlnsPrefixAttribute))]
[assembly:TypeForwardedTo(typeof(System.Windows.Markup.RuntimeNamePropertyAttribute))]

[assembly:SecurityCritical] //needed to run critical code


// XAML namespace definitions
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Diagnostics")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "av")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml", "System.Windows.Markup")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/composite-font", "System.Windows.Media")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Diagnostics")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "wpf")]

[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Input")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Diagnostics")]
[assembly: System.Windows.Markup.XmlnsPrefix("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "wpf")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/xps/2005/06", "metro")]

