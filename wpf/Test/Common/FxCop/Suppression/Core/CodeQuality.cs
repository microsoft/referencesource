//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//***** SherifM
//***** Bug# 1038710
//***** Approved by Actprog
[module: SuppressMessage("Microsoft.Naming", "CA1724:TypeNamesShouldNotMatchNamespaces", Scope="type", Target="System.Windows.Media.Drawing")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1010161, 1126535, 1434239
// Developer: MCalkins, GillesK
// Reason: Most of these pass the new requirements for this, one specific category
//         does not and we will have to fix those in M11.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeKeyFrame..ctor(System.Windows.Size,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeKeyFrame..ctor(System.Windows.Media.Animation.SizeKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeKeyFrame..ctor(System.Windows.Size)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalKeyFrame..ctor(System.Decimal,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalKeyFrame..ctor(System.Decimal)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalKeyFrame..ctor(System.Windows.Media.Animation.DecimalKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Animation.Point3DAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Rect,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Media.Animation.RectAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Rect,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.MatrixKeyFrame..ctor(System.Windows.Media.Matrix,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.MatrixKeyFrame..ctor(System.Windows.Media.Matrix)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.MatrixKeyFrame..ctor(System.Windows.Media.Animation.MatrixKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DKeyFrame..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DKeyFrame..ctor(System.Windows.Media.Media3D.Point3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DKeyFrame..ctor(System.Windows.Media.Animation.Point3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSize3DKeyFrame..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSize3DKeyFrame..ctor(System.Windows.Media.Animation.SplineSize3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Timeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Timeline..ctor(System.Nullable`1<System.TimeSpan>)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Timeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.RepeatBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.RectAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.PointAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRectKeyFrame..ctor(System.Windows.Media.Animation.SplineRectKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRectKeyFrame..ctor(System.Windows.Rect,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleKeyFrame..ctor(System.Double,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleKeyFrame..ctor(System.Windows.Media.Animation.DoubleKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleKeyFrame..ctor(System.Double)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.SingleAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineByteKeyFrame..ctor(System.Windows.Media.Animation.SplineByteKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineByteKeyFrame..ctor(System.Byte,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt64KeyFrame..ctor(System.Int64,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt64KeyFrame..ctor(System.Windows.Media.Animation.SplineInt64KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteKeyFrame..ctor(System.Byte,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteKeyFrame..ctor(System.Windows.Media.Animation.ByteKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteKeyFrame..ctor(System.Byte)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Color,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Color,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Animation.ColorAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt32KeyFrame..ctor(System.Int32,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt32KeyFrame..ctor(System.Windows.Media.Animation.SplineInt32KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Windows.Media.Animation.Int64Animation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Int64,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Int64,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16KeyFrame..ctor(System.Int16,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16KeyFrame..ctor(System.Windows.Media.Animation.Int16KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16KeyFrame..ctor(System.Int16)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSizeKeyFrame..ctor(System.Windows.Media.Animation.SplineSizeKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSizeKeyFrame..ctor(System.Windows.Size,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Double,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Windows.Media.Animation.DoubleAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Double,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineDecimalKeyFrame..ctor(System.Decimal,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineDecimalKeyFrame..ctor(System.Windows.Media.Animation.SplineDecimalKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimation..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimation..ctor(System.Windows.Media.Animation.Size3DAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimation..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimation..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimation..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Decimal,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Windows.Media.Animation.DecimalAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Decimal,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.VectorAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorKeyFrame..ctor(System.Windows.Media.Color)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorKeyFrame..ctor(System.Windows.Media.Animation.ColorKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorKeyFrame..ctor(System.Windows.Media.Color,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Animation.Vector3DAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.ColorAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Rect3DAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Point,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Media.Animation.PointAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Point,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Vector3DAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSingleKeyFrame..ctor(System.Windows.Media.Animation.SplineSingleKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineSingleKeyFrame..ctor(System.Single,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Rotation3DAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineVectorKeyFrame..ctor(System.Windows.Media.Animation.SplineVectorKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineVectorKeyFrame..ctor(System.Windows.Vector,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointKeyFrame..ctor(System.Windows.Point)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointKeyFrame..ctor(System.Windows.Point,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointKeyFrame..ctor(System.Windows.Media.Animation.PointKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectKeyFrame..ctor(System.Windows.Media.Animation.RectKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectKeyFrame..ctor(System.Windows.Rect)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectKeyFrame..ctor(System.Windows.Rect,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DKeyFrame..ctor(System.Windows.Media.Animation.Size3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DKeyFrame..ctor(System.Windows.Media.Media3D.Size3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DKeyFrame..ctor(System.Windows.Media.Media3D.Size3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64AnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Int64AnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleKeyFrame..ctor(System.Single,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleKeyFrame..ctor(System.Single)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleKeyFrame..ctor(System.Windows.Media.Animation.SingleKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineVector3DKeyFrame..ctor(System.Windows.Media.Animation.SplineVector3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineVector3DKeyFrame..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineColorKeyFrame..ctor(System.Windows.Media.Animation.SplineColorKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineColorKeyFrame..ctor(System.Windows.Media.Color,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorKeyFrame..ctor(System.Windows.Media.Animation.VectorKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorKeyFrame..ctor(System.Windows.Vector)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorKeyFrame..ctor(System.Windows.Vector,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64KeyFrame..ctor(System.Int64)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64KeyFrame..ctor(System.Windows.Media.Animation.Int64KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64KeyFrame..ctor(System.Int64,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32KeyFrame..ctor(System.Windows.Media.Animation.Int32KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32KeyFrame..ctor(System.Int32)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32KeyFrame..ctor(System.Int32,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Int32,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Int32,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Windows.Media.Animation.Int32Animation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Point3DAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DKeyFrame..ctor(System.Windows.Media.Media3D.Vector3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DKeyFrame..ctor(System.Windows.Media.Animation.Vector3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DKeyFrame..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16AnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Int16AnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Size3DAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Size3DAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.CharKeyFrame..ctor(System.Windows.Media.Animation.CharKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.CharKeyFrame..ctor(System.Char)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.CharKeyFrame..ctor(System.Char,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Animation.Rotation3DAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimation..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimation..ctor(System.Windows.Media.Animation.Rect3DAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimation..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimation..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DAnimation..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.SizeAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32AnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.Int32AnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Single,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Windows.Media.Animation.SingleAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Single,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Vector,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Media.Animation.VectorAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Vector,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt16KeyFrame..ctor(System.Int16,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineInt16KeyFrame..ctor(System.Windows.Media.Animation.SplineInt16KeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Byte,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Windows.Media.Animation.ByteAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Byte,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRotation3DKeyFrame..ctor(System.Windows.Media.Animation.SplineRotation3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRotation3DKeyFrame..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Int16,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Int16,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Windows.Media.Animation.Int16Animation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrame..ctor(System.Windows.Media.Animation.Rotation3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrame..ctor(System.Windows.Media.Media3D.Rotation3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DKeyFrame..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DKeyFrame..ctor(System.Windows.Media.Animation.Rect3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DKeyFrame..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rect3DKeyFrame..ctor(System.Windows.Media.Media3D.Rect3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.DoubleAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.StringKeyFrame..ctor(System.Windows.Media.Animation.StringKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.StringKeyFrame..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.StringKeyFrame..ctor(System.String,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.DecimalAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplinePointKeyFrame..ctor(System.Windows.Media.Animation.SplinePointKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplinePointKeyFrame..ctor(System.Windows.Point,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.ByteAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.BooleanKeyFrame..ctor(System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.BooleanKeyFrame..ctor(System.Windows.Media.Animation.BooleanKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.BooleanKeyFrame..ctor(System.Boolean,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplinePoint3DKeyFrame..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplinePoint3DKeyFrame..ctor(System.Windows.Media.Animation.SplinePoint3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRect3DKeyFrame..ctor(System.Windows.Media.Animation.SplineRect3DKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineRect3DKeyFrame..ctor(System.Windows.Media.Media3D.Rect3D,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Media.Animation.SizeAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Size,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Size,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineDoubleKeyFrame..ctor(System.Double,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineDoubleKeyFrame..ctor(System.Windows.Media.Animation.SplineDoubleKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ObjectKeyFrame..ctor(System.Object,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ObjectKeyFrame..ctor(System.Object)")]

//**************************************************************************************************************************
// Bug ID: 1124175
// Developer: Ahodsdon
// Reason: This will require further investigation in M11.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.ImageSource..ctor(System.Windows.Media.ImageSource,System.Windows.Media.Animation.Animatable+CloneType")]


//**************************************************************************************************************************
// Bug ID: 1038659 1038694
// Developer: SherifM
// Reason: UIAutimation code, they got exemption for FxCop.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Automation.TextProviderWrapper.RangeFromChild(System.Windows.Automation.InteropProvider.IRawElementProviderSimple):System.Windows.Automation.InteropProvider.ITextRangeInteropProvider")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Automation.TextRangeProviderWrapper.MoveEndpoint(System.Windows.Automation.Text.TextPatternRangeEndpoint,System.Windows.Automation.InteropProvider.ITextRangeInteropProvider,System.Windows.Automation.Text.TextPatternRangeEndpoint):System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.AddPatternProvider(System.Windows.UIElement,System.Windows.Automation.AutomationPattern,System.Windows.Automation.Provider.IAutomationPatternProvider,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Automation.Provider.AutomationProvider.RaiseAutomationEvent(System.Windows.Automation.AutomationEvent,System.Windows.UIElement,System.Windows.Automation.AutomationEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038716
// Developer: SherifM
// Reason: See bug for more details.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope="member", Target="System.Windows.Media.Media3D.Quaternion..ctor(System.Windows.Media.Media3D.Vector3D,System.Double)")]
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope="member", Target="System.Windows.Media.Media3D.Quaternion.Slerp(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Media3D.Quaternion,System.Double):System.Windows.Media.Media3D.Quaternion")]

