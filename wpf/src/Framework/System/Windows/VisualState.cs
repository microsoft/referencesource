// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------

using System.Windows;
using System.Windows.Markup;
using System.Windows.Media.Animation;

namespace System.Windows
{
    /// <summary>
    ///     A visual state that can be transitioned into.
    /// </summary>
    [ContentProperty("Storyboard")]
    [RuntimeNameProperty("Name")]
    public class VisualState : DependencyObject
    {
        /// <summary>
        ///     The name of the VisualState.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        private static readonly DependencyProperty StoryboardProperty = 
            DependencyProperty.Register(
            "Storyboard", 
            typeof(Storyboard), 
            typeof(VisualState));

        /// <summary>
        ///     Storyboard defining the values of properties in this visual state.
        /// </summary>        
        public Storyboard Storyboard
        {
            get { return (Storyboard)GetValue(StoryboardProperty); }
            set { SetValue(StoryboardProperty, value); }
        }
    }
}
