//------------------------------------------------------------------------------
// <copyright file="Columns.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop
{
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Globalization; //for CultureInfo
    using Microsoft.Win32;

    internal class Columns
    {
        private int maxCount;
        private MQCOLUMNSET columnSet = new MQCOLUMNSET();

        public Columns(int maxCount)
        {
            this.maxCount = maxCount;
            this.columnSet.columnIdentifiers = Marshal.AllocHGlobal(maxCount * 4);
            this.columnSet.columnCount = 0;
        }

        public virtual void AddColumnId(int columnId)
        {
            lock (this)
            {
                if (this.columnSet.columnCount >= this.maxCount)
                    throw new InvalidOperationException(Res.GetString(Res.TooManyColumns, this.maxCount.ToString(CultureInfo.CurrentCulture)));

                ++this.columnSet.columnCount;
                this.columnSet.SetId(columnId, this.columnSet.columnCount - 1);
            }
        }

        public virtual MQCOLUMNSET GetColumnsRef()
        {
            return this.columnSet;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class MQCOLUMNSET
        {
            public int columnCount;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr columnIdentifiers;

            ~MQCOLUMNSET()
            {
                if (this.columnIdentifiers != (IntPtr)0)
                {
                    Marshal.FreeHGlobal(this.columnIdentifiers);
                    this.columnIdentifiers = (IntPtr)0;
                }
            }

            public virtual void SetId(int columnId, int index)
            {
                Marshal.WriteInt32((IntPtr)((long)this.columnIdentifiers + (index * 4)), columnId);
            }
        }
    }
}
