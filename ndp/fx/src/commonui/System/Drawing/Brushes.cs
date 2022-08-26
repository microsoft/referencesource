//------------------------------------------------------------------------------
// <copyright file="Brushes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.Versioning;

    /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes"]/*' />
    /// <devdoc>
    ///     Brushes for all the standard colors.
    /// </devdoc>
    public sealed class Brushes {
        static readonly object TransparentKey = new object();
        static readonly object AliceBlueKey = new object();
        static readonly object AntiqueWhiteKey = new object();
        static readonly object AquaKey = new object();
        static readonly object AquamarineKey = new object();
        static readonly object AzureKey = new object();
        static readonly object BeigeKey = new object();
        static readonly object BisqueKey = new object();
        static readonly object BlackKey = new object();
        static readonly object BlanchedAlmondKey = new object();
        static readonly object BlueKey = new object();
        static readonly object BlueVioletKey = new object();
        static readonly object BrownKey = new object();
        static readonly object BurlyWoodKey = new object();
        static readonly object CadetBlueKey = new object();
        static readonly object ChartreuseKey = new object();
        static readonly object ChocolateKey = new object();
        static readonly object ChoralKey = new object();
        static readonly object CornflowerBlueKey = new object();
        static readonly object CornsilkKey = new object();
        static readonly object CrimsonKey = new object();
        static readonly object CyanKey = new object();
        static readonly object DarkBlueKey = new object();
        static readonly object DarkCyanKey = new object();
        static readonly object DarkGoldenrodKey = new object();
        static readonly object DarkGrayKey = new object();
        static readonly object DarkGreenKey = new object();
        static readonly object DarkKhakiKey = new object();
        static readonly object DarkMagentaKey = new object();
        static readonly object DarkOliveGreenKey = new object();
        static readonly object DarkOrangeKey = new object();
        static readonly object DarkOrchidKey = new object();
        static readonly object DarkRedKey = new object();
        static readonly object DarkSalmonKey = new object();
        static readonly object DarkSeaGreenKey = new object();
        static readonly object DarkSlateBlueKey = new object();
        static readonly object DarkSlateGrayKey = new object();
        static readonly object DarkTurquoiseKey = new object();
        static readonly object DarkVioletKey = new object();
        static readonly object DeepPinkKey = new object();
        static readonly object DeepSkyBlueKey = new object();
        static readonly object DimGrayKey = new object();
        static readonly object DodgerBlueKey = new object();
        static readonly object FirebrickKey = new object();
        static readonly object FloralWhiteKey = new object();
        static readonly object ForestGreenKey = new object();
        static readonly object FuchiaKey = new object();
        static readonly object GainsboroKey = new object();
        static readonly object GhostWhiteKey = new object();
        static readonly object GoldKey = new object();
        static readonly object GoldenrodKey = new object();
        static readonly object GrayKey = new object();
        static readonly object GreenKey = new object();
        static readonly object GreenYellowKey = new object();
        static readonly object HoneydewKey = new object();
        static readonly object HotPinkKey = new object();
        static readonly object IndianRedKey = new object();
        static readonly object IndigoKey = new object();
        static readonly object IvoryKey = new object();
        static readonly object KhakiKey = new object();
        static readonly object LavenderKey = new object();
        static readonly object LavenderBlushKey = new object();
        static readonly object LawnGreenKey = new object();
        static readonly object LemonChiffonKey = new object();
        static readonly object LightBlueKey = new object();
        static readonly object LightCoralKey = new object();
        static readonly object LightCyanKey = new object();
        static readonly object LightGoldenrodYellowKey = new object();
        static readonly object LightGreenKey = new object();
        static readonly object LightGrayKey = new object();
        static readonly object LightPinkKey = new object();
        static readonly object LightSalmonKey = new object();
        static readonly object LightSeaGreenKey = new object();
        static readonly object LightSkyBlueKey = new object();
        static readonly object LightSlateGrayKey = new object();
        static readonly object LightSteelBlueKey = new object();
        static readonly object LightYellowKey = new object();
        static readonly object LimeKey = new object();
        static readonly object LimeGreenKey = new object();
        static readonly object LinenKey = new object();
        static readonly object MagentaKey = new object();
        static readonly object MaroonKey = new object();
        static readonly object MediumAquamarineKey = new object();
        static readonly object MediumBlueKey = new object();
        static readonly object MediumOrchidKey = new object();
        static readonly object MediumPurpleKey = new object();
        static readonly object MediumSeaGreenKey = new object();
        static readonly object MediumSlateBlueKey = new object();
        static readonly object MediumSpringGreenKey = new object();
        static readonly object MediumTurquoiseKey = new object();
        static readonly object MediumVioletRedKey = new object();
        static readonly object MidnightBlueKey = new object();
        static readonly object MintCreamKey = new object();
        static readonly object MistyRoseKey = new object();
        static readonly object MoccasinKey = new object();
        static readonly object NavajoWhiteKey = new object();
        static readonly object NavyKey = new object();
        static readonly object OldLaceKey = new object();
        static readonly object OliveKey = new object();
        static readonly object OliveDrabKey = new object();
        static readonly object OrangeKey = new object();
        static readonly object OrangeRedKey = new object();
        static readonly object OrchidKey = new object();
        static readonly object PaleGoldenrodKey = new object();
        static readonly object PaleGreenKey = new object();
        static readonly object PaleTurquoiseKey = new object();
        static readonly object PaleVioletRedKey = new object();
        static readonly object PapayaWhipKey = new object();
        static readonly object PeachPuffKey = new object();
        static readonly object PeruKey = new object();
        static readonly object PinkKey = new object();
        static readonly object PlumKey = new object();
        static readonly object PowderBlueKey = new object();
        static readonly object PurpleKey = new object();
        static readonly object RedKey = new object();
        static readonly object RosyBrownKey = new object();
        static readonly object RoyalBlueKey = new object();
        static readonly object SaddleBrownKey = new object();
        static readonly object SalmonKey = new object();
        static readonly object SandyBrownKey = new object();
        static readonly object SeaGreenKey = new object();
        static readonly object SeaShellKey = new object();
        static readonly object SiennaKey = new object();
        static readonly object SilverKey = new object();
        static readonly object SkyBlueKey = new object();
        static readonly object SlateBlueKey = new object();
        static readonly object SlateGrayKey = new object();
        static readonly object SnowKey = new object();
        static readonly object SpringGreenKey = new object();
        static readonly object SteelBlueKey = new object();
        static readonly object TanKey = new object();
        static readonly object TealKey = new object();
        static readonly object ThistleKey = new object();
        static readonly object TomatoKey = new object();
        static readonly object TurquoiseKey = new object();
        static readonly object VioletKey = new object();
        static readonly object WheatKey = new object();
        static readonly object WhiteKey = new object();
        static readonly object WhiteSmokeKey = new object();
        static readonly object YellowKey = new object();
        static readonly object YellowGreenKey = new object();

        private Brushes() {
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Transparent"]/*' />
        /// <devdoc>
        ///    A transparent brush.
        /// </devdoc>
        public static Brush Transparent {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush transparent = (Brush)SafeNativeMethods.Gdip.ThreadData[TransparentKey];
                if (transparent == null) {
                    transparent = new SolidBrush(Color.Transparent);
                    SafeNativeMethods.Gdip.ThreadData[TransparentKey] = transparent;
                }
                return transparent;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.AliceBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush AliceBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush aliceBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[AliceBlueKey];
                if (aliceBlue == null) {
                    aliceBlue = new SolidBrush(Color.AliceBlue);
                    SafeNativeMethods.Gdip.ThreadData[AliceBlueKey] = aliceBlue;
                }
                return aliceBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.AntiqueWhite"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush AntiqueWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush antiqueWhite = (Brush)SafeNativeMethods.Gdip.ThreadData[AntiqueWhiteKey];
                if (antiqueWhite == null) {
                    antiqueWhite = new SolidBrush(Color.AntiqueWhite);
                    SafeNativeMethods.Gdip.ThreadData[AntiqueWhiteKey] = antiqueWhite;
                }
                return antiqueWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Aqua"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Aqua {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush aqua = (Brush)SafeNativeMethods.Gdip.ThreadData[AquaKey];
                if (aqua == null) {
                    aqua = new SolidBrush(Color.Aqua);
                    SafeNativeMethods.Gdip.ThreadData[AquaKey] = aqua;
                }
                return aqua;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Aquamarine"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Aquamarine {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush aquamarine = (Brush)SafeNativeMethods.Gdip.ThreadData[AquamarineKey];
                if (aquamarine == null) {
                    aquamarine = new SolidBrush(Color.Aquamarine);
                    SafeNativeMethods.Gdip.ThreadData[AquamarineKey] = aquamarine;
                }
                return aquamarine;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Azure"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Azure {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush azure = (Brush)SafeNativeMethods.Gdip.ThreadData[AzureKey];
                if (azure == null) {
                    azure = new SolidBrush(Color.Azure);
                    SafeNativeMethods.Gdip.ThreadData[AzureKey] = azure;
                }
                return azure;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Beige"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Beige {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush beige = (Brush)SafeNativeMethods.Gdip.ThreadData[BeigeKey];
                if (beige == null) {
                    beige = new SolidBrush(Color.Beige);
                    SafeNativeMethods.Gdip.ThreadData[BeigeKey] = beige;
                }
                return beige;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Bisque"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Bisque {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush bisque = (Brush)SafeNativeMethods.Gdip.ThreadData[BisqueKey];
                if (bisque == null) {
                    bisque = new SolidBrush(Color.Bisque);
                    SafeNativeMethods.Gdip.ThreadData[BisqueKey] = bisque;
                }
                return bisque;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Black"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Black {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush black = (Brush)SafeNativeMethods.Gdip.ThreadData[BlackKey];
                if (black == null) {
                    black = new SolidBrush(Color.Black);
                    SafeNativeMethods.Gdip.ThreadData[BlackKey] = black;
                }
                return black;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BlanchedAlmond"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush BlanchedAlmond {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush blanchedAlmond = (Brush)SafeNativeMethods.Gdip.ThreadData[BlanchedAlmondKey];
                if (blanchedAlmond == null) {
                    blanchedAlmond = new SolidBrush(Color.BlanchedAlmond);
                    SafeNativeMethods.Gdip.ThreadData[BlanchedAlmondKey] = blanchedAlmond;
                }
                return blanchedAlmond;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Blue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Blue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush blue = (Brush)SafeNativeMethods.Gdip.ThreadData[BlueKey];
                if (blue == null) {
                    blue = new SolidBrush(Color.Blue);
                    SafeNativeMethods.Gdip.ThreadData[BlueKey] = blue;
                }
                return blue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BlueViolet"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush BlueViolet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush blueViolet = (Brush)SafeNativeMethods.Gdip.ThreadData[BlueVioletKey];
                if (blueViolet == null) {
                    blueViolet = new SolidBrush(Color.BlueViolet);
                    SafeNativeMethods.Gdip.ThreadData[BlueVioletKey] = blueViolet;
                }
                return blueViolet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Brown"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Brown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush brown = (Brush)SafeNativeMethods.Gdip.ThreadData[BrownKey];
                if (brown == null) {
                    brown = new SolidBrush(Color.Brown);
                    SafeNativeMethods.Gdip.ThreadData[BrownKey] = brown;
                }
                return brown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.BurlyWood"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush BurlyWood {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush burlyWood = (Brush)SafeNativeMethods.Gdip.ThreadData[BurlyWoodKey];
                if (burlyWood == null) {
                    burlyWood = new SolidBrush(Color.BurlyWood);
                    SafeNativeMethods.Gdip.ThreadData[BurlyWoodKey] = burlyWood;
                }
                return burlyWood;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.CadetBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush CadetBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush cadetBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[CadetBlueKey];
                if (cadetBlue == null) {
                    cadetBlue = new SolidBrush(Color.CadetBlue);
                    SafeNativeMethods.Gdip.ThreadData[CadetBlueKey] = cadetBlue;
                }
                return cadetBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Chartreuse"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Chartreuse {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush chartreuse = (Brush)SafeNativeMethods.Gdip.ThreadData[ChartreuseKey];
                if (chartreuse == null) {
                    chartreuse = new SolidBrush(Color.Chartreuse);
                    SafeNativeMethods.Gdip.ThreadData[ChartreuseKey] = chartreuse;
                }
                return chartreuse;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Chocolate"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Chocolate {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush chocolate = (Brush)SafeNativeMethods.Gdip.ThreadData[ChocolateKey];
                if (chocolate == null) {
                    chocolate = new SolidBrush(Color.Chocolate);
                    SafeNativeMethods.Gdip.ThreadData[ChocolateKey] = chocolate;
                }
                return chocolate;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Coral"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Coral {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush choral = (Brush)SafeNativeMethods.Gdip.ThreadData[ChoralKey];
                if (choral == null) {
                    choral = new SolidBrush(Color.Coral);
                    SafeNativeMethods.Gdip.ThreadData[ChoralKey] = choral;
                }
                return choral;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.CornflowerBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush CornflowerBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush cornflowerBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[CornflowerBlueKey];
                if (cornflowerBlue== null) {
                    cornflowerBlue = new SolidBrush(Color.CornflowerBlue);
                    SafeNativeMethods.Gdip.ThreadData[CornflowerBlueKey] = cornflowerBlue;
                }
                return cornflowerBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Cornsilk"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Cornsilk {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush cornsilk = (Brush)SafeNativeMethods.Gdip.ThreadData[CornsilkKey];
                if (cornsilk == null) {
                    cornsilk = new SolidBrush(Color.Cornsilk);
                    SafeNativeMethods.Gdip.ThreadData[CornsilkKey] = cornsilk;
                }
                return cornsilk;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Crimson"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Crimson {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush crimson = (Brush)SafeNativeMethods.Gdip.ThreadData[CrimsonKey];
                if (crimson == null) {
                    crimson = new SolidBrush(Color.Crimson);
                    SafeNativeMethods.Gdip.ThreadData[CrimsonKey] = crimson;
                }
                return crimson;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Cyan"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Cyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush cyan = (Brush)SafeNativeMethods.Gdip.ThreadData[CyanKey];
                if (cyan == null) {
                    cyan = new SolidBrush(Color.Cyan);
                    SafeNativeMethods.Gdip.ThreadData[CyanKey] = cyan;
                }
                return cyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkBlueKey];
                if (darkBlue == null) {
                    darkBlue = new SolidBrush(Color.DarkBlue);
                    SafeNativeMethods.Gdip.ThreadData[DarkBlueKey] = darkBlue;
                }
                return darkBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkCyan"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkCyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkCyan = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkCyanKey];
                if (darkCyan == null) {
                    darkCyan = new SolidBrush(Color.DarkCyan);
                    SafeNativeMethods.Gdip.ThreadData[DarkCyanKey] = darkCyan;
                }
                return darkCyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGoldenrod"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkGoldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkGoldenrod = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkGoldenrodKey];
                if (darkGoldenrod == null) {
                    darkGoldenrod = new SolidBrush(Color.DarkGoldenrod);
                    SafeNativeMethods.Gdip.ThreadData[DarkGoldenrodKey] = darkGoldenrod;
                }
                return darkGoldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkGray = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkGrayKey];
                if (darkGray == null) {
                    darkGray = new SolidBrush(Color.DarkGray);
                    SafeNativeMethods.Gdip.ThreadData[DarkGrayKey] = darkGray;
                }
                return darkGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkGreenKey];
                if (darkGreen == null) {
                    darkGreen = new SolidBrush(Color.DarkGreen);
                    SafeNativeMethods.Gdip.ThreadData[DarkGreenKey] = darkGreen;
                }
                return darkGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkKhaki"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkKhaki {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkKhaki = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkKhakiKey];
                if (darkKhaki == null) {
                    darkKhaki = new SolidBrush(Color.DarkKhaki);
                    SafeNativeMethods.Gdip.ThreadData[DarkKhakiKey] = darkKhaki;
                }
                return darkKhaki;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkMagenta"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkMagenta {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkMagenta = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkMagentaKey];
                if (darkMagenta == null) {
                    darkMagenta = new SolidBrush(Color.DarkMagenta);
                    SafeNativeMethods.Gdip.ThreadData[DarkMagentaKey] = darkMagenta;
                }
                return darkMagenta;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOliveGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkOliveGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkOliveGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkOliveGreenKey];
                if (darkOliveGreen == null) {
                    darkOliveGreen = new SolidBrush(Color.DarkOliveGreen);
                    SafeNativeMethods.Gdip.ThreadData[DarkOliveGreenKey] = darkOliveGreen;
                }
                return darkOliveGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOrange"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkOrange {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkOrange = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkOrangeKey];
                if (darkOrange == null) {
                    darkOrange = new SolidBrush(Color.DarkOrange);
                    SafeNativeMethods.Gdip.ThreadData[DarkOrangeKey] = darkOrange;
                }
                return darkOrange;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkOrchid"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkOrchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkOrchid = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkOrchidKey];
                if (darkOrchid == null) {
                    darkOrchid = new SolidBrush(Color.DarkOrchid);
                    SafeNativeMethods.Gdip.ThreadData[DarkOrchidKey] = darkOrchid;
                }
                return darkOrchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkRed"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkRed = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkRedKey];
                if (darkRed == null) {
                    darkRed = new SolidBrush(Color.DarkRed);
                    SafeNativeMethods.Gdip.ThreadData[DarkRedKey] = darkRed;
                }
                return darkRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSalmon"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkSalmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkSalmon = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkSalmonKey];
                if (darkSalmon == null) {
                    darkSalmon = new SolidBrush(Color.DarkSalmon);
                    SafeNativeMethods.Gdip.ThreadData[DarkSalmonKey] = darkSalmon;
                }
                return darkSalmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSeaGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkSeaGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkSeaGreenKey];
                if (darkSeaGreen == null) {
                    darkSeaGreen = new SolidBrush(Color.DarkSeaGreen);
                    SafeNativeMethods.Gdip.ThreadData[DarkSeaGreenKey] = darkSeaGreen;
                }
                return darkSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSlateBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkSlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkSlateBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkSlateBlueKey];
                if (darkSlateBlue == null) {
                    darkSlateBlue = new SolidBrush(Color.DarkSlateBlue);
                    SafeNativeMethods.Gdip.ThreadData[DarkSlateBlueKey] = darkSlateBlue;
                }
                return darkSlateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkSlateGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkSlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkSlateGray = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkSlateGrayKey];
                if (darkSlateGray == null) {
                    darkSlateGray = new SolidBrush(Color.DarkSlateGray);
                    SafeNativeMethods.Gdip.ThreadData[DarkSlateGrayKey] = darkSlateGray;
                }
                return darkSlateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkTurquoise"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkTurquoise = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkTurquoiseKey];
                if (darkTurquoise == null) {
                    darkTurquoise = new SolidBrush(Color.DarkTurquoise);
                    SafeNativeMethods.Gdip.ThreadData[DarkTurquoiseKey] = darkTurquoise;
                }
                return darkTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DarkViolet"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DarkViolet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush darkViolet = (Brush)SafeNativeMethods.Gdip.ThreadData[DarkVioletKey];
                if (darkViolet == null) {
                    darkViolet = new SolidBrush(Color.DarkViolet);
                    SafeNativeMethods.Gdip.ThreadData[DarkVioletKey] = darkViolet;
                }
                return darkViolet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DeepPink"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DeepPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush deepPink = (Brush)SafeNativeMethods.Gdip.ThreadData[DeepPinkKey];
                if (deepPink == null) {
                    deepPink = new SolidBrush(Color.DeepPink);
                    SafeNativeMethods.Gdip.ThreadData[DeepPinkKey] = deepPink;
                }
                return deepPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DeepSkyBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DeepSkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush deepSkyBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[DeepSkyBlueKey];
                if (deepSkyBlue == null) {
                    deepSkyBlue = new SolidBrush(Color.DeepSkyBlue);
                    SafeNativeMethods.Gdip.ThreadData[DeepSkyBlueKey] = deepSkyBlue;
                }
                return deepSkyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DimGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DimGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush dimGray = (Brush)SafeNativeMethods.Gdip.ThreadData[DimGrayKey];
                if (dimGray == null) {
                    dimGray = new SolidBrush(Color.DimGray);
                    SafeNativeMethods.Gdip.ThreadData[DimGrayKey] = dimGray;
                }
                return dimGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.DodgerBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush DodgerBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush dodgerBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[DodgerBlueKey];
                if (dodgerBlue == null) {
                    dodgerBlue = new SolidBrush(Color.DodgerBlue);
                    SafeNativeMethods.Gdip.ThreadData[DodgerBlueKey] = dodgerBlue;
                }
                return dodgerBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Firebrick"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Firebrick {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush firebrick = (Brush)SafeNativeMethods.Gdip.ThreadData[FirebrickKey];
                if (firebrick == null) {
                    firebrick = new SolidBrush(Color.Firebrick);
                    SafeNativeMethods.Gdip.ThreadData[FirebrickKey] = firebrick;
                }
                return firebrick;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.FloralWhite"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush FloralWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush floralWhite = (Brush)SafeNativeMethods.Gdip.ThreadData[FloralWhiteKey];
                if (floralWhite == null) {
                    floralWhite = new SolidBrush(Color.FloralWhite);
                    SafeNativeMethods.Gdip.ThreadData[FloralWhiteKey] = floralWhite;
                }
                return floralWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.ForestGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush ForestGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush forestGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[ForestGreenKey];
                if (forestGreen == null) {
                    forestGreen = new SolidBrush(Color.ForestGreen);
                    SafeNativeMethods.Gdip.ThreadData[ForestGreenKey] = forestGreen;
                }
                return forestGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Fuchsia"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Fuchsia {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush fuchia = (Brush)SafeNativeMethods.Gdip.ThreadData[FuchiaKey];
                if (fuchia == null) {
                    fuchia = new SolidBrush(Color.Fuchsia);
                    SafeNativeMethods.Gdip.ThreadData[FuchiaKey] = fuchia;
                }
                return fuchia;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gainsboro"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Gainsboro {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush gainsboro = (Brush)SafeNativeMethods.Gdip.ThreadData[GainsboroKey];
                if (gainsboro == null) {
                    gainsboro = new SolidBrush(Color.Gainsboro);
                    SafeNativeMethods.Gdip.ThreadData[GainsboroKey] = gainsboro;
                }
                return gainsboro;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.GhostWhite"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush GhostWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush ghostWhite = (Brush)SafeNativeMethods.Gdip.ThreadData[GhostWhiteKey];
                if (ghostWhite == null) {
                    ghostWhite = new SolidBrush(Color.GhostWhite);
                    SafeNativeMethods.Gdip.ThreadData[GhostWhiteKey] = ghostWhite;
                }
                return ghostWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gold"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Gold {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush gold = (Brush)SafeNativeMethods.Gdip.ThreadData[GoldKey];
                if (gold == null) {
                    gold = new SolidBrush(Color.Gold);
                    SafeNativeMethods.Gdip.ThreadData[GoldKey] = gold;
                }
                return gold;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Goldenrod"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Goldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush goldenrod = (Brush)SafeNativeMethods.Gdip.ThreadData[GoldenrodKey];
                if (goldenrod == null) {
                    goldenrod = new SolidBrush(Color.Goldenrod);
                    SafeNativeMethods.Gdip.ThreadData[GoldenrodKey] = goldenrod;
                }
                return goldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Gray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Gray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush gray = (Brush)SafeNativeMethods.Gdip.ThreadData[GrayKey];
                if (gray == null) {
                    gray = new SolidBrush(Color.Gray);
                    SafeNativeMethods.Gdip.ThreadData[GrayKey] = gray;
                }
                return gray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Green"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Green {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush green = (Brush)SafeNativeMethods.Gdip.ThreadData[GreenKey];
                if (green == null) {
                    green = new SolidBrush(Color.Green);
                    SafeNativeMethods.Gdip.ThreadData[GreenKey] = green;
                }
                return green;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.GreenYellow"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush GreenYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush greenYellow = (Brush)SafeNativeMethods.Gdip.ThreadData[GreenYellowKey];
                if (greenYellow == null) {
                    greenYellow = new SolidBrush(Color.GreenYellow);
                    SafeNativeMethods.Gdip.ThreadData[GreenYellowKey] = greenYellow;
                }
                return greenYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Honeydew"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Honeydew {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush honeydew = (Brush)SafeNativeMethods.Gdip.ThreadData[HoneydewKey];
                if (honeydew == null) {
                    honeydew = new SolidBrush(Color.Honeydew);
                    SafeNativeMethods.Gdip.ThreadData[HoneydewKey] = honeydew;
                }
                return honeydew;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.HotPink"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush HotPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush hotPink = (Brush)SafeNativeMethods.Gdip.ThreadData[HotPinkKey];
                if (hotPink == null) {
                    hotPink = new SolidBrush(Color.HotPink);
                    SafeNativeMethods.Gdip.ThreadData[HotPinkKey] = hotPink;
                }
                return hotPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.IndianRed"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush IndianRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush indianRed = (Brush)SafeNativeMethods.Gdip.ThreadData[IndianRedKey];
                if (indianRed == null) {
                    indianRed = new SolidBrush(Color.IndianRed);
                    SafeNativeMethods.Gdip.ThreadData[IndianRedKey] = indianRed;
                }
                return indianRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Indigo"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Indigo {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush indigo = (Brush)SafeNativeMethods.Gdip.ThreadData[IndigoKey];
                if (indigo == null) {
                    indigo = new SolidBrush(Color.Indigo);
                    SafeNativeMethods.Gdip.ThreadData[IndigoKey] = indigo;
                }
                return indigo;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Ivory"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Ivory {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush ivory = (Brush)SafeNativeMethods.Gdip.ThreadData[IvoryKey];
                if (ivory == null) {
                    ivory = new SolidBrush(Color.Ivory);
                    SafeNativeMethods.Gdip.ThreadData[IvoryKey] = ivory;
                }
                return ivory;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Khaki"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Khaki {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush khaki = (Brush)SafeNativeMethods.Gdip.ThreadData[KhakiKey];
                if (khaki == null) {
                    khaki = new SolidBrush(Color.Khaki);
                    SafeNativeMethods.Gdip.ThreadData[KhakiKey] = khaki;
                }
                return khaki;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Lavender"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Lavender {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lavender = (Brush)SafeNativeMethods.Gdip.ThreadData[LavenderKey];
                if (lavender == null) {
                    lavender = new SolidBrush(Color.Lavender);
                    SafeNativeMethods.Gdip.ThreadData[LavenderKey] = lavender;
                }
                return lavender;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LavenderBlush"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LavenderBlush {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lavenderBlush = (Brush)SafeNativeMethods.Gdip.ThreadData[LavenderBlushKey];
                if (lavenderBlush == null) {
                    lavenderBlush = new SolidBrush(Color.LavenderBlush);
                    SafeNativeMethods.Gdip.ThreadData[LavenderBlushKey] = lavenderBlush;
                }
                return lavenderBlush;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LawnGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LawnGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lawnGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[LawnGreenKey];
                if (lawnGreen == null) {
                    lawnGreen = new SolidBrush(Color.LawnGreen);
                    SafeNativeMethods.Gdip.ThreadData[LawnGreenKey] = lawnGreen;
                }
                return lawnGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LemonChiffon"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LemonChiffon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lemonChiffon = (Brush)SafeNativeMethods.Gdip.ThreadData[LemonChiffonKey];
                if (lemonChiffon == null) {
                    lemonChiffon = new SolidBrush(Color.LemonChiffon);
                    SafeNativeMethods.Gdip.ThreadData[LemonChiffonKey] = lemonChiffon;
                }
                return lemonChiffon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[LightBlueKey];
                if (lightBlue == null) {
                    lightBlue = new SolidBrush(Color.LightBlue);
                    SafeNativeMethods.Gdip.ThreadData[LightBlueKey] = lightBlue;
                }
                return lightBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightCoral"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightCoral {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightCoral = (Brush)SafeNativeMethods.Gdip.ThreadData[LightCoralKey];
                if (lightCoral == null) {
                    lightCoral = new SolidBrush(Color.LightCoral);
                    SafeNativeMethods.Gdip.ThreadData[LightCoralKey] = lightCoral;
                }
                return lightCoral;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightCyan"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightCyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightCyan = (Brush)SafeNativeMethods.Gdip.ThreadData[LightCyanKey];
                if (lightCyan == null) {
                    lightCyan = new SolidBrush(Color.LightCyan);
                    SafeNativeMethods.Gdip.ThreadData[LightCyanKey] = lightCyan;
                }
                return lightCyan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGoldenrodYellow"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightGoldenrodYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightGoldenrodYellow = (Brush)SafeNativeMethods.Gdip.ThreadData[LightGoldenrodYellowKey];
                if (lightGoldenrodYellow == null) {
                    lightGoldenrodYellow = new SolidBrush(Color.LightGoldenrodYellow);
                    SafeNativeMethods.Gdip.ThreadData[LightGoldenrodYellowKey] = lightGoldenrodYellow;
                }
                return lightGoldenrodYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[LightGreenKey];
                if (lightGreen == null) {
                    lightGreen = new SolidBrush(Color.LightGreen);
                    SafeNativeMethods.Gdip.ThreadData[LightGreenKey] = lightGreen;
                }
                return lightGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightGray = (Brush)SafeNativeMethods.Gdip.ThreadData[LightGrayKey];
                if (lightGray == null) {
                    lightGray = new SolidBrush(Color.LightGray);
                    SafeNativeMethods.Gdip.ThreadData[LightGrayKey] = lightGray;
                }
                return lightGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightPink"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightPink = (Brush)SafeNativeMethods.Gdip.ThreadData[LightPinkKey];
                if (lightPink == null) {
                    lightPink = new SolidBrush(Color.LightPink);
                    SafeNativeMethods.Gdip.ThreadData[LightPinkKey] = lightPink;
                }
                return lightPink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSalmon"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightSalmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightSalmon = (Brush)SafeNativeMethods.Gdip.ThreadData[LightSalmonKey];
                if (lightSalmon == null) {
                    lightSalmon = new SolidBrush(Color.LightSalmon);
                    SafeNativeMethods.Gdip.ThreadData[LightSalmonKey] = lightSalmon;
                }
                return lightSalmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSeaGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightSeaGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[LightSeaGreenKey];
                if (lightSeaGreen == null) {
                    lightSeaGreen = new SolidBrush(Color.LightSeaGreen);
                    SafeNativeMethods.Gdip.ThreadData[LightSeaGreenKey] = lightSeaGreen;
                }
                return lightSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSkyBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightSkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightSkyBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[LightSkyBlueKey];
                if (lightSkyBlue == null) {
                    lightSkyBlue = new SolidBrush(Color.LightSkyBlue);
                    SafeNativeMethods.Gdip.ThreadData[LightSkyBlueKey] = lightSkyBlue;
                }
                return lightSkyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSlateGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightSlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightSlateGray = (Brush)SafeNativeMethods.Gdip.ThreadData[LightSlateGrayKey];
                if (lightSlateGray == null) {
                    lightSlateGray = new SolidBrush(Color.LightSlateGray);
                    SafeNativeMethods.Gdip.ThreadData[LightSlateGrayKey] = lightSlateGray;
                }
                return lightSlateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightSteelBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightSteelBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightSteelBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[LightSteelBlueKey];
                if (lightSteelBlue == null) {
                    lightSteelBlue = new SolidBrush(Color.LightSteelBlue);
                    SafeNativeMethods.Gdip.ThreadData[LightSteelBlueKey] = lightSteelBlue;
                }
                return lightSteelBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LightYellow"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LightYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lightYellow = (Brush)SafeNativeMethods.Gdip.ThreadData[LightYellowKey];
                if (lightYellow == null) {
                    lightYellow = new SolidBrush(Color.LightYellow);
                    SafeNativeMethods.Gdip.ThreadData[LightYellowKey] = lightYellow;
                }
                return lightYellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Lime"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Lime {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush lime = (Brush)SafeNativeMethods.Gdip.ThreadData[LimeKey];
                if (lime == null) {
                    lime = new SolidBrush(Color.Lime);
                    SafeNativeMethods.Gdip.ThreadData[LimeKey] = lime;
                }
                return lime;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.LimeGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush LimeGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush limeGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[LimeGreenKey];
                if (limeGreen == null) {
                    limeGreen = new SolidBrush(Color.LimeGreen);
                    SafeNativeMethods.Gdip.ThreadData[LimeGreenKey] = limeGreen;
                }
                return limeGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Linen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Linen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush linen = (Brush)SafeNativeMethods.Gdip.ThreadData[LinenKey];
                if (linen == null) {
                    linen = new SolidBrush(Color.Linen);
                    SafeNativeMethods.Gdip.ThreadData[LinenKey] = linen;
                }
                return linen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Magenta"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Magenta {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush magenta = (Brush)SafeNativeMethods.Gdip.ThreadData[MagentaKey];
                if (magenta == null) {
                    magenta = new SolidBrush(Color.Magenta);
                    SafeNativeMethods.Gdip.ThreadData[MagentaKey] = magenta;
                }
                return magenta;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Maroon"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Maroon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush maroon = (Brush)SafeNativeMethods.Gdip.ThreadData[MaroonKey];
                if (maroon == null) {
                    maroon = new SolidBrush(Color.Maroon);
                    SafeNativeMethods.Gdip.ThreadData[MaroonKey] = maroon;
                }
                return maroon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumAquamarine"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumAquamarine {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumAquamarine = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumAquamarineKey];
                if (mediumAquamarine == null) {
                    mediumAquamarine = new SolidBrush(Color.MediumAquamarine);
                    SafeNativeMethods.Gdip.ThreadData[MediumAquamarineKey] = mediumAquamarine;
                }
                return mediumAquamarine;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumBlueKey];
                if (mediumBlue == null) {
                    mediumBlue = new SolidBrush(Color.MediumBlue);
                    SafeNativeMethods.Gdip.ThreadData[MediumBlueKey] = mediumBlue;
                }
                return mediumBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumOrchid"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumOrchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumOrchid = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumOrchidKey];
                if (mediumOrchid == null) {
                    mediumOrchid = new SolidBrush(Color.MediumOrchid);
                    SafeNativeMethods.Gdip.ThreadData[MediumOrchidKey] = mediumOrchid;
                }
                return mediumOrchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumPurple"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumPurple {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumPurple = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumPurpleKey];
                if (mediumPurple == null) {
                    mediumPurple = new SolidBrush(Color.MediumPurple);
                    SafeNativeMethods.Gdip.ThreadData[MediumPurpleKey] = mediumPurple;
                }
                return mediumPurple;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSeaGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumSeaGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumSeaGreenKey];
                if (mediumSeaGreen == null) {
                    mediumSeaGreen = new SolidBrush(Color.MediumSeaGreen);
                    SafeNativeMethods.Gdip.ThreadData[MediumSeaGreenKey] = mediumSeaGreen;
                }
                return mediumSeaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSlateBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumSlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumSlateBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumSlateBlueKey];
                if (mediumSlateBlue == null) {
                    mediumSlateBlue = new SolidBrush(Color.MediumSlateBlue);
                    SafeNativeMethods.Gdip.ThreadData[MediumSlateBlueKey] = mediumSlateBlue;
                }
                return mediumSlateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumSpringGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumSpringGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumSpringGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumSpringGreenKey];
                if (mediumSpringGreen == null) {
                    mediumSpringGreen = new SolidBrush(Color.MediumSpringGreen);
                    SafeNativeMethods.Gdip.ThreadData[MediumSpringGreenKey] = mediumSpringGreen;
                }
                return mediumSpringGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumTurquoise"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumTurquoise = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumTurquoiseKey];
                if (mediumTurquoise == null) {
                    mediumTurquoise = new SolidBrush(Color.MediumTurquoise);
                    SafeNativeMethods.Gdip.ThreadData[MediumTurquoiseKey] = mediumTurquoise;
                }
                return mediumTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MediumVioletRed"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MediumVioletRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mediumVioletRed = (Brush)SafeNativeMethods.Gdip.ThreadData[MediumVioletRedKey];
                if (mediumVioletRed == null) {
                    mediumVioletRed = new SolidBrush(Color.MediumVioletRed);
                    SafeNativeMethods.Gdip.ThreadData[MediumVioletRedKey] = mediumVioletRed;
                }
                return mediumVioletRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MidnightBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MidnightBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush midnightBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[MidnightBlueKey];
                if (midnightBlue == null) {
                    midnightBlue = new SolidBrush(Color.MidnightBlue);
                    SafeNativeMethods.Gdip.ThreadData[MidnightBlueKey] = midnightBlue;
                }
                return midnightBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MintCream"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MintCream {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mintCream = (Brush)SafeNativeMethods.Gdip.ThreadData[MintCreamKey];
                if (mintCream == null) {
                    mintCream = new SolidBrush(Color.MintCream);
                    SafeNativeMethods.Gdip.ThreadData[MintCreamKey] = mintCream;
                }
                return mintCream;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.MistyRose"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush MistyRose {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush mistyRose = (Brush)SafeNativeMethods.Gdip.ThreadData[MistyRoseKey];
                if (mistyRose == null) {
                    mistyRose = new SolidBrush(Color.MistyRose);
                    SafeNativeMethods.Gdip.ThreadData[MistyRoseKey] = mistyRose;
                }
                return mistyRose;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Moccasin"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Moccasin {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush moccasin = (Brush)SafeNativeMethods.Gdip.ThreadData[MoccasinKey];
                if (moccasin == null) {
                    moccasin = new SolidBrush(Color.Moccasin);
                    SafeNativeMethods.Gdip.ThreadData[MoccasinKey] = moccasin;
                }
                return moccasin;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.NavajoWhite"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush NavajoWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush navajoWhite = (Brush)SafeNativeMethods.Gdip.ThreadData[NavajoWhiteKey];
                if (navajoWhite == null) {
                    navajoWhite = new SolidBrush(Color.NavajoWhite);
                    SafeNativeMethods.Gdip.ThreadData[NavajoWhiteKey] = navajoWhite;
                }
                return navajoWhite;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Navy"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Navy {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush navy = (Brush)SafeNativeMethods.Gdip.ThreadData[NavyKey];
                if (navy == null) {
                    navy = new SolidBrush(Color.Navy);
                    SafeNativeMethods.Gdip.ThreadData[NavyKey] = navy;
                }
                return navy;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OldLace"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush OldLace {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush oldLace = (Brush)SafeNativeMethods.Gdip.ThreadData[OldLaceKey];
                if (oldLace == null) {
                    oldLace = new SolidBrush(Color.OldLace);
                    SafeNativeMethods.Gdip.ThreadData[OldLaceKey] = oldLace;
                }
                return oldLace;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Olive"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Olive {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush olive = (Brush)SafeNativeMethods.Gdip.ThreadData[OliveKey];
                if (olive == null) {
                    olive = new SolidBrush(Color.Olive);
                    SafeNativeMethods.Gdip.ThreadData[OliveKey] = olive;
                }
                return olive;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OliveDrab"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush OliveDrab {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush oliveDrab = (Brush)SafeNativeMethods.Gdip.ThreadData[OliveDrabKey];
                if (oliveDrab == null) {
                    oliveDrab = new SolidBrush(Color.OliveDrab);
                    SafeNativeMethods.Gdip.ThreadData[OliveDrabKey] = oliveDrab;
                }
                return oliveDrab;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Orange"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Orange {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush orange = (Brush)SafeNativeMethods.Gdip.ThreadData[OrangeKey];
                if (orange == null) {
                    orange = new SolidBrush(Color.Orange);
                    SafeNativeMethods.Gdip.ThreadData[OrangeKey] = orange;
                }
                return orange;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.OrangeRed"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush OrangeRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush orangeRed = (Brush)SafeNativeMethods.Gdip.ThreadData[OrangeRedKey];
                if (orangeRed == null) {
                    orangeRed = new SolidBrush(Color.OrangeRed);
                    SafeNativeMethods.Gdip.ThreadData[OrangeRedKey] = orangeRed;
                }
                return orangeRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Orchid"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Orchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush orchid = (Brush)SafeNativeMethods.Gdip.ThreadData[OrchidKey];
                if (orchid == null) {
                    orchid = new SolidBrush(Color.Orchid);
                    SafeNativeMethods.Gdip.ThreadData[OrchidKey] = orchid;
                }
                return orchid;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleGoldenrod"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PaleGoldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush paleGoldenrod = (Brush)SafeNativeMethods.Gdip.ThreadData[PaleGoldenrodKey];
                if (paleGoldenrod == null) {
                    paleGoldenrod = new SolidBrush(Color.PaleGoldenrod);
                    SafeNativeMethods.Gdip.ThreadData[PaleGoldenrodKey] = paleGoldenrod;
                }
                return paleGoldenrod;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PaleGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush paleGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[PaleGreenKey];
                if (paleGreen == null) {
                    paleGreen = new SolidBrush(Color.PaleGreen);
                    SafeNativeMethods.Gdip.ThreadData[PaleGreenKey] = paleGreen;
                }
                return paleGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleTurquoise"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PaleTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush paleTurquoise = (Brush)SafeNativeMethods.Gdip.ThreadData[PaleTurquoiseKey];
                if (paleTurquoise == null) {
                    paleTurquoise = new SolidBrush(Color.PaleTurquoise);
                    SafeNativeMethods.Gdip.ThreadData[PaleTurquoiseKey] = paleTurquoise;
                }
                return paleTurquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PaleVioletRed"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PaleVioletRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush paleVioletRed = (Brush)SafeNativeMethods.Gdip.ThreadData[PaleVioletRedKey];
                if (paleVioletRed == null) {
                    paleVioletRed = new SolidBrush(Color.PaleVioletRed);
                    SafeNativeMethods.Gdip.ThreadData[PaleVioletRedKey] = paleVioletRed;
                }
                return paleVioletRed;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PapayaWhip"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PapayaWhip {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush papayaWhip = (Brush)SafeNativeMethods.Gdip.ThreadData[PapayaWhipKey];
                if (papayaWhip == null) {
                    papayaWhip = new SolidBrush(Color.PapayaWhip);
                    SafeNativeMethods.Gdip.ThreadData[PapayaWhipKey] = papayaWhip;
                }
                return papayaWhip;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PeachPuff"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PeachPuff {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush peachPuff = (Brush)SafeNativeMethods.Gdip.ThreadData[PeachPuffKey];
                if (peachPuff == null) {
                    peachPuff = new SolidBrush(Color.PeachPuff);
                    SafeNativeMethods.Gdip.ThreadData[PeachPuffKey] = peachPuff;
                }
                return peachPuff;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Peru"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Peru {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush peru = (Brush)SafeNativeMethods.Gdip.ThreadData[PeruKey];
                if (peru == null) {
                    peru = new SolidBrush(Color.Peru);
                    SafeNativeMethods.Gdip.ThreadData[PeruKey] = peru;
                }
                return peru;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Pink"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Pink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush pink = (Brush)SafeNativeMethods.Gdip.ThreadData[PinkKey];
                if (pink == null) {
                    pink = new SolidBrush(Color.Pink);
                    SafeNativeMethods.Gdip.ThreadData[PinkKey] = pink;
                }
                return pink;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Plum"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Plum {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush plum = (Brush)SafeNativeMethods.Gdip.ThreadData[PlumKey];
                if (plum == null) {
                    plum = new SolidBrush(Color.Plum);
                    SafeNativeMethods.Gdip.ThreadData[PlumKey] = plum;
                }
                return plum;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.PowderBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush PowderBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush powderBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[PowderBlueKey];
                if (powderBlue == null) {
                    powderBlue = new SolidBrush(Color.PowderBlue);
                    SafeNativeMethods.Gdip.ThreadData[PowderBlueKey] = powderBlue;
                }
                return powderBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Purple"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Purple {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush purple = (Brush)SafeNativeMethods.Gdip.ThreadData[PurpleKey];
                if (purple == null) {
                    purple = new SolidBrush(Color.Purple);
                    SafeNativeMethods.Gdip.ThreadData[PurpleKey] = purple;
                }
                return purple;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Red"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Red {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush red = (Brush)SafeNativeMethods.Gdip.ThreadData[RedKey];
                if (red == null) {
                    red = new SolidBrush(Color.Red);
                    SafeNativeMethods.Gdip.ThreadData[RedKey] = red;
                }
                return red;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.RosyBrown"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush RosyBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush rosyBrown = (Brush)SafeNativeMethods.Gdip.ThreadData[RosyBrownKey];
                if (rosyBrown == null) {
                    rosyBrown = new SolidBrush(Color.RosyBrown);
                    SafeNativeMethods.Gdip.ThreadData[RosyBrownKey] = rosyBrown;
                }
                return rosyBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.RoyalBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush RoyalBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush royalBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[RoyalBlueKey];
                if (royalBlue == null) {
                    royalBlue = new SolidBrush(Color.RoyalBlue);
                    SafeNativeMethods.Gdip.ThreadData[RoyalBlueKey] = royalBlue;
                }
                return royalBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SaddleBrown"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SaddleBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush saddleBrown = (Brush)SafeNativeMethods.Gdip.ThreadData[SaddleBrownKey];
                if (saddleBrown == null) {
                    saddleBrown = new SolidBrush(Color.SaddleBrown);
                    SafeNativeMethods.Gdip.ThreadData[SaddleBrownKey] = saddleBrown;
                }
                return saddleBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Salmon"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Salmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush salmon = (Brush)SafeNativeMethods.Gdip.ThreadData[SalmonKey];
                if (salmon == null) {
                    salmon = new SolidBrush(Color.Salmon);
                    SafeNativeMethods.Gdip.ThreadData[SalmonKey] = salmon;
                }
                return salmon;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SandyBrown"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SandyBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush sandyBrown = (Brush)SafeNativeMethods.Gdip.ThreadData[SandyBrownKey];
                if (sandyBrown == null) {
                    sandyBrown = new SolidBrush(Color.SandyBrown);
                    SafeNativeMethods.Gdip.ThreadData[SandyBrownKey] = sandyBrown;
                }
                return sandyBrown;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SeaGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush seaGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[SeaGreenKey];
                if (seaGreen == null) {
                    seaGreen = new SolidBrush(Color.SeaGreen);
                    SafeNativeMethods.Gdip.ThreadData[SeaGreenKey] = seaGreen;
                }
                return seaGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SeaShell"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SeaShell {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush seaShell = (Brush)SafeNativeMethods.Gdip.ThreadData[SeaShellKey];
                if (seaShell == null) {
                    seaShell = new SolidBrush(Color.SeaShell);
                    SafeNativeMethods.Gdip.ThreadData[SeaShellKey] = seaShell;
                }
                return seaShell;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Sienna"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Sienna {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush sienna = (Brush)SafeNativeMethods.Gdip.ThreadData[SiennaKey];
                if (sienna == null) {
                    sienna = new SolidBrush(Color.Sienna);
                    SafeNativeMethods.Gdip.ThreadData[SiennaKey] = sienna;
                }
                return sienna;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Silver"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Silver {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush silver = (Brush)SafeNativeMethods.Gdip.ThreadData[SilverKey];
                if (silver == null) {
                    silver = new SolidBrush(Color.Silver);
                    SafeNativeMethods.Gdip.ThreadData[SilverKey] = silver;
                }
                return silver;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SkyBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush skyBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[SkyBlueKey];
                if (skyBlue == null) {
                    skyBlue = new SolidBrush(Color.SkyBlue);
                    SafeNativeMethods.Gdip.ThreadData[SkyBlueKey] = skyBlue;
                }
                return skyBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SlateBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush slateBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[SlateBlueKey];
                if (slateBlue == null) {
                    slateBlue = new SolidBrush(Color.SlateBlue);
                    SafeNativeMethods.Gdip.ThreadData[SlateBlueKey] = slateBlue;
                }
                return slateBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SlateGray"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush slateGray = (Brush)SafeNativeMethods.Gdip.ThreadData[SlateGrayKey];
                if (slateGray == null) {
                    slateGray = new SolidBrush(Color.SlateGray);
                    SafeNativeMethods.Gdip.ThreadData[SlateGrayKey] = slateGray;
                }
                return slateGray;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Snow"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Snow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush snow = (Brush)SafeNativeMethods.Gdip.ThreadData[SnowKey];
                if (snow == null) {
                    snow = new SolidBrush(Color.Snow);
                    SafeNativeMethods.Gdip.ThreadData[SnowKey] = snow;
                }
                return snow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SpringGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SpringGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush springGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[SpringGreenKey];
                if (springGreen == null) {
                    springGreen = new SolidBrush(Color.SpringGreen);
                    SafeNativeMethods.Gdip.ThreadData[SpringGreenKey] = springGreen;
                }
                return springGreen;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.SteelBlue"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush SteelBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush steelBlue = (Brush)SafeNativeMethods.Gdip.ThreadData[SteelBlueKey];
                if (steelBlue == null) {
                    steelBlue = new SolidBrush(Color.SteelBlue);
                    SafeNativeMethods.Gdip.ThreadData[SteelBlueKey] = steelBlue;
                }
                return steelBlue;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Tan"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Tan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush tan = (Brush)SafeNativeMethods.Gdip.ThreadData[TanKey];
                if (tan == null) {
                    tan = new SolidBrush(Color.Tan);
                    SafeNativeMethods.Gdip.ThreadData[TanKey] = tan;
                }
                return tan;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Teal"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Teal {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush teal = (Brush)SafeNativeMethods.Gdip.ThreadData[TealKey];
                if (teal == null) {
                    teal = new SolidBrush(Color.Teal);
                    SafeNativeMethods.Gdip.ThreadData[TealKey] = teal;
                }
                return teal;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Thistle"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Thistle {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush thistle = (Brush)SafeNativeMethods.Gdip.ThreadData[ThistleKey];
                if (thistle == null) {
                    thistle = new SolidBrush(Color.Thistle);
                    SafeNativeMethods.Gdip.ThreadData[ThistleKey] = thistle;
                }
                return thistle;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Tomato"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Tomato {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush tomato = (Brush)SafeNativeMethods.Gdip.ThreadData[TomatoKey];
                if (tomato == null) {
                    tomato = new SolidBrush(Color.Tomato);
                    SafeNativeMethods.Gdip.ThreadData[TomatoKey] = tomato;
                }
                return tomato;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Turquoise"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Turquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush turquoise = (Brush)SafeNativeMethods.Gdip.ThreadData[TurquoiseKey];
                if (turquoise == null) {
                    turquoise = new SolidBrush(Color.Turquoise);
                    SafeNativeMethods.Gdip.ThreadData[TurquoiseKey] = turquoise;
                }
                return turquoise;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Violet"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Violet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush violet = (Brush)SafeNativeMethods.Gdip.ThreadData[VioletKey];
                if (violet == null) {
                    violet = new SolidBrush(Color.Violet);
                    SafeNativeMethods.Gdip.ThreadData[VioletKey] = violet;
                }
                return violet;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Wheat"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Wheat {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush wheat = (Brush)SafeNativeMethods.Gdip.ThreadData[WheatKey];
                if (wheat == null) {
                    wheat = new SolidBrush(Color.Wheat);
                    SafeNativeMethods.Gdip.ThreadData[WheatKey] = wheat;
                }
                return wheat;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.White"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush White {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush white = (Brush)SafeNativeMethods.Gdip.ThreadData[WhiteKey];
                if (white == null) {
                    white = new SolidBrush(Color.White);
                    SafeNativeMethods.Gdip.ThreadData[WhiteKey] = white;
                }
                return white;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.WhiteSmoke"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush WhiteSmoke {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush whiteSmoke = (Brush)SafeNativeMethods.Gdip.ThreadData[WhiteSmokeKey];
                if (whiteSmoke == null) {
                    whiteSmoke = new SolidBrush(Color.WhiteSmoke);
                    SafeNativeMethods.Gdip.ThreadData[WhiteSmokeKey] = whiteSmoke;
                }
                return whiteSmoke;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.Yellow"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush Yellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush yellow = (Brush)SafeNativeMethods.Gdip.ThreadData[YellowKey];
                if (yellow == null) {
                    yellow = new SolidBrush(Color.Yellow);
                    SafeNativeMethods.Gdip.ThreadData[YellowKey] = yellow;
                }
                return yellow;
            }
        }

        /// <include file='doc\Brushes.uex' path='docs/doc[@for="Brushes.YellowGreen"]/*' />
        /// <devdoc>
        ///    A brush of the given color.
        /// </devdoc>
        public static Brush YellowGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {
                Brush yellowGreen = (Brush)SafeNativeMethods.Gdip.ThreadData[YellowGreenKey];
                if (yellowGreen == null) {
                    yellowGreen = new SolidBrush(Color.YellowGreen);
                    SafeNativeMethods.Gdip.ThreadData[YellowGreenKey] = yellowGreen;
                }
                return yellowGreen;
            }
        }
    }
}

