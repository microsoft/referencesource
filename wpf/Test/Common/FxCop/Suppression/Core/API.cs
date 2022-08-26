//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//**************************************************************************************************************************
// Bug ID: 1010990
// Developer: SherifM
// Reason: Approved by actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int16KeyFrameCollection.Add(System.Windows.Media.Animation.Int16KeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int16KeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Int16KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int16KeyFrameCollection.Remove(System.Windows.Media.Animation.Int16KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.StringKeyFrameCollection.Remove(System.Windows.Media.Animation.StringKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.StringKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.StringKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.StringKeyFrameCollection.Add(System.Windows.Media.Animation.StringKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ByteKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.ByteKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ByteKeyFrameCollection.Remove(System.Windows.Media.Animation.ByteKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ByteKeyFrameCollection.Add(System.Windows.Media.Animation.ByteKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DoubleKeyFrameCollection.Add(System.Windows.Media.Animation.DoubleKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DoubleKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.DoubleKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DoubleKeyFrameCollection.Remove(System.Windows.Media.Animation.DoubleKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int64KeyFrameCollection.Remove(System.Windows.Media.Animation.Int64KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int64KeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Int64KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int64KeyFrameCollection.Add(System.Windows.Media.Animation.Int64KeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SizeKeyFrameCollection.Remove(System.Windows.Media.Animation.SizeKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SizeKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.SizeKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SizeKeyFrameCollection.Add(System.Windows.Media.Animation.SizeKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SingleKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.SingleKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SingleKeyFrameCollection.Remove(System.Windows.Media.Animation.SingleKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.SingleKeyFrameCollection.Add(System.Windows.Media.Animation.SingleKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Rect3DKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Rect3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Rect3DKeyFrameCollection.Add(System.Windows.Media.Animation.Rect3DKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Rect3DKeyFrameCollection.Remove(System.Windows.Media.Animation.Rect3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Point3DKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Point3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Point3DKeyFrameCollection.Add(System.Windows.Media.Animation.Point3DKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Point3DKeyFrameCollection.Remove(System.Windows.Media.Animation.Point3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Vector3DKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Vector3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Vector3DKeyFrameCollection.Add(System.Windows.Media.Animation.Vector3DKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Vector3DKeyFrameCollection.Remove(System.Windows.Media.Animation.Vector3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DecimalKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.DecimalKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DecimalKeyFrameCollection.Remove(System.Windows.Media.Animation.DecimalKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.DecimalKeyFrameCollection.Add(System.Windows.Media.Animation.DecimalKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ColorKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.ColorKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ColorKeyFrameCollection.Add(System.Windows.Media.Animation.ColorKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ColorKeyFrameCollection.Remove(System.Windows.Media.Animation.ColorKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Size3DKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Size3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Size3DKeyFrameCollection.Remove(System.Windows.Media.Animation.Size3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Size3DKeyFrameCollection.Add(System.Windows.Media.Animation.Size3DKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.VectorKeyFrameCollection.Add(System.Windows.Media.Animation.VectorKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.VectorKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.VectorKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.VectorKeyFrameCollection.Remove(System.Windows.Media.Animation.VectorKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.PointKeyFrameCollection.Add(System.Windows.Media.Animation.PointKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.PointKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.PointKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.PointKeyFrameCollection.Remove(System.Windows.Media.Animation.PointKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.MatrixKeyFrameCollection.Add(System.Windows.Media.Animation.MatrixKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.MatrixKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.MatrixKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.MatrixKeyFrameCollection.Remove(System.Windows.Media.Animation.MatrixKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.CharKeyFrameCollection.Remove(System.Windows.Media.Animation.CharKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.CharKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.CharKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.CharKeyFrameCollection.Add(System.Windows.Media.Animation.CharKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int32KeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Int32KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int32KeyFrameCollection.Remove(System.Windows.Media.Animation.Int32KeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Int32KeyFrameCollection.Add(System.Windows.Media.Animation.Int32KeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.BooleanKeyFrameCollection.Add(System.Windows.Media.Animation.BooleanKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.BooleanKeyFrameCollection.Remove(System.Windows.Media.Animation.BooleanKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.BooleanKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.BooleanKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.RectKeyFrameCollection.Add(System.Windows.Media.Animation.RectKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.RectKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.RectKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.RectKeyFrameCollection.Remove(System.Windows.Media.Animation.RectKeyFrame):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1011007
// Developer: Sherifm
// Reason: Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="type", Target="System.Windows.Media.Media3D.SpotLight")]


//**************************************************************************************************************************
// Bug ID: Multiple bugs
// Developer: Sherifm
// Reason: Avoid excessive complexity, Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.EditCommands.GetUIText(System.Windows.Input.EditCommands+CommandId):System.String")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.EditCommands.LoadDefaultGestureFromResource(System.Windows.Input.EditCommands+CommandId,System.Windows.Input.UICommand):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.EditCommands.GetPropertyName(System.Windows.Input.EditCommands+CommandId):System.String")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.ApplicationCommands.LoadDefaultGestureFromResource(System.Windows.Input.ApplicationCommands+CommandId,System.Windows.Input.UICommand):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CommandManager.OnExecuteEventHelper(System.Windows.DependencyObject,System.Windows.Input.RoutedExecuteEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.MediaCommands.LoadDefaultGestureFromResource(System.Windows.Input.MediaCommands+CommandId,System.Windows.Input.UICommand):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CommandDevice.GetRoutedCommand(System.Int32):System.Windows.Input.RoutedCommand")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.ComponentCommands.GetUIText(System.Windows.Input.ComponentCommands+CommandId):System.String")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.ComponentCommands.LoadDefaultGestureFromResource(System.Windows.Input.ComponentCommands+CommandId,System.Windows.Input.UICommand):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.ComponentCommands.GetPropertyName(System.Windows.Input.ComponentCommands+CommandId):System.String")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsc_SetupScan(StateVars*,MS.Internal.TrueType.Rect*,System.UInt16,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,RevRoots*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModHdmx(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt16,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sbit_EmboldenGray(System.Byte*,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.Int16,System.Int16):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsc_MeasureGlyph(ContourList*,GlyphBitMap*,WorkScan*,System.UInt16,System.UInt16,System.Int16,System.Int16):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.MyQSort(System.Void*,System.UInt32,System.UInt32,System.function pointer System.Int32(System.optional(IsConst) Void*,System.optional(IsConst) Void*)):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sbit_EmboldenSubPixel(System.Byte*,System.UInt16,System.UInt16,System.UInt16,System.Int16,System.Int16):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sfac_GetSbitBitmap(sfac_ClientRec*,System.UInt16,System.UInt32,System.UInt32,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.Byte*,System.UInt16*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModGlyfLocaAndHead(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt32*,System.UInt32*,System.Int32):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.CreateDeltaTTFEx(System.Byte*,System.UInt32,System.Byte**,System.UInt32*,System.UInt32*,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt16,System.UInt32*,System.UInt16,System.function pointer System.Void*(System.Void*,System.UInt32),System.function pointer System.Void(System.Void*),System.UInt32,System.Void*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sfac_ShaveSbitMetrics(sfac_ClientRec*,System.UInt16,System.UInt32,System.UInt32,System.UInt16,System.UInt16*,System.UInt16*,System.UInt16*,System.UInt16*,System.UInt16*,System.UInt16*,System.Int16*,System.Int16*,System.Int16*,System.Int16*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModCmap(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt16*,System.UInt16*,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fs_ContourScan(fs_GlyphInputType*,fs_GlyphInfoType*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModSbit(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.MakeKeepGlyphList(TTFACC_FILEBUFFERINFO*,System.UInt16,System.UInt16,System.UInt16,System.UInt32*,System.UInt16,System.Byte*,System.UInt16,System.UInt16*,System.UInt16*,System.Int32,System.Int32):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModXmtxXhea(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt16,System.UInt16,System.Int32,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fs_FindBitMapSize(fs_GlyphInputType*,fs_GlyphInfoType*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sbit_GetBitmap(sbit_State*,sfac_ClientRec*,System.Byte*,System.Byte*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.Misoriented(System.Int32,System.UInt16,System.Int16,MS.Internal.TrueType.Vector,fnt_ElementType*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsc_CalcSpline(StateVars*,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.UInt16):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.TTOAutoMap(TTFACC_FILEBUFFERINFO*,System.Byte*,System.UInt16,System.UInt16):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsc_FillGlyph(fs_GlyphInputType*,ContourList*,GlyphBitMap*,WorkScan*,System.UInt16,System.UInt16):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.scl_InitializeScaling(System.Void*,System.Int32,transMatrix*,System.UInt16,System.Int32,System.Int16,System.Int16,System.UInt16,System.UInt16,System.Int16*,System.Int16*,System.Int32,System.UInt32*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.Calc90degClosestRotationFactor(transMatrix*):System.UInt16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.WriteNameRecords(TTFACC_FILEBUFFERINFO*,namerecord*,System.UInt16,System.Int32,System.Int32,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sfac_ReadOutlineData(System.Byte*,System.Int32*,System.Int32*,sfac_GHandle*,LocalMaxProfile*,System.Int32,System.Int16,System.Int16*,System.Int16*,System.UInt16*,System.Byte**,System.UInt32*,System.UInt32*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sfac_SearchForBitmap(sfac_ClientRec*,System.UInt16,System.UInt32,System.Int32*,System.UInt16*,System.UInt16*,System.UInt32*,System.UInt16*,System.UInt32*,System.UInt32*):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsg_CheckOutlineOrientation(fnt_ElementType*):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.fsc_CalcLine(StateVars*,System.Int32,System.Int32,System.Int32,System.Int32,System.UInt16):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.ModVDMX(CONST_TTFACC_FILEBUFFERINFO*,TTFACC_FILEBUFFERINFO*,System.UInt16,System.UInt32*):System.Int16")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="<Module>.sbit_Embolden(System.Byte*,System.UInt16,System.UInt16,System.UInt16,System.Int16,System.Int16):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.SerializationHelper.ConvertToVarEnum(System.Type,System.Boolean):System.Runtime.InteropServices.VarEnum")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.ExtendedPropertySerializer.EncodeAttribute(System.Guid,System.Object,System.Runtime.InteropServices.VarEnum,System.IO.MemoryStream):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.ExtendedPropertySerializer.DecodeAttribute(System.Guid,System.IO.MemoryStream,System.Runtime.InteropServices.VarEnum&):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.StrokeCollectionSerializer.DecodeRawISF(System.Byte[]):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.DrawingAttributeSerializer.DecodeAsISF(System.IO.MemoryStream,MS.Internal.Ink.InkSerializedFormat.GuidList,System.UInt32,System.Windows.Ink.DrawingAttributes):System.UInt32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.RenderData.BaseValueDrawingContextWalk(System.Windows.Media.DrawingContextWalker):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.RenderData.DrawingContextWalk(System.Windows.Media.DrawingContextWalker):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.RenderData.MarshalToDUCE(System.Windows.Media.Composition.DUCE+Channel):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.ColorContext..ctor(System.Windows.Media.PixelFormat)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.ColorContext.FromRawBytes(System.Byte[]):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Pen..ctor(System.Windows.Media.Pen,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.KnownColors.ColorStringToKnownColor(System.String):System.Windows.Media.KnownColor")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.FlagsFromEnum(MS.Internal.PixelFormatEnum):System.Windows.Media.PixelFormatFlags")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.GetPixelFormat(MS.Internal.PixelFormatEnum):System.Windows.Media.PixelFormat")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.AdjustGuid():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.TileBrush..ctor(System.Windows.Media.TileBrush,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.TransformConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.GlyphRun..ctor(System.Windows.Media.GlyphTypeface,System.Int32,System.Boolean,System.Double,System.Collections.Generic.IList`1<System.UInt16>,System.Windows.Point,System.Collections.Generic.IList`1<System.Double>,System.Collections.Generic.IList`1<System.Windows.Point>,System.Collections.Generic.IList`1<System.Char>,System.String,System.Collections.Generic.IList`1<System.UInt16>,System.Collections.Generic.IList`1<System.Boolean>,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.RectAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.PointAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.SingleAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.VectorAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.BooleanAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.ColorAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Rect3DAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Vector3DAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Rotation3DAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Int64AnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Point3DAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Int16AnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Size3DAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.TimingEventList.WalkEventList(System.Windows.Media.Animation.TimingEventList+WalkFlags,System.Windows.Media.Animation.TimeRestart,System.Windows.Media.Animation.LocalTime,System.Windows.Media.Animation.TimingEventList&,System.Windows.Media.Animation.TimeSyncBase,System.Windows.Media.Animation.LocalTime):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.SizeAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Int32AnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.MatrixAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.DoubleAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.CharAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.DecimalAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.ByteAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.StringAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Clock.ResolveNextTimesNewInterval(System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.Clock.ResolveTimes(System.Boolean,System.Boolean):System.Windows.Media.Animation.Clock+ResolveCode")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.EllipticalNodeOperations.CutTest(MS.Internal.Ink.StrokeNodeData,MS.Internal.Ink.StrokeNodeData,MS.Internal.Ink.Quad,System.Collections.Generic.IEnumerable`1<MS.Internal.Ink.ContourSegment>):MS.Internal.Ink.StrokeFIndices")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.StrokeNodeOperations.HitTestInkContour(System.Collections.Generic.IEnumerable`1<MS.Internal.Ink.ContourSegment>,MS.Internal.Ink.Quad,MS.Internal.Ink.StrokeNodeData,MS.Internal.Ink.StrokeNodeData):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.StrokeRenderer.RenderStroke(System.Windows.Media.DrawingContext,System.Windows.Ink.Stroke,System.Guid[],System.Object[]):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Ink.ApplicationGestureHelper.IsDefined(System.Windows.Ink.ApplicationGesture):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.LayoutEngine.GetComplexLanguageList(MS.Internal.Shaping.OpenTypeTags,MS.Internal.Shaping.FontTable,System.UInt32[],System.UInt32[],System.UInt16,System.UInt16,MS.Internal.Shaping.WritingSystem[]&,System.Int32&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.LayoutEngine.ApplyLookup(MS.Internal.Shaping.IOpenTypeFont,MS.Internal.Shaping.OpenTypeTags,MS.Internal.Shaping.FontTable,MS.Internal.Shaping.LayoutMetrics,MS.Internal.Shaping.LookupTable,System.Int32,System.UInt16*,MS.Internal.Shaping.GlyphInfoList,System.Int32*,MS.Internal.Shaping.LayoutOffset*,System.Int32,System.Int32,System.UInt32,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.HebrewClusterCop.AddToCluster(System.Char*,System.UInt16,MS.Internal.Shaping.HebrewCharClass&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.ArabicFSM.ShapeArabicText(MS.Internal.Shaping.Item,System.Char*,System.Int32,MS.Internal.Shaping.GlyphInfoList,System.UInt16*,System.Windows.Media.GlyphTypeface,System.UInt32,System.Byte[]&,System.Byte[]&,MS.Internal.Shaping.Feature[]&,System.Int32&):System.Byte[]")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.HRESULT.Check(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.SpanVector.Set(System.Int32,System.Int32,System.Object,MS.Internal.Equals):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Parsers+GeometryParseState.GetNextState(System.IFormatProvider,System.String&,System.Int32&,MS.Internal.Parsers+GeometryParseState&,System.Double&,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Parsers.ParsePath(System.String,System.IFormatProvider):System.Windows.Media.PathGeometry")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.LSRun.CompileFeatureSet(MS.Internal.TextFormatting.LSRun[],System.Int32*):MS.Internal.Shaping.FeatureSet")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.SimpleTextLine.Create(MS.Internal.TextFormatting.FormatSettings,System.Int32,System.Int32):System.Windows.Media.TextFormatting.TextLine")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.BidiAnalyzeInternal(MS.Internal.CharacterBuffer,System.Int32,System.Int32,System.Int32,MS.Internal.TextFormatting.Bidi+Flags,MS.Internal.TextFormatting.Bidi+State,System.Byte[]&,MS.Internal.DirectionClass[]&,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextMetrics+FullTextLine.GetTextBounds(System.Int32,System.Int32):System.Collections.IList")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.DataObject.GetDataIntoOleStructs(MS.Win32.NativeMethods+FORMATETC,MS.Win32.NativeMethods+STGMEDIUM):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Interop.HwndMouseInputProvider.FilterMessage(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Interop.HwndStylusInputProvider.FilterMessage(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.StylusLogic.PromoteMouseToStylus(System.Windows.Input.PreProcessInputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Imaging.CachedBitmap..ctor(System.Int32,System.Int32,System.Double,System.Double,System.Windows.Media.PixelFormat,System.Windows.Media.Imaging.BitmapPalette,System.Array,System.Int32)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Imaging.BitmapImage..ctor(System.Windows.Media.Imaging.BitmapImage,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.IncrementalLassoHitTester.HitTestStroke(MS.Internal.Ink.StrokeInfo,MS.Internal.Ink.Lasso,System.Windows.Ink.StrokeCollection&,System.Windows.Ink.StrokeCollection&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Imaging.BitmapImage.FinalizeCreation():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.ExtendedPropertySerializer.Validate(System.Guid,System.Object):System.Void")]

