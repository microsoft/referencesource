//------------------------------------------------------------------------------
//
// <copyright file="LoadedOrUnloadedOperation.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//      A pending loaded or unloaded operation, to be run by the MediaContext.
//
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;           // Debug.Assert

using System.Windows;               // DependencyObject
using System.Windows.Threading;     // DispatcherOperationCallback
using MS.Internal.PresentationCore; // [FriendAccessAllowed]

namespace MS.Internal
{
    [FriendAccessAllowed] // Built into Core, also used by Framework.
    internal class LoadedOrUnloadedOperation
    {
        internal LoadedOrUnloadedOperation(
            DispatcherOperationCallback callback,
            DependencyObject target)
        {
            Debug.Assert(callback != null && target != null);
            _callback = callback;
            _target = target;
        }

        internal void DoWork()
        {
            if (!_cancelled)
            {
                _callback(_target);
            }
        }

        internal void Cancel()
        {
            _cancelled = true;
        }

        private DispatcherOperationCallback _callback;
        private DependencyObject _target;
        private bool _cancelled;
    }
}
