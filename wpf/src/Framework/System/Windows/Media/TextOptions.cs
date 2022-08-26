//---------------------------------------------------------------------------
//
// Copyright(C) Microsoft Corporation.  All rights reserved.
// 
// File: TextOptions.cs
//
// Description: TextOptions groups attached properties that affect the way 
//              WPF displays text such as TextFormattingMode 
//              and TextRenderingMode.
//
// History:  
//              05/05/2009 : Microsoft - created.
//
//---------------------------------------------------------------------------

using System.Windows;
using MS.Internal.Media;
namespace System.Windows.Media
{
    /// <summary>
    /// Provide access to text options of element in syntax of TextOptions.xxx = yyy;
    /// Actual data is stored in the owner.
    /// </summary>
    public static class TextOptions
    {
        #region Dependency Properties

        /// <summary> Text formatting mode Property </summary>
        public static readonly DependencyProperty TextFormattingModeProperty =
                DependencyProperty.RegisterAttached(
                        "TextFormattingMode",
                        typeof(TextFormattingMode),
                        typeof(TextOptions),
                        new FrameworkPropertyMetadata(
                                TextFormattingMode.Ideal,
                                FrameworkPropertyMetadataOptions.AffectsMeasure | FrameworkPropertyMetadataOptions.AffectsRender | FrameworkPropertyMetadataOptions.Inherits),
                        new ValidateValueCallback(IsTextFormattingModeValid));

        internal static bool IsTextFormattingModeValid(object valueObject)
        {
            TextFormattingMode value = (TextFormattingMode) valueObject;

            return (value == TextFormattingMode.Ideal) || 
                   (value == TextFormattingMode.Display);
        }                                       
        
        /// <summary> Text rendering Property </summary>
        public static readonly DependencyProperty TextRenderingModeProperty =
                DependencyProperty.RegisterAttached(
                        "TextRenderingMode",
                        typeof(TextRenderingMode),
                        typeof(TextOptions),
                        new FrameworkPropertyMetadata(
                                TextRenderingMode.Auto,
                                FrameworkPropertyMetadataOptions.AffectsRender | FrameworkPropertyMetadataOptions.Inherits),
                        new ValidateValueCallback(System.Windows.Media.ValidateEnums.IsTextRenderingModeValid));

        /// <summary> Text hinting property </summary>
        public static readonly DependencyProperty TextHintingModeProperty = 
                TextOptionsInternal.TextHintingModeProperty.AddOwner(
                        typeof(TextOptions));

        #endregion Dependency Properties

        
        #region Attached Properties Setters

        public static void SetTextFormattingMode(DependencyObject element, TextFormattingMode value)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            element.SetValue(TextFormattingModeProperty, value);
        }

        public static TextFormattingMode GetTextFormattingMode(DependencyObject element)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            return (TextFormattingMode)element.GetValue(TextFormattingModeProperty);
        }
        
        public static void SetTextRenderingMode(DependencyObject element, TextRenderingMode value)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            element.SetValue(TextRenderingModeProperty, value);
        }

        public static TextRenderingMode GetTextRenderingMode(DependencyObject element)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            return (TextRenderingMode)element.GetValue(TextRenderingModeProperty);
        }


        public static void SetTextHintingMode(DependencyObject element, TextHintingMode value)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            element.SetValue(TextHintingModeProperty, value);
        }

        public static TextHintingMode GetTextHintingMode(DependencyObject element)
        {
            if (element == null)
            {
                throw new ArgumentNullException("element");
            }

            return (TextHintingMode)element.GetValue(TextHintingModeProperty);
        }


        #endregion Attached Properties Getters and Setters
    }
}