//**************************************************************************************************************************
// Bug ID: multiple
// Developer: Sherifm
// Reason: EnumStorageShouldBeInt32, Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Input.KeyState")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Input.MouseAction")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Media.Animation.PathAnimationSource")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Media.Animation.KeyTimeType")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Media.Animation.AnimationType")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Visibility")]


//**************************************************************************************************************************
// Bug ID: multiple, 1182081
// Developer: Sherifm
// Reason: System.Windows.Automation is excluded, Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.BeginInvoke(System.Windows.UIElement,System.Windows.DependencyProperty,System.Object&,System.Object&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.BeginInvoke(System.Windows.UIElement,System.Windows.DependencyProperty,System.Object&,System.Object&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.EndInvoke(System.Object&,System.Object&,System.IAsyncResult):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.EndInvoke(System.Object&,System.Object&,System.IAsyncResult):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.Invoke(System.Windows.UIElement,System.Windows.DependencyProperty,System.Object&,System.Object&):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Automation.Provider.AutomationEventAssociationCallback.Invoke(System.Windows.UIElement,System.Windows.DependencyProperty,System.Object&,System.Object&):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.Activated")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.AddPatternProvider(System.Windows.UIElement,System.Windows.Automation.AutomationPattern,System.Windows.Automation.Provider.IAutomationPatternProvider,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.AutomationElementFromUIElement(System.Windows.UIElement):System.Windows.Automation.AutomationElement")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.GetKeyboardHelpURI(System.Windows.DependencyObject):System.String")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.HandleWMGetObject(System.IntPtr,System.IntPtr,System.Windows.Media.Visual,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.KeyboardHelpURIProperty")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAsyncContentLoadedEvent(System.Windows.UIElement,System.Windows.Automation.AsyncContentLoadedState,System.Int64,System.Int64):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAsyncContentLoadedEvent(System.Windows.UIElement,System.Windows.Automation.AsyncContentLoadedState,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement,System.Windows.Automation.AutomationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationPropertyChangedEvent(System.Windows.UIElement,System.Windows.Automation.AutomationProperty,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseStructureChangedEvent(System.Windows.UIElement,System.Windows.UIElement,System.Windows.Automation.StructureChangeType):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RemovePatternProvider(System.Windows.UIElement,System.Windows.Automation.AutomationPattern,System.Windows.Automation.Provider.IAutomationPatternProvider,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.SetKeyboardHelpURI(System.Windows.DependencyObject,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Scope="member", Target="System.Windows.Automation.Provider.ITextRangeProvider.Children")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Automation.Provider.IWindowProvider.IsTopMost")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Provider.IWindowProvider.Maximizable")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Provider.IWindowProvider.Minimizable")]

//**************************************************************************************************************************
// Bug ID: multiple
// Developer: Sherifm
// Reason: Color stuff, Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Brushes.Cornsilk")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Brushes.Gainsboro")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Colors.Cornsilk")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Colors.Gainsboro")]


//**************************************************************************************************************************
// Bug ID: 1011003
// Developer: Sherifm
// Reason: FxCop tool bug, approved by PMVT
//
// GSchneid: The FxCop tool does not understand the pack 1 concept which indicates to the compiler to generate code that deals with alignment 
// issues on various platforms. Technically in Arrowhead we do not need to have unaligned structures anymore since we do not support
// TS cross platform any longer.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_SOLIDCOLORBRUSH")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_POINTLIGHT")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_DIRECTIONALLIGHT")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_AMBIENTLIGHT")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_SCREENSPACELINES3D")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_SPOTLIGHT")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE", MessageId="Origin")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="ManagedBounds", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="MuSize", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="X", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="Y", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="_height", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="_width", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="_x", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="_y", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_GLYPHRUN_CREATE")]


//**************************************************************************************************************************
// Bug ID: 1010967 & 1021095
// Developer: Sherifm
// Reason: FxCop tool bug, approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LinearGradientBrush.EndPointProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LineGeometry.EndPointProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LinearGradientBrush..ctor(System.Windows.Media.Color,System.Windows.Media.Color,System.Windows.Point,System.Windows.Point)")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LineGeometry..ctor(System.Windows.Point,System.Windows.Point)")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LineGeometry..ctor(System.Windows.Point,System.Windows.Point,System.Windows.Media.Transform)")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LinearGradientBrush.EndPoint")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LineGeometry.EndPoint")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.Brushes.SeaShell")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.Colors.SeaShell")]

//**************************************************************************************************************************
// Bug ID: 1010929
// Developer: Sherifm
// Reason: Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="PreProcess", Scope="member", Target="e:System.Windows.Input.InputManager.#PreProcessInput")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="type", Target="System.Windows.Input.PreProcessInputEventArgs")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="type", Target="System.Windows.Input.PreProcessInputEventHandler")]


//**************************************************************************************************************************
// Bug ID:
// Developer: Sherifm
// Reason: Approved by PMVT
// NEED dictionary modifications
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.GlyphRun.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.TextDecorationUnit.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.TextFormatting.TextRunProperties.FontHintingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextRunProperties.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextRunProperties.FontHintingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.GlyphRun.BidiLevel")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.GlyphRun.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.TextDecorationUnit.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.TextFormatting.TextRunProperties.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.FormattedText.SetFontSize(System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.GlyphTypeface.GetGlyphOutline(System.UInt16,System.Double,System.Double):System.Windows.Media.Geometry")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.GlyphTypeface.GetGlyphOutline(System.UInt16,System.Double,System.Double):System.Windows.Media.Geometry")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.GlyphTypeface.GetGlyphOutline(System.UInt16,System.Double,System.Double):System.Windows.Media.Geometry")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.GlyphTypeface.GetGlyphOutline(System.UInt16,System.Double,System.Double):System.Windows.Media.Geometry")]


//**************************************************************************************************************************
// Bug ID: 1010942
// Developer: Sherifm
// Reason: Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Input.Cursors.SizeNESW")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Input.Cursors.SizeNWSE")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Input.CursorType.SizeNESW")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Input.CursorType.SizeNWSE")]


//**************************************************************************************************************************
// Bug ID: 1081374
// Developer: Sherifm
// Reason: Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.FontWeights.ExtraBold")]


//**************************************************************************************************************************
// Bug ID: 1038649
// Developer: Sherifm
// Reason: Approved by PMVT
// Need dictionary update.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.Viewbox")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.ViewboxProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.Viewport")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.ViewportProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.ViewportUnits")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TileBrush.ViewportUnitsProperty")]

