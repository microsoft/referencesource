// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       IpcPort.cs
//  Author:   Microsoft@Microsoft.Com
//  Summary:    Implements an abstraction over Named pipes
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Threading;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Runtime.Remoting.Channels.Ipc
{
    internal class IpcPort : IDisposable
    {
        private PipeHandle _handle;
        private string _portName;
        private bool _cacheable;
        private const string prefix = @"\\.\pipe\";  // To make sure we are on the same machine
        private const string networkSidSddlForm = @"S-1-5-2";  // This is the wellknown sid for network sid
        private const string authenticatedUserSidSddlForm = @"S-1-5-11";  // This is the wellknown sid for authenticated user sid
        private static CommonSecurityDescriptor s_securityDescriptor = CreateSecurityDescriptor(null);
        
        private IpcPort(string portName, PipeHandle handle)
        {
            _portName = portName;
            _handle = handle;
            _cacheable = true;
#pragma warning disable 618
            // Bind the current handle to the threadpool for IOCompletion
            ThreadPool.BindHandle(_handle.Handle);
#pragma warning restore 618
        }

        internal string Name { get { return _portName; } }

        internal bool Cacheable{ get { return _cacheable;} set { _cacheable = value;} }

        internal static CommonSecurityDescriptor CreateSecurityDescriptor(SecurityIdentifier userSid)
        {
            SecurityIdentifier sid = new SecurityIdentifier(networkSidSddlForm);
            DiscretionaryAcl dacl = new DiscretionaryAcl(false, false, 1);
            // Deny all access to NetworkSid
            dacl.AddAccess(AccessControlType.Deny, sid, -1, InheritanceFlags.None, PropagationFlags.None);
            if (userSid != null)
                dacl.AddAccess(AccessControlType.Allow, userSid, -1, InheritanceFlags.None, PropagationFlags.None);
            // Add access to the current user creating the pipe
            dacl.AddAccess(AccessControlType.Allow, WindowsIdentity.GetCurrent().User, -1, InheritanceFlags.None, PropagationFlags.None);
            // Initialize and return the CommonSecurityDescriptor
            return new CommonSecurityDescriptor(false, false, ControlFlags.OwnerDefaulted | ControlFlags.GroupDefaulted | ControlFlags.DiscretionaryAclPresent, null, null, null, dacl);;
        }
            
        internal static IpcPort Create(String portName, CommonSecurityDescriptor securityDescriptor, bool exclusive)
        {
            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                throw new NotSupportedException(CoreChannel.GetResourceString("Remoting_Ipc_Win9x"));
            }
            PipeHandle handle = null;
            // Add the prefix to the portName
            string pipeName = prefix + portName;
            SECURITY_ATTRIBUTES attr = new SECURITY_ATTRIBUTES();
            attr.nLength = (int)Marshal.SizeOf(attr);
            byte[] sd = null;
            // If no securityDescriptor was set by the user use the default
            if (securityDescriptor == null)
            {
                securityDescriptor = s_securityDescriptor;
            }

            sd = new byte[securityDescriptor.BinaryLength];
            // Get the binary form of the descriptor
            securityDescriptor.GetBinaryForm(sd, 0);
               
            GCHandle pinningHandle = GCHandle.Alloc(sd, GCHandleType.Pinned);
            // get the address of the security descriptor
            attr.lpSecurityDescriptor = Marshal.UnsafeAddrOfPinnedArrayElement(sd, 0);

            // Create the named pipe with the appropriate name
            handle = NativePipe.CreateNamedPipe(pipeName, 
                                      NativePipe.PIPE_ACCESS_DUPLEX  | NativePipe.FILE_FLAG_OVERLAPPED 
                                            | (exclusive ? NativePipe.FILE_FLAG_FIRST_PIPE_INSTANCE : 0x0), // Or exclusive flag 
                                      NativePipe.PIPE_TYPE_BYTE | NativePipe.PIPE_READMODE_BYTE | NativePipe.PIPE_WAIT,
                                      NativePipe.PIPE_UNLIMITED_INSTANCES,
                                      8192,
                                      8192,
                                      NativePipe.NMPWAIT_WAIT_FOREVER,
                                      attr); 

            pinningHandle.Free();
            if (handle.Handle.ToInt32() == NativePipe.INVALID_HANDLE_VALUE){
                int error = Marshal.GetLastWin32Error();
                throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_CreateIpcFailed"), GetMessage(error)));
            }

             return new IpcPort(portName, handle);   
        }

        public bool WaitForConnect()
        {
            // Wait for clients to connect
            bool status = NativePipe.ConnectNamedPipe(_handle, null);
            
            return status ? true : (Marshal.GetLastWin32Error() == NativePipe.ERROR_PIPE_CONNECTED);
        }

        internal static IpcPort Connect(String portName, bool secure, TokenImpersonationLevel impersonationLevel, int timeout)
        {
            string pipeName = prefix + portName;
            uint impersonation = NativePipe.SECURITY_SQOS_PRESENT;

            // convert the impersonation Level to the correct flag
            if (secure) {
                switch (impersonationLevel)
                {
                    case TokenImpersonationLevel.None:
                            impersonation = NativePipe.SECURITY_SQOS_PRESENT;
                            break;
                    case TokenImpersonationLevel.Identification:
                            impersonation = NativePipe.SECURITY_SQOS_PRESENT | NativePipe.SECURITY_IDENTIFICATION;
                            break;
                    case TokenImpersonationLevel.Impersonation:
                            impersonation = NativePipe.SECURITY_SQOS_PRESENT | NativePipe.SECURITY_IMPERSONATION;
                            break;
                    case TokenImpersonationLevel.Delegation:
                            impersonation = NativePipe.SECURITY_SQOS_PRESENT | NativePipe.SECURITY_DELEGATION;
                            break;
                }
            }

            while (true)
            {
                // Invoke CreateFile with the pipeName to open a client side connection                
                PipeHandle handle = NativePipe.CreateFile(pipeName, 
                                     NativePipe.GENERIC_READ | NativePipe.GENERIC_WRITE , 
                                     NativePipe.FILE_SHARE_READ | 
                                     NativePipe.FILE_SHARE_WRITE,
                                     IntPtr.Zero, 
                                     NativePipe.OPEN_EXISTING, 
                                     NativePipe.FILE_ATTRIBUTE_NORMAL | 
                                     NativePipe.FILE_FLAG_OVERLAPPED |
                                     impersonation, 
                                     IntPtr.Zero);

                if(handle.Handle.ToInt32() != NativePipe.INVALID_HANDLE_VALUE)
                    return new IpcPort(portName, handle);

                int error = Marshal.GetLastWin32Error();
                if(error != NativePipe.ERROR_PIPE_BUSY)
                {
                    throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_ConnectIpcFailed"), GetMessage(error)));
                }

                if(!NativePipe.WaitNamedPipe(pipeName, timeout))
                {
                    throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_Busy"), GetMessage(error)));
                }
            }            

        }

        // Gets an error message for a Win32 error code.
        internal static String GetMessage(int errorCode) {
            StringBuilder sb = new StringBuilder(512);
            int result = NativePipe.FormatMessage(NativePipe.FORMAT_MESSAGE_IGNORE_INSERTS |
                NativePipe.FORMAT_MESSAGE_FROM_SYSTEM | NativePipe.FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NativePipe.NULL, errorCode, 0, sb, sb.Capacity, NativePipe.NULL);
            if (result != 0) {
                // result is the # of characters copied to the StringBuilder on NT,
                // but on Win9x, it appears to be the number of MBCS bytes.
                // Just give up and return the String as-is...
                String s = sb.ToString();
                return s;
            }
            else {
                return String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_UnknownError_Num"), errorCode.ToString(CultureInfo.InvariantCulture));
            }
        }

        internal void ImpersonateClient()
        {
            bool status = NativePipe.ImpersonateNamedPipeClient(_handle);

            if (!status)
            {
                int error = Marshal.GetLastWin32Error();
                throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_ImpersonationFailed"), GetMessage(error)));
            }
        }

        internal unsafe int Read(byte[] data, int offset, int length)
        {
            bool status = false;
            int numBytesRead = 0;

            fixed(byte* p = data) {
                    status = NativePipe.ReadFile(_handle, p + offset, length, ref numBytesRead, IntPtr.Zero);
            }
            if (!status) {
                int error = Marshal.GetLastWin32Error();
                throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_ReadFailure"), GetMessage(error)));
            }
            else
                return numBytesRead;
        }

        internal unsafe IAsyncResult BeginRead(byte[] data, int offset, int size, AsyncCallback callback, object state)
        {
            PipeAsyncResult asyncResult = new PipeAsyncResult(callback);
            // Create a managed overlapped class
            // We will set the file offsets later
            Overlapped overlapped = new Overlapped(0, 0, IntPtr.Zero, asyncResult);

            // Pack the Overlapped class, and store it in the async result
            NativeOverlapped* intOverlapped;
            intOverlapped = overlapped.UnsafePack(IOCallback, data);
            asyncResult._overlapped = intOverlapped;
            bool status;

            // pin the buffer and read data with overlapped
            fixed(byte* p = data) {
                    status = NativePipe.ReadFile(_handle, p + offset, size, IntPtr.Zero, intOverlapped);
            }
            if (!status)
            {
                int error = Marshal.GetLastWin32Error();
                // For pipes, when they hit EOF, they will come here.
                if (error == NativePipe.ERROR_BROKEN_PIPE) {
                    // Not an error, but EOF.  AsyncFSCallback will NOT be 
                    // called.  Call the user callback here.
                    asyncResult.CallUserCallback();                 
                    // EndRead will free the Overlapped struct correctly.
                }
                else if (error != NativePipe.ERROR_IO_PENDING)
                    throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_ReadFailure"), GetMessage(error)));
            }
            return asyncResult;
        }

        private unsafe static readonly IOCompletionCallback IOCallback = new IOCompletionCallback(IpcPort.AsyncFSCallback);
        unsafe private static void AsyncFSCallback(uint errorCode, uint numBytes, NativeOverlapped* pOverlapped)
        {
            // Unpack overlapped
            Overlapped overlapped = Overlapped.Unpack(pOverlapped);
            // Free the overlapped struct in EndRead/EndWrite.
            
            // Extract async result from overlapped 
            PipeAsyncResult asyncResult = 
                (PipeAsyncResult)overlapped.AsyncResult;
            asyncResult._numBytes = (int)numBytes;
            
            // Handle reading from & writing to closed pipes.  While I'm not sure
            // this is entirely necessary anymore, maybe it's possible for 
            // an async read on a pipe to be issued and then the pipe is closed, 
            // returning this error.  This may very well be necessary.
            if (errorCode == NativePipe.ERROR_BROKEN_PIPE)
                errorCode = 0;
            
            asyncResult._errorCode = (int)errorCode;
            // Call the user-provided callback.  It can and often should
            // call EndRead or EndWrite.  There's no reason to use an async 
            // delegate here - we're already on a threadpool thread.  
            // IAsyncResult's completedSynchronously property must return
            // false here, saying the user callback was called on another thread.
            AsyncCallback userCallback = asyncResult._userCallback;
            userCallback(asyncResult);
        }

        internal unsafe int EndRead(IAsyncResult iar)
        {
            PipeAsyncResult ar = iar as PipeAsyncResult;
            // Free memory & GC handles.
            NativeOverlapped* overlappedPtr = ar._overlapped;
            if (overlappedPtr != null)
                Overlapped.Free(overlappedPtr);

            // Now check for any error during the read.
            if (ar._errorCode != 0)
                throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_ReadFailure"), GetMessage(ar._errorCode)));

            return ar._numBytes;
        }

        internal unsafe void Write(byte[] data, int offset, int size)
        {
            int numBytesWritten = 0;
            bool status = false;

            // pin the buffer and write data
            fixed(byte* p = data) {
                status = NativePipe.WriteFile(_handle, p + offset, size, ref numBytesWritten, IntPtr.Zero);
            }

            if (!status) {
                int error = Marshal.GetLastWin32Error();
                throw new RemotingException(String.Format(CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Ipc_WriteFailure"), GetMessage(error)));
            }
        }

        private bool isDisposed = false;

        ~IpcPort(){
            Dispose();
        }
        
        public void Dispose()
        {
            InternalRemotingServices.RemotingAssert(_handle.Handle != IntPtr.Zero, "Handle should be valid");
            if (!isDisposed){
                _handle.Close();
                isDisposed = true;
                GC.SuppressFinalize(this);
            }
        }

        public bool IsDisposed {
            get { return isDisposed; }
        }
    }

    internal unsafe class PipeAsyncResult: IAsyncResult
    {
        internal NativeOverlapped* _overlapped;
        internal AsyncCallback _userCallback;
        internal int _numBytes;
        internal int _errorCode;

        internal PipeAsyncResult(AsyncCallback callback)
        {
            _userCallback = callback;
        }

        public bool IsCompleted { get { throw new NotSupportedException();} }
        public WaitHandle AsyncWaitHandle { get { throw new NotSupportedException();} }
        public Object     AsyncState      { get { throw new NotSupportedException();} }
        public bool       CompletedSynchronously { get { return false;} }

        
        internal void CallUserCallback()
        {
            // Call user's callback on a threadpool thread.  
            // Set completedSynchronously to false, since it's on another 
            // thread, not the main thread.
            ThreadPool.QueueUserWorkItem(new WaitCallback(CallUserCallbackWorker));
        }

        private void CallUserCallbackWorker(Object callbackState)
        {
            // This needs to call the user callback, then set _isComplete to
            // true and set the event if it exists.  This is similar to the
            // logic in AsyncFSCallback.
            _userCallback(this);
        }
    }
    
}
