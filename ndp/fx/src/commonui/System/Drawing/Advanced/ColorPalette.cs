//------------------------------------------------------------------------------
// <copyright file="ColorPalette.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing;    

    /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette"]/*' />
    /// <devdoc>
    ///    Defines an array of colors that make up a
    ///    color palette.
    /// </devdoc>
    public sealed class ColorPalette {
        ///    Note (From VSWhidbey#444618): We don't provide a public constructor for ColorPalette because if we allow 
        ///    arbitrary creation of color palettes you could in theroy not only change the color entries, but the size 
        ///    of the palette and that is not valid for an image (meaning you cannot change the palette size for an image).  
        ///    ColorPalettes are only valid for "indexed" images like GIFs.

        private int flags;
        private Color[] entries;

        /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette.Flags"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies how to interpret the color
        ///       information in the array of colors.
        ///    </para>
        /// </devdoc>
        public int Flags
        {
            get {
                return flags;
            }
        }
        
        /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette.Entries"]/*' />
        /// <devdoc>
        ///    Specifies an array of <see cref='System.Drawing.Color'/> objects.
        /// </devdoc>
        public Color[] Entries
        {
            get {
                return entries;
            }
        }
        
        internal ColorPalette(int count) {
            entries = new Color[count];
        }

        internal ColorPalette() {
            entries = new Color[1];
        }

        internal void ConvertFromMemory(IntPtr memory)
        {
            // Memory layout is:
            //    UINT Flags
            //    UINT Count
            //    ARGB Entries[size]

            flags = Marshal.ReadInt32(memory);

            int size;

            size = Marshal.ReadInt32((IntPtr)((long)memory + 4));  // Marshal.SizeOf(size.GetType())

            entries = new Color[size];

            for (int i=0; i<size; i++)
            {
                // use Marshal.SizeOf()
                int argb = Marshal.ReadInt32((IntPtr)((long)memory + 8 + i*4));
                entries[i] = Color.FromArgb(argb);
            }    
        }
    
        internal IntPtr ConvertToMemory()
        {
            // Memory layout is:
            //    UINT Flags
            //    UINT Count
            //    ARGB Entries[size]

            // use Marshal.SizeOf()
            int length = entries.Length;
            IntPtr memory = Marshal.AllocHGlobal(checked(4 * (2 + length)));
            
            Marshal.WriteInt32(memory, 0, flags);
            // use Marshal.SizeOf()
            Marshal.WriteInt32((IntPtr)checked((long)memory + 4), 0, length);
            
            for (int i=0; i<length; i++)
            {
                // use Marshal.SizeOf()
                Marshal.WriteInt32((IntPtr)((long)memory + 4*(i+2)), 0, entries[i].ToArgb());
            }
            
            return memory;
        }

    }
}