//**************************************************************************************************************************
// Bug ID: 1061870
// Developer: timothyc
// Reason: This is by-design.  All Freezable object properties, including collections, are animatable and modifiable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.DrawingGroup.Children")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GeometryGroup.Children")]

//**************************************************************************************************************************
// Bug ID: 1081884
// Developer: chandras
// Reason: This is by-design as the intention is to Scroll By one line. Byline is a word which has no relevance here.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Input.ComponentCommands.ScrollByLine")]

//**************************************************************************************************************************
// Bug ID: 1061874
// Developer: MCalkins
// Reason: Base type is not appropriate, API requires specific derived type for parameters.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrameCollection.Remove(System.Windows.Media.Animation.Rotation3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.Rotation3DKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrameCollection.Add(System.Windows.Media.Animation.Rotation3DKeyFrame):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1080736, 1080464
// Developer: MCalkins
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.StringAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.StringAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.MatrixAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.MatrixAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int64AnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int64AnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int32AnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int32AnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int16AnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.Int16AnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.CharAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.CharAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.BooleanAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.BooleanAnimationUsingKeyFrames.KeyFrames")]

//*******************************************************************************************************************
// Bug ID: 1086744
// Developer: SherifM
// Reason: Waiting for approval from actprog.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_COMPTARGET_HWND")]
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_COMPTARGET_HWND")]

//*******************************************************************************************************************
// Bug ID: 1080721 & 1080449
// Developer: SherifM
// Reason: Waiting for approval from actprog.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Ink.ApplicationGesture.DownRightLong")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Ink.ApplicationGesture.DownRight")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Ink.ApplicationGesture.UpRightLong")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Ink.ApplicationGesture.UpRight")]

//*******************************************************************************************************************
// Bug ID: 1080388, 1080258, 1080738, 1080466, 1080114, 1080180, 1079511, 1011013
// Developer: Danwo
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.Transform3DGroup.Children")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.MaterialGroup.Children")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.MeshGeometry3D.TriangleIndices")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.MeshGeometry3D.Normals")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.MeshGeometry3D.Positions")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.MeshGeometry3D.TextureCoordinates")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.ScreenSpaceLines3D.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.Model3DGroup.Children")]

//**************************************************************************************************************************
// Bug ID: 1106757
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way for better readability.
//         There is a bug in FxCop SuppressMessage Copy code. When SuppressMessage strings are copied for these violations
//         it copied them without mentioning parameter name or number as below. This way we cannot distinguish the parameters.
//	   I am putting the copied text here for all the  violations listed in the bug as they all need to be suppressed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="member", Target="System.Windows.Interop.HwndSource.#AcquireHwndFocusInMenuMode")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="member", Target="System.Windows.Interop.HwndSource.#DefaultAcquireHwndFocusInMenuMode")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#AcquireHwndFocusInMenuMode")]

//***************************************************************************************************************************
// Bug ID: 1111084
// Developer: sergeym
// Reason: NlcKanji is correct name. NLC Kanji is a ideograph forms standard in Japan. NLC acronim had been changed to
//         capitalized form recently as FxCop bug 939314 fix.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.FontEastAsianLanguage.NlcKanji")]

//*******************************************************************************************************************
// Bug ID: 1106764
// Developer: yutakas
// Reason: This is expected after fixing 1103017. These values needs to be able to be used with OR. AND.
// Status: approved, but weird...but not worth fixing (pmvt)
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2217:DoNotMarkEnumsWithFlags", Scope="type", Target="System.Windows.Input.ImeConversionModeValues")]
[module: SuppressMessage("Microsoft.Usage", "CA2217:DoNotMarkEnumsWithFlags", Scope="type", Target="System.Windows.Input.ImeSentenceModeValues")]

//*******************************************************************************************************************
// Bug ID: 1106758
// Developer: bchapman
// Reason: opened separate bug to move hwndhost to s.w.interop (pmvt)
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1070218
// Developer: xiaotu
// Reason: These are named correctly or represent legacy terms
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Ink.StrokeCollection..ctor(System.Byte[])")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Ink.StrokeCollection..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Ink.StrokeHitChangedEventArgs.UnhitStrokes")]

//**************************************************************************************************************************
// Bug ID: 1129686
// Developer: MLeonov
// Reason: This is a false positive. Although "topside" exists as a separate word, it has nothing to do with sidebearings.
// This was confirmed with Michael Fanning from FxCop team and DBrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Media.GlyphTypeface.TopSideBearings")]

//**************************************************************************************************************************
// Bug ID: 1086741
// Developer: cedricd
// Reason: This structure goes straight into unmanaged code and cannot be changed.  I've verfied it is always 64bit aligned
//         so this rule appears bogus.  I have not had the time to debug the rule to verify why it flagged this as a problem
//         so I've assigned M11 bug 1132271 to myself.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="MS.Utility.TraceProvider+BaseEvent")]

//**************************************************************************************************************************
// Bug ID: 1010740
// Developer: SherifM
// Reason: Approved by ActProg
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2217:DoNotMarkEnumsWithFlags", Scope="type", Target="System.Windows.DragDropEffects")]

//**************************************************************************************************************************
// Bug ID: 1010750
// Developer: SherifM
// Reason: Approved by ActProg
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.FontEastAsianLanguage.HojoKanji")]

//**************************************************************************************************************************
// Bug ID: 1010970
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1027:MarkEnumsWithFlags", Scope="type", Target="System.Windows.Media.HitTestFilterBehavior")]

//**************************************************************************************************************************
// Bug ID: 1010996
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]

//**************************************************************************************************************************
// Bug ID: 1010736
// Developer: a-shsolk
// Reason: Closed by bchapman; Resolved as Won't Fix by bchapman
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.PresentationSource.FromVisual(System.Windows.Media.Visual):System.Windows.PresentationSource")]

//**************************************************************************************************************************
// Bug ID: 1080735
// Developer: a-shsolk
// Reason: Resolved as By Design by adsmith; Closed by tyjones
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.TransformGroup.Children")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.DashStyle.Dashes")]

//**************************************************************************************************************************
// Bug ID: 1081373
// Developer: a-shsolk
// Reason: resolved by design, violation on 'demi'; Demi is a valid prefix; needing an FxCop dictionary fix.
// Status: little weird, but ok given that you have Extra, Demi, Semi, etc...
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.FontWeights.DemiBold")]

//**************************************************************************************************************************
// Bug ID: 1038650
// Developer: Sherifm
// Reason: See bug
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.VisualCollection+Enumerator")]

//**************************************************************************************************************************
// Developer: RRelyea
// Reason: bulk exclude of warning level AvoidExcessiveComplexity
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.StylusLogic.PreProcessInput(System.Object,System.Windows.Input.PreProcessInputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CursorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Visual.RenderRecursive(System.Windows.Media.RenderContext):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Parsers.ParseTransform(System.String,System.IFormatProvider):System.Windows.Media.Transform")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Parsers.ParsePathFigureCollection(System.String,System.Int32,System.IFormatProvider):System.Windows.Media.PathFigureCollection")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.LayoutEngine.ApplyLookup(MS.Internal.Shaping.IOpenTypeFont,MS.Internal.Shaping.OpenTypeTags,MS.Internal.Shaping.FontTable,MS.Internal.Shaping.LayoutMetrics,MS.Internal.Shaping.LookupTable,System.Int32,System.UInt16*,MS.Internal.Shaping.GlyphInfoList,System.Int32*,MS.Internal.Shaping.LayoutOffset*,System.Int32,System.Int32,System.UInt32,System.Int32,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Shaping.OpenTypeLayoutCache.CreateTableCache(MS.Internal.Shaping.IOpenTypeFont,MS.Internal.Shaping.OpenTypeTags,System.Int32,System.Int32&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextStore.GetTextChunk(System.Int32,System.Int32,System.Int32,System.Boolean,System.Int32,System.Int32&,System.Int32&,System.Globalization.CultureInfo&,MS.Internal.SpanVector&):System.Char[]")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextMetrics+FullTextLine.GetTextBounds(System.Int32,System.Int32):System.Collections.Generic.IList`1<System.Windows.Media.TextFormatting.TextBounds>")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Interop.HwndStylusInputProvider.FilterMessage(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1112577
// Developer: ABaioura
// Reason: MediaContext class is internal and its lifetime is not explicitly controlled by the user.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2216:DisposableTypesShouldDeclareFinalizer", Scope = "member", Target = "System.Windows.Media.MediaContext.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1233096
// Developer: dwaynen
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsHitTestVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsKeyboardFocusWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsHitTestVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsKeyboardFocusWithinChanged")]

//**************************************************************************************************************************
// Bug IDs: 1240383, 1319516, 1319517
// Developer: mikhaill
// Reason: Concluded to be "By Design" by adsmith; See dup bug #1080735
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.GuidelineSet.GuidelinesX")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.GuidelineSet.GuidelinesY")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Visual.VisualXSnappingGuidelines")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Visual.VisualYSnappingGuidelines")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.ContainerVisual.XSnappingGuidelines")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.ContainerVisual.YSnappingGuidelines")]

//**************************************************************************************************************************
// Bug ID: 1286120
// Developer: danlehen
// Reason: General DependencyObjects do not support cloning by design.  This is a Freezable specific service.
//***************************************************************************************************************************
[module: SuppressMessage("System.Windows", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Freezable.CloneCore(System.Windows.Freezable):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1678387
// Developer: alikk
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Vxxxx")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Input.StylusPointPropertyInfo.AreCompatible(System.Windows.Input.StylusPointPropertyInfo,System.Windows.Input.StylusPointPropertyInfo):System.Boolean")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextCharacters..ctor(System.Windows.Media.TextFormatting.CharacterBufferReference,System.Int32,System.Windows.Media.TextFormatting.TextRunProperties)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextEndOfLine..ctor(System.Int32,System.Windows.Media.TextFormatting.TextRunProperties)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.TextFormatting.TextFormatterImp.VerifyTextFormattingArguments(System.Windows.Media.TextFormatting.TextSource,System.Int32,System.Double,System.Windows.Media.TextFormatting.TextParagraphProperties,System.Windows.Media.TextFormatting.TextRunCache):System.Void")]
#endif

//**************************************************************************************************************************
// Bug ID: 1070229
// Developer: xiaotu
// Reason: This is by-design. In addition, the whole class Renderer will go internal in B2
// Status: bug is linked to task to remove class in m11.  don't exclude. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Ink.Renderer.Strokes")]

//**************************************************************************************************************************
// Bug ID: 1080723
// Developer: xiaotu
// Reason: This is by-design.  The reference type 'System.Windows.RoutedEvent' is immutable, so no fix is needed.
// Status: we need to teach fxcop that event is immutable. don't exclude...teach fxcop about eventid. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.StylusButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.Stylus.PreviewStylusButtonDownEvent")]


//**************************************************************************************************************************
// Bug ID: 1124174
// Developer: rajatg
// Reason: Need dictionary update for Rle, Png, Lzw (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Imaging.PngBitmapDecoder")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Imaging.PngBitmapEncoder")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Imaging.PngInterlaceOption")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Imaging.TiffCompressOption.Lzw")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Imaging.TiffCompressOption.Rle")]


//**************************************************************************************************************************
// Bug ID: 1282788 1282789
// Developer: oliverfo
// Reason: Need dictionary update for Wmp
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Imaging.WmpBitmapDecoder")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Imaging.WmpBitmapEncoder")]

//**************************************************************************************************************************
// Bug ID: 1110967
// Developer: garyyang
// Reason:"emSize" is a wellknown text term. It is not in the fxcop dictionary.
// Status: Shakeel is planning on adding em to the dictionary...then we can remove this. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.FormattedText..ctor(System.String,System.Globalization.CultureInfo,System.Windows.FlowDirection,System.Windows.Media.Typeface,System.Double,System.Windows.Media.Brush)")]


