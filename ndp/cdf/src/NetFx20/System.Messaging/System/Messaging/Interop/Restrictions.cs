//------------------------------------------------------------------------------
// <copyright file="Restrictions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop
{
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.ComponentModel;

    using Microsoft.Win32;

    internal class Restrictions
    {
        private MQRESTRICTION restrictionStructure;
        public const int PRLT = 0;
        public const int PRLE = 1;
        public const int PRGT = 2;
        public const int PRGE = 3;
        public const int PREQ = 4;
        public const int PRNE = 5;

        public Restrictions(int maxRestrictions)
        {
            this.restrictionStructure = new MQRESTRICTION(maxRestrictions);
        }

        public virtual void AddGuid(int propertyId, int op, Guid value)
        {
            IntPtr data = Marshal.AllocHGlobal(16);
            Marshal.Copy(value.ToByteArray(), 0, data, 16);
            this.AddItem(propertyId, op, MessagePropertyVariants.VT_CLSID, data);
        }

        public virtual void AddGuid(int propertyId, int op)
        {
            IntPtr data = Marshal.AllocHGlobal(16);
            this.AddItem(propertyId, op, MessagePropertyVariants.VT_CLSID, data);
        }

        private void AddItem(int id, int op, short vt, IntPtr data)
        {
            Marshal.WriteInt32(restrictionStructure.GetNextValidPtr(0), op);
            Marshal.WriteInt32(restrictionStructure.GetNextValidPtr(4), id);
            Marshal.WriteInt16(restrictionStructure.GetNextValidPtr(8), vt);
            Marshal.WriteInt16(restrictionStructure.GetNextValidPtr(10), (short)0);
            Marshal.WriteInt16(restrictionStructure.GetNextValidPtr(12), (short)0);
            Marshal.WriteInt16(restrictionStructure.GetNextValidPtr(14), (short)0);
            Marshal.WriteIntPtr(restrictionStructure.GetNextValidPtr(16), data);
            Marshal.WriteIntPtr(restrictionStructure.GetNextValidPtr(16 + IntPtr.Size), (IntPtr)0);
            ++restrictionStructure.restrictionCount;
        }

        public virtual void AddI4(int propertyId, int op, int value)
        {
            this.AddItem(propertyId, op, MessagePropertyVariants.VT_I4, (IntPtr)value);
        }

        public virtual void AddString(int propertyId, int op, string value)
        {
            if (value == null)
                this.AddItem(propertyId, op, MessagePropertyVariants.VT_NULL, (IntPtr)0);
            else
            {
                IntPtr data = Marshal.StringToHGlobalUni(value);
                this.AddItem(propertyId, op, MessagePropertyVariants.VT_LPWSTR, data);
            }

        }

        public virtual MQRESTRICTION GetRestrictionsRef()
        {
            return this.restrictionStructure;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class MQRESTRICTION
        {
            public int restrictionCount;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr restrinctions;

            public MQRESTRICTION(int maxCount)
            {
                this.restrinctions = Marshal.AllocHGlobal(maxCount * GetRestrictionSize());
            }

            ~MQRESTRICTION()
            {
                if (this.restrinctions != (IntPtr)0)
                {
                    for (int index = 0; index < this.restrictionCount; ++index)
                    {
                        short vt = Marshal.ReadInt16((IntPtr)((long)this.restrinctions + (index * GetRestrictionSize()) + 8));
                        if (vt != MessagePropertyVariants.VT_I4)
                        {
                            IntPtr dataPtr = (IntPtr)((long)this.restrinctions + (index * GetRestrictionSize()) + 16);
                            IntPtr data = Marshal.ReadIntPtr(dataPtr);
                            Marshal.FreeHGlobal(data);
                        }
                    }

                    Marshal.FreeHGlobal(this.restrinctions);
                    this.restrinctions = (IntPtr)0;
                }
            }

            public IntPtr GetNextValidPtr(int offset)
            {
                return (IntPtr)((long)restrinctions + restrictionCount * GetRestrictionSize() + offset);
            }

            public static int GetRestrictionSize()
            {
                return 16 + (IntPtr.Size * 2);
            }
        }
    }
}
