//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingContext.DrawText(System.Windows.Media.FormattedText,System.Windows.Point):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawDrawing(System.Windows.Media.Drawing):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushGuidelineSet(System.Windows.Media.GuidelineSet):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawRectangle(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Rect):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawRectangle(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Rect,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushTransform(System.Windows.Media.Transform):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushClip(System.Windows.Media.Geometry):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawGeometry(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Media.Geometry):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushGuidelineY2(System.Double,System.Double):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawImage(System.Windows.Media.ImageSource,System.Windows.Rect,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawEllipse(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Point,System.Double,System.Double):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawRoundedRectangle(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Rect,System.Windows.Media.Animation.AnimationClock,System.Double,System.Windows.Media.Animation.AnimationClock,System.Double,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawVideo(System.Windows.Media.MediaPlayer,System.Windows.Rect):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawImage(System.Windows.Media.ImageSource,System.Windows.Rect):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawVideo(System.Windows.Media.MediaPlayer,System.Windows.Rect,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawGlyphRun(System.Windows.Media.Brush,System.Windows.Media.GlyphRun):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushOpacityMask(System.Windows.Media.Brush):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawLine(System.Windows.Media.Pen,System.Windows.Point,System.Windows.Point):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushEffect(System.Windows.Media.Effects.BitmapEffect,System.Windows.Media.Effects.BitmapEffectInput):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushOpacity(System.Double):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushGuidelineY1(System.Double):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.PushOpacity(System.Double,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawLine(System.Windows.Media.Pen,System.Windows.Point,System.Windows.Media.Animation.AnimationClock,System.Windows.Point,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawEllipse(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Point,System.Windows.Media.Animation.AnimationClock,System.Double,System.Windows.Media.Animation.AnimationClock,System.Double,System.Windows.Media.Animation.AnimationClock):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Media.DrawingDrawingContext.DrawRoundedRectangle(System.Windows.Media.Brush,System.Windows.Media.Pen,System.Windows.Rect,System.Double,System.Double):System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Interop.HwndTarget.OnResize():System.Void", MessageId="MS.Internal.MediaTrace.Trace(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Media.Animation.Timeline.BuildInfo(System.Text.StringBuilder,System.Int32,System.Boolean):System.Void", MessageId="System.Double.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Media.VisualTreeUtils.AsVisualInternal(System.Windows.DependencyObject,System.Windows.Media.Visual&,System.Windows.Media.Media3D.Visual3D&):System.Boolean", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Media.VisualTreeUtils.AsNearestPointHitTestResult(System.Windows.Media.HitTestResult):System.Windows.Media.PointHitTestResult", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Input.StylusPlugIns.DynamicRendererThreadManager.Dispose(System.Boolean):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.FontCache.ClientManager.Dump():System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.FontCache.LPCMessage.ToString():System.String", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.FontCache.LPCMessage.ToString():System.String", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.FontCache.SecurityDescriptor.ToString():System.String", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.FontCache.LPCServer.PrintException(System.Int32,System.String):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
#endif

//**************************************************************************************************************************
// Bug ID: 1038724
// Developer: chaiwatp (adsmith)
// Reason: The code in question appears to be  
//	  return new COMException("", hr);
//               Yes, this could be String.Empty instead of "", but otherwise this appears to be fine.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.HRESULT.ConvertToException(System.Int32):System.Exception")]

//**************************************************************************************************************************
// Bug ID: 1038756
// Developer: ChaiwatP (dwaynen)
// Reason: The System.Windows.Input.KeyboardDevice.Validate_Key method is reporting an ArgumentException,
//                and passing the name of the argument.  This is not a localizable string, as it just refers to the name of the argument in code
//                - which will be part of our public documentation.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Input.KeyboardDevice.Validate_Key(System.Windows.Input.Key):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038743
// Developer: ChaiwatP (sangilj)
// Reason: All string are not localizable string, so resolved it as by-design.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Clipboard.IsCurrent(System.Windows.IDataObject):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1038750
// Developer: ChaiwatP (actacc)
// Reason: The accessibility team is asking for an exemption for all the FxCop violations for Automation namespaces.
// For that reason, I suggest you guys should go ahead and move the violations below to the suppression assemblies. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement,System.Windows.Automation.AutomationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RemovePatternProvider(System.Windows.UIElement,System.Windows.Automation.AutomationPattern,System.Windows.Automation.Provider.IAutomationPatternProvider,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.AddPatternProvider(System.Windows.UIElement,System.Windows.Automation.AutomationPattern,System.Windows.Automation.Provider.IAutomationPatternProvider,System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038726
// Developer: ChaiwatP (actacc)
// Reason: The accessibility team is asking for an exemption for all the FxCop violations for Automation namespaces.
// For that reason, I suggest you guys should go ahead and move the violations below to the suppression assemblies. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Automation.TextRangeProviderWrapper.MoveEndpoint(System.Windows.Automation.Text.TextPatternRangeEndpoint,System.Windows.Automation.InteropProvider.ITextRangeInteropProvider,System.Windows.Automation.Text.TextPatternRangeEndpoint):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Automation.TextProviderWrapper.RangeFromChild(System.Windows.Automation.InteropProvider.IRawElementProviderSimple):System.Windows.Automation.InteropProvider.ITextRangeInteropProvider")]

//**************************************************************************************************************************
// Bug ID: 1066050
// Developer: ChaiwatP (actperf)
// Reason: internal code for perf debugging.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.UIElement.Measure(System.Windows.Size):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1196989
// Developer: samgeo
// Reason: index is the name of the argument, it is not localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Ink.Quad.get_Item(System.Int32):System.Windows.Point")]

//**************************************************************************************************************************
// Bug ID: 1196990
// Developer: samgeo
// Reason: index is the name of the argument, it is not localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Ink.StrokeCollection.set_Item(System.Int32,System.Windows.Ink.Stroke):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1199497
// Developer: samgeo
// Reason: resolution is the name of the argument.  it is not localized
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Input.StylusPointPropertyInfo..ctor(System.Windows.Input.StylusPointProperty,System.Int32,System.Int32,System.Windows.Input.StylusPointPropertyUnit,System.Single)")]

//**************************************************************************************************************************
// Bug ID: 1199498
// Developer: samgeo
// Reason: value is the name of the argument.  it is not localized
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Input.StylusPoint.SetPropertyValue(System.Windows.Input.StylusPointProperty,System.Int32,System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1199499
// Developer: samgeo
// Reason: stylusPointProperty is the name of the argument.  it is not localized
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Input.StylusPointDescription.GetPropertyInfo(System.Guid):System.Windows.Input.StylusPointPropertyInfo")]

//**************************************************************************************************************************
// Bug ID: 1199500
// Developer: samgeo
// Reason: index is the name of the argument, it is not localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Input.StylusPlugIns.StylusPlugInCollection.RemoveAt(System.Int32):System.Void")]



//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