//**************************************************************************************************************************
// Bug ID: 1080707
// Developer: SherifM
// Reason: Waiting for approval from actprog.
// Status: we need to teach fxcop that event is immutable. don't exclude...teach fxcop about eventid. (pmvt)
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.TextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewGotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewGiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewMouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.LostStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewQueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.QueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.GotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewLostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewDragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.KeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.DragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.MouseLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.StylusLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewKeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.KeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewKeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewStylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.PreviewTextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.ContentElement.QueryCursorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.TextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.KeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewLostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewGiveFeedbackEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewKeyUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewQueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusInAirMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusInRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.MouseLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusOutOfRangeEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.KeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewKeyDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.QueryCursorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusSystemGestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseWheelEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostStylusCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotMouseCaptureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewGotKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DropEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewTextInputEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.LostKeyboardFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewMouseMoveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragEnterEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.DragLeaveEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewDragOverEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.QueryContinueDragEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.PreviewStylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.StylusUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.UIElement.GotStylusCaptureEvent")]

//**************************************************************************************************************************
// Bug ID: 1010667
// Developer: SherifM
// Reason: Waiting for approval from actprog.
// Status: don't exclude individual violations of this rule...exclude the whole rule. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.ResolveNeutralAndWeak(MS.Internal.DirectionClass[],System.Int32,System.Int32,MS.Internal.DirectionClass,MS.Internal.DirectionClass,System.Byte,MS.Internal.TextFormatting.Bidi+State,MS.Internal.TextFormatting.Bidi+State,System.Boolean):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1010976
// Developer: RobertWl
// Reason: Words are not in the fxcop dictionary (Imaging)
// Status: look ok for now...should discuss with devdiv dictionary folks best way to suppress. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormat.IsSRgb")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormat.IsCmyk")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormat.IsScRgb")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormatChannelDescription.Bgr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormatChannelDescription.Rgb")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormatChannelDescription.Argb")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.PixelFormatChannelDescription.Cmyk")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.FromArgb(System.Byte,System.Byte,System.Byte,System.Byte):System.Windows.Media.Color")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.FromRgb(System.Byte,System.Byte,System.Byte):System.Windows.Media.Color")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.FromScRgb(System.Single,System.Single,System.Single,System.Single):System.Windows.Media.Color")]


//*******************************************************************************************************************
// Bug ID: 1010927
// Developer: SherifM
// Reason: Waiting for approval from actprog.
// Status: need exclusion of this entire rule...not just these exclusions (pmvt)
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.InputScope.IsValidInputScopeName(System.Windows.Input.InputScopeName):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.TextCompositionManager.PostProcessInput(System.Object,System.Windows.Input.ProcessInputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.MouseDevice.PostProcessInput(System.Object,System.Windows.Input.ProcessInputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.MouseDevice.PreNotifyInput(System.Object,System.Windows.Input.NotifyInputEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.MouseDevice.PreProcessInput(System.Object,System.Windows.Input.PreProcessInputEventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1010976
// Developer: AHodsdon
// Reason: Words are not in the fxcop dictionary (Imaging)
// Status: wrong bug #
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.MediaClock.MediaErrored")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.MediaData.MediaErrored")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type",   Target = "System.Windows.Media.MediaErroredEventHandler")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type",   Target = "System.Windows.Media.MediaErroredEventArgs")]

//**************************************************************************************************************************
// Bug ID: 1124174, 1097193
// Developer: AHodsdon
// Reason: Passed architect review.
// Status:
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1724:TypeNamesShouldNotMatchNamespaces", Scope = "member", Target ="System.Windows.Interop.Imaging")]
[module: SuppressMessage("Microsoft.Naming", "CA1724:TypeNamesShouldNotMatchNamespaces", Scope="type", Target="System.Windows.Interop.Imaging")]


//**************************************************************************************************************************
// Bug ID: 1129968
// Developer: sergeym
// Reason: Unicase is a standard name for one of glyph capitallzing modes.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.FontCapitals.Unicase")]

//**************************************************************************************************************************
// Bug ID: 1048332, 1048337
// Developer: RajatG
//
// Reason: Words are not in the fxcop dictionary (Imaging)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.ColorInterpolationMode.ScRgbLinearInterpolation")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.ColorInterpolationMode.ScRgbLinearInterpolation")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.ColorInterpolationMode.SRgbLinearInterpolation")]


//**************************************************************************************************************************
// Bug ID: 1106763
// Developer: SherifM
// Status: need to pick up converstaion with Kryz...in m11.
// Reason: As per actprog request.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.MSInternal", "CA904:DeclareTypesInMicrosoftOrSystemNamespace", Scope="namespace", Target="MS.Win32.PresentationCore")]

//**************************************************************************************************************************
// Bug ID: 1010938
// Developer: a-shsolk
// Reason: Resolved as Fixed (for remaining violations) by yutakan but for this target ytakan requested an exclusion; Closed by ryosukem
// Status: should rediscuss in m11...seems questionable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Input.InputScopeName.Srgs")]

//**************************************************************************************************************************
// Bug ID: 1010935
// Developer: yutakan
// Reason: These names are designed to be compatible with win32 state for discoverbility
// Status: should revisit in m11.  this is questionable. (pmvt)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1714:FlagsEnumsShouldHavePluralNames", Scope="type", Target="System.Windows.Input.KeyState")]

//**************************************************************************************************************************
// Bug ID: 1038641
// Developer: Sherifm
// Reason: See bug
// Status: should rediscuss in m11...seems questionable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.InputScope.SrgsMarkup")]

//**************************************************************************************************************************
// Bug ID: 1080287
// Developer: MKarr
// Reason: Planned Breaking Change to UI Automation includes this fix, so suppress for now
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.GetKeyboardHelpURI")]
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.SetKeyboardHelpURI")]


//**************************************************************************************************************************
// Bug ID: No ID
// Developer: RajatG MStokes SherifM
// Reason: Color naming stuff, agreed on by MStokes.
// Status: confirm with gregsc in m11.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.Color.ScR")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.Color.FromScRgb(System.Single,System.Single,System.Single,System.Single):System.Windows.Media.Color")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.Color.ScB")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.Color.ScG")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Media.Color.ScA")]

//**************************************************************************************************************************
// Bug ID: 1010160 1010163).
// Developer: RRelyea
// Reason: m11 will need tweaks.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Geometry..ctor(System.Windows.Media.Geometry,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.MediaData..ctor(System.Uri)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.MediaData..ctor(System.IO.Stream)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.MediaData..ctor(System.Windows.Media.MediaData,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.GradientBrush..ctor(System.Windows.Media.GradientStopCollection)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.GradientBrush..ctor(System.Windows.Media.GradientBrush,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.PathSegment..ctor(System.Windows.Media.PathSegment,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.TileBrush..ctor(System.Windows.Media.TileBrush,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Brush..ctor(System.Windows.Media.Brush,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Media3D.PointLight..ctor(System.Windows.Media.Color,System.Windows.Media.Media3D.Point3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Media3D.PointLight..ctor(System.Windows.Media.Media3D.PointLight,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Media3D.ProjectionCamera..ctor(System.Windows.Media.Media3D.ProjectionCamera,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Media3D.Model3D..ctor(System.Windows.Media.Media3D.Model3D,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Media3D.Light..ctor(System.Windows.Media.Media3D.Light,System.Windows.Media.Animation.Animatable+CloneType)")]

//*******************************************************************************************************************
// Bug ID: no bug
// Developer: ahodsdon
// Reason: MediaEventsHelperStruct does not own the unmanaged resource it holds on to.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1049:TypesThatOwnNativeResourcesShouldBeDisposable", Scope="type", Target="System.Windows.Media.MediaEventsHelperStruct")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Media.MediaEventsHelperStruct.UnmanagedProxy")]

//*******************************************************************************************************************
// Bug ID: no bug
// Developer: bkaneva
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.Effects.BitmapEffectGroup.Children")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GeneralTransformGroup.Children")]

//**************************************************************************************************************************
// Bug ID: 1199488
// Developer: samgeo
// Reason: We searched msdn for HIMETRIC, and found a precedent HiMetric name.  This casing was also suggested by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member",
Target="System.Windows.Input.StylusPointCollection.ToHiMetricArray():System.Int32[]")]

//**************************************************************************************************************************
// Bug ID: 1199492
// Developer: samgeo
// Reason: It is a key scenario to be able to replace the StylusPointCollection on a Stroke.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member",
Target="System.Windows.Ink.Stroke.StylusPoints")]

//**************************************************************************************************************************
// Bug ID: 1199491
// Developer: samgeo
// Reason: It is too risky for us to destabilize this by refactoring.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member",
Target="MS.Internal.Ink.InkSerializedFormat.StrokeSerializer.DecodeISFIntoStroke(MS.Internal.Ink.InkSerializedFormat.Compressor,System.IO.MemoryStream,System.UInt32,MS.Internal.Ink.InkSerializedFormat.GuidList,MS.Internal.Ink.InkSerializedFormat.StrokeDescriptor,System.Windows.Input.StylusPointDescription,System.Windows.Media.Matrix,System.Windows.Input.StylusPointCollection&,System.Windows.Ink.ExtendedPropertyCollection&):System.UInt32")]

//**************************************************************************************************************************
// Bug ID: 1199487
// Developer: samgeo
// Reason: stylusPointPropertyInfos is not hungarian, it is a plural of the type StylusPointPropertyInfo.  this is the correct spelling
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member",
Target="System.Windows.Input.StylusPointDescription..ctor(System.Collections.Generic.IEnumerable`1<System.Windows.Input.StylusPointPropertyInfo>)")]

//**************************************************************************************************************************
// Bug ID: 1182078
// Developer: samgeo
// Reason: It is too risky for us to destabilize this by refactoring.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member",
Target="System.Windows.Ink.IncrementalLassoHitTester.InternalAddPoints(System.Windows.Point[]):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1199489
// Developer: samgeo
// Reason: the reference type 'System.Windows.Input.StylusPointProperty' is immutable
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TipButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Height")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.YTiltOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Width")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.PitchRotation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.NormalPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.BarrelButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.X")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Y")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.Z")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.RollRotation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.AzimuthOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TangentPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.SecondaryTipButton")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.PacketStatus")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.XTiltOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.AltitudeOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.TwistOrientation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.ButtonPressure")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.SerialNumber")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.YawRotation")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.StylusPointProperties.SystemTouch")]

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: bkaneva
// Reason: See bug 1038650
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.GeneralTransformCollection+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1191897
// Developer: bkaneva
// Reason: See bug 1038650
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Effects.BitmapEffectCollection+Enumerator")]


//*******************************************************************************************************************
// Bug ID: 1196962
// Developer: jdmack
// Reason: Waiting for approval from actprog.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Input.CommandManager.TranslateInput(System.Windows.IInputElement,System.Windows.Input.InputEventArgs):System.Void")]

//*******************************************************************************************************************
// Bug ID: 1219910
// Developer: jdmack
// Reason: Waiting for approval from actprog.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Input.CursorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1196956
// Developer: jdmack
// Reason: the reference types
//  'System.Windows.ContentElement.PreviewMouseRightButtonDownEvent'
//  'System.Windows.ContentElement.PreviewMouseRightButtonUpEvent'
//  'System.Windows.ContentElement.PreviewMouseLeftButtonDownEvent'
//  'System.Windows.ContentElement.PreviewMouseLeftButtonUpEvent'
//  'System.Windows.ContentElement.MouseRightButtonDownEvent'
//  'System.Windows.ContentElement.MouseRightButtonUpEvent'
//  'System.Windows.ContentElement.MouseLeftButtonDownEvent'
//  'System.Windows.ContentElement.MouseLeftButtonUpEvent'
//      are immutable
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.PreviewMouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.PreviewMouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.PreviewMouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.PreviewMouseLeftButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.MouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.MouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.MouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.ContentElement.MouseLeftButtonUpEvent")]

