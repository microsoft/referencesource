using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Threading;
using System.Security;
using Microsoft.Win32.SafeHandles;

namespace System.Runtime.Remoting.Channels.Ipc
{
    internal class PipeHandle : CriticalHandleMinusOneIsInvalid
    {
        internal PipeHandle() : base() { }

        internal PipeHandle(IntPtr handle) : base()
        {
            SetHandle(handle);
        }

        public IntPtr Handle
        {
            get { return handle; }
        }

        protected override bool ReleaseHandle()
        {
            return NativePipe.CloseHandle(handle) != 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal class SECURITY_ATTRIBUTES {
        internal int nLength = 0;
        internal IntPtr lpSecurityDescriptor = IntPtr.Zero;
        internal int bInheritHandle = 0;
    }

    [SuppressUnmanagedCodeSecurity]
    internal static class NativePipe
    {
        private const String Kernel32 = "kernel32.dll";
        private const String AdvApi32 = "advapi32.dll";
        internal static readonly IntPtr NULL = IntPtr.Zero;
        public const uint PIPE_ACCESS_OUTBOUND = 0x00000002;
        public const uint PIPE_ACCESS_DUPLEX = 0x00000003;
        public const uint PIPE_ACCESS_INBOUND = 0x00000001;
        
        public const uint PIPE_WAIT = 0x00000000;
        public const uint PIPE_NOWAIT = 0x00000001;
        public const uint PIPE_READMODE_BYTE = 0x00000000;
        public const uint PIPE_READMODE_MESSAGE = 0x00000002;
        public const uint PIPE_TYPE_BYTE = 0x00000000;
        public const uint PIPE_TYPE_MESSAGE = 0x00000004;
        
        public const uint PIPE_CLIENT_END = 0x00000000;
        public const uint PIPE_SERVER_END = 0x00000001;
        public const uint FILE_FLAG_OVERLAPPED = 0x40000000;
        public const uint FILE_ATTRIBUTE_NORMAL = 0x00000080;
        public const uint FILE_SHARE_READ = 0x00000001;
        public const uint FILE_SHARE_WRITE = 0x00000002;
        
        public const uint PIPE_UNLIMITED_INSTANCES = 255;
        
        public const uint SECURITY_SQOS_PRESENT = 0x00100000;
        public const uint SECURITY_ANONYMOUS = 0 << 16;
        public const uint SECURITY_IDENTIFICATION = 1 << 16;
        public const uint SECURITY_IMPERSONATION = 2 << 16;
        public const uint SECURITY_DELEGATION = 3 << 16;
        
        internal const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        internal const int FORMAT_MESSAGE_FROM_SYSTEM    = 0x00001000;
        internal const int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;

        public const uint NMPWAIT_WAIT_FOREVER = 0xffffffff;
        public const uint NMPWAIT_NOWAIT = 0x00000001;
        public const uint NMPWAIT_USE_DEFAULT_WAIT = 0x00000000;
        
        public const uint GENERIC_READ = (0x80000000);
        public const uint GENERIC_WRITE = (0x40000000);
        public const uint GENERIC_EXECUTE = (0x20000000);
        public const uint GENERIC_ALL = (0x10000000);
        
        public const uint CREATE_NEW        = 1;
        public const uint CREATE_ALWAYS     = 2;
        public const uint OPEN_EXISTING     = 3;
        public const uint OPEN_ALWAYS       = 4;
        public const uint TRUNCATE_EXISTING = 5;
        public const uint FILE_FLAG_FIRST_PIPE_INSTANCE = 0x00080000;
        
        public const int INVALID_HANDLE_VALUE = -1;
        public const long ERROR_BROKEN_PIPE = 109;
        public const long ERROR_IO_PENDING = 997;
        public const long ERROR_PIPE_BUSY = 231;
        public const long ERROR_NO_DATA = 232;
        public const long ERROR_PIPE_NOT_CONNECTED = 233;
        public const long ERROR_PIPE_CONNECTED = 535;
        public const long ERROR_PIPE_LISTENING = 536;
        
        //CreatePipe

        [DllImport(Kernel32, SetLastError=true)]
        public static extern PipeHandle CreateNamedPipe(String lpName,             // pipe name
                                                 uint dwOpenMode,            // pipe open mode
                                                 uint dwPipeMode,             // pipe-specific modes
                                                 uint nMaxInstances,          // maximum number of instances
                                                 uint nOutBufferSize,         // output buffer size
                                                 uint nInBufferSize,          // input buffer size
                                                 uint nDefaultTimeOut,       // time-out interval
                                                 SECURITY_ATTRIBUTES pipeSecurityDescriptor	//SecurityAttributes attr					 // SD
                                                 );
        
        
        [DllImport(Kernel32, SetLastError=true)]
        public static extern bool ConnectNamedPipe(PipeHandle hNamedPipe,        // handle to named pipe
                                                    Overlapped lpOverlapped  // overlapped structure
                                                    );
        
        
        [DllImport(AdvApi32, SetLastError=true)]
        public static extern bool ImpersonateNamedPipeClient(PipeHandle hNamedPipe        // handle to named pipe
                                                       );
        
        [DllImport(AdvApi32)]
        public static extern bool RevertToSelf();

        [DllImport(Kernel32, SetLastError=true)]
        public static extern PipeHandle CreateFile(String lpFileName,               // file name
                                             uint dwDesiredAccess,          // access mode
                                             uint dwShareMode,             // share mode
                                             IntPtr attr,        // SecurityDescriptor
                                             uint dwCreationDisposition,     // how to create
                                             uint dwFlagsAndAttributes,    // file attributes
                                             IntPtr hTemplateFile);           // handle to template file
        
        
        [DllImport(Kernel32, SetLastError=true)]
        public static unsafe extern bool ReadFile(PipeHandle hFile,                       // handle to file
                                           byte* lpBuffer,                 // data buffer
                                           int nNumberOfBytesToRead,   // number of bytes to read
                                           ref int lpNumberOfBytesRead,  // number of bytes read
                                           IntPtr mustBeZero               // this should be IntPtr.Zero
                                           );

        [DllImport(Kernel32, SetLastError=true)]
        public static unsafe extern bool ReadFile(PipeHandle hFile,                       // handle to file
                                           byte* lpBuffer,                 // data buffer
                                           int nNumberOfBytesToRead,   // number of bytes to read
                                           IntPtr numBytesRead_mustBeZero,  // number of bytes must be zero
                                           NativeOverlapped* lpOverlapped   // overlapped buffer
                                           );
        
        
        [DllImport(Kernel32, SetLastError=true)]
        public static unsafe extern bool WriteFile(
                                            PipeHandle hFile,                          // handle to file
                                            byte* lpBuffer,                  // data buffer
                                            int nNumberOfBytesToWrite,   // number of bytes to write
                                            ref int lpNumberOfBytesWritten, // number of bytes written
                                            IntPtr lpOverlapped                 // overlapped buffer
                                            );
        
        [DllImport(Kernel32, SetLastError=true)]
        public static extern bool WaitNamedPipe(String name,
                                                 int timeout);
        
        [DllImport(Kernel32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern int FormatMessage(int dwFlags, IntPtr lpSource,
                    int dwMessageId, int dwLanguageId, StringBuilder lpBuffer,
                    int nSize, IntPtr va_list_arguments);
        
        [DllImport(Kernel32, SetLastError=true)]
        public static extern int CloseHandle(IntPtr hObject);        
        
    }
    
}


