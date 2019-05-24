//------------------------------------------------------------------------------
// <copyright file="SessionStateChangeControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess
{
    using System;

    /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange"]/*' />
    /// <devdoc>
    ///    <para>Status code describing the reason the session state change notification was sent.</para>
    /// </devdoc>
    public enum SessionChangeReason
    {
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.BatteryLow"]/*' />
        /// <devdoc>
        ///    <para>A session was connected to the console session. </para>
        /// </devdoc>
        ConsoleConnect = NativeMethods.WTS_CONSOLE_CONNECT,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.ConsoleDisconnect"]/*' />
        /// <devdoc>
        ///    <para>A session was disconnected from the console session. </para>
        /// </devdoc>
        ConsoleDisconnect = NativeMethods.WTS_CONSOLE_DISCONNECT,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.RemoteConnect"]/*' />
        /// <devdoc>
        ///    <para>A session was connected to the remote session. </para>
        /// </devdoc>
        RemoteConnect = NativeMethods.WTS_REMOTE_CONNECT,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.RemoteDisconnect"]/*' />
        /// <devdoc>
        ///    <para>A session was disconnected from the remote session. </para>
        /// </devdoc>
        RemoteDisconnect = NativeMethods.WTS_REMOTE_DISCONNECT,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.SessionLogon"]/*' />
        /// <devdoc>
        ///    <para>A user has logged on to the session. </para>
        /// </devdoc>
        SessionLogon = NativeMethods.WTS_SESSION_LOGON,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.SessionLogoff"]/*' />
        /// <devdoc>
        ///    <para>A user has logged off the session. </para>
        /// </devdoc>
        SessionLogoff = NativeMethods.WTS_SESSION_LOGOFF,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.SessionLock"]/*' />
        /// <devdoc>
        ///    <para>A session has been locked. </para>
        /// </devdoc>
        SessionLock = NativeMethods.WTS_SESSION_LOCK,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.SessionUnlock"]/*' />
        /// <devdoc>
        ///    <para>A session has been unlocked. </para>
        /// </devdoc>
        SessionUnlock = NativeMethods.WTS_SESSION_UNLOCK,
        /// <include file='doc\SessionStateChange.uex' path='docs/doc[@for="SessionStateChange.SessionRemoteControl"]/*' />
        /// <devdoc>
        ///    <para>A session has changed its remote controlled status. </para>
        /// </devdoc>
        SessionRemoteControl = NativeMethods.WTS_SESSION_REMOTE_CONTROL
    }


    public struct SessionChangeDescription {
        SessionChangeReason _reason;
        int _id;
        
        internal SessionChangeDescription(SessionChangeReason reason, int id) {
            _reason = reason;
            _id = id;
        }

        public SessionChangeReason Reason {
            get {
                return _reason; 
            }
        }

        public int SessionId {
            get {
                return _id;
            }
        }

        public override bool Equals(object obj) {
            if(obj == null || !(obj is SessionChangeDescription)) {
                return false;
            }
            else {
                return Equals((SessionChangeDescription)obj);
            }
        }

        public override int GetHashCode() {
            return (int)_reason ^ _id;
        }

        public bool Equals(SessionChangeDescription changeDescription) {
            return (_reason == changeDescription._reason) && (_id == changeDescription._id);
        }

        public static bool operator ==(SessionChangeDescription a, SessionChangeDescription b) {
            return a.Equals(b);
        }

        public static bool operator !=(SessionChangeDescription a, SessionChangeDescription b) {
            return !a.Equals(b);
        }
    }
}