//**************************************************************************************************************************
// Bug ID: 1196957
// Developer: jdmack
// Reason: the reference types
//  'System.Windows.UIElement.PreviewMouseRightButtonDownEvent'
//  'System.Windows.UIElement.PreviewMouseRightButtonUpEvent'
//  'System.Windows.UIElement.PreviewMouseLeftButtonDownEvent'
//  'System.Windows.UIElement.PreviewMouseLeftButtonUpEvent'
//  'System.Windows.UIElement.MouseRightButtonDownEvent'
//  'System.Windows.UIElement.MouseRightButtonUpEvent'
//  'System.Windows.UIElement.MouseLeftButtonDownEvent'
//  'System.Windows.UIElement.MouseLeftButtonUpEvent'
//      are immutable
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.PreviewMouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.PreviewMouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.PreviewMouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.PreviewMouseLeftButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.MouseRightButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.MouseRightButtonUpEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.MouseLeftButtonDownEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.UIElement.MouseLeftButtonUpEvent")]

//**************************************************************************************************************************
// Bug ID: 1191889, 1191890, 1191891, 1191892, 1191893, 1191894, 1191895, 1191896, 1191898, 1191899, 1191900
// Developer: adsmith
// Reason: These types are not intended to be compared as a usage scenario, so the default, reflection-based comparison is sufficient.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.GeometryCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.TransformCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.PathFigureCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.DrawingCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.PointCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.DoubleCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.GradientStopCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.TextEffectCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.PathSegmentCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.VectorCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.IntegerCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.FreezableCollection`1+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1191902, 1191903, 1191904, 1191905, 1191906
// Developer: adsmith
// Reason: These types are not intended to be compared as a usage scenario, so the default, reflection-based comparison is sufficient.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.Vector3DCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.Model3DCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.Point3DCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.MaterialCollection+Enumerator")]
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.Transform3DCollection+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1206639
// Developer: RajatG
// Reason: Words are not in the fxcop dictionary (Imaging)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.PixelFormats.PrgbaFloat128")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.PixelFormats.RgbaFloat128")]

//**************************************************************************************************************************
// Bug ID: 1192027
// Developer: mleonov
// Reason: The term "subsetting" is well known to the audience of this API.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.PreviewAndPrintButNoSubsetting")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.InstallableButNoSubsetting")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.EditableButNoSubsettingAndWithBitmapsOnly")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.InstallableButNoSubsettingAndWithBitmapsOnly")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.EditableButNoSubsetting")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Media.FontEmbeddingRight.PreviewAndPrintButNoSubsettingAndWithBitmapsOnly")]

//**************************************************************************************************************************
// Bug ID: 1206644
// Developer: niklasb
// Reason: Current case makes sense (it's "Top" + "SideBearing" not "Topside" + "Bearing") and is consistent with other properties (e.g., LeftSideBearing).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Media.CharacterMetrics.TopSideBearing")]

//**************************************************************************************************************************
// Bug ID: 1222524
// Developer: adsmith
// Reason: This type is not intended to be compared as a usage scenario, so the default, reflection-based comparison is sufficient.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Media3D.Visual3DCollection+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1218566
// Developer: adsmith
// Reason: This type is not intended to be compared as a usage scenario, so the default, reflection-based comparison is sufficient.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.Media.Int32Collection+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1167228
// Developer: jdmack
// Reason: These are equivilents of the native the Win32 message API parameters, and as such should preserve the Win32 spelling
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Interop.HwndSourceHook.BeginInvoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Interop.HwndSourceHook.Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1191907
// Developer: garyyang
// Reason: It is not meaningful to compare two Enumerators.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope="type", Target="System.Windows.TextDecorationCollection+Enumerator")]

//**************************************************************************************************************************
// Bug ID: 1219910
// Developer: dwaynen
// Reason: Complex code happens
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CursorConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1222525, 1224348
// Developer: Wchao
// Reason: It is possible to refactor this routine into calling some shared methods which would reduce the number of edges.
//         However doing so at this stage may risk destabilizing text layout process of those cases involving hyphenation.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextStore.GetTextChunk(System.Int32,System.Int32,System.Int32,System.Int32&,System.Int32&,System.Globalization.CultureInfo&,MS.Internal.SpanVector&):System.Char[]")]

//**************************************************************************************************************************
// Bug ID: n/a
// Developer: bencar
// Reason: This is the correct spelling according to the spec.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.CommandManager.InvalidateRequerySuggested():System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.CommandManager.RequerySuggested")]

//**************************************************************************************************************************
// Bug ID: 1249903, 1249904
// Developer: ericvan
// Reason: BitmapMetadata implements IEnumerable to enumerate query strings, but otherwise acts more like a query service
//         than a collection.  Ending in 'Collection' would be misleading.  This has been reviewed by Cabal and PMVT.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Media.Imaging.BitmapMetadata")]
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Media.Imaging.InPlaceBitmapMetadataWriter")]

//**************************************************************************************************************************
// Bug ID: 1249918, 1286121, 1283112
// Developer: ericvan
// Reason: According to rule it's safe to suppress if the method is easy to understand, test, and maintain.  This is
//         the case here as it contains a large switch for all the different kinds of PROPVARIANT values.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.ToObject(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.Init(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.GetBitsPerPixelFromEnum(MS.Internal.PixelFormatEnum):System.UInt32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.GetPixelFormatFlagsFromEnum(MS.Internal.PixelFormatEnum):System.Windows.Media.PixelFormatFlags")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.PixelFormat.GetGuidFromFormat(MS.Internal.PixelFormatEnum):System.Guid")]

//**************************************************************************************************************************
// Bug ID: 1246764
// Developer: dwaynen
// Reason: We sometimes return an array of these enums, so the size would matter.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Input.KeyStates")]

//**************************************************************************************************************************
// Bug ID: n/a
// Developer: bkaneva
// Reason: This is the correct spelling according to the spec. Add to dictionary.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Effects.GrainType.Contrasty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Effects.SumieBitmapEffect")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Effects.PosterEdgesBitmapEffect.PosterizationProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Effects.PosterEdgesBitmapEffect.Posterization")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Media.Effects.CraquelureBitmapEffect")]

//**************************************************************************************************************************
// Bug ID: 1250490
// Developer: garyyang
// Reason: It is a series of "if" tests to validate input parameters and has been optimized for performance.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.GlyphRun.Initialize(System.Windows.Media.GlyphTypeface,System.Int32,System.Boolean,System.Double,System.Collections.Generic.IList`1<System.UInt16>,System.Windows.Point,System.Collections.Generic.IList`1<System.Double>,System.Collections.Generic.IList`1<System.Windows.Point>,System.Collections.Generic.IList`1<System.Char>,System.String,System.Collections.Generic.IList`1<System.UInt16>,System.Collections.Generic.IList`1<System.Boolean>,System.Globalization.CultureInfo):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1286119
// Developer: mcalkins
// Reason: In this case it would be inappropriate to take a base class as the parameter must always be of the derived type.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Animation.Animatable.CloneCurrentValueCore(System.Windows.Media.Animation.Animatable):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1293967
// Developer: waynezen
// Reason: The violation has been suppressed by Tabavltr
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Ink.IncrementalLassoHitTester.AddPointsCore(System.Windows.Point[]):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313620
// Developer: grzegorz
// Reason: Paginator name has been already approved. Moving it from Framework to Core.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Documents.IDocumentPaginatorSource")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.IDocumentPaginatorSource.DocumentPaginator")]

//**************************************************************************************************************************
// Bug ID: 1313621
// Developer: grzegorz
// Reason: Paginator name has been already approved. Moving it from Framework to Core.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Documents.DynamicDocumentPaginator")]

//**************************************************************************************************************************
// Bug ID: 1313622
// Developer: grzegorz
// Reason: Paginator name has been already approved. Moving it from Framework to Core.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Documents.DocumentPaginator")]

//**************************************************************************************************************************
// Bug ID: 1313624
// Developer: grzegorz
// Reason: Paginator name has been already approved.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.IO.Stream)")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.Windows.Annotations.Storage.AnnotationStore)")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.Windows.Annotations.Storage.AnnotationStore)", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.IO.Stream)", MessageId="0#Paginator")]

//**************************************************************************************************************************
// Bug ID: 1313625
// Developer: grzegorz
// Reason: Paginator name has been already approved.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.DocumentPageView.DocumentPaginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.DocumentPageView.DocumentPaginator", MessageId="Paginator")]

//**************************************************************************************************************************
// Bug ID: 1313628
// Developer: grzegorz
// Reason: DocumentPage.MissingDocumentPage is immutable, so there is pretty good reason not to call the base.Dispose().
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope="member", Target="System.Windows.Documents.DocumentPage+MissingDocumentPage.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1416119
// Developer: pravirg
// Reason: We don't want base.Dispose() to be called if we are on a different thread. So VerifyAccess is outside try-finally block
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "System.Windows.Interop.HwndTarget.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313629
// Developer: grzegorz
// Reason: ContentPosition.Missing is immutable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.ContentPosition.Missing")]

//**************************************************************************************************************************
// Bug ID: 1313630
// Developer: grzegorz
// Reason: DocumentPage.Missing is immutable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.DocumentPage.Missing")]

//**************************************************************************************************************************
// Bug ID: 1299424
// Developer: niklasb
// Reason: This method needs to be the same as System.Windows.Markup.XamlReaderHelper.CompilePI, for which there is already an approved suppression.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "MS.Internal.FontFace.CompositeFontParser.CompilePI():System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1299425
// Developer: garyyang
// Reason: It is a finite state machine that is optimized for Bidi analysis. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.ResolveNeutralAndWeak(System.Collections.Generic.IList`1<MS.Internal.DirectionClass>,System.Int32,System.Int32,MS.Internal.DirectionClass,MS.Internal.DirectionClass,System.Byte,MS.Internal.TextFormatting.Bidi+State,MS.Internal.TextFormatting.Bidi+State,System.Boolean,MS.Internal.TextFormatting.Bidi+Flags):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.BidiAnalyzeInternal(MS.Internal.CharacterBuffer,System.Int32,System.Int32,System.Int32,MS.Internal.TextFormatting.Bidi+Flags,MS.Internal.TextFormatting.Bidi+State,System.Collections.Generic.IList`1<System.Byte>,System.Collections.Generic.IList`1<MS.Internal.DirectionClass>,System.Int32&):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1299426
// Developer: garyyang
// Reason: The method needs to handle a number of special cases thus it has more branching. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextStore.FetchLSRun(System.Int32,System.Boolean,MS.Internal.TextFormatting.Plsrun&,System.Int32&):MS.Internal.TextFormatting.LSRun")]

//*******************************************************************************************************************
// Bug ID: 1302460
// Developer: gillesk
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.Animation.TimelineGroup.Children")]

//*******************************************************************************************************************
// Bug ID: 1321137
// Developer: gillesk
// Reason: Code is well commented and easy to understand. We just have a lot of possible conditions to check
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.TimeIntervalCollection.IntersectsPeriodicCollection(System.TimeSpan,System.Windows.Duration,System.Double,System.Double,System.Double,System.Boolean):System.Boolean")]

//*******************************************************************************************************************
// Bug ID: 1337258
// Developer: dwaynen
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#IsMouseCaptureWithinChanged")]
// 1465803
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#IsMouseCaptureWithinChanged")]

