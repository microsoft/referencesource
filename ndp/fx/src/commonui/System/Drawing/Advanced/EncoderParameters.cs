//------------------------------------------------------------------------------
// <copyright file="EncoderParameters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {
    using System.Text;
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;

    using System;
    using System.Drawing.Internal;
    using Marshal = System.Runtime.InteropServices.Marshal;
    using System.Drawing;

    //[StructLayout(LayoutKind.Sequential)]
    /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class EncoderParameters : IDisposable {
        EncoderParameter[] param;

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.EncoderParameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameters(int count) {
             param = new EncoderParameter[count];
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.EncoderParameters1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameters() {
             param = new EncoderParameter[1];
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.Param"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameter[] Param {
            // 

            get {
                return param;
            }
            set {
                param = value;
            }
        }

        /// <devdoc>
        ///     Copy the EncoderParameters data into a chunk of memory to be consumed by native GDI+ code.
        ///     
        ///     We need to marshal the EncoderParameters info from/to native GDI+ ourselve since the definition of the managed/unmanaged classes 
        ///     are different and the native class is a bit weird. The native EncoderParameters class is defined in GDI+ as follows:
        /// 
        ///      class EncoderParameters {
        ///          UINT Count;                      // Number of parameters in this structure
        ///          EncoderParameter Parameter[1];   // Parameter values
        ///      };
        /// 
        ///     We don't have the 'Count' field since the managed array contains it. In order for this structure to work with more than one 
        ///     EncoderParameter we need to preallocate memory for the extra n-1 elements, something like this:
        ///     
        ///         EncoderParameters* pEncoderParameters = (EncoderParameters*) malloc(sizeof(EncoderParameters) + (n-1) * sizeof(EncoderParameter));
        ///         
        ///     Also, in 64-bit platforms, 'Count' is aligned in 8 bytes (4 extra padding bytes) so we use IntPtr instead of Int32 to account for 
        ///     that (See VSW#451333).
        /// </devdoc>
        internal IntPtr ConvertToMemory() {
            int size = Marshal.SizeOf(typeof(EncoderParameter));

            int length = param.Length;
            IntPtr memory = Marshal.AllocHGlobal(checked(length * size + Marshal.SizeOf(typeof(IntPtr))));
            
            if (memory == IntPtr.Zero){
                throw SafeNativeMethods.Gdip.StatusException(SafeNativeMethods.Gdip.OutOfMemory);
            }

            Marshal.WriteIntPtr(memory, (IntPtr) length);
            
            long arrayOffset = checked((long) memory + Marshal.SizeOf(typeof(IntPtr)));

            for (int i=0; i<length; i++) {                                          
                Marshal.StructureToPtr(param[i], (IntPtr) (arrayOffset + i * size), false);
            }

            return memory;
        }

        /// <devdoc>
        ///     Copy the native GDI+ EncoderParameters data from a chunk of memory into a managed EncoderParameters object.
        ///     See ConvertToMemory for more info.
        /// </devdoc>
        [SuppressMessage("Microsoft.Performance", "CA1808:AvoidCallsThatBoxValueTypes")]
        internal static EncoderParameters ConvertFromMemory(IntPtr memory) {
            if (memory == IntPtr.Zero) {
                throw SafeNativeMethods.Gdip.StatusException(SafeNativeMethods.Gdip.InvalidParameter);
            }

            int count = Marshal.ReadIntPtr(memory).ToInt32();

            EncoderParameters p = new EncoderParameters(count);
            int size = Marshal.SizeOf(typeof(EncoderParameter));
            long arrayOffset = (long)memory + Marshal.SizeOf(typeof(IntPtr));

            IntSecurity.UnmanagedCode.Assert();
      
            try {     
            
                for (int i=0; i<count; i++) {
                    Guid guid = (Guid) UnsafeNativeMethods.PtrToStructure((IntPtr)( i * size + arrayOffset), typeof(Guid));
                    int numberOfValues = Marshal.ReadInt32((IntPtr)( i * size + arrayOffset + 16));
                    EncoderParameterValueType type = (EncoderParameterValueType) Marshal.ReadInt32((IntPtr)(i * size + arrayOffset + 20));
                    IntPtr value = Marshal.ReadIntPtr((IntPtr)(i * size + arrayOffset + 24));

                    p.param[i] = new EncoderParameter(new Encoder(guid), numberOfValues, type, value);
                }
            }
            finally {
                System.Security.CodeAccessPermission.RevertAssert();
            } 
            
            return p;
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.Dispose"]/*' />
        public void Dispose() {
            foreach (EncoderParameter p in param) {
                if( p != null ){
                    p.Dispose();
                }
            }
            param = null;
        }
    }
}

