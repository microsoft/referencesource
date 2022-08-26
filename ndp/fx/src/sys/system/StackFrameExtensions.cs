// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.ComponentModel;

namespace System.Diagnostics
{
    [EditorBrowsable(EditorBrowsableState.Never)]
    public static partial class StackFrameExtensions
    {
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool HasNativeImage(this StackFrame stackFrame)
        {
            return stackFrame.GetNativeImageBase() != IntPtr.Zero;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool HasMethod(this StackFrame stackFrame)
        {
            return stackFrame.GetMethod() != null;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool HasILOffset(this StackFrame stackFrame)
        {
            return stackFrame.GetILOffset() != StackFrame.OFFSET_UNKNOWN;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static bool HasSource(this StackFrame stackFrame)
        {
            return stackFrame.GetFileName() != null;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr GetNativeIP(this StackFrame stackFrame)
        {
            return IntPtr.Zero;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr GetNativeImageBase(this StackFrame stackFrame)
        {
            return IntPtr.Zero;
        }
    }
}