[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.ContentElement.#FocusableChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.UIElement.#FocusableChanged")]

//*******************************************************************************************************************
// Bug ID: 1302461
// Developer: mcalkins
// Reason: We don’t need to provide .Equals for our enumerators – comparisons of Enumerators is not a high pri usage scenario, and thus we don’t need to optimize beyond the built in, reflection based equality.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope = "type", Target = "System.Windows.Media.Animation.TimelineCollection+Enumerator")]

//*******************************************************************************************************************
// Bug ID: 1322632
// Developer: garyyang
// Reason: The method uses two while loops to find the begining and end of a word. Each while loop contains a number of checks.
//         It is not hard to understand.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.TextStore.CollectRawWord(System.Int32,System.Boolean,System.Int32&,System.Globalization.CultureInfo&,System.Int32&,MS.Internal.Generic.SpanVector`1<System.Int32>&):System.Char[]")]

//*******************************************************************************************************************
// Bug ID: 1322631
// Developer: garyyang
// Reason: The method has the same logic as the one in the non-generic SpanVector which is already approved to be suppressed.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Generic.SpanVector`1.Set(System.Int32,System.Int32,T):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1332207
// Developer: jonesw
// Reason: This is readonly Collection.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Imaging.BitmapEncoder.ColorContexts")]

//**************************************************************************************************************************
// Bug ID: 1340741
// Developer: danlehen
// Reason: Consistency with 2D.  Scale is used both as a noun and a verb.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1719:ParameterNamesShouldNotMatchMemberNames", Scope="member", Target="System.Windows.Media.Media3D.Matrix3D.Scale(System.Windows.Media.Media3D.Vector3D):System.Void", MessageId="0#")]

//**************************************************************************************************************************
// Bug ID: 1340745
// Developer: gillesk
// Reason: whitespace is a word and correct TopSide and BottomSide are also correct they refer to properties
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1701:ResourceStringCompoundWordsShouldBeCasedCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="whitespace")]
[module: SuppressMessage("Microsoft.Naming", "CA1701:ResourceStringCompoundWordsShouldBeCasedCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="TopSide")]

//**************************************************************************************************************************
// Bug ID: 1340746
// Developer: gillesk
// Reason:  vxxxx is expressing version xxxx and is correct
//          StylusPointPropertyInfos is an API object
//          Scanlines is a valid term
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="vxxxx")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Infos")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="scanlines")]

//**************************************************************************************************************************
// Bug ID: 1340738, 1340739
// Developer: niklasb
// Reason: Whitespace as a single word is a computer term
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Media.TextFormatting.TextLine.TrailingWhitespaceLength")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Media.FormattedText.WidthIncludingTrailingWhitespace")]

//***************************************************************************************************************************
// Bug ID: 1337040
// Developer: garyyang
// Reason: The methods represent finite state machines that are optimized for Unicode Bidi Algorithm. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.BidiAnalyzeInternal(MS.Internal.CharacterBuffer,System.Int32,System.Int32,System.Int32,MS.Internal.TextFormatting.Bidi+Flags,MS.Internal.TextFormatting.Bidi+State,System.Collections.Generic.IList`1<System.Byte>,System.Collections.Generic.IList`1<MS.Internal.DirectionClass>,System.Collections.Generic.IList`1<MS.Internal.TextFormatting.Bidi+CharacterContext>,System.Int32&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.TextFormatting.Bidi.ResolveNeutralAndWeak(System.Collections.Generic.IList`1<MS.Internal.DirectionClass>,System.Collections.Generic.IList`1<MS.Internal.TextFormatting.Bidi+CharacterContext>,System.Int32,System.Int32,MS.Internal.DirectionClass,MS.Internal.DirectionClass,System.Byte,MS.Internal.TextFormatting.Bidi+State,MS.Internal.TextFormatting.Bidi+State,System.Boolean,MS.Internal.TextFormatting.Bidi+Flags):System.Int32")]

//***************************************************************************************************************************
// Windows Client Task: 46774
// Developer: niklasb; mleonov
// Reason: String/Uri manipulations in font code require both string and Uri representations to be present, and we would like to avoid unnecessary roundtrips.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1057:StringUriOverloadsCallSystemUriOverloads", Scope = "member", Target = "System.Windows.Media.Fonts.GetFontFamilies(System.String):System.Collections.Generic.ICollection`1<System.Windows.Media.FontFamily>")]
[module: SuppressMessage("Microsoft.Design", "CA1057:StringUriOverloadsCallSystemUriOverloads", Scope = "member", Target = "System.Windows.Media.Fonts.GetTypefaces(System.String):System.Collections.Generic.ICollection`1<System.Windows.Media.Typeface>")]

//***************************************************************************************************************************
// Bug ID: 1425552
// Developer: mleonov
// Reason: String/Uri manipulations in font code require both string and Uri representations to be present, and we would like to avoid unnecessary roundtrips.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2234:PassSystemUriObjectsInsteadOfStrings", Scope = "member", Target = "System.Windows.Media.Fonts.GetFontFamilies(System.Uri,System.String):System.Collections.Generic.ICollection`1<System.Windows.Media.FontFamily>")]

//***************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: this is a bug in FxCop.  EventArgs *should* be named 'e'
//***************************************************************************************************************************
//1340685
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsKeyboardFocusWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsStylusCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsMouseCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsMouseCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsStylusDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsStylusCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
//1340686
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsKeyboardFocusWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsMouseCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsMouseCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsStylusCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsStylusCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsStylusDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
//1340687
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.PresentationSource.#AddSourceChangedHandler(System.Windows.IInputElement,System.Windows.SourceChangedEventHandler)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.PresentationSource.#RemoveSourceChangedHandler(System.Windows.IInputElement,System.Windows.SourceChangedEventHandler)", MessageId="e")]
//1465800
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsMouseDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.UIElement.#OnIsKeyboardFocusedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
//1465801
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsMouseDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.ContentElement.#OnIsKeyboardFocusedChanged(System.Windows.DependencyPropertyChangedEventArgs)", MessageId="e")]

