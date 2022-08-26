//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Drawing.UnsafeNativeMethods..ctor()")]

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.Versioning;

    [
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
        
        [SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage")]
        [DllImport(ExternDll.Kernel32, SetLastError=true, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        public static extern void CopyMemory(HandleRef destData, HandleRef srcData, int size);

        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true, EntryPoint="GetDC", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.Process)]
        private static extern IntPtr IntGetDC(HandleRef hWnd);
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public static IntPtr GetDC(HandleRef hWnd) {
            return System.Internal.HandleCollector.Add(IntGetDC(hWnd), SafeNativeMethods.CommonHandles.HDC);
        }

        [DllImport(ExternDll.Gdi32, SetLastError=true, ExactSpelling=true, EntryPoint="DeleteDC", CharSet=CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        private static extern bool IntDeleteDC(HandleRef hDC);
        public static bool DeleteDC(HandleRef hDC) {
            System.Internal.HandleCollector.Remove((IntPtr)hDC, SafeNativeMethods.CommonHandles.GDI);
            return IntDeleteDC(hDC);
        }
        
        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true, EntryPoint="ReleaseDC", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        private static extern int IntReleaseDC(HandleRef hWnd, HandleRef hDC);
        public static int ReleaseDC(HandleRef hWnd, HandleRef hDC) {
            System.Internal.HandleCollector.Remove((IntPtr)hDC, SafeNativeMethods.CommonHandles.HDC);
            return IntReleaseDC(hWnd, hDC);
        }

#if false
        // Not currently used.
        [DllImport(ExternDll.Kernel32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        public static extern IntPtr GetModuleHandle(string modName);

        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true)]
        public static extern IntPtr GetProcessWindowStation();
        
        [DllImport(ExternDll.Gdi32, SetLastError=true, EntryPoint="CreateDC", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateDC(string lpszDriverName, string lpszDeviceName, string lpszOutput, HandleRef /*DEVMODE*/ lpInitData);
        public static IntPtr CreateDC(String lpszDriverName, string lpszDeviceName, string lpszOutput, HandleRef /*DEVMODE*/ lpInitData) {
            return System.Internal.HandleCollector.Add(IntCreateDC(lpszDriverName, lpszDeviceName, lpszOutput, lpInitData), SafeNativeMethods.CommonHandles.HDC);
        }

        [DllImport(ExternDll.User32, SetLastError=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool PeekMessage([In, Out] ref SafeNativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        
        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern bool PeekMessageA([In, Out] ref SafeNativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        
        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern bool PeekMessageW([In, Out] ref SafeNativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        
        [DllImport(ExternDll.Gdi32, SetLastError=true, EntryPoint="CreateIC", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateIC(string lpszDriverName, string lpszDeviceName, string lpszOutput, HandleRef /*DEVMODE*/ lpInitData);
        public static IntPtr CreateIC(string lpszDriverName, string lpszDeviceName, string lpszOutput, HandleRef /*DEVMODE*/ lpInitData) {
            return System.Internal.HandleCollector.Add(IntCreateIC(lpszDriverName, lpszDeviceName, lpszOutput, lpInitData), SafeNativeMethods.CommonHandles.HDC);
        }

        public static bool DeleteHDC(HandleRef hDC) {
            System.Internal.HandleCollector.Remove((IntPtr)hDC, SafeNativeMethods.CommonHandles.HDC);
            return IntDeleteDC(hDC);
        }

        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling = true)]
        public static extern IntPtr WindowFromDC( HandleRef hDC );
#endif

        [DllImport(ExternDll.Gdi32, SetLastError=true, ExactSpelling=true, EntryPoint="CreateCompatibleDC", CharSet=CharSet.Auto)]
        [ResourceExposure(ResourceScope.Process)]
        private static extern IntPtr IntCreateCompatibleDC(HandleRef hDC);
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public static IntPtr CreateCompatibleDC(HandleRef hDC) {
            return System.Internal.HandleCollector.Add(IntCreateCompatibleDC(hDC), SafeNativeMethods.CommonHandles.GDI);
        }
        
        [DllImport(ExternDll.Gdi32, SetLastError=true, ExactSpelling=true, CharSet=CharSet.Auto)]
        [ResourceExposure(ResourceScope.Process)]
        public static extern IntPtr GetStockObject(int nIndex);
        
        [DllImport(ExternDll.Kernel32, SetLastError=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.AppDomain)]
        public static extern int GetSystemDefaultLCID();
       
        [DllImport(ExternDll.User32, SetLastError=true, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        public static extern int GetSystemMetrics(int nIndex);
       
        [DllImport(ExternDll.User32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        [ResourceExposure(ResourceScope.None)]
        public static extern bool SystemParametersInfo(int uiAction, int uiParam, [In, Out] NativeMethods.NONCLIENTMETRICS pvParam, int fWinIni);
       
        [DllImport(ExternDll.User32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        [ResourceExposure(ResourceScope.None)]
        public static extern bool SystemParametersInfo(int uiAction, int uiParam, [In, Out] SafeNativeMethods.LOGFONT pvParam, int fWinIni);
       
        [DllImport(ExternDll.Gdi32, SetLastError=true, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        public static extern int GetDeviceCaps(HandleRef hDC, int nIndex);
       
        [DllImport(ExternDll.Gdi32, SetLastError=true, ExactSpelling=true, CharSet=CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        public static extern int GetObjectType(HandleRef hObject);

        // SECUNDONE : For some reason "PtrToStructure" requires super high permission.. put this 
        //           : assert here until we can get a resolution on this.
        //           : this is ok as long as the lparam is not obtained from external code.
        [ReflectionPermission(SecurityAction.Assert, Unrestricted=true),
         SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static object PtrToStructure(IntPtr lparam, Type cls) {
            return Marshal.PtrToStructure(lparam, cls);
        }

        // SECUNDONE : For some reason "PtrToStructure" requires super high permission.. put this 
        //           : assert here until we can get a resolution on this.
        //           : this is ok as long as the lparam is not obtained from external code.
        [ReflectionPermission(SecurityAction.Assert, Unrestricted=true),
         SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void PtrToStructure(IntPtr lparam, object data) {
            Marshal.PtrToStructure(lparam, data);
        }

        [ComImport(), Guid("0000000C-0000-0000-C000-000000000046"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        public interface IStream {
             int Read(
                    [In] 
                     IntPtr buf,
                    [In] 
                     int len);

            
             int Write(
                    [In] 
                     IntPtr buf,
                    [In] 
                     int len);

            [return: MarshalAs(UnmanagedType.I8)]
             long Seek(
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long dlibMove,
                    [In] 
                     int dwOrigin);

            
             void SetSize(
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long libNewSize);

            [return: MarshalAs(UnmanagedType.I8)]
             long CopyTo(
                    [In, MarshalAs(UnmanagedType.Interface)] 
                      UnsafeNativeMethods.IStream pstm,
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long cb,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                     long[] pcbRead);

            
             void Commit(
                    [In] 
                     int grfCommitFlags);

            
             void Revert();

            
             void LockRegion(
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long libOffset,
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long cb,
                    [In] 
                     int dwLockType);

            
             void UnlockRegion(
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long libOffset,
                    [In, MarshalAs(UnmanagedType.I8)] 
                     long cb,
                    [In] 
                     int dwLockType);

            
             void Stat(
                    [In] 
                     IntPtr pStatstg,
                    [In] 
                     int grfStatFlag);

            [return: MarshalAs(UnmanagedType.Interface)]
              UnsafeNativeMethods.IStream Clone();
        }

        internal class ComStreamFromDataStream : IStream {
            protected Stream dataStream;

            // to support seeking ahead of the stream length...
            long virtualPosition = -1;

            internal ComStreamFromDataStream(Stream dataStream) {
                if (dataStream == null) throw new ArgumentNullException("dataStream");
                this.dataStream = dataStream;
            }

            private void ActualizeVirtualPosition() {
                if (virtualPosition == -1) return;

                if (virtualPosition > dataStream.Length)
                    dataStream.SetLength(virtualPosition);

                dataStream.Position = virtualPosition;

                virtualPosition = -1;
            }

            public virtual IStream Clone() {
                NotImplemented();
                return null;
            }

            public virtual void Commit(int grfCommitFlags) {
                dataStream.Flush();
                // Extend the length of the file if needed.
                ActualizeVirtualPosition();
            }

            public virtual long CopyTo(IStream pstm, long cb, long[] pcbRead) {
                int bufsize = 4096; // one page
                IntPtr buffer = Marshal.AllocHGlobal(bufsize);
                if (buffer == IntPtr.Zero) throw new OutOfMemoryException();
                long written = 0;
                try {
                    while (written < cb) {
                        int toRead = bufsize;
                        if (written + toRead > cb) toRead  = (int) (cb - written);
                        int read = Read(buffer, toRead);
                        if (read == 0) break;
                        if (pstm.Write(buffer, read) != read) {
                            throw EFail("Wrote an incorrect number of bytes");
                        }
                        written += read;
                    }
                }
                finally {
                    Marshal.FreeHGlobal(buffer);
                }
                if (pcbRead != null && pcbRead.Length > 0) {
                    pcbRead[0] = written;
                }

                return written;
            }

            public virtual Stream GetDataStream() {
                return dataStream;
            }

            public virtual void LockRegion(long libOffset, long cb, int dwLockType) {
            }

            protected static ExternalException EFail(string msg) {
                throw new ExternalException(msg, SafeNativeMethods.E_FAIL);
            }

            protected static void NotImplemented() {
                throw new ExternalException(SR.GetString(SR.NotImplemented), SafeNativeMethods.E_NOTIMPL);
            }

            public virtual int Read(IntPtr buf, /* cpr: int offset,*/  int length) {
                //        System.Text.Out.WriteLine("IStream::Read(" + length + ")");
                byte[] buffer = new byte[length];
                int count = Read(buffer, length);
                Marshal.Copy(buffer, 0, buf, length);
                return count;
            }

            public virtual int Read(byte[] buffer, /* cpr: int offset,*/  int length) {
                ActualizeVirtualPosition();
                return dataStream.Read(buffer, 0, length);
            }

            public virtual void Revert() {
                NotImplemented();
            }

            public virtual long Seek(long offset, int origin) {
                // Console.WriteLine("IStream::Seek("+ offset + ", " + origin + ")");
                long pos = virtualPosition;
                if (virtualPosition == -1) {
                    pos = dataStream.Position;
                }
                long len = dataStream.Length;
                switch (origin) {
                    case SafeNativeMethods.StreamConsts.STREAM_SEEK_SET:
                        if (offset <= len) {
                            dataStream.Position = offset;
                            virtualPosition = -1;
                        }
                        else {
                            virtualPosition = offset;
                        }
                        break;
                    case SafeNativeMethods.StreamConsts.STREAM_SEEK_END:
                        if (offset <= 0) {
                            dataStream.Position = len + offset;
                            virtualPosition = -1;
                        }
                        else {
                            virtualPosition = len + offset;
                        }
                        break;
                    case SafeNativeMethods.StreamConsts.STREAM_SEEK_CUR:
                        if (offset+pos <= len) {
                            dataStream.Position = pos + offset;
                            virtualPosition = -1;
                        }
                        else {
                            virtualPosition = offset + pos;
                        }
                        break;
                }
                if (virtualPosition != -1) {
                    return virtualPosition;
                }
                else {
                    return dataStream.Position;
                }
            }

            public virtual void SetSize(long value) {
                dataStream.SetLength(value);
            }

            public virtual void Stat(IntPtr pstatstg, int grfStatFlag) {
                // GpStream has a partial implementation, but it's so partial rather 
                // restrict it to use with GDI+
                NotImplemented();
            }

            public virtual void UnlockRegion(long libOffset, long cb, int dwLockType) {
            }

            public virtual int Write(IntPtr buf, /* cpr: int offset,*/ int length) {
                byte[] buffer = new byte[length];
                Marshal.Copy(buf, buffer, 0, length);
                return Write(buffer, length);
            }

            public virtual int Write(byte[] buffer, /* cpr: int offset,*/ int length) {
                ActualizeVirtualPosition();
                dataStream.Write(buffer, 0, length);
                return length;
            }
        }

    }
}

