//---------------------------------------------------------------------------
//
// <copyright file=InputScopeAttribute.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: InputScopeAttribute is an image object that links IOleDataObject.
//
// History:
//  10/02/2003 : yutakas
//
//---------------------------------------------------------------------------

using System;
using System.Security;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Windows.Threading;

using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Input;
using System.Windows.Documents;
using System.Windows.Controls;
using MS.Win32;


namespace System.Windows.Documents
{

    //
    // InputScopeAttribute is Image object that links IOleDataObject, which 
    // is insterted by ITextStoreACP::InsertEmbedded().
    //
    internal class InputScopeAttribute : UnsafeNativeMethods.ITfInputScope
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        // Creates a new InputScopeAttribute instance.
        internal InputScopeAttribute(InputScope inputscope)
        {
            _inputScope= inputscope;

        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------

        #region Internal Methods

        // A method of ITfInputScope.
        // This returns InputScopes in the array that is allocated by CoTaskMemAlloc.
        /// <SecurityNote>
        /// Critical - it satisfies Marshal.* LinkDemands for unmanaged code permissions. handles out a valid pointer to unmanaged memory.
        /// </SecurityNote>
        [SecurityCritical]
        public void GetInputScopes(out IntPtr ppinputscopes, out int count)
        {
            
            if (_inputScope != null)
            {
                int offset = 0;
                count = _inputScope.Names.Count;
                try 
                {
                    ppinputscopes = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(Int32)) * count);
                }
                catch (OutOfMemoryException)
                {
                    throw new COMException(SR.Get(SRID.InputScopeAttribute_E_OUTOFMEMORY), NativeMethods.E_OUTOFMEMORY);
                }

                for (int i = 0; i < count; i++)
                {
                    Marshal.WriteInt32(ppinputscopes, offset, (Int32)((InputScopeName)_inputScope.Names[i]).NameValue);
                    offset += Marshal.SizeOf(typeof(Int32));
                }
            }
            else
            {
                ppinputscopes = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(Int32)) * 1);
                Marshal.WriteInt32(ppinputscopes, (Int32)InputScopeNameValue.Default);
                count = 1;
            }

        }

        // A method of ITfInputScope.
        // This returns BSTRs in the array that is allocated by CoTaskMemAlloc.
        /// <SecurityNote>
        /// Critical - it satisfies Marshal.AllocCoTaskMem() LinkDemand for unmanaged code permissions. handles out a valid pointer to unmanaged memory.
        /// </SecurityNote>
        [SecurityCritical]
        public int GetPhrase(out IntPtr ppbstrPhrases, out int count)
        {
            count = _inputScope == null ? 0 : _inputScope.PhraseList.Count;
            try
            {
                ppbstrPhrases = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(IntPtr))*count);
            }
            catch (OutOfMemoryException)
            {
                throw new COMException(SR.Get(SRID.InputScopeAttribute_E_OUTOFMEMORY), NativeMethods.E_OUTOFMEMORY);
            }

            int offset = 0;
            for(int i=0; i <count; i++)
            {
                IntPtr pbstr;
                try
                {
                    pbstr = Marshal.StringToBSTR(((InputScopePhrase)_inputScope.PhraseList[i]).Name);
                }
                catch (OutOfMemoryException)
                {
                    offset = 0;
                    for (int j=0; j < i; j++)
                    {
                        Marshal.FreeBSTR(Marshal.ReadIntPtr(ppbstrPhrases,  offset));
                        offset += Marshal.SizeOf(typeof(IntPtr));
                    }
                    throw new COMException(SR.Get(SRID.InputScopeAttribute_E_OUTOFMEMORY), NativeMethods.E_OUTOFMEMORY);
                }

                Marshal.WriteIntPtr(ppbstrPhrases , offset, pbstr);
                offset += Marshal.SizeOf(typeof(IntPtr));
            }
             
            return  count > 0 ? NativeMethods.S_OK : NativeMethods.S_FALSE;
        }

        // A method of ITfInputScope.
        public int GetRegularExpression(out string desc)
        {
            desc = null;

            if (_inputScope != null)
            {
                desc = _inputScope.RegularExpression;
            }
            return desc != null ? NativeMethods.S_OK : NativeMethods.S_FALSE;
        }

        // A method of ITfInputScope.
        public int GetSRGC(out string desc)
        {
            desc = null;
            
            if (_inputScope != null)
            {
                desc = _inputScope.SrgsMarkup;
            }
            return desc != null ? NativeMethods.S_OK : NativeMethods.S_FALSE;
        }

        // A method of ITfInputScope.
        public int GetXML(out string desc)
        {
            desc = null;
            return NativeMethods.S_FALSE;
        }

        #endregion Internal Methods

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

        #region Internal Properties
        #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        // InputScope value for this instance for ITfInputScope.
        private InputScope _inputScope;
        

        #endregion Private Fields
    }
}

