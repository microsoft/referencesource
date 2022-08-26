// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------

using System.Windows.Controls;

namespace System.Windows
{
    /// <summary>
    ///     EventArgs for VisualStateGroup.CurrentStateChanging and CurrentStateChanged events.
    /// </summary>
    /// <remark>
    ///     This class works on Framework elements, however we call the property 'Control' for name-compat with what SL already released.
    /// </remark>
    public sealed class VisualStateChangedEventArgs : EventArgs
    {
        internal VisualStateChangedEventArgs(VisualState oldState, VisualState newState, FrameworkElement control, FrameworkElement stateGroupsRoot)
        {
            _oldState = oldState;
            _newState = newState;
            _control = control;
            _stateGroupsRoot = stateGroupsRoot;
        }

        /// <summary>
        ///     The old state the control is transitioning from
        /// </summary>
        public VisualState OldState
        {
            get
            {
                return _oldState;
            }
        }

        /// <summary>
        ///     The new state the control is transitioning to
        /// </summary>
        public VisualState NewState
        {
            get
            {
                return _newState;
            }
        }

        /// <summary>
        ///     The control involved in the state change
        /// </summary>
        public FrameworkElement Control
        {
            get
            {
                return _control;
            }
        }

        /// <summary>
        ///     The element that contained the VisualStateGroups and/or custom VSM
        /// </summary>
        public FrameworkElement StateGroupsRoot
        {
            get
            {
                return _stateGroupsRoot;
            }
        }

        private VisualState _oldState;
        private VisualState _newState;
        private FrameworkElement _control;
        private FrameworkElement _stateGroupsRoot;
    }
}
