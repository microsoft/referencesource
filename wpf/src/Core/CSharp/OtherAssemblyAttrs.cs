//
// This file specifies various assembly level attributes.
//

using System;
using MS.Internal.PresentationCore;
using System.Resources;
using System.Security;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using System.Windows.Markup;

// This netmodule doesn't use the build-system provided assembly
// attributes so we have to explicitly add these here
[assembly:CLSCompliant(true)]

[assembly:InternalsVisibleTo(BuildInfo.PresentationFramework)]
[assembly:InternalsVisibleTo(BuildInfo.ReachFramework)]
[assembly:InternalsVisibleTo(BuildInfo.SystemPrinting)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationUI)]
[assembly:InternalsVisibleTo(BuildInfo.SystemWindowsPresentation)]
[assembly:InternalsVisibleTo(BuildInfo.PresentationFrameworkSystemDrawing)]
[assembly:InternalsVisibleTo(BuildInfo.SystemWindowsControlsRibbon)]
[assembly:InternalsVisibleTo(BuildInfo.WindowsFormsIntegration)]

[assembly: AllowPartiallyTrustedCallers]
//[assembly: SecurityCritical] - applied in the main C++ project to avoid the horrible ALINK hack that slaps SecurityTreatAsSafe on everything.

// [assembly:DependencyAttribute("System,", LoadHint.Always)]
// [assembly:DependencyAttribute("WindowsBase,", LoadHint.Always)]
// [assembly:NGenAttribute(NGenHint.Eager)]

[assembly: TypeForwardedTo(typeof(System.Windows.Markup.IUriContext))]

// Namespace information for Xaml

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media.Animation")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media.Media3D")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media.Imaging")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media.Effects")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Ink")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Media.TextFormatting")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "System.Windows.Automation")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "av")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml/composite-font", "System.Windows.Media")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml", "System.Windows.Markup")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/winfx/2006/xaml", "x")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media.Animation")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media.Media3D")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media.Imaging")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media.Effects")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Media.TextFormatting")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/xps/2005/06", "System.Windows.Automation")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/xps/2005/06", "xps")]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media.Animation")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media.Media3D")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media.Imaging")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media.Effects")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Input")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Ink")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Media.TextFormatting")]
[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "System.Windows.Automation")]
[assembly:System.Windows.Markup.XmlnsPrefix    ("http://schemas.microsoft.com/netfx/2007/xaml/presentation", "wpf")]

[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media.Animation")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media.Media3D")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media.Imaging")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media.Effects")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Input")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Ink")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Media.TextFormatting")]
[assembly: System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "System.Windows.Automation")]
[assembly: System.Windows.Markup.XmlnsPrefix("http://schemas.microsoft.com/netfx/2009/xaml/presentation", "wpf")]
