
//---------------------------------------------------------------------------
//
// <copyright file="GetWinFXPath.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Return the path for current WinFX runtime.
//
//
// History:
//  11/16/2005 weibz   Created
//
//---------------------------------------------------------------------------

using System;
using System.IO;
using System.Collections;

using System.Globalization;
using System.Diagnostics;
using System.Reflection;
using System.Resources;
using System.Runtime.InteropServices;


using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

using MS.Utility;
using MS.Internal.Tasks;

// Since we disable PreSharp warnings in this file, PreSharp warning is unknown to C# compiler.
// We first need to disable warnings about unknown message numbers and unknown pragmas.
#pragma warning disable 1634, 1691

namespace Microsoft.Build.Tasks.Windows
{
    #region GetWinFXPath Task class

    /// <summary>
    /// Return the directory of current WinFX run time.
    /// </summary>
    public sealed class GetWinFXPath : Task
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        /// <summary>
        /// Constrcutor
        /// </summary>
        public GetWinFXPath()
            : base(SR.ResourceManager)
        {

        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        /// <summary>
        /// ITask Execute method
        /// </summary>
        /// <returns></returns>
        public override bool Execute()
        {
            bool ret = true;

            try
            {
                TaskHelper.DisplayLogo(Log, SR.Get(SRID.GetWinFXPathTask));

                bool isWow64;

                isWow64 = IsWow64CLRRun();

                if (isWow64)
                {
                    WinFXPath = WinFXWowPath;
                }
                else
                {
                    WinFXPath = WinFXNativePath;
                }
            }
            catch (Exception e)
            {
                // PreSharp Complaint 6500 - do not handle null-ref or SEH exceptions.
                if (e is NullReferenceException || e is SEHException)
                {
                    throw;
                }
                else
                {
                    string message;
                    string errorId;

                    errorId = Log.ExtractMessageCode(e.Message, out message);

                    if (String.IsNullOrEmpty(errorId))
                    {
                        errorId = UnknownErrorID;
                        message = SR.Get(SRID.UnknownBuildError, message);
                    }

                    Log.LogError(null, errorId, null, null, 0, 0, 0, 0, message, null);
                }

                ret = false;
            }
#pragma warning disable 6500
            catch // Non-CLS compliant errors
            {
                Log.LogErrorWithCodeFromResources(SRID.NonClsError);
                ret = false;
            }
#pragma warning restore 6500


            return ret;
        }

        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        #region Public Properties

        /// <summary>
        /// The path for native WinFX runtime.
        /// </summary>
        [Required]
        public string WinFXNativePath
        {
            get { return _winFXNativePath; }
            set { _winFXNativePath = value; }
        }

        /// <summary>
        /// The path for WoW WinFX run time.
        /// </summary>
        [Required]
        public string WinFXWowPath
        {
            get { return _winFXWowPath; }
            set { _winFXWowPath = value; }
        }

        /// <summary>
        /// The real path for the WinFX runtime
        /// </summary>
        [Output]
        public string WinFXPath
        {
            get { return _winFXPath;  }
            set { _winFXPath = value; }
        }

        #endregion Public Properties


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        // PInvoke delegate for IsWow64Process
        [UnmanagedFunctionPointer(CallingConvention.Winapi)]
        private delegate bool IsWow64ProcessDelegate([In] IntPtr hProcess, [Out] out bool Wow64Process);

        [DllImport(Kernel32Dll, PreserveSig = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr LoadLibrary(string fileName);

        [DllImport(Kernel32Dll, PreserveSig = true)]
        private static extern bool FreeLibrary([In] IntPtr module);

        [DllImport(Kernel32Dll, PreserveSig = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr GetProcAddress(IntPtr module, string procName);

        [DllImport(Kernel32Dll, PreserveSig = true)]
        private static extern IntPtr GetCurrentProcess();

        // <summary>
        // Detect if the msbuild is running under WOW64 environment.
        // </summary>
        static private bool IsWow64CLRRun()
        {
            bool isWow64 = false;
            IntPtr NullIntPtr = new IntPtr(0);

            IntPtr kernel32Dll = LoadLibrary(Kernel32Dll);

            if (kernel32Dll != NullIntPtr)
            {
                try
                {
                    IntPtr isWow64ProcessHandle = GetProcAddress(kernel32Dll, IsWow64ProcessMethodName);

                    // if the entry point is missing, it cannot be Wow64
                    if (isWow64ProcessHandle != NullIntPtr)
                    {
                        // entry point present, check if running in WOW64
                        IsWow64ProcessDelegate isWow64Process = (IsWow64ProcessDelegate)Marshal.GetDelegateForFunctionPointer(isWow64ProcessHandle, typeof(IsWow64ProcessDelegate));
                        isWow64Process(GetCurrentProcess(), out isWow64);
                    }
                }
                finally
                {
                    FreeLibrary(kernel32Dll);
                }
            }

            return isWow64;
        }


        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Properties
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private string _winFXNativePath = string.Empty;
        private string _winFXWowPath = string.Empty;
        private string _winFXPath = string.Empty;

        private const string UnknownErrorID = "FX1000";

        private const string Kernel32Dll = "kernel32.dll";
        private const string IsWow64ProcessMethodName = "IsWow64Process";

        #endregion Private Fields

    }

    #endregion GetWinFXPath Task class
}