//**************************************************************************************************************************
// Bug ID: 1106767
// Developer: garyyang
// Reason: Reviewed the callstacks. This is a false alarm caused by C++ compiler emitting Dispose() calls in the constructor
//         C++ compiler team will fix the code emit issue. For more details please refer to the bug.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Win32.SafePortHandle..ctor(System.Void*,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 1100745
// Developer: NiklasB
// Reason: LanguageSpecificStringDictionary implements IDictionary<XmlLanguage,string>, and the indexer
//         in question is part of the interface.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1043:UseIntegralOrStringArgumentForIndexers", Scope="member", Target="System.Windows.Media.LanguageSpecificStringDictionary.Item[System.Windows.Markup.XmlLanguage]")]

//**************************************************************************************************************************
// Bug ID: 1199494
// Developer: samgeo
// Reason: This is on a set property.  There is no param name, the guidelines as we understand it is to simply throw ArgumentNullException
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Input.StylusPlugIns.DynamicRenderer.set_DrawingAttributes(System.Windows.Ink.DrawingAttributes):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1199495
// Developer: samgeo
// Reason: We're instancing an ArgumentNullException correctly: ArgumentNullException("stylusPoints");
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Input.StylusPlugIns.RawStylusInput.SetStylusPoints(System.Windows.Input.StylusPointCollection):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1163772
// Developer: cedricd
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineRectKeyFrame..ctor(System.Windows.Rect)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineRectKeyFrame..ctor(System.Windows.Rect,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineByteKeyFrame..ctor(System.Byte,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineByteKeyFrame..ctor(System.Byte)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt64KeyFrame..ctor(System.Int64)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt64KeyFrame..ctor(System.Int64,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt32KeyFrame..ctor(System.Int32)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt32KeyFrame..ctor(System.Int32,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineDecimalKeyFrame..ctor(System.Decimal)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineDecimalKeyFrame..ctor(System.Decimal,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineSingleKeyFrame..ctor(System.Single,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineSingleKeyFrame..ctor(System.Single)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineVectorKeyFrame..ctor(System.Windows.Vector)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineVectorKeyFrame..ctor(System.Windows.Vector,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineVector3DKeyFrame..ctor(System.Windows.Media.Media3D.Vector3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineVector3DKeyFrame..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineColorKeyFrame..ctor(System.Windows.Media.Color)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineColorKeyFrame..ctor(System.Windows.Media.Color,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt16KeyFrame..ctor(System.Int16,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineInt16KeyFrame..ctor(System.Int16)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineRotation3DKeyFrame..ctor(System.Windows.Media.Media3D.Rotation3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineRotation3DKeyFrame..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineSizeKeyFrame..ctor(System.Windows.Size,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineSizeKeyFrame..ctor(System.Windows.Size)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplinePointKeyFrame..ctor(System.Windows.Point,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplinePointKeyFrame..ctor(System.Windows.Point)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplinePoint3DKeyFrame..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplinePoint3DKeyFrame..ctor(System.Windows.Media.Media3D.Point3D)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineDoubleKeyFrame..ctor(System.Double,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineDoubleKeyFrame..ctor(System.Double)")]

//**************************************************************************************************************************
// Bug ID: 1169106
// Developer: niklasb
// Reason: We should not dispose _formatter in LineEnumerator.Dispose. TextFormatter that is used by FormattedText is from the current Dispatcher. Its intended lifetime is to be clean up by finalizer thread after Dispatcher is gone during process shutdown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope = "member", Target = "System.Windows.Media.FormattedText+LineEnumerator.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1199496
// Developer: toddt
// Reason: The PenThread just caches the last reference to the PenContext it saw and it is used for buffering input.
// I have code that I am checking in shortly that will be better about making sure we clear this cache reference
// when that PenContext is destroyed but we don't need to do anything to dispose this object in this class.
// It is all handled by the PenContexts class.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="System.Windows.Input.PenThread.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1182082
// Developer: niklasb
// Reason: This is an internal type, and adding operator== would just result in an FxCop bug for uncalled private code
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2231:OverrideOperatorEqualsOnOverridingValueTypeEquals", Scope = "member", Target = "MS.Internal.FontFace.FontFamilyIdentifier.Equals(System.Object):System.Boolean")]
[module: SuppressMessage("Microsoft.Usage", "CA2231:OverrideOperatorEqualsOnOverridingValueTypeEquals", Scope = "member", Target = "MS.Internal.FontFace.FontFamilyIdentifier.Equals(MS.Internal.FontFace.FontFamilyIdentifier):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1206720
// Developer: lblanco
// Reason: The MediaContext doesn't own the object referenced by the member variable in question (_notificationHandler), so
//         it shouldn't dispose it. The HwndTarget being referenced will always be disposed directly by the dispatcher if the
//         MediaContext is disposed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope = "member", Target = "System.Windows.Media.MediaContext.Dispose():System.Void")]


//**************************************************************************************************************************
// Bug IDs: 1309439, 1309440, 1309441, 1309442, 1309444, 1309445, 1309447, 1309448, 1309450, 1309451, 1309452, 1309453, 1309454, 1309455, 1309456, 1309457, 1309458
// Developer: gillesk
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Byte,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Byte,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ByteAnimation..ctor(System.Byte,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Color,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ColorAnimation..ctor(System.Windows.Media.Color,System.Windows.Media.Color,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Decimal,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Decimal,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DecimalAnimation..ctor(System.Decimal,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Double,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Double,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.DoubleAnimation..ctor(System.Double,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Int16,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Int16,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int16Animation..ctor(System.Int16,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Int32,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Int32,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int32Animation..ctor(System.Int32,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Int64,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Int64,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Int64Animation..ctor(System.Int64,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Media3D.Point3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Point3DAnimation..ctor(System.Windows.Media.Media3D.Point3D,System.Windows.Media.Media3D.Point3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Point,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.PointAnimation..ctor(System.Windows.Point,System.Windows.Point,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Rect,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.RectAnimation..ctor(System.Windows.Rect,System.Windows.Rect,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Media3D.Rotation3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Rotation3DAnimation..ctor(System.Windows.Media.Media3D.Rotation3D,System.Windows.Media.Media3D.Rotation3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Single,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Single,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SingleAnimation..ctor(System.Single,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Size,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SizeAnimation..ctor(System.Windows.Size,System.Windows.Size,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Timeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Timeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Duration,System.Windows.Media.Animation.RepeatBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Media3D.Vector3D,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.Vector3DAnimation..ctor(System.Windows.Media.Media3D.Vector3D,System.Windows.Media.Media3D.Vector3D,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Vector,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.VectorAnimation..ctor(System.Windows.Vector,System.Windows.Vector,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]

//**************************************************************************************************************************
// Bug ID: 1340796, 1386614, 1386615, 1587069, 1587068
// Developer: samgeo
// Reason: We are instantiating the exception correctly.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Ink.Stroke.Draw(System.Windows.Media.DrawingContext,System.Windows.Ink.DrawingAttributes):System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Input.StylusPointCollection..ctor(System.Windows.Input.StylusPointDescription)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Input.StylusPointDescription.AreCompatible(System.Windows.Input.StylusPointDescription,System.Windows.Input.StylusPointDescription):System.Boolean")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.ExtendedPropertySerializer.DecodeAsISF(System.IO.Stream,System.UInt32,MS.Internal.Ink.InkSerializedFormat.GuidList,MS.Internal.Ink.InkSerializedFormat.KnownTagCache+KnownTagIndex,System.Guid&,System.Object&):System.UInt32")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.DrawingAttributeSerializer.DecodeAsISF(System.IO.Stream,MS.Internal.Ink.InkSerializedFormat.GuidList,System.UInt32,System.Windows.Ink.DrawingAttributes):System.UInt32")]

//**************************************************************************************************************************
// Bug ID: 1335602
// Developer: tmulcahy
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Duration,System.Windows.Media.Animation.RepeatBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor(System.ComponentModel.ITypeDescriptorContext,System.Uri)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor(System.Nullable`1<System.TimeSpan>)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor(System.Nullable`1<System.TimeSpan>,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor(System.Uri)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.MediaTimeline..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1070233
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.DrawingAttributeSerializer.DecodeAsISF(System.IO.MemoryStream,MS.Internal.Ink.InkSerializedFormat.GuidList,System.UInt32,System.Windows.Ink.DrawingAttributes):System.UInt32")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Ink.InkSerializedFormat.ExtendedPropertySerializer.DecodeAsISF(System.IO.MemoryStream,System.UInt32,MS.Internal.Ink.InkSerializedFormat.GuidList,MS.Internal.Ink.InkSerializedFormat.KnownTagCache+KnownTagIndex,System.Guid&,System.Object&):System.UInt32")]

//**************************************************************************************************************************
// Bug IDs: 1388909, 1388910 1388911
// Developer: cedricd
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.QuaternionAnimation..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.QuaternionAnimation..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Media3D.Quaternion,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.QuaternionAnimation..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.QuaternionAnimation..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Media3D.Quaternion,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.QuaternionKeyFrame..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.QuaternionKeyFrame..ctor(System.Windows.Media.Media3D.Quaternion)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineQuaternionKeyFrame..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineQuaternionKeyFrame..ctor(System.Windows.Media.Media3D.Quaternion)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineQuaternionKeyFrame..ctor(System.Windows.Media.Media3D.Quaternion,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]

//**************************************************************************************************************************
// Bug ID: 1539509
// Developer: lblanco
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Win32.NativeMethods+BitmapHandle..ctor(System.IntPtr,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 1679737
// Developer: adsmith
// Reason: The virtual method being called is internal only.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.FreezableCollection`1..ctor(System.Collections.Generic.IEnumerable`1<T>)")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 121633
// Developer: psarrett
// Reason: The last reference to a string was removed, but we're not allowed to modify the string table for 3.5
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.PresentationCore.SRID.get_DataObject_HasInvalidDataForOleDataObject():MS.Internal.PresentationCore.SRID")]


//**************************************************************************************************************************
// Bug IDs: 140708
// Developer: sambent
// Reason: A new ShutDownListener is "used" because it listens to events like AppDomain.DomainUnload.
//      The reference implicit in the event's delegate list keeps the object alive.  FxCop doesn't understand this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="System.Windows.Input.TextServicesContext.#.ctor()", MessageId="System.Windows.Input.TextServicesContext+TextServicesContextShutDownListener")]


//**************************************************************************************************************************
// Bug IDs: 199045, 199068
// Developer: brandf
// Reason: AddRef increments the reference count and returns the current count, but we dont care about the current count.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Media.Imaging.WriteableBitmap.#UpdateBitmapSourceResource(System.Windows.Media.Composition.DUCE+Channel,System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Interop.D3DImage.#UpdateResource(System.Windows.Media.Composition.DUCE+Channel,System.Boolean)")]

//**************************************************************************************************************************
// Bug IDs: 692903
// Developer: BrenClar
// Reason: BitmapDecoder does not implement IDisposable.  It only uses its finalizer to close the URI stream, and uses 
//         GC.SuppressFinalize to avoid this call when unnecessary.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.#.ctor(System.IO.Stream,System.Windows.Media.Imaging.BitmapCreateOptions,System.Windows.Media.Imaging.BitmapCacheOption,System.Guid)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.#.ctor(System.Uri,System.Windows.Media.Imaging.BitmapCreateOptions,System.Windows.Media.Imaging.BitmapCacheOption,System.Guid)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.#.ctor(System.Windows.Media.SafeMILHandle,System.Windows.Media.Imaging.BitmapDecoder,System.Uri,System.Uri,System.IO.Stream,System.Windows.Media.Imaging.BitmapCreateOptions,System.Windows.Media.Imaging.BitmapCacheOption,System.Boolean,System.Boolean,System.IO.Stream,System.IO.UnmanagedMemoryStream,Microsoft.Win32.SafeHandles.SafeFileHandle)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Media.Imaging.BitmapDecoder.#CloseStream()")]

//**************************************************************************************************************************
// Bug IDs: 692903
// Developer: BrenClar
// Reason: D3DImage does not implement IDisposable.  It only uses its finalizer to release its reference to the user's
//         surface, and uses GC.SuppressFinalize to avoid this call when unnecessary.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Interop.D3DImage.#MS.Internal.IAppDomainShutdownListener.NotifyShutdown()")]


//**************************************************************************************************************************
// Bug IDs: 692903
// Developer: BrenClar
// Reason: AddRef and Release just return the ref count, not an HRESULT, and we don't care about the ref count in many places.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.#ReleaseInterface(System.IntPtr&)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeReversePInvokeWrapper)", Scope="member", Target="System.Windows.Media.GlyphCache.#SendCallbackEntryPoint()")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Media.MediaPlayerState.#SendMediaPlayerCommand(System.Windows.Media.Composition.DUCE+Channel,System.Windows.Media.Composition.DUCE+ResourceHandle,System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata.#InitializeFromBlockWriter(System.Guid,System.Boolean,System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata.#InitializeFromBlockWriter(System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter,System.Object)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata.#InitializeFromMetadataWriter(System.Windows.Media.SafeMILHandle,System.Object)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.#.ctor(System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter,System.Object)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.#AddWriter(System.IntPtr)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.#InitializeFromBlockReader(System.IntPtr)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Media.Imaging.BitmapMetadata+BitmapMetadataBlockWriter.#SetWriterByIndex(System.UInt32,System.IntPtr)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.AddRef(System.Windows.Media.SafeMILHandle)", Scope="member", Target="System.Windows.Media.Imaging.BitmapSource.#get_DUCECompatiblePtr()")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.PresentationCore.UnsafeNativeMethods+MILUnknown.Release(System.IntPtr)", Scope="member", Target="System.Windows.Media.Imaging.PROPVARIANT.#Clear()")]

//**************************************************************************************************************************
// Bug IDs: 784238
// Developer: BChapman
// Reason: The GCNotificationToken object is created only for the executing code to be notified of a GC and is intended to be released immediately.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "MS.Internal.PresentationCore.GCNotificationToken", Scope = "member", Target = "MS.Internal.PresentationCore.GCNotificationToken.#RegisterCallback(System.Threading.WaitCallback,System.Object)", Justification = "The GCNotificationToken object is created only for the executing code to be notified of a GC and is intended to be released immediately")]