//***************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: These are coordinates T, W, X, Y, Z, etc.
//***************************************************************************************************************************
//1340668
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TranslateTransform.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.TranslateTransform.Y", MessageId="Y")]
//1340688
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.SetPosition(System.Int32,System.Int32):System.Void", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.SetPosition(System.Int32,System.Int32):System.Void", MessageId="1#y")]
//1340672
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Animation.PathAnimationSource.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Animation.PathAnimationSource.Y", MessageId="Y")]
//1340677
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D..ctor(System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D..ctor(System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D..ctor(System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Vector3D.Z", MessageId="Z")]
//1340678
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D..ctor(System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D..ctor(System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D..ctor(System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point3D.Z", MessageId="Z")]
//1340679, 1392460
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="3#w")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.Slerp(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Media3D.Quaternion,System.Double):System.Windows.Media.Media3D.Quaternion", MessageId="2#t")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.Slerp(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Media3D.Quaternion,System.Double,System.Boolean):System.Windows.Media.Media3D.Quaternion", MessageId="2#t")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.W", MessageId="W")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.Z", MessageId="Z")]
//1340680
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D..ctor(System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D..ctor(System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D..ctor(System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Size3D.Z", MessageId="Z")]
//1340682
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="3#w")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D..ctor(System.Double,System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D.W", MessageId="W")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Point4D.Z", MessageId="Z")]
//1340683
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D..ctor(System.Double,System.Double,System.Double,System.Double,System.Double,System.Double)", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D..ctor(System.Double,System.Double,System.Double,System.Double,System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D..ctor(System.Double,System.Double,System.Double,System.Double,System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.Contains(System.Double,System.Double,System.Double):System.Boolean", MessageId="2#z")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.Contains(System.Double,System.Double,System.Double):System.Boolean", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.Contains(System.Double,System.Double,System.Double):System.Boolean", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Media3D.Rect3D.Z", MessageId="Z")]
//1352704
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double,System.Single)", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double,System.Single)", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double,System.Single,System.Windows.Input.StylusPointDescription,System.Int32[])", MessageId="0#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint..ctor(System.Double,System.Double,System.Single,System.Windows.Input.StylusPointDescription,System.Int32[])", MessageId="1#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPoint.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPointProperties.X", MessageId="X")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPointProperties.Y", MessageId="Y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Input.StylusPointProperties.Z", MessageId="Z")]
//1369008
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", MessageId="4#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", MessageId="3#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,System.Boolean)", MessageId="4#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,System.Boolean)", MessageId="3#x")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", MessageId="4#y")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSource..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", MessageId="3#x")]

//***************************************************************************************************************************
// Bug ID: 1340688
// Developer: kenlai
// Reason: Equals(a, b) is OK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.op_Equality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters):System.Boolean", MessageId="0#a")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.op_Equality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters):System.Boolean", MessageId="1#b")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.op_Inequality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters):System.Boolean", MessageId="0#a")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.op_Inequality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters):System.Boolean", MessageId="1#b")]

//***************************************************************************************************************************
// Bug ID: 1340671
// Developer: kenlai
// Reason: color values: A, R, G, B
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.A", MessageId="A")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.B", MessageId="B")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.FromAValues(System.Single,System.Single[],System.Uri):System.Windows.Media.Color", MessageId="0#a")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.G", MessageId="G")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Color.R", MessageId="R")]


//***************************************************************************************************************************
// Bug ID: 1356393 
// Developer: SherifM
// Reason: These are all by design - all MIL collections aside from Visual3D's children are R/W.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.PolyQuadraticBezierSegment.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.PolyBezierSegment.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.PathFigure.Segments")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.PolyLineSegment.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GradientBrush.GradientStops")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.PathGeometry.Figures")]

//***************************************************************************************************************************
// Bug ID: 1186028, 1186029, 1186033 
// Developer: BruceMac
// Reason: As per email from Bruce, these should be excluded.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2237:MarkISerializableTypesWithSerializable", Scope="type", Target="System.IO.Packaging.PackWebRequest")]
[module: SuppressMessage("Microsoft.Usage", "CA2237:MarkISerializableTypesWithSerializable", Scope="type", Target="System.IO.Packaging.PackWebResponse")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.IO.Packaging.PackWebRequest.Headers")]

//**************************************************************************************************************************
// Bug ID: 138646
// Developer: gillesk
// Reason:  endPoint is not a compound word but two words just likes startPoint. This has been approved for the same class in
// bugs: 1010967 & 1021095
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Media.LinearGradientBrush..ctor(System.Windows.Media.GradientStopCollection,System.Windows.Point,System.Windows.Point)", MessageId="endPoint")]

//**************************************************************************************************************************
// Bug ID: 1377218
// Developer: olego
// Reason:  "Offscreen" derives from AccessebilityTech's property name. IsOffscreen and IsOffsceenCore members names are just reflections of ATG names.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.IsOffscreen():System.Boolean", MessageId="Offscreen")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.IsOffscreenCore():System.Boolean", MessageId="Offscreen")]

//**************************************************************************************************************************
// Bug ID: 1380339
// Developer: dwaynen
// Reason:  Too late for API changes
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.UIElement.TranslatePoint(System.Windows.Point,System.Windows.UIElement):System.Windows.Point")]

//**************************************************************************************************************************
// Bug ID: 1380345
// Developer: dwaynen
// Reason:  This is our event pattern
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.IInputElement.RaiseEvent(System.Windows.RoutedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1377223, 1592326
// Developer: olego, dmitryt
// Reason:  RaisePropertyChangedEvent, RaiseAutomationEvent and RaiseAsyncContentLoadedEvent are helper methods that internally raise events. The names of the methods properly reflect what they do. They are not events by themselves, nor they raise Avalon events.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.RaisePropertyChangedEvent(System.Windows.Automation.AutomationProperty,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.RaiseAutomationEvent(System.Windows.Automation.Peers.AutomationEvents):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.RaiseAsyncContentLoadedEvent(System.Windows.Automation.AsyncContentLoadedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1397333, 1397334
// Developer: alikk
// Reason: These match the MSFT SDK requirements/guidelines.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1701:ResourceStringCompoundWordsShouldBeCasedCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="PreProcess")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="codecs")]

//**************************************************************************************************************************
// Bug ID: 1388908, 1434238
// Developer: jordanpa, gillesk
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.QuaternionAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.QuaternionAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.Animation.ObjectAnimationUsingKeyFrames.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.Animation.ObjectAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]

//**************************************************************************************************************************
// Bug ID: 1244508
// Developer: jerryd
// Reason: IetfLanguageTag is a pre-existing name, currently a property of CUltureInfo.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.XmlLanguage.GetLanguage(System.String):System.Windows.Markup.XmlLanguage", MessageId="0#ietf")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.XmlLanguage.IetfLanguageTag", MessageId="Ietf")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Ietf")]

//**************************************************************************************************************************
// Bug ID: 1488185
// Developer: leov, mcalkins
// Reason: DesiredFrameRate is semantically meaningful only to Timelines and derived objects, not to the base class.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Timeline.GetDesiredFrameRate(System.Windows.Media.Animation.Timeline):System.Nullable`1<System.Int32>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Timeline.SetDesiredFrameRate(System.Windows.Media.Animation.Timeline,System.Nullable`1<System.Int32>):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1474547
// Developer: jonesw
// Reason: This is readonly Collection.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Imaging.BitmapMetadata.Keywords")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Imaging.BitmapMetadata.Author")]

//**************************************************************************************************************************
// Bug ID: 1511712
// Developer: niklasb
// Reason: IetfLanguageTag is a pre-existing name, currently a property of CultureInfo.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type", Target = "System.Windows.CultureInfoIetfLanguageTagConverter")]

//**************************************************************************************************************************
// Bug ID: 1535623
// Developer: tmulcahy
// Reason: WMP is a common abbreviation for Windows Media Player
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type", Target = "System.Windows.Media.InvalidWmpVersionException")]


//**************************************************************************************************************************
// Bug ID: 1598210
// Developer: dmitryt
// Reason: The limitation on argument type is by very design of this method - it only sends notification of certain type, even if it uses internally more generic method to do it.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Automation.Peers.AutomationPeer.RaiseAsyncContentLoadedEvent(System.Windows.Automation.AsyncContentLoadedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1561840
// Developer: cedricd
// Reason: We don't want users to be able to throw this exception, so we have no need to expose the common constructors
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1032:ImplementStandardExceptionConstructors", Scope = "type", Target = "System.Windows.Media.Animation.AnimationException")]


//**************************************************************************************************************************
// Bug ID: 1665936
// Developer: gillesk
// Reason: The structure is packed on 1 byte boundary, we handle this alignment issue in the code.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_PARTITION_NOTIFYPRESENT", MessageId="FrameTime")]

//**************************************************************************************************************************
// Bug ID: 1682381 
// Developer: weibz
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Vxxxx")]

//**************************************************************************************************************************
// Bug ID: 1695527 
// Developer: ccooney
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Imaging.WmpBitmapEncoder.SubsamplingLevel", MessageId="Subsampling")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.Imaging.WmpBitmapEncoder.CompressedDomainTranscode", MessageId="Transcode")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: No bug
// Developer: kurtb
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons (also done previously on UIElement/ContentElement)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsHitTestVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsKeyboardFocusWithinChanged")]

//***************************************************************************************************************************
// Bug ID: No bug
// Developer: kurtb
// Reason: this is a bug in FxCop.  EventArgs *should* be named 'e' (also done previously on UIElement/ContentElement)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsKeyboardFocusWithinChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsStylusCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsMouseCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsMouseCaptureWithinChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsStylusDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsStylusCapturedChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsMouseDirectlyOverChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.OnIsKeyboardFocusedChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId = "e")]


//**************************************************************************************************************************
//  BugID: No bug
//  Developer: kurtb
//  Reason: Viewport2DVisual3D will only use this property on Materials.  The method type gives developers some warning of that.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Media3D.Viewport2DVisual3D.SetIsVisualHostMaterial(System.Windows.Media.Media3D.Material,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Media.Media3D.Viewport2DVisual3D.GetIsVisualHostMaterial(System.Windows.Media.Media3D.Material):System.Boolean")]

//**************************************************************************************************************************
//  BugID: No bug
//  Developer: kurtb
//  Reason: We want to be able to say 2DOn3D
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Media.VisualTreeHelper.HitTest(System.Windows.Media.Visual,System.Windows.Point,System.Boolean):System.Windows.Media.HitTestResult")]

//*******************************************************************************************************************
// Bug ID: No bug
// Developer: kurtb
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#IsMouseCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "e:System.Windows.UIElement3D.#FocusableChanged")]

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: kurtb
// Reason: Same as above but for GeneralTransform3DCollection
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes", Scope = "type", Target = "System.Windows.Media.Media3D.GeneralTransform3DCollection+Enumerator")]

//*******************************************************************************************************************
// Bug ID: no bug
// Developer: kurtb
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Media3D.GeneralTransform3DGroup.Children")]

//*******************************************************************************************************************
// Bug ID: 
// Developer: ChangoV
// Reason: We can't add resource strings in v3.5. And this exception shold be thrown only in the "impossible" 
//  condition when malicious code somehow manages to pass us a bogus WebResponse object.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.WpfWebRequestHelper.EndGetResponse(System.Net.WebRequest,System.IAsyncResult):System.Net.WebResponse")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.WpfWebRequestHelper.GetResponse(System.Net.WebRequest):System.Net.WebResponse")]

//*******************************************************************************************************************
// Bug ID: no bug
// Developer: dwaynen
// Reason: Fant is the name of the individual who implemented this scaling type.
//*******************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Media.BitmapScalingMode.#Fant", MessageId="Fant")]

//**************************************************************************************************************************
// Bug IDs: 199045
// Developer: brandf
// Reason: SetBack doesn't refer to the english word 'Setback'
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="SetBack", Scope="member", Target="System.Windows.Interop.D3DImage.#SetBackBuffer(System.Windows.Interop.D3DResourceType,System.IntPtr)")]

//**************************************************************************************************************************
// Bug IDs: 202854
// Developer: bchapman
// Reason:  FxCop doesn't like Uid as a name.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Uid", Scope="member", Target="System.Windows.UIElement.#Uid")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Uid", Scope="member", Target="System.Windows.UIElement.#UidProperty")]

//**************************************************************************************************************************
// Bug ID: 632951
// Developer: brenclar
// Reason: Our DP system does follow the event handler convention but our DependencyPropertyChangedEventArgs doesn't derive 
//         from EventArgs (it's a struct for perf reasons).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="System.Windows.Interop.D3DImage.#IsFrontBufferAvailableChanged")]

//**************************************************************************************************************************
// Bug IDs: 632952
// Developer: kedecond
// Reason:  parameter names audioBytes, displayString, guid
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "bytes", Scope = "member", Target = "System.Windows.Clipboard.#SetAudio(System.Byte[])")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "bytes", Scope = "member", Target = "System.Windows.DataObject.#SetAudio(System.Byte[])")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "guid", Scope = "member", Target = "System.Windows.Ink.PropertyDataChangedEventArgs.#.ctor(System.Guid,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "string", Scope = "member", Target = "System.Windows.Input.KeyGesture.#.ctor(System.Windows.Input.Key,System.Windows.Input.ModifierKeys,System.String)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "guid", Scope = "member", Target = "System.Windows.Input.StylusButtonCollection.#GetStylusButtonByGuid(System.Guid)")]
//**************************************************************************************************************************
// Bug IDs: 632952
// Developer: kedecond
// Reason:  DependencyPropertyChangedEventArgs is not derived from EventArgs. This cannot be changed now
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#FocusableChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsKeyboardFocusWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsMouseCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.ContentElement.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#FocusableChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsHitTestVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsKeyboardFocusWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsMouseCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement.#IsVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#FocusableChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsEnabledChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsHitTestVisibleChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsKeyboardFocusedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsKeyboardFocusWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsMouseCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsMouseCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsMouseDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsStylusCapturedChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsStylusCaptureWithinChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsStylusDirectlyOverChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.UIElement3D.#IsVisibleChanged")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: Event is flagged as keyword
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", MessageId = "RemoveHandler", Scope = "member", Target = "System.Windows.IInputElement.#RemoveHandler(System.Windows.RoutedEvent,System.Delegate)")]
[module: SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", MessageId = "AddHandler", Scope = "member", Target = "System.Windows.IInputElement.#AddHandler(System.Windows.RoutedEvent,System.Delegate)")]
[module: SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", MessageId = "RaiseEvent", Scope = "member", Target = "System.Windows.IInputElement.#RaiseEvent(System.Windows.RoutedEventArgs)")]
//**************************************************************************************************************************
// Bug IDs: 632952
// Developer: kedecond
// Reason:  Value and Type property names are flagged by fxcop as violations 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Input.TabletDevice.#Type")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.BooleanKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.ByteKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.CharKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.ColorKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.DecimalKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.DoubleKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Int16KeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Int32KeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Int64KeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.KeyTime.#Type")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.MatrixKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.ObjectKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Point3DKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.PointKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.QuaternionKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.RectKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.SingleKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.SizeKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.StringKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DKeyFrame.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.VectorKeyFrame.#Value")]
//**************************************************************************************************************************
// Bug IDs: 632952
// Developer: kedecond
// Reason:  ArgumentException thrown inside a property setter should have the paramName as the property name. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Ink.DrawingAttributes.#set_Width(System.Double)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Ink.DrawingAttributes.#set_Height(System.Double)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Input.StylusPoint.#set_PressureFactor(System.Single)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Input.StylusPoint.#set_X(System.Double)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Input.StylusPoint.#set_Y(System.Double)")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: Preprocess and Subtree are dictionary words, but this would be a breaking change. 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "PreProcess", Scope = "member", Target = "System.Windows.Input.InputManager.#PreProcessInput")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "SubTree", Scope = "member", Target = "System.Windows.Input.CaptureMode.#SubTree")]

//**************************************************************************************************************************
// Bug ID: n/a
// Developer: BrenClar
// Reason: Changing a parameter name in a public API signature is a breaking change.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1720:IdentifiersShouldNotContainTypeNames", MessageId="string", Scope="member", Target="System.Windows.Media.TextFormatting.CharacterBufferRange.#.ctor(System.String,System.Int32,System.Int32)")]
[module: SuppressMessage("Microsoft.Naming","CA1720:IdentifiersShouldNotContainTypeNames", MessageId="string", Scope="member", Target="System.Windows.Media.TextFormatting.CharacterBufferReference.#.ctor(System.String,System.Int32)")]
[module: SuppressMessage("Microsoft.Naming","CA1720:IdentifiersShouldNotContainTypeNames", MessageId="string", Scope="member", Target="System.Windows.Media.TextFormatting.TextCharacters.#.ctor(System.String,System.Int32,System.Int32,System.Windows.Media.TextFormatting.TextRunProperties)")]
[module: SuppressMessage("Microsoft.Naming","CA1720:IdentifiersShouldNotContainTypeNames", MessageId="string", Scope="member", Target="System.Windows.Media.TextFormatting.TextCharacters.#.ctor(System.String,System.Windows.Media.TextFormatting.TextRunProperties)")]


//**************************************************************************************************************************
// Bug ID: 632951
// Developer: brenclar
// Reason: The property name would be even more confusing if cased differently, and it would require a breaking change to a 
//         public API to make this change anyway.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1709:IdentifiersShouldBeCasedCorrectly", MessageId="Uv", Scope="member", Target="System.Windows.Media.Effects.ShaderEffect.#DdxUvDdyUvRegisterIndex")]

//**************************************************************************************************************************
// Bug ID: 632951
// Developer: brenclar
// Reason: DIPs is an acronym referring to device-independent pixels, not the English word "dips".
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1709:IdentifiersShouldBeCasedCorrectly", MessageId="DIPs", Scope="member", Target="System.Windows.Media.ImageSource.#PixelsToDIPs(System.Double,System.Int32)")]

//**************************************************************************************************************************
// Bug ID: 632951
// Developer: brenclar
// Reason: Transform and Transform3D.Value are not confusingly named, and changing this would require a breaking change to
//         a public API.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1721:PropertyNamesShouldNotMatchGetMethods", Scope="member", Target="System.Windows.Media.Transform.#Value")]
[module: SuppressMessage("Microsoft.Naming","CA1721:PropertyNamesShouldNotMatchGetMethods", Scope="member", Target="System.Windows.Media.Media3D.Transform3D.#Value")]

//**************************************************************************************************************************
// Developer: bencar
// Reason: TouchDown does not refer to the American football term here. It refers to putting your finger on the screen.
//         TouchUp does not refer to fixing a paint job. It refers to lifting your finger from the screen.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#OnPreviewTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#OnPreviewTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#OnTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#OnTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#PreviewTouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#PreviewTouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#TouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#TouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#PreviewTouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#PreviewTouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.ContentElement.#TouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.ContentElement.#TouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#OnPreviewTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#OnPreviewTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#OnTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#OnTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#PreviewTouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#PreviewTouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#TouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#TouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#PreviewTouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#PreviewTouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement.#TouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement.#TouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#OnPreviewTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#OnPreviewTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#OnTouchDown(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#OnTouchUp(System.Windows.Input.TouchEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#PreviewTouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#PreviewTouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#TouchDownEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#TouchUpEvent")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#PreviewTouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#PreviewTouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchDown", Scope="member", Target="System.Windows.UIElement3D.#TouchDown")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.UIElement3D.#TouchUp")]
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="TouchUp", Scope="member", Target="System.Windows.Input.TouchFrameEventArgs.#SuspendMousePromotionUntilTouchUp()")]
//**************************************************************************************************************************
// Bug: 692903
// Developer: kedecond
// Reason: Quartic and Quintic are math words. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Quartic", Scope = "type", Target = "System.Windows.Media.Animation.QuarticEase")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Quintic", Scope = "type", Target = "System.Windows.Media.Animation.QuinticEase")]


