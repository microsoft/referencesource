//------------------------------------------------------------------------------
// <copyright file="Color.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System.Globalization;
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
#if !FEATURE_PAL
    using System.Drawing.Design;
#endif
    using System.Runtime.InteropServices;
    
    /// <include file='doc\Color.uex' path='docs/doc[@for="Color"]/*' />
    /// <devdoc>
    ///    Represents an ARGB color.
    /// </devdoc>
    [
    Serializable(),
    TypeConverter(typeof(ColorConverter)),
    DebuggerDisplay("{NameAndARGBValue}"),
#if !FEATURE_PAL
    Editor("System.Drawing.Design.ColorEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor)),
#endif
    ]
    public struct Color {
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Empty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly Color Empty = new Color();

        // -------------------------------------------------------------------
        //  static list of "web" colors...
        //
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Transparent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Transparent {
            get {
                return new Color(KnownColor.Transparent);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.AliceBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color AliceBlue {
            get {
                return new Color(KnownColor.AliceBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.AntiqueWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color AntiqueWhite {
            get {
                return new Color(KnownColor.AntiqueWhite);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Aqua"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Aqua {
            get {
                return new Color(KnownColor.Aqua);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Aquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Aquamarine {
            get {
                return new Color(KnownColor.Aquamarine);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Azure"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Azure {
            get {
                return new Color(KnownColor.Azure);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Beige"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Beige {
            get {
                return new Color(KnownColor.Beige);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Bisque"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Bisque {
            get {
                return new Color(KnownColor.Bisque);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Black"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Black {
            get {
                return new Color(KnownColor.Black);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.BlanchedAlmond"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color BlanchedAlmond {
            get {
                return new Color(KnownColor.BlanchedAlmond);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Blue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Blue {
            get {
                return new Color(KnownColor.Blue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.BlueViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color BlueViolet {
            get {
                return new Color(KnownColor.BlueViolet);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Brown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Brown {
            get {
                return new Color(KnownColor.Brown);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.BurlyWood"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color BurlyWood {
            get {
                return new Color(KnownColor.BurlyWood);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.CadetBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color CadetBlue {
            get {
                return new Color(KnownColor.CadetBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Chartreuse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Chartreuse {
            get {
                return new Color(KnownColor.Chartreuse);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Chocolate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Chocolate {
            get {
                return new Color(KnownColor.Chocolate);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Coral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Coral {
            get {
                return new Color(KnownColor.Coral);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.CornflowerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color CornflowerBlue {
            get {
                return new Color(KnownColor.CornflowerBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Cornsilk"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Cornsilk {
            get {
                return new Color(KnownColor.Cornsilk);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Crimson"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Crimson {
            get {
                return new Color(KnownColor.Crimson);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Cyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Cyan {
            get {
                return new Color(KnownColor.Cyan);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkBlue {
            get {
                return new Color(KnownColor.DarkBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkCyan {
            get {
                return new Color(KnownColor.DarkCyan);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkGoldenrod {
            get {
                return new Color(KnownColor.DarkGoldenrod);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkGray {
            get {
                return new Color(KnownColor.DarkGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkGreen {
            get {
                return new Color(KnownColor.DarkGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkKhaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkKhaki {
            get {
                return new Color(KnownColor.DarkKhaki);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkMagenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkMagenta {
            get {
                return new Color(KnownColor.DarkMagenta);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkOliveGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkOliveGreen {
            get {
                return new Color(KnownColor.DarkOliveGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkOrange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkOrange {
            get {
                return new Color(KnownColor.DarkOrange);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkOrchid {
            get {
                return new Color(KnownColor.DarkOrchid);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkRed {
            get {
                return new Color(KnownColor.DarkRed);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkSalmon {
            get {
                return new Color(KnownColor.DarkSalmon);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkSeaGreen {
            get {
                return new Color(KnownColor.DarkSeaGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkSlateBlue {
            get {
                return new Color(KnownColor.DarkSlateBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkSlateGray {
            get {
                return new Color(KnownColor.DarkSlateGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkTurquoise {
            get {
                return new Color(KnownColor.DarkTurquoise);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DarkViolet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DarkViolet {
            get {
                return new Color(KnownColor.DarkViolet);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DeepPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DeepPink {
            get {
                return new Color(KnownColor.DeepPink);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DeepSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DeepSkyBlue {
            get {
                return new Color(KnownColor.DeepSkyBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DimGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DimGray {
            get {
                return new Color(KnownColor.DimGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.DodgerBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color DodgerBlue {
            get {
                return new Color(KnownColor.DodgerBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Firebrick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Firebrick {
            get {
                return new Color(KnownColor.Firebrick);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FloralWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color FloralWhite {
            get {
                return new Color(KnownColor.FloralWhite);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.ForestGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color ForestGreen {
            get {
                return new Color(KnownColor.ForestGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Fuchsia"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Fuchsia {
            get {
                return new Color(KnownColor.Fuchsia);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Gainsboro"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Gainsboro {
            get {
                return new Color(KnownColor.Gainsboro);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.GhostWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color GhostWhite {
            get {
                return new Color(KnownColor.GhostWhite);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Gold"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Gold {
            get {
                return new Color(KnownColor.Gold);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Goldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Goldenrod {
            get {
                return new Color(KnownColor.Goldenrod);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Gray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Gray {
            get {
                return new Color(KnownColor.Gray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Green"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Green {
            get {
                return new Color(KnownColor.Green);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.GreenYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color GreenYellow {
            get {
                return new Color(KnownColor.GreenYellow);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Honeydew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Honeydew {
            get {
                return new Color(KnownColor.Honeydew);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.HotPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color HotPink {
            get {
                return new Color(KnownColor.HotPink);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.IndianRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color IndianRed {
            get {
                return new Color(KnownColor.IndianRed);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Indigo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Indigo {
            get {
                return new Color(KnownColor.Indigo);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Ivory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Ivory {
            get {
                return new Color(KnownColor.Ivory);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Khaki"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Khaki {
            get {
                return new Color(KnownColor.Khaki);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Lavender"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Lavender {
            get {
                return new Color(KnownColor.Lavender);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LavenderBlush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LavenderBlush {
            get {
                return new Color(KnownColor.LavenderBlush);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LawnGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LawnGreen {
            get {
                return new Color(KnownColor.LawnGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LemonChiffon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LemonChiffon {
            get {
                return new Color(KnownColor.LemonChiffon);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightBlue {
            get {
                return new Color(KnownColor.LightBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightCoral"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightCoral {
            get {
                return new Color(KnownColor.LightCoral);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightCyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightCyan {
            get {
                return new Color(KnownColor.LightCyan);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightGoldenrodYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightGoldenrodYellow {
            get {
                return new Color(KnownColor.LightGoldenrodYellow);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightGreen {
            get {
                return new Color(KnownColor.LightGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightGray {
            get {
                return new Color(KnownColor.LightGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightPink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightPink {
            get {
                return new Color(KnownColor.LightPink);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightSalmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightSalmon {
            get {
                return new Color(KnownColor.LightSalmon);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightSeaGreen {
            get {
                return new Color(KnownColor.LightSeaGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightSkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightSkyBlue {
            get {
                return new Color(KnownColor.LightSkyBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightSlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightSlateGray {
            get {
                return new Color(KnownColor.LightSlateGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightSteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightSteelBlue {
            get {
                return new Color(KnownColor.LightSteelBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LightYellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LightYellow {
            get {
                return new Color(KnownColor.LightYellow);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Lime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Lime {
            get {
                return new Color(KnownColor.Lime);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.LimeGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color LimeGreen {
            get {
                return new Color(KnownColor.LimeGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Linen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Linen {
            get {
                return new Color(KnownColor.Linen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Magenta"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Magenta {
            get {
                return new Color(KnownColor.Magenta);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Maroon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Maroon {
            get {
                return new Color(KnownColor.Maroon);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumAquamarine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumAquamarine {
            get {
                return new Color(KnownColor.MediumAquamarine);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumBlue {
            get {
                return new Color(KnownColor.MediumBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumOrchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumOrchid {
            get {
                return new Color(KnownColor.MediumOrchid);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumPurple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumPurple {
            get {
                return new Color(KnownColor.MediumPurple);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumSeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumSeaGreen {
            get {
                return new Color(KnownColor.MediumSeaGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumSlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumSlateBlue {
            get {
                return new Color(KnownColor.MediumSlateBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumSpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumSpringGreen {
            get {
                return new Color(KnownColor.MediumSpringGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumTurquoise {
            get {
                return new Color(KnownColor.MediumTurquoise);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MediumVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MediumVioletRed {
            get {
                return new Color(KnownColor.MediumVioletRed);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MidnightBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MidnightBlue {
            get {
                return new Color(KnownColor.MidnightBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MintCream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MintCream {
            get {
                return new Color(KnownColor.MintCream);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MistyRose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color MistyRose {
            get {
                return new Color(KnownColor.MistyRose);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Moccasin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Moccasin {
            get {
                return new Color(KnownColor.Moccasin);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.NavajoWhite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color NavajoWhite {
            get {
                return new Color(KnownColor.NavajoWhite);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Navy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Navy {
            get {
                return new Color(KnownColor.Navy);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.OldLace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color OldLace {
            get {
                return new Color(KnownColor.OldLace);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Olive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Olive {
            get {
                return new Color(KnownColor.Olive);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.OliveDrab"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color OliveDrab {
            get {
                return new Color(KnownColor.OliveDrab);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Orange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Orange {
            get {
                return new Color(KnownColor.Orange);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.OrangeRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color OrangeRed {
            get {
                return new Color(KnownColor.OrangeRed);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Orchid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Orchid {
            get {
                return new Color(KnownColor.Orchid);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PaleGoldenrod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PaleGoldenrod {
            get {
                return new Color(KnownColor.PaleGoldenrod);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PaleGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PaleGreen {
            get {
                return new Color(KnownColor.PaleGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PaleTurquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PaleTurquoise {
            get {
                return new Color(KnownColor.PaleTurquoise);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PaleVioletRed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PaleVioletRed {
            get {
                return new Color(KnownColor.PaleVioletRed);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PapayaWhip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PapayaWhip {
            get {
                return new Color(KnownColor.PapayaWhip);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PeachPuff"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PeachPuff {
            get {
                return new Color(KnownColor.PeachPuff);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Peru"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Peru {
            get {
                return new Color(KnownColor.Peru);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Pink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Pink {
            get {
                return new Color(KnownColor.Pink);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Plum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Plum {
            get {
                return new Color(KnownColor.Plum);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.PowderBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color PowderBlue {
            get {
                return new Color(KnownColor.PowderBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Purple"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Purple {
            get {
                return new Color(KnownColor.Purple);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Red"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Red {
            get {
                return new Color(KnownColor.Red);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.RosyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color RosyBrown {
            get {
                return new Color(KnownColor.RosyBrown);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.RoyalBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color RoyalBlue {
            get {
                return new Color(KnownColor.RoyalBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SaddleBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SaddleBrown {
            get {
                return new Color(KnownColor.SaddleBrown);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Salmon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Salmon {
            get {
                return new Color(KnownColor.Salmon);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SandyBrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SandyBrown {
            get {
                return new Color(KnownColor.SandyBrown);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SeaGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SeaGreen {
            get {
                return new Color(KnownColor.SeaGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SeaShell"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SeaShell {
            get {
                return new Color(KnownColor.SeaShell);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Sienna"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Sienna {
            get {
                return new Color(KnownColor.Sienna);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Silver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Silver {
            get {
                return new Color(KnownColor.Silver);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SkyBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SkyBlue {
            get {
                return new Color(KnownColor.SkyBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SlateBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SlateBlue {
            get {
                return new Color(KnownColor.SlateBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SlateGray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SlateGray {
            get {
                return new Color(KnownColor.SlateGray);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Snow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Snow {
            get {
                return new Color(KnownColor.Snow);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SpringGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SpringGreen {
            get {
                return new Color(KnownColor.SpringGreen);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.SteelBlue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color SteelBlue {
            get {
                return new Color(KnownColor.SteelBlue);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Tan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Tan {
            get {
                return new Color(KnownColor.Tan);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Teal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Teal {
            get {
                return new Color(KnownColor.Teal);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Thistle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Thistle {
            get {
                return new Color(KnownColor.Thistle);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Tomato"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Tomato {
            get {
                return new Color(KnownColor.Tomato);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Turquoise"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Turquoise {
            get {
                return new Color(KnownColor.Turquoise);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Violet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Violet {
            get {
                return new Color(KnownColor.Violet);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Wheat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Wheat {
            get {
                return new Color(KnownColor.Wheat);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.White"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color White {
            get {
                return new Color(KnownColor.White);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.WhiteSmoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color WhiteSmoke {
            get {
                return new Color(KnownColor.WhiteSmoke);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Yellow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color Yellow {
            get {
                return new Color(KnownColor.Yellow);
            }
        }
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.YellowGreen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Color YellowGreen {
            get {
                return new Color(KnownColor.YellowGreen);
            }
        }
        //
        //  end "web" colors
        // -------------------------------------------------------------------

        // NOTE : The "zero" pattern (all members being 0) must represent
        //      : "not set". This allows "Color c;" to be correct.

        private static short StateKnownColorValid   = 0x0001;
        private static short StateARGBValueValid    = 0x0002;
        private static short StateValueMask         = (short)(StateARGBValueValid);
        private static short StateNameValid         = 0x0008;
        private static long NotDefinedValue = 0;

        /**
         * Shift count and bit mask for A, R, G, B components in ARGB mode!
         */
        private const int ARGBAlphaShift  = 24;
        private const int ARGBRedShift    = 16;
        private const int ARGBGreenShift  = 8;
        private const int ARGBBlueShift   = 0;

        ///    WARNING!!! WARNING!!! WARNING!!! WARNING!!! 
        ///    WARNING!!! WARNING!!! WARNING!!! WARNING!!!
        ///    We can never change the layout of this class (adding or removing or changing the 
        ///    order of member variables) if you want to be compatible v1.0 version of the runtime.
        ///    This is so that we can push into the runtime a custom marshaller for OLE_COLOR to Color.

        // user supplied name of color. Will not be filled in if
        // we map to a "knowncolor"
        //
        private readonly string name;
        
        // will contain standard 32bit sRGB (ARGB)
        //
        private readonly long value;

        // ignored, unless "state" says it is valid
        //
        private readonly short knownColor;

        // implementation specific information
        //
        private readonly short state;


        internal Color(KnownColor knownColor) {
            value = 0;
            state = StateKnownColorValid;
            name = null;
            this.knownColor = unchecked((short)knownColor);
        }

        private Color(long value, short state, string name, KnownColor knownColor) {
            this.value = value;
            this.state = state;
            this.name = name;
            this.knownColor = unchecked((short)knownColor);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.R"]/*' />
        /// <devdoc>
        ///    Gets the red component value for this <see cref='System.Drawing.Color'/>.
        /// </devdoc>
        public byte R {
            get {
                return(byte)((Value >> ARGBRedShift) & 0xFF);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.G"]/*' />
        /// <devdoc>
        ///    Gets the green component value for this <see cref='System.Drawing.Color'/>.
        /// </devdoc>
        public byte G {
            get {
                return(byte)((Value >> ARGBGreenShift) & 0xFF);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.B"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the blue component value for this <see cref='System.Drawing.Color'/>.
        ///    </para>
        /// </devdoc>
        public byte B {
            get {
                return(byte)((Value >> ARGBBlueShift) & 0xFF);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.A"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the alpha component value for this <see cref='System.Drawing.Color'/>.
        ///    </para>
        /// </devdoc>
        public byte A {
            get {
                return(byte)((Value >> ARGBAlphaShift) & 0xFF);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.IsKnownColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies whether this <see cref='System.Drawing.Color'/> is a known (predefined) color.
        ///       Predefined colors are defined in the <see cref='System.Drawing.KnownColor'/>
        ///       enum.
        ///    </para>
        /// </devdoc>
        public bool IsKnownColor {
            get {
                return((state & StateKnownColorValid) != 0);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.IsEmpty"]/*' />
        /// <devdoc>
        ///    Specifies whether this <see cref='System.Drawing.Color'/> is uninitialized.
        /// </devdoc>
        public bool IsEmpty {
            get {
                return state == 0;
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.IsNamedColor"]/*' />
        /// <devdoc>
        ///    Specifies whether this <see cref='System.Drawing.Color'/> has a name or is a <see cref='System.Drawing.KnownColor'/>.
        /// </devdoc>
        public bool IsNamedColor {
            get {
                return ((state & StateNameValid) != 0) || IsKnownColor;
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.IsSystemColor"]/*' />
        /// <devdoc>
        ///     Determines if this color is a system color.
        /// </devdoc>
        public bool IsSystemColor {
            get {
                return IsKnownColor && ((((KnownColor) knownColor) <= KnownColor.WindowText) || (((KnownColor) knownColor) > KnownColor.YellowGreen));
            }
        }

        // Not localized because it's only used for the DebuggerDisplayAttribute, and the values are
        // programmatic items.
        // Also, don't inline into the attribute for performance reasons.  This way means the debugger
        // does 1 func-eval instead of 5.
        [SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters")]
        private string NameAndARGBValue {
            get {
                return string.Format(CultureInfo.CurrentCulture, 
                                     "{{Name={0}, ARGB=({1}, {2}, {3}, {4})}}",
                                              Name,      A,   R,   G,   B);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of this <see cref='System.Drawing.Color'/> . This will either return the user
        ///       defined name of the color, if the color was created from a name, or
        ///       the name of the known color. For custom colors, the RGB value will
        ///       be returned.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                if ((state & StateNameValid) != 0) {
                    return name;
                }

                if (IsKnownColor) {
                    // first try the table so we can avoid the (slow!) .ToString()
                    string tablename = KnownColorTable.KnownColorToName((KnownColor) knownColor);
                    if (tablename != null)
                        return tablename;

                    Debug.Assert(false, "Could not find known color '" + ((KnownColor) knownColor) + "' in the KnownColorTable");
                    
                    return ((KnownColor)knownColor).ToString();
                }

                // if we reached here, just encode the value
                //
                return Convert.ToString(value, 16);
            }
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Value"]/*' />
        /// <devdoc>
        ///     Actual color to be rendered.
        /// </devdoc>
        private long Value {
            get {
                if ((state & StateValueMask) != 0) {
                    return value;
                }
                if (IsKnownColor) {
                    return unchecked((int)KnownColorTable.KnownColorToArgb((KnownColor)knownColor));
                }

                return NotDefinedValue;
            }
        }

        private static void CheckByte(int value, string name) {
            if (value < 0 || value > 255)
                throw new ArgumentException(SR.GetString(SR.InvalidEx2BoundArgument, name, value, 0, 255));
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.MakeArgb"]/*' />
        /// <devdoc>
        ///     Encodes the four values into ARGB (32 bit) format.
        /// </devdoc>
        private static long MakeArgb(byte alpha, byte red, byte green, byte blue) {
            return(long)(unchecked((uint)(red << ARGBRedShift |
                         green << ARGBGreenShift | 
                         blue << ARGBBlueShift | 
                         alpha << ARGBAlphaShift))) & 0xffffffff;
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromArgb"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a Color from its 32-bit component
        ///       (alpha, red, green, and blue) values.
        ///    </para>
        /// </devdoc>
        public static Color FromArgb(int argb) {
            return new Color((long)argb & 0xffffffff, StateARGBValueValid, null, (KnownColor)0);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromArgb1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a Color from its 32-bit component (alpha, red,
        ///       green, and blue) values.
        ///    </para>
        /// </devdoc>
        public static Color FromArgb(int alpha, int red, int green, int blue) {
            CheckByte(alpha, "alpha");
            CheckByte(red, "red");
            CheckByte(green, "green");
            CheckByte(blue, "blue");
            return new Color(MakeArgb((byte) alpha, (byte) red, (byte) green, (byte) blue), StateARGBValueValid, null, (KnownColor)0);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromArgb2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Drawing.Color'/> from the specified <see cref='System.Drawing.Color'/>, but with
        ///       the new specified alpha value.
        ///    </para>
        /// </devdoc>
        public static Color FromArgb(int alpha, Color baseColor) {
            CheckByte(alpha, "alpha");
            // unchecked - because we already checked that alpha is a byte in CheckByte above
            return new Color(MakeArgb(unchecked((byte) alpha), baseColor.R, baseColor.G, baseColor.B), StateARGBValueValid, null, (KnownColor)0);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromArgb3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Color'/> from the specified red, green, and
        ///       blue values.
        ///    </para>
        /// </devdoc>
        public static Color FromArgb(int red, int green, int blue) {
            return FromArgb(255, red, green, blue);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromKnownColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Color'/> from the specified <see cref='System.Drawing.KnownColor'/> .
        ///    </para>
        /// </devdoc>
        public static Color FromKnownColor(KnownColor color) {
            if( !ClientUtils.IsEnumValid( color, unchecked((int) color), (int) KnownColor.ActiveBorder, (int) KnownColor.MenuHighlight ) ) {
                return Color.FromName(color.ToString());
            }
            return new Color(color);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.FromName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Color'/> with the specified name.
        ///    </para>
        /// </devdoc>
        public static Color FromName(string name) {
            // try to get a known color first
            object color = ColorConverter.GetNamedColor(name);
            if (color != null) {
                return (Color)color;
            }
            // otherwise treat it as a named color
            return new Color(NotDefinedValue, StateNameValid, name, (KnownColor)0);
        }

        /// <summary>
        ///       Returns the Hue-Saturation-Lightness (HSL) lightness
        ///       for this <see cref='System.Drawing.Color'/> .
        /// </summary>
        public float GetBrightness() {
            float r = (float)R / 255.0f;
            float g = (float)G / 255.0f;
            float b = (float)B / 255.0f;

            float max, min;

            max = r; min = r;

            if (g > max) max = g;
            if (b > max) max = b;

            if (g < min) min = g;
            if (b < min) min = b;

            return(max + min) / 2;
        }

        /// <summary>
        ///       Returns the Hue-Saturation-Lightness (HSL) hue
        ///       value, in degrees, for this <see cref='System.Drawing.Color'/> .  
        ///       If R == G == B, the hue is meaningless, and the return value is 0.
        /// </summary>
        public Single GetHue() {
            if (R == G && G == B)
                return 0; // 0 makes as good an UNDEFINED value as any
            
            float r = (float)R / 255.0f;
            float g = (float)G / 255.0f;
            float b = (float)B / 255.0f;

            float max, min;
            float delta;
            float hue = 0.0f;

            max = r; min = r;

            if (g > max) max = g;
            if (b > max) max = b;

            if (g < min) min = g;
            if (b < min) min = b;

            delta = max - min;

            if (r == max) {
                hue = (g - b) / delta;
            }
            else if (g == max) {
                hue = 2 + (b - r) / delta;
            }
            else if (b == max) {
                hue = 4 + (r - g) / delta;
            }
            hue *= 60;

            if (hue < 0.0f) {
                hue += 360.0f;
            }
            return hue;
        }

        /// <summary>
        ///   The Hue-Saturation-Lightness (HSL) saturation for this
        ///    <see cref='System.Drawing.Color'/>
        /// </summary>
        public float GetSaturation() {
            float r = (float)R / 255.0f;
            float g = (float)G / 255.0f;
            float b = (float)B / 255.0f;

            float max, min;
            float l, s = 0;

            max = r; min = r;

            if (g > max) max = g;
            if (b > max) max = b;

            if (g < min) min = g;
            if (b < min) min = b;

            // if max == min, then there is no color and
            // the saturation is zero.
            //
            if (max != min) {
                l = (max + min) / 2;

                if (l <= .5) {
                    s = (max - min)/(max + min);
                }
                else {
                    s = (max - min)/(2 - max - min);
                }
            }
            return s;
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.ToArgb"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the ARGB value of this <see cref='System.Drawing.Color'/> .
        ///    </para>
        /// </devdoc>
        public int ToArgb() {
            return unchecked((int)Value);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.ToKnownColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Drawing.KnownColor'/> value for this color, if it is
        ///       based on a <see cref='System.Drawing.KnownColor'/> .
        ///    </para>
        /// </devdoc>
        public KnownColor ToKnownColor() {
            return(KnownColor)knownColor;
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.ToString"]/*' />
        /// <devdoc>
        ///    Converts this <see cref='System.Drawing.Color'/> to a human-readable
        ///    string.
        /// </devdoc>
        public override string ToString() {
            StringBuilder sb = new StringBuilder(32);
            sb.Append(GetType().Name);
            sb.Append(" [");

            if ((state & StateNameValid) != 0) {
                sb.Append(Name);
            }
            else if ((state & StateKnownColorValid) != 0) {
                sb.Append(Name);
            }
            else if ((state & StateValueMask) != 0) {
                sb.Append("A=");
                sb.Append(A);
                sb.Append(", R=");
                sb.Append(R);
                sb.Append(", G=");
                sb.Append(G);
                sb.Append(", B=");
                sb.Append(B);
            }
            else {
                sb.Append("Empty");
            }


            sb.Append("]");

            return sb.ToString();
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.operator=="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two specified <see cref='System.Drawing.Color'/> objects
        ///       are equivalent.
        ///    </para>
        /// </devdoc>
        public static bool operator ==(Color left, Color right) {
            if (left.value == right.value
                && left.state == right.state
                && left.knownColor == right.knownColor) {

                if (left.name == right.name) {
                    return true;
                }

                if (left.name == (object) null || right.name == (object) null) {
                    return false;
                }

                return left.name.Equals(right.name);
            }

            return false;
        }
        
        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two specified <see cref='System.Drawing.Color'/> objects
        ///       are equivalent.
        ///    </para>
        /// </devdoc>
        public static bool operator !=(Color left, Color right) {
            return !(left == right);
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.Equals"]/*' />
        /// <devdoc>
        ///    Tests whether the specified object is a
        /// <see cref='System.Drawing.Color'/> 
        /// and is equivalent to this <see cref='System.Drawing.Color'/>.
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj is Color) {
                Color right = (Color)obj;
                if (value == right.value
                    && state == right.state
                    && knownColor == right.knownColor) {

                    if (name == right.name) {
                        return true;
                    }

                    if (name == (object) null || right.name == (object) null) {
                        return false;
                    }

                    return name.Equals(name);
                }
            }
            return false;
        }

        /// <include file='doc\Color.uex' path='docs/doc[@for="Color.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return unchecked( value.GetHashCode() ^
                    state.GetHashCode() ^
                    knownColor.GetHashCode());
        }
    }
}
