//------------------------------------------------------------------------------
// <copyright file="Pens.cs" company="Microsoft">
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


    /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens"]/*' />
    /// <devdoc>
    ///     Pens for all the standard colors.
    /// </devdoc>
    public sealed class Pens {
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

        private Pens() {
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Transparent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Transparent {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen transparent = (Pen)SafeNativeMethods.Gdip.ThreadData[TransparentKey];
                if (transparent == null) {
                    transparent = new Pen(Color.Transparent, true);
                    SafeNativeMethods.Gdip.ThreadData[TransparentKey] = transparent;
                }
                return transparent;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.AliceBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen AliceBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen aliceBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[AliceBlueKey];
                if (aliceBlue == null) {
                    aliceBlue = new Pen(Color.AliceBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[AliceBlueKey] = aliceBlue;
                }
                return aliceBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.AntiqueWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen AntiqueWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen antiqueWhite = (Pen)SafeNativeMethods.Gdip.ThreadData[AntiqueWhiteKey];
                if (antiqueWhite == null) {
                    antiqueWhite = new Pen(Color.AntiqueWhite, true);
                    SafeNativeMethods.Gdip.ThreadData[AntiqueWhiteKey] = antiqueWhite;
                }
                return antiqueWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Aqua"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Aqua {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen aqua = (Pen)SafeNativeMethods.Gdip.ThreadData[AquaKey];
                if (aqua == null) {
                    aqua = new Pen(Color.Aqua, true);
                    SafeNativeMethods.Gdip.ThreadData[AquaKey] = aqua;
                }
                return aqua;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Aquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Aquamarine {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen aquamarine = (Pen)SafeNativeMethods.Gdip.ThreadData[AquamarineKey];
                if (aquamarine == null) {
                    aquamarine = new Pen(Color.Aquamarine, true);
                    SafeNativeMethods.Gdip.ThreadData[AquamarineKey] = aquamarine;
                }
                return aquamarine;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Azure"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Azure {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen azure = (Pen)SafeNativeMethods.Gdip.ThreadData[AzureKey];
                if (azure == null) {
                    azure = new Pen(Color.Azure, true);
                    SafeNativeMethods.Gdip.ThreadData[AzureKey] = azure;
                }
                return azure;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Beige"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Beige {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen beige = (Pen)SafeNativeMethods.Gdip.ThreadData[BeigeKey];
                if (beige == null) {
                    beige = new Pen(Color.Beige, true);
                    SafeNativeMethods.Gdip.ThreadData[BeigeKey] = beige;
                }
                return beige;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Bisque"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Bisque {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen bisque = (Pen)SafeNativeMethods.Gdip.ThreadData[BisqueKey];
                if (bisque == null) {
                    bisque = new Pen(Color.Bisque, true);
                    SafeNativeMethods.Gdip.ThreadData[BisqueKey] = bisque;
                }
                return bisque;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Black"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Black {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen black = (Pen)SafeNativeMethods.Gdip.ThreadData[BlackKey];
                if (black == null) {
                    black = new Pen(Color.Black, true);
                    SafeNativeMethods.Gdip.ThreadData[BlackKey] = black;
                }
                return black;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BlanchedAlmond"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BlanchedAlmond {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen blanchedAlmond = (Pen)SafeNativeMethods.Gdip.ThreadData[BlanchedAlmondKey];
                if (blanchedAlmond == null) {
                    blanchedAlmond = new Pen(Color.BlanchedAlmond, true);
                    SafeNativeMethods.Gdip.ThreadData[BlanchedAlmondKey] = blanchedAlmond;
                }
                return blanchedAlmond;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Blue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Blue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen blue = (Pen)SafeNativeMethods.Gdip.ThreadData[BlueKey];
                if (blue == null) {
                    blue = new Pen(Color.Blue, true);
                    SafeNativeMethods.Gdip.ThreadData[BlueKey] = blue;
                }
                return blue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BlueViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BlueViolet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen blueViolet = (Pen)SafeNativeMethods.Gdip.ThreadData[BlueVioletKey];
                if (blueViolet == null) {
                    blueViolet = new Pen(Color.BlueViolet, true);
                    SafeNativeMethods.Gdip.ThreadData[BlueVioletKey] = blueViolet;
                }
                return blueViolet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Brown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Brown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen brown = (Pen)SafeNativeMethods.Gdip.ThreadData[BrownKey];
                if (brown == null) {
                    brown = new Pen(Color.Brown, true);
                    SafeNativeMethods.Gdip.ThreadData[BrownKey] = brown;
                }
                return brown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.BurlyWood"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen BurlyWood {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen burlyWood = (Pen)SafeNativeMethods.Gdip.ThreadData[BurlyWoodKey];
                if (burlyWood == null) {
                    burlyWood = new Pen(Color.BurlyWood, true);
                    SafeNativeMethods.Gdip.ThreadData[BurlyWoodKey] = burlyWood;
                }
                return burlyWood;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.CadetBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen CadetBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen cadetBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[CadetBlueKey];
                if (cadetBlue == null) {
                    cadetBlue = new Pen(Color.CadetBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[CadetBlueKey] = cadetBlue;
                }
                return cadetBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Chartreuse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Chartreuse {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen chartreuse = (Pen)SafeNativeMethods.Gdip.ThreadData[ChartreuseKey];
                if (chartreuse == null) {
                    chartreuse = new Pen(Color.Chartreuse, true);
                    SafeNativeMethods.Gdip.ThreadData[ChartreuseKey] = chartreuse;
                }
                return chartreuse;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Chocolate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Chocolate {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen chocolate = (Pen)SafeNativeMethods.Gdip.ThreadData[ChocolateKey];
                if (chocolate == null) {
                    chocolate = new Pen(Color.Chocolate, true);
                    SafeNativeMethods.Gdip.ThreadData[ChocolateKey] = chocolate;
                }
                return chocolate;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Coral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Coral {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen choral = (Pen)SafeNativeMethods.Gdip.ThreadData[ChoralKey];
                if (choral == null) {
                    choral = new Pen(Color.Coral, true);
                    SafeNativeMethods.Gdip.ThreadData[ChoralKey] = choral;
                }
                return choral;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.CornflowerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen CornflowerBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen cornflowerBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[CornflowerBlueKey];
                if (cornflowerBlue == null) {
                    cornflowerBlue = new Pen(Color.CornflowerBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[CornflowerBlueKey] = cornflowerBlue;
                }
                return cornflowerBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Cornsilk"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Cornsilk {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen cornsilk = (Pen)SafeNativeMethods.Gdip.ThreadData[CornsilkKey];
                if (cornsilk == null) {
                    cornsilk = new Pen(Color.Cornsilk, true);
                    SafeNativeMethods.Gdip.ThreadData[CornsilkKey] = cornsilk;
                }
                return cornsilk;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Crimson"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Crimson {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen crimson = (Pen)SafeNativeMethods.Gdip.ThreadData[CrimsonKey];
                if (crimson == null) {
                    crimson = new Pen(Color.Crimson, true);
                    SafeNativeMethods.Gdip.ThreadData[CrimsonKey] = crimson;
                }
                return crimson;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Cyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Cyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen cyan = (Pen)SafeNativeMethods.Gdip.ThreadData[CyanKey];
                if (cyan == null) {
                    cyan = new Pen(Color.Cyan, true);
                    SafeNativeMethods.Gdip.ThreadData[CyanKey] = cyan;
                }
                return cyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkBlueKey];
                if (darkBlue == null) {
                    darkBlue = new Pen(Color.DarkBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkBlueKey] = darkBlue;
                }
                return darkBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkCyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkCyan = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkCyanKey];
                if (darkCyan == null) {
                    darkCyan = new Pen(Color.DarkCyan, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkCyanKey] = darkCyan;
                }
                return darkCyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGoldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkGoldenrod = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkGoldenrodKey];
                if (darkGoldenrod == null) {
                    darkGoldenrod = new Pen(Color.DarkGoldenrod, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkGoldenrodKey] = darkGoldenrod;
                }
                return darkGoldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkGray = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkGrayKey];
                if (darkGray == null) {
                    darkGray = new Pen(Color.DarkGray, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkGrayKey] = darkGray;
                }
                return darkGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkGreenKey];
                if (darkGreen == null) {
                    darkGreen = new Pen(Color.DarkGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkGreenKey] = darkGreen;
                }
                return darkGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkKhaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkKhaki {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkKhaki = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkKhakiKey];
                if (darkKhaki == null) {
                    darkKhaki = new Pen(Color.DarkKhaki, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkKhakiKey] = darkKhaki;
                }
                return darkKhaki;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkMagenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkMagenta {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkMagenta = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkMagentaKey];
                if (darkMagenta == null) {
                    darkMagenta = new Pen(Color.DarkMagenta, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkMagentaKey] = darkMagenta;
                }
                return darkMagenta;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOliveGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOliveGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkOliveGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkOliveGreenKey];
                if (darkOliveGreen == null) {
                    darkOliveGreen = new Pen(Color.DarkOliveGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkOliveGreenKey] = darkOliveGreen;
                }
                return darkOliveGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOrange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOrange {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkOrange = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkOrangeKey];
                if (darkOrange == null) {
                    darkOrange = new Pen(Color.DarkOrange, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkOrangeKey] = darkOrange;
                }
                return darkOrange;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkOrchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkOrchid = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkOrchidKey];
                if (darkOrchid == null) {
                    darkOrchid = new Pen(Color.DarkOrchid, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkOrchidKey] = darkOrchid;
                }
                return darkOrchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkRed = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkRedKey];
                if (darkRed == null) {
                    darkRed = new Pen(Color.DarkRed, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkRedKey] = darkRed;
                }
                return darkRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSalmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkSalmon = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkSalmonKey];
                if (darkSalmon == null) {
                    darkSalmon = new Pen(Color.DarkSalmon, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkSalmonKey] = darkSalmon;
                }
                return darkSalmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkSeaGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkSeaGreenKey];
                if (darkSeaGreen == null) {
                    darkSeaGreen = new Pen(Color.DarkSeaGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkSeaGreenKey] = darkSeaGreen;
                }
                return darkSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkSlateBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkSlateBlueKey];
                if (darkSlateBlue == null) {
                    darkSlateBlue = new Pen(Color.DarkSlateBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkSlateBlueKey] = darkSlateBlue;
                }
                return darkSlateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkSlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkSlateGray = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkSlateGrayKey];
                if (darkSlateGray == null) {
                    darkSlateGray = new Pen(Color.DarkSlateGray, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkSlateGrayKey] = darkSlateGray;
                }
                return darkSlateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkTurquoise = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkTurquoiseKey];
                if (darkTurquoise == null) {
                    darkTurquoise = new Pen(Color.DarkTurquoise, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkTurquoiseKey] = darkTurquoise;
                }
                return darkTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DarkViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DarkViolet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen darkViolet = (Pen)SafeNativeMethods.Gdip.ThreadData[DarkVioletKey];
                if (darkViolet == null) {
                    darkViolet = new Pen(Color.DarkViolet, true);
                    SafeNativeMethods.Gdip.ThreadData[DarkVioletKey] = darkViolet;
                }
                return darkViolet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DeepPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DeepPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen deepPink = (Pen)SafeNativeMethods.Gdip.ThreadData[DeepPinkKey];
                if (deepPink == null) {
                    deepPink = new Pen(Color.DeepPink, true);
                    SafeNativeMethods.Gdip.ThreadData[DeepPinkKey] = deepPink;
                }
                return deepPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DeepSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DeepSkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen deepSkyBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[DeepSkyBlueKey];
                if (deepSkyBlue == null) {
                    deepSkyBlue = new Pen(Color.DeepSkyBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[DeepSkyBlueKey] = deepSkyBlue;
                }
                return deepSkyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DimGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DimGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen dimGray = (Pen)SafeNativeMethods.Gdip.ThreadData[DimGrayKey];
                if (dimGray == null) {
                    dimGray = new Pen(Color.DimGray, true);
                    SafeNativeMethods.Gdip.ThreadData[DimGrayKey] = dimGray;
                }
                return dimGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.DodgerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen DodgerBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen dodgerBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[DodgerBlueKey];
                if (dodgerBlue == null) {
                    dodgerBlue = new Pen(Color.DodgerBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[DodgerBlueKey] = dodgerBlue;
                }
                return dodgerBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Firebrick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Firebrick {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen firebrick = (Pen)SafeNativeMethods.Gdip.ThreadData[FirebrickKey];
                if (firebrick == null) {
                    firebrick = new Pen(Color.Firebrick, true);
                    SafeNativeMethods.Gdip.ThreadData[FirebrickKey] = firebrick;
                }
                return firebrick;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.FloralWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen FloralWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen floralWhite = (Pen)SafeNativeMethods.Gdip.ThreadData[FloralWhiteKey];
                if (floralWhite == null) {
                    floralWhite = new Pen(Color.FloralWhite, true);
                    SafeNativeMethods.Gdip.ThreadData[FloralWhiteKey] = floralWhite;
                }
                return floralWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.ForestGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen ForestGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen forestGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[ForestGreenKey];
                if (forestGreen == null) {
                    forestGreen = new Pen(Color.ForestGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[ForestGreenKey] = forestGreen;
                }
                return forestGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Fuchsia"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Fuchsia {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen fuchia = (Pen)SafeNativeMethods.Gdip.ThreadData[FuchiaKey];
                if (fuchia == null) {
                    fuchia = new Pen(Color.Fuchsia, true);
                    SafeNativeMethods.Gdip.ThreadData[FuchiaKey] = fuchia;
                }
                return fuchia;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gainsboro"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gainsboro {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen gainsboro = (Pen)SafeNativeMethods.Gdip.ThreadData[GainsboroKey];
                if (gainsboro == null) {
                    gainsboro = new Pen(Color.Gainsboro, true);
                    SafeNativeMethods.Gdip.ThreadData[GainsboroKey] = gainsboro;
                }
                return gainsboro;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.GhostWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen GhostWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen ghostWhite = (Pen)SafeNativeMethods.Gdip.ThreadData[GhostWhiteKey];
                if (ghostWhite == null) {
                    ghostWhite = new Pen(Color.GhostWhite, true);
                    SafeNativeMethods.Gdip.ThreadData[GhostWhiteKey] = ghostWhite;
                }
                return ghostWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gold"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gold {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen gold = (Pen)SafeNativeMethods.Gdip.ThreadData[GoldKey];
                if (gold == null) {
                    gold = new Pen(Color.Gold, true);
                    SafeNativeMethods.Gdip.ThreadData[GoldKey] = gold;
                }
                return gold;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Goldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Goldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen goldenrod = (Pen)SafeNativeMethods.Gdip.ThreadData[GoldenrodKey];
                if (goldenrod == null) {
                    goldenrod = new Pen(Color.Goldenrod, true);
                    SafeNativeMethods.Gdip.ThreadData[GoldenrodKey] = goldenrod;
                }
                return goldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Gray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Gray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen gray = (Pen)SafeNativeMethods.Gdip.ThreadData[GrayKey];
                if (gray == null) {
                    gray = new Pen(Color.Gray, true);
                    SafeNativeMethods.Gdip.ThreadData[GrayKey] = gray;
                }
                return gray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Green"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Green {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen green = (Pen)SafeNativeMethods.Gdip.ThreadData[GreenKey];
                if (green == null) {
                    green = new Pen(Color.Green, true);
                    SafeNativeMethods.Gdip.ThreadData[GreenKey] = green;
                }
                return green;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.GreenYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen GreenYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen greenYellow = (Pen)SafeNativeMethods.Gdip.ThreadData[GreenYellowKey];
                if (greenYellow == null) {
                    greenYellow = new Pen(Color.GreenYellow, true);
                    SafeNativeMethods.Gdip.ThreadData[GreenYellowKey] = greenYellow;
                }
                return greenYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Honeydew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Honeydew {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen honeydew = (Pen)SafeNativeMethods.Gdip.ThreadData[HoneydewKey];
                if (honeydew == null) {
                    honeydew = new Pen(Color.Honeydew, true);
                    SafeNativeMethods.Gdip.ThreadData[HoneydewKey] = honeydew;
                }
                return honeydew;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.HotPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen HotPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen hotPink = (Pen)SafeNativeMethods.Gdip.ThreadData[HotPinkKey];
                if (hotPink == null) {
                    hotPink = new Pen(Color.HotPink, true);
                    SafeNativeMethods.Gdip.ThreadData[HotPinkKey] = hotPink;
                }
                return hotPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.IndianRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen IndianRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen indianRed = (Pen)SafeNativeMethods.Gdip.ThreadData[IndianRedKey];
                if (indianRed == null) {
                    indianRed = new Pen(Color.IndianRed, true);
                    SafeNativeMethods.Gdip.ThreadData[IndianRedKey] = indianRed;
                }
                return indianRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Indigo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Indigo {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen indigo = (Pen)SafeNativeMethods.Gdip.ThreadData[IndigoKey];
                if (indigo == null) {
                    indigo = new Pen(Color.Indigo, true);
                    SafeNativeMethods.Gdip.ThreadData[IndigoKey] = indigo;
                }
                return indigo;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Ivory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Ivory {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen ivory = (Pen)SafeNativeMethods.Gdip.ThreadData[IvoryKey];
                if (ivory == null) {
                    ivory = new Pen(Color.Ivory, true);
                    SafeNativeMethods.Gdip.ThreadData[IvoryKey] = ivory;
                }
                return ivory;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Khaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Khaki {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen khaki = (Pen)SafeNativeMethods.Gdip.ThreadData[KhakiKey];
                if (khaki == null) {
                    khaki = new Pen(Color.Khaki, true);
                    SafeNativeMethods.Gdip.ThreadData[KhakiKey] = khaki;
                }
                return khaki;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Lavender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Lavender {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lavender = (Pen)SafeNativeMethods.Gdip.ThreadData[LavenderKey];
                if (lavender == null) {
                    lavender = new Pen(Color.Lavender, true);
                    SafeNativeMethods.Gdip.ThreadData[LavenderKey] = lavender;
                }
                return lavender;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LavenderBlush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LavenderBlush {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lavenderBlush = (Pen)SafeNativeMethods.Gdip.ThreadData[LavenderBlushKey];
                if (lavenderBlush == null) {
                    lavenderBlush = new Pen(Color.LavenderBlush, true);
                    SafeNativeMethods.Gdip.ThreadData[LavenderBlushKey] = lavenderBlush;
                }
                return lavenderBlush;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LawnGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LawnGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lawnGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[LawnGreenKey];
                if (lawnGreen == null) {
                    lawnGreen = new Pen(Color.LawnGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[LawnGreenKey] = lawnGreen;
                }
                return lawnGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LemonChiffon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LemonChiffon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lemonChiffon = (Pen)SafeNativeMethods.Gdip.ThreadData[LemonChiffonKey];
                if (lemonChiffon == null) {
                    lemonChiffon = new Pen(Color.LemonChiffon, true);
                    SafeNativeMethods.Gdip.ThreadData[LemonChiffonKey] = lemonChiffon;
                }
                return lemonChiffon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[LightBlueKey];
                if (lightBlue == null) {
                    lightBlue = new Pen(Color.LightBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[LightBlueKey] = lightBlue;
                }
                return lightBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightCoral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightCoral {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightCoral = (Pen)SafeNativeMethods.Gdip.ThreadData[LightCoralKey];
                if (lightCoral == null) {
                    lightCoral = new Pen(Color.LightCoral, true);
                    SafeNativeMethods.Gdip.ThreadData[LightCoralKey] = lightCoral;
                }
                return lightCoral;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightCyan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightCyan = (Pen)SafeNativeMethods.Gdip.ThreadData[LightCyanKey];
                if (lightCyan == null) {
                    lightCyan = new Pen(Color.LightCyan, true);
                    SafeNativeMethods.Gdip.ThreadData[LightCyanKey] = lightCyan;
                }
                return lightCyan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGoldenrodYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGoldenrodYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightGoldenrodYellow = (Pen)SafeNativeMethods.Gdip.ThreadData[LightGoldenrodYellowKey];
                if (lightGoldenrodYellow == null) {
                    lightGoldenrodYellow = new Pen(Color.LightGoldenrodYellow, true);
                    SafeNativeMethods.Gdip.ThreadData[LightGoldenrodYellowKey] = lightGoldenrodYellow;
                }
                return lightGoldenrodYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[LightGreenKey];
                if (lightGreen == null) {
                    lightGreen = new Pen(Color.LightGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[LightGreenKey] = lightGreen;
                }
                return lightGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightGray = (Pen)SafeNativeMethods.Gdip.ThreadData[LightGrayKey];
                if (lightGray == null) {
                    lightGray = new Pen(Color.LightGray, true);
                    SafeNativeMethods.Gdip.ThreadData[LightGrayKey] = lightGray;
                }
                return lightGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightPink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightPink = (Pen)SafeNativeMethods.Gdip.ThreadData[LightPinkKey];
                if (lightPink == null) {
                    lightPink = new Pen(Color.LightPink, true);
                    SafeNativeMethods.Gdip.ThreadData[LightPinkKey] = lightPink;
                }
                return lightPink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSalmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightSalmon = (Pen)SafeNativeMethods.Gdip.ThreadData[LightSalmonKey];
                if (lightSalmon == null) {
                    lightSalmon = new Pen(Color.LightSalmon, true);
                    SafeNativeMethods.Gdip.ThreadData[LightSalmonKey] = lightSalmon;
                }
                return lightSalmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightSeaGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[LightSeaGreenKey];
                if (lightSeaGreen == null) {
                    lightSeaGreen = new Pen(Color.LightSeaGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[LightSeaGreenKey] = lightSeaGreen;
                }
                return lightSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightSkyBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[LightSkyBlueKey];
                if (lightSkyBlue == null) {
                    lightSkyBlue = new Pen(Color.LightSkyBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[LightSkyBlueKey] = lightSkyBlue;
                }
                return lightSkyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightSlateGray = (Pen)SafeNativeMethods.Gdip.ThreadData[LightSlateGrayKey];
                if (lightSlateGray == null) {
                    lightSlateGray = new Pen(Color.LightSlateGray, true);
                    SafeNativeMethods.Gdip.ThreadData[LightSlateGrayKey] = lightSlateGray;
                }
                return lightSlateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightSteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightSteelBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightSteelBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[LightSteelBlueKey];
                if (lightSteelBlue == null) {
                    lightSteelBlue = new Pen(Color.LightSteelBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[LightSteelBlueKey] = lightSteelBlue;
                }
                return lightSteelBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LightYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LightYellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lightYellow = (Pen)SafeNativeMethods.Gdip.ThreadData[LightYellowKey];
                if (lightYellow == null) {
                    lightYellow = new Pen(Color.LightYellow, true);
                    SafeNativeMethods.Gdip.ThreadData[LightYellowKey] = lightYellow;
                }
                return lightYellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Lime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Lime {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen lime = (Pen)SafeNativeMethods.Gdip.ThreadData[LimeKey];
                if (lime == null) {
                    lime = new Pen(Color.Lime, true);
                    SafeNativeMethods.Gdip.ThreadData[LimeKey] = lime;
                }
                return lime;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.LimeGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen LimeGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen limeGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[LimeGreenKey];
                if (limeGreen == null) {
                    limeGreen = new Pen(Color.LimeGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[LimeGreenKey] = limeGreen;
                }
                return limeGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Linen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Linen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen linen = (Pen)SafeNativeMethods.Gdip.ThreadData[LinenKey];
                if (linen == null) {
                    linen = new Pen(Color.Linen, true);
                    SafeNativeMethods.Gdip.ThreadData[LinenKey] = linen;
                }
                return linen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Magenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Magenta {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen magenta = (Pen)SafeNativeMethods.Gdip.ThreadData[MagentaKey];
                if (magenta == null) {
                    magenta = new Pen(Color.Magenta, true);
                    SafeNativeMethods.Gdip.ThreadData[MagentaKey] = magenta;
                }
                return magenta;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Maroon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Maroon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen maroon = (Pen)SafeNativeMethods.Gdip.ThreadData[MaroonKey];
                if (maroon == null) {
                    maroon = new Pen(Color.Maroon, true);
                    SafeNativeMethods.Gdip.ThreadData[MaroonKey] = maroon;
                }
                return maroon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumAquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumAquamarine {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumAquamarine = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumAquamarineKey];
                if (mediumAquamarine == null) {
                    mediumAquamarine = new Pen(Color.MediumAquamarine, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumAquamarineKey] = mediumAquamarine;
                }
                return mediumAquamarine;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumBlueKey];
                if (mediumBlue == null) {
                    mediumBlue = new Pen(Color.MediumBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumBlueKey] = mediumBlue;
                }
                return mediumBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumOrchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumOrchid = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumOrchidKey];
                if (mediumOrchid == null) {
                    mediumOrchid = new Pen(Color.MediumOrchid, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumOrchidKey] = mediumOrchid;
                }
                return mediumOrchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumPurple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumPurple {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumPurple = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumPurpleKey];
                if (mediumPurple == null) {
                    mediumPurple = new Pen(Color.MediumPurple, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumPurpleKey] = mediumPurple;
                }
                return mediumPurple;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumSeaGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumSeaGreenKey];
                if (mediumSeaGreen == null) {
                    mediumSeaGreen = new Pen(Color.MediumSeaGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumSeaGreenKey] = mediumSeaGreen;
                }
                return mediumSeaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumSlateBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumSlateBlueKey];
                if (mediumSlateBlue == null) {
                    mediumSlateBlue = new Pen(Color.MediumSlateBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumSlateBlueKey] = mediumSlateBlue;
                }
                return mediumSlateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumSpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumSpringGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumSpringGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumSpringGreenKey];
                if (mediumSpringGreen == null) {
                    mediumSpringGreen = new Pen(Color.MediumSpringGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumSpringGreenKey] = mediumSpringGreen;
                }
                return mediumSpringGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumTurquoise = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumTurquoiseKey];
                if (mediumTurquoise == null) {
                    mediumTurquoise = new Pen(Color.MediumTurquoise, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumTurquoiseKey] = mediumTurquoise;
                }
                return mediumTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MediumVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MediumVioletRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mediumVioletRed = (Pen)SafeNativeMethods.Gdip.ThreadData[MediumVioletRedKey];
                if (mediumVioletRed == null) {
                    mediumVioletRed = new Pen(Color.MediumVioletRed, true);
                    SafeNativeMethods.Gdip.ThreadData[MediumVioletRedKey] = mediumVioletRed;
                }
                return mediumVioletRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MidnightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MidnightBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen midnightBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[MidnightBlueKey];
                if (midnightBlue == null) {
                    midnightBlue = new Pen(Color.MidnightBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[MidnightBlueKey] = midnightBlue;
                }
                return midnightBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MintCream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MintCream {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mintCream = (Pen)SafeNativeMethods.Gdip.ThreadData[MintCreamKey];
                if (mintCream == null) {
                    mintCream = new Pen(Color.MintCream, true);
                    SafeNativeMethods.Gdip.ThreadData[MintCreamKey] = mintCream;
                }
                return mintCream;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.MistyRose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen MistyRose {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen mistyRose = (Pen)SafeNativeMethods.Gdip.ThreadData[MistyRoseKey];
                if (mistyRose == null) {
                    mistyRose = new Pen(Color.MistyRose, true);
                    SafeNativeMethods.Gdip.ThreadData[MistyRoseKey] = mistyRose;
                }
                return mistyRose;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Moccasin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Moccasin {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen moccasin = (Pen)SafeNativeMethods.Gdip.ThreadData[MoccasinKey];
                if (moccasin == null) {
                    moccasin = new Pen(Color.Moccasin, true);
                    SafeNativeMethods.Gdip.ThreadData[MoccasinKey] = moccasin;
                }
                return moccasin;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.NavajoWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen NavajoWhite {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen navajoWhite = (Pen)SafeNativeMethods.Gdip.ThreadData[NavajoWhiteKey];
                if (navajoWhite == null) {
                    navajoWhite = new Pen(Color.NavajoWhite, true);
                    SafeNativeMethods.Gdip.ThreadData[NavajoWhiteKey] = navajoWhite;
                }
                return navajoWhite;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Navy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Navy {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen navy = (Pen)SafeNativeMethods.Gdip.ThreadData[NavyKey];
                if (navy == null) {
                    navy = new Pen(Color.Navy, true);
                    SafeNativeMethods.Gdip.ThreadData[NavyKey] = navy;
                }
                return navy;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OldLace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OldLace {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen oldLace = (Pen)SafeNativeMethods.Gdip.ThreadData[OldLaceKey];
                if (oldLace == null) {
                    oldLace = new Pen(Color.OldLace, true);
                    SafeNativeMethods.Gdip.ThreadData[OldLaceKey] = oldLace;
                }
                return oldLace;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Olive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Olive {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen olive = (Pen)SafeNativeMethods.Gdip.ThreadData[OliveKey];
                if (olive == null) {
                    olive = new Pen(Color.Olive, true);
                    SafeNativeMethods.Gdip.ThreadData[OliveKey] = olive;
                }
                return olive;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OliveDrab"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OliveDrab {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen oliveDrab = (Pen)SafeNativeMethods.Gdip.ThreadData[OliveDrabKey];
                if (oliveDrab == null) {
                    oliveDrab = new Pen(Color.OliveDrab, true);
                    SafeNativeMethods.Gdip.ThreadData[OliveDrabKey] = oliveDrab;
                }
                return oliveDrab;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Orange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Orange {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen orange = (Pen)SafeNativeMethods.Gdip.ThreadData[OrangeKey];
                if (orange == null) {
                    orange = new Pen(Color.Orange, true);
                    SafeNativeMethods.Gdip.ThreadData[OrangeKey] = orange;
                }
                return orange;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.OrangeRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen OrangeRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen orangeRed = (Pen)SafeNativeMethods.Gdip.ThreadData[OrangeRedKey];
                if (orangeRed == null) {
                    orangeRed = new Pen(Color.OrangeRed, true);
                    SafeNativeMethods.Gdip.ThreadData[OrangeRedKey] = orangeRed;
                }
                return orangeRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Orchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Orchid {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen orchid = (Pen)SafeNativeMethods.Gdip.ThreadData[OrchidKey];
                if (orchid == null) {
                    orchid = new Pen(Color.Orchid, true);
                    SafeNativeMethods.Gdip.ThreadData[OrchidKey] = orchid;
                }
                return orchid;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleGoldenrod {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen paleGoldenrod = (Pen)SafeNativeMethods.Gdip.ThreadData[PaleGoldenrodKey];
                if (paleGoldenrod == null) {
                    paleGoldenrod = new Pen(Color.PaleGoldenrod, true);
                    SafeNativeMethods.Gdip.ThreadData[PaleGoldenrodKey] = paleGoldenrod;
                }
                return paleGoldenrod;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen paleGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[PaleGreenKey];
                if (paleGreen == null) {
                    paleGreen = new Pen(Color.PaleGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[PaleGreenKey] = paleGreen;
                }
                return paleGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleTurquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen paleTurquoise = (Pen)SafeNativeMethods.Gdip.ThreadData[PaleTurquoiseKey];
                if (paleTurquoise == null) {
                    paleTurquoise = new Pen(Color.PaleTurquoise, true);
                    SafeNativeMethods.Gdip.ThreadData[PaleTurquoiseKey] = paleTurquoise;
                }
                return paleTurquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PaleVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PaleVioletRed {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen paleVioletRed = (Pen)SafeNativeMethods.Gdip.ThreadData[PaleVioletRedKey];
                if (paleVioletRed == null) {
                    paleVioletRed = new Pen(Color.PaleVioletRed, true);
                    SafeNativeMethods.Gdip.ThreadData[PaleVioletRedKey] = paleVioletRed;
                }
                return paleVioletRed;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PapayaWhip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PapayaWhip {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen papayaWhip = (Pen)SafeNativeMethods.Gdip.ThreadData[PapayaWhipKey];
                if (papayaWhip == null) {
                    papayaWhip = new Pen(Color.PapayaWhip, true);
                    SafeNativeMethods.Gdip.ThreadData[PapayaWhipKey] = papayaWhip;
                }
                return papayaWhip;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PeachPuff"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PeachPuff {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen peachPuff = (Pen)SafeNativeMethods.Gdip.ThreadData[PeachPuffKey];
                if (peachPuff == null) {
                    peachPuff = new Pen(Color.PeachPuff, true);
                    SafeNativeMethods.Gdip.ThreadData[PeachPuffKey] = peachPuff;
                }
                return peachPuff;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Peru"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Peru {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen peru = (Pen)SafeNativeMethods.Gdip.ThreadData[PeruKey];
                if (peru == null) {
                    peru = new Pen(Color.Peru, true);
                    SafeNativeMethods.Gdip.ThreadData[PeruKey] = peru;
                }
                return peru;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Pink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Pink {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen pink = (Pen)SafeNativeMethods.Gdip.ThreadData[PinkKey];
                if (pink == null) {
                    pink = new Pen(Color.Pink, true);
                    SafeNativeMethods.Gdip.ThreadData[PinkKey] = pink;
                }
                return pink;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Plum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Plum {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen plum = (Pen)SafeNativeMethods.Gdip.ThreadData[PlumKey];
                if (plum == null) {
                    plum = new Pen(Color.Plum, true);
                    SafeNativeMethods.Gdip.ThreadData[PlumKey] = plum;
                }
                return plum;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.PowderBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen PowderBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen powderBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[PowderBlueKey];
                if (powderBlue == null) {
                    powderBlue = new Pen(Color.PowderBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[PowderBlueKey] = powderBlue;
                }
                return powderBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Purple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Purple {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen purple = (Pen)SafeNativeMethods.Gdip.ThreadData[PurpleKey];
                if (purple == null) {
                    purple = new Pen(Color.Purple, true);
                    SafeNativeMethods.Gdip.ThreadData[PurpleKey] = purple;
                }
                return purple;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Red"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Red {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen red = (Pen)SafeNativeMethods.Gdip.ThreadData[RedKey];
                if (red == null) {
                    red = new Pen(Color.Red, true);
                    SafeNativeMethods.Gdip.ThreadData[RedKey] = red;
                }
                return red;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.RosyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen RosyBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen rosyBrown = (Pen)SafeNativeMethods.Gdip.ThreadData[RosyBrownKey];
                if (rosyBrown == null) {
                    rosyBrown = new Pen(Color.RosyBrown, true);
                    SafeNativeMethods.Gdip.ThreadData[RosyBrownKey] = rosyBrown;
                }
                return rosyBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.RoyalBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen RoyalBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen royalBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[RoyalBlueKey];
                if (royalBlue == null) {
                    royalBlue = new Pen(Color.RoyalBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[RoyalBlueKey] = royalBlue;
                }
                return royalBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SaddleBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SaddleBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen saddleBrown = (Pen)SafeNativeMethods.Gdip.ThreadData[SaddleBrownKey];
                if (saddleBrown == null) {
                    saddleBrown = new Pen(Color.SaddleBrown, true);
                    SafeNativeMethods.Gdip.ThreadData[SaddleBrownKey] = saddleBrown;
                }
                return saddleBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Salmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Salmon {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen salmon = (Pen)SafeNativeMethods.Gdip.ThreadData[SalmonKey];
                if (salmon == null) {
                    salmon = new Pen(Color.Salmon, true);
                    SafeNativeMethods.Gdip.ThreadData[SalmonKey] = salmon;
                }
                return salmon;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SandyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SandyBrown {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen sandyBrown = (Pen)SafeNativeMethods.Gdip.ThreadData[SandyBrownKey];
                if (sandyBrown == null) {
                    sandyBrown = new Pen(Color.SandyBrown, true);
                    SafeNativeMethods.Gdip.ThreadData[SandyBrownKey] = sandyBrown;
                }
                return sandyBrown;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SeaGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen seaGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[SeaGreenKey];
                if (seaGreen == null) {
                    seaGreen = new Pen(Color.SeaGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[SeaGreenKey] = seaGreen;
                }
                return seaGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SeaShell"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SeaShell {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen seaShell = (Pen)SafeNativeMethods.Gdip.ThreadData[SeaShellKey];
                if (seaShell == null) {
                    seaShell = new Pen(Color.SeaShell, true);
                    SafeNativeMethods.Gdip.ThreadData[SeaShellKey] = seaShell;
                }
                return seaShell;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Sienna"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Sienna {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen sienna = (Pen)SafeNativeMethods.Gdip.ThreadData[SiennaKey];
                if (sienna == null) {
                    sienna = new Pen(Color.Sienna, true);
                    SafeNativeMethods.Gdip.ThreadData[SiennaKey] = sienna;
                }
                return sienna;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Silver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Silver {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen silver = (Pen)SafeNativeMethods.Gdip.ThreadData[SilverKey];
                if (silver == null) {
                    silver = new Pen(Color.Silver, true);
                    SafeNativeMethods.Gdip.ThreadData[SilverKey] = silver;
                }
                return silver;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SkyBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen skyBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[SkyBlueKey];
                if (skyBlue == null) {
                    skyBlue = new Pen(Color.SkyBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[SkyBlueKey] = skyBlue;
                }
                return skyBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SlateBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen slateBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[SlateBlueKey];
                if (slateBlue == null) {
                    slateBlue = new Pen(Color.SlateBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[SlateBlueKey] = slateBlue;
                }
                return slateBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SlateGray {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen slateGray = (Pen)SafeNativeMethods.Gdip.ThreadData[SlateGrayKey];
                if (slateGray == null) {
                    slateGray = new Pen(Color.SlateGray, true);
                    SafeNativeMethods.Gdip.ThreadData[SlateGrayKey] = slateGray;
                }
                return slateGray;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Snow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Snow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen snow = (Pen)SafeNativeMethods.Gdip.ThreadData[SnowKey];
                if (snow == null) {
                    snow = new Pen(Color.Snow, true);
                    SafeNativeMethods.Gdip.ThreadData[SnowKey] = snow;
                }
                return snow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SpringGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen springGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[SpringGreenKey];
                if (springGreen == null) {
                    springGreen = new Pen(Color.SpringGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[SpringGreenKey] = springGreen;
                }
                return springGreen;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.SteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen SteelBlue {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen steelBlue = (Pen)SafeNativeMethods.Gdip.ThreadData[SteelBlueKey];
                if (steelBlue == null) {
                    steelBlue = new Pen(Color.SteelBlue, true);
                    SafeNativeMethods.Gdip.ThreadData[SteelBlueKey] = steelBlue;
                }
                return steelBlue;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Tan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Tan {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen tan = (Pen)SafeNativeMethods.Gdip.ThreadData[TanKey];
                if (tan == null) {
                    tan = new Pen(Color.Tan, true);
                    SafeNativeMethods.Gdip.ThreadData[TanKey] = tan;
                }
                return tan;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Teal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Teal {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen teal = (Pen)SafeNativeMethods.Gdip.ThreadData[TealKey];
                if (teal == null) {
                    teal = new Pen(Color.Teal, true);
                    SafeNativeMethods.Gdip.ThreadData[TealKey] = teal;
                }
                return teal;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Thistle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Thistle {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen thistle = (Pen)SafeNativeMethods.Gdip.ThreadData[ThistleKey];
                if (thistle == null) {
                    thistle = new Pen(Color.Thistle, true);
                    SafeNativeMethods.Gdip.ThreadData[ThistleKey] = thistle;
                }
                return thistle;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Tomato"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Tomato {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen tomato = (Pen)SafeNativeMethods.Gdip.ThreadData[TomatoKey];
                if (tomato == null) {
                    tomato = new Pen(Color.Tomato, true);
                    SafeNativeMethods.Gdip.ThreadData[TomatoKey] = tomato;
                }
                return tomato;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Turquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Turquoise {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen turquoise = (Pen)SafeNativeMethods.Gdip.ThreadData[TurquoiseKey];
                if (turquoise == null) {
                    turquoise = new Pen(Color.Turquoise, true);
                    SafeNativeMethods.Gdip.ThreadData[TurquoiseKey] = turquoise;
                }
                return turquoise;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Violet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Violet {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen violet = (Pen)SafeNativeMethods.Gdip.ThreadData[VioletKey];
                if (violet == null) {
                    violet = new Pen(Color.Violet, true);
                    SafeNativeMethods.Gdip.ThreadData[VioletKey] = violet;
                }
                return violet;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Wheat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Wheat {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen wheat = (Pen)SafeNativeMethods.Gdip.ThreadData[WheatKey];
                if (wheat == null) {
                    wheat = new Pen(Color.Wheat, true);
                    SafeNativeMethods.Gdip.ThreadData[WheatKey] = wheat;
                }
                return wheat;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.White"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen White {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen white = (Pen)SafeNativeMethods.Gdip.ThreadData[WhiteKey];
                if (white == null) {
                    white = new Pen(Color.White, true);
                    SafeNativeMethods.Gdip.ThreadData[WhiteKey] = white;
                }
                return white;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.WhiteSmoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen WhiteSmoke {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen whiteSmoke = (Pen)SafeNativeMethods.Gdip.ThreadData[WhiteSmokeKey];
                if (whiteSmoke == null) {
                    whiteSmoke = new Pen(Color.WhiteSmoke, true);
                    SafeNativeMethods.Gdip.ThreadData[WhiteSmokeKey] = whiteSmoke;
                }
                return whiteSmoke;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.Yellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen Yellow {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen yellow = (Pen)SafeNativeMethods.Gdip.ThreadData[YellowKey];
                if (yellow == null) {
                    yellow = new Pen(Color.Yellow, true);
                    SafeNativeMethods.Gdip.ThreadData[YellowKey] = yellow;
                }
                return yellow;
            }
        }

        /// <include file='doc\Pens.uex' path='docs/doc[@for="Pens.YellowGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen YellowGreen {
            [ResourceExposure(ResourceScope.Process)]
            [ResourceConsumption(ResourceScope.Process | ResourceScope.AppDomain, ResourceScope.Process | ResourceScope.AppDomain)]
            get {  
                Pen yellowGreen = (Pen)SafeNativeMethods.Gdip.ThreadData[YellowGreenKey];
                if (yellowGreen == null) {
                    yellowGreen = new Pen(Color.YellowGreen, true);
                    SafeNativeMethods.Gdip.ThreadData[YellowGreenKey] = yellowGreen;
                }
                return yellowGreen;
            }
        }
    }
}