//***************************************************************************************************************************
// Suppressing because these would be a API breaking change.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#AdvanceWidths", Justification="Cannot fix this anymore since it would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#CaretStops", Justification="Cannot fix this anymore since it would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#Characters", Justification="Cannot fix this anymore since it would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#ClusterMap", Justification="Cannot fix this anymore since it would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#GlyphIndices", Justification="Cannot fix this anymore since it would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Media.GlyphRun.#GlyphOffsets", Justification="Cannot fix this anymore since it would be a breaking change.")]


//***************************************************************************************************************************
// ArgumentException in these instances should never bubble to customers since they are real bugs.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#.ctor(System.IO.UnmanagedMemoryStream)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#CheckedProbe(System.Int32,System.Int32)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#CopyTo(MS.Internal.FontCache.CheckedPointer)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#OffsetOf(System.Void*)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#Probe(System.Int32,System.Int32)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#ToArray()", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.FontCache.CheckedPointer.#op_Addition(MS.Internal.FontCache.CheckedPointer,System.Int32)", Justification="Exceptions should never be thrown under normal circumstances. Cannot change exception type to OperationExeption though since that would be a breaking change.")]


//***************************************************************************************************************************
// While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. 
// This has also been pre-existing.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.TextFormatting.TextFormatterImp.#VerifyTextFormattingArguments(System.Windows.Media.TextFormatting.TextSource,System.Int32,System.Double,System.Windows.Media.TextFormatting.TextParagraphProperties,System.Windows.Media.TextFormatting.TextRunCache)", Justification="While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. This has also been pre-existing.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.TextFormatting.TextRunCacheImp.#FetchTextRun(MS.Internal.TextFormatting.FormatSettings,System.Int32,System.Int32,System.Int32&,System.Int32&)", Justification="While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. This has also been pre-existing.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextEndOfLine.#.ctor(System.Int32,System.Windows.Media.TextFormatting.TextRunProperties)", Justification="While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. This has also been pre-existing.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextCharacters.#MS.Internal.TextFormatting.ITextSymbols.GetTextShapeableSymbols(MS.Internal.Shaping.GlyphingCache,System.Windows.Media.TextFormatting.CharacterBufferReference,System.Int32,System.Boolean,System.Boolean,System.Globalization.CultureInfo,System.Windows.Media.TextFormatting.TextModifierScope,System.Windows.Media.TextFormattingMode,System.Boolean)", Justification="While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. This has also been pre-existing.")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextCharacters.#.ctor(System.Windows.Media.TextFormatting.CharacterBufferReference,System.Int32,System.Windows.Media.TextFormatting.TextRunProperties)", Justification="While not compliant with CLR guidelines argument naming adds significant value to customer. Therefore keeping as is. This has also been pre-existing.")]

//***************************************************************************************************************************
// Funny text names which are not conforming to CLR naming conventions.
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Jis", Scope="member", Target="System.Windows.FontEastAsianLanguage.#Jis04", Justification="Font types do not have CLR compliant names. ")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Jis", Scope="member", Target="System.Windows.FontEastAsianLanguage.#Jis78", Justification="Font types do not have CLR compliant names. ")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Jis", Scope="member", Target="System.Windows.FontEastAsianLanguage.#Jis83", Justification="Font types do not have CLR compliant names. ")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Jis", Scope="member", Target="System.Windows.FontEastAsianLanguage.#Jis90", Justification="Font type names are not CLR compliant.")]

[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="bidi", Scope="member", Target="System.Windows.Media.GlyphRun.#.ctor(System.Windows.Media.GlyphTypeface,System.Int32,System.Boolean,System.Double,System.Collections.Generic.IList`1<System.UInt16>,System.Windows.Point,System.Collections.Generic.IList`1<System.Double>,System.Collections.Generic.IList`1<System.Windows.Point>,System.Collections.Generic.IList`1<System.Char>,System.String,System.Collections.Generic.IList`1<System.UInt16>,System.Collections.Generic.IList`1<System.Boolean>,System.Windows.Markup.XmlLanguage)")]


//***************************************************************************************************************************
// Dispose pattern not properly implemented. Bug: Dev10 Bug #704357
//***************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#~Factory()", Justification="Suppressing for now. Bug filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.Factory.#Dispose(System.Boolean)", Justification="Suppressing for now. Bug filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.Line.#Dispose()", Justification="Suppressing for now. Bug filed to correct pattern")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.ItemProps.#Dispose(System.Boolean)", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.ItemProps.#~ItemProps()", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#Dispose(System.Boolean)", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.Text.TextInterface.TextAnalyzer.#~TextAnalyzer()", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.TextFormatting.SimpleTextLine.#Dispose()", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="MS.Internal.TextFormatting.TextMetrics+FullTextLine.#FormatLine(MS.Internal.TextFormatting.FullTextState,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,MS.Internal.TextFormatting.LineFlags,MS.Internal.TextFormatting.FormattedTextSymbols)", Justification="Suppressing those for now. But filed to correct pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.FontFamilyMapCollection+Enumerator.#Dispose()", Justification="Suppressing for now. Bug filed to correct Dispose pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.FamilyTypefaceCollection+Enumerator.#Dispose()", Justification="Suppressing for now. Bug filed to correct Dispose pattern.")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.TextFormatting.TextFormatter.#Dispose()", Justification="Suppressing warning. Bug filed to correct pattern.")]

//**************************************************************************************************************************
// Bug ID: Dev10 692903
// Developer: bartde
// Reason: This would break public API; well-justified design patterns.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="hwnd", Scope="member", Target="System.Windows.Interop.HwndSource.#FromHwnd(System.IntPtr)", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="member", Target="System.Windows.Interop.HwndSource.#FromHwnd(System.IntPtr)", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="hwnd", Scope="type", Target="System.Windows.Interop.HwndSourceHook", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="type", Target="System.Windows.Interop.HwndSource", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="type", Target="System.Windows.Interop.HwndSourceHook", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#HwndSourceHook", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="type", Target="System.Windows.Interop.HwndSourceParameters", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="type", Target="System.Windows.Interop.HwndTarget", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="hwnd", Scope="member", Target="System.Windows.Interop.HwndTarget.#.ctor(System.IntPtr)", Justification="Standard spelling for HWND.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,System.Boolean)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,System.Boolean)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Interop.HwndSource.#.ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="x", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#SetPosition(System.Int32,System.Int32)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="y", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#SetPosition(System.Int32,System.Int32)", Justification="Obvious naming for coordinates.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Param", Scope="type", Target="System.Windows.Interop.HwndSourceHook", Justification="lParam and wParam are most meaningful names in context of interop.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="w", Scope="type", Target="System.Windows.Interop.HwndSourceHook", Justification="lParam and wParam are most meaningful names in context of interop.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="l", Scope="type", Target="System.Windows.Interop.HwndSourceHook", Justification="lParam and wParam are most meaningful names in context of interop.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="b", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#op_Inequality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters)", Justification="Breaking change.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="a", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#op_Inequality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters)", Justification="Breaking change.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="b", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#op_Equality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters)", Justification="Breaking change.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="a", Scope="member", Target="System.Windows.Interop.HwndSourceParameters.#op_Equality(System.Windows.Interop.HwndSourceParameters,System.Windows.Interop.HwndSourceParameters)", Justification="Breaking change.")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndSource.#TranslateCharCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)", Justification="Established pattern of by-ref passing for MSG argument.")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndSource.#TranslateAcceleratorCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)", Justification="Established pattern of by-ref passing for MSG argument.")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndSource.#OnMnemonicCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)", Justification="Established pattern of by-ref passing for MSG argument.")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="4#", Scope="member", Target="System.Windows.Interop.HwndSourceHook.#Invoke(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&)", Justification="Would be breaking change; reasonable pattern here.")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="System.Windows.Interop.HwndTarget.VisualTarget_DetachFromHwnd(System.IntPtr)", Scope="member", Target="System.Windows.Interop.HwndTarget.#.ctor(System.IntPtr)", Justification="Return value ignored on purpose in finally block. See code for #pragma warning suppress.")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.UnsafeNativeMethods.CriticalSetWindowTheme(System.Runtime.InteropServices.HandleRef,System.String,System.String)", Justification="This is fine either way, but not taking the risk to regress in case themes are disabled and the function may fail.")]



//**************************************************************************************************************************
// Bug ID: NA
// Developer: pantal
// Reason: Shader is a valid word, but as there is no WPF scoped dictionary, we have to suppress.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Shader", Scope="member", Target="System.Windows.Media.RenderCapability.#IsPixelShaderVersionSupportedInSoftware(System.Int16,System.Int16)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Shader", Scope="member", Target="System.Windows.Media.RenderCapability.#MaxPixelShaderInstructionSlots(System.Int16,System.Int16)")]


//**************************************************************************************************************************
// New since v4 RTM:
//**************************************************************************************************************************

//**************************************************************************************************************************
// This is similar to other 1-byte packed structures where suppressions already exist.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability","CA1900:ValueTypeFieldsShouldBePortable", MessageId="DirtyRect", Scope="type", Target="System.Windows.Media.Composition.DUCE+MILCMD_BITMAP_INVALIDATE")]

//**************************************************************************************************************************
// FxCop thinks the API is named after "setback buffer" (wrong!) so complains about "SetBack" capitalization.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="SetBack", Scope="member", Target="System.Windows.Interop.D3DImage.#SetBackBuffer(System.Windows.Interop.D3DResourceType,System.IntPtr,System.Boolean)")]
