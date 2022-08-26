//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Diagnostics;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using Microsoft.Win32;

    internal delegate void ShowWindowEventHander(bool show);

    // return: true for successful validation, false for failure
    internal delegate bool BeforeApplyChangesEventHandler();
    internal delegate bool ApplyChangesEventHandler();
    
    // exposed COM class
    [GuidAttribute(WsatKeys.WsatPropertySheetGUID), ComVisible(true)]
    public class WsatPropertySheet : IExtendPropertySheet, IExtendPropertySheet2, IDisposable
    {
        internal ShowWindowEventHander ShowPropertyPage;
        internal BeforeApplyChangesEventHandler BeforeApplyChanges;
        internal ApplyChangesEventHandler ApplyChanges;

        PropSheetPage propSheet;
        SafePropertyPage handlePropSheetPage;
        SafeLocalAllocation dlgTemplate;
        IntPtr windowUnmanagedParent = IntPtr.Zero;
        string machineName = null;
        string virtualServer;

        static IntPtr PSNRET_NOERROR = new IntPtr(0);

        WsatControl wsatControl;
        ArrayList activeItem;
        bool disposed;

        // flags that mark whether we can call the SetWindowLongPtr function
        static bool firstCallToSetWindowLongPtr = true;
        static bool canCallSetWindowLongPtr;

        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        public WsatPropertySheet()
        {
            activeItem = new ArrayList();

            propSheet = new PropSheetPage();
            propSheet.dwSize = Marshal.SizeOf(typeof(PropSheetPage));
            propSheet.dwFlags = PSP.DEFAULT | PSP.USETITLE | PSP.DLGINDIRECT;
            propSheet.pfnDlgProc = new DialogProc(PropPageDialogProc);
            propSheet.pfnCallback = null;
            propSheet.pszTitle = SR.GetString(SR.WSATPropPageTitle);
            propSheet.pResource = GetDialogTemplate(250, 230);
            propSheet.longParameter = IntPtr.Zero;

            handlePropSheetPage = new SafePropertyPage(propSheet, false);
        }

        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        ~WsatPropertySheet()
        {
            Dispose(false);
        }

        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        public int CreatePropertyPages(IPropertySheetCallback provider, IntPtr handle, System.Runtime.InteropServices.ComTypes.IDataObject dataObject)
        {
            const int E_UNEXPECTED = unchecked((int)0x8000ffff);
            if (dataObject != null)
            {
                if (Utilities.OSMajor <= 5)
                {
                    // REMARK: there is a BUG here:
                    // CCF_COM_WORKSTATION should return the machine name, 
                    // not CCF_COM_OBJECTKEY 
                    if (!TryGetResourceValue(dataObject, "CCF_COM_OBJECTKEY", out machineName))
                    {
                        return E_UNEXPECTED; 
                    }

                    if (!Utilities.IsLocalMachineName(machineName) && MsdtcClusterUtils.IsClusterServer(machineName))
                    {
                        //Remote cluster configuration is not supported
                        return HRESULT.S_FALSE;
                    }
                }
                else
                {
                    string dtcHostName;
                    if (!TryGetResourceValue(dataObject, "CCF_DTC_HOSTNAME", out dtcHostName))
                    {
                        return E_UNEXPECTED;
                    }

                    bool isDtcNode = false;
                    string dtcResource;
                    if (TryGetResourceValue(dataObject, "CCF_DTC_RESOURCE", out dtcResource) && !string.IsNullOrEmpty(dtcResource))
                    {
                        isDtcNode = true;
                    }

                    if (isDtcNode)
                    {
                        if (!MsdtcClusterUtils.ResolveVirtualServerName(dtcHostName, dtcResource, out virtualServer))
                        {
                            return E_UNEXPECTED;
                        }

                        if (!Utilities.SafeCompare(dtcHostName, virtualServer))
                        {
                            //Remote cluster configuration is not supported
                            return HRESULT.S_FALSE;
                        }
                    }
                    else
                    {
                        this.machineName = dtcHostName;
                    }
                }
            }

            int hr = 0;
            if (provider != null)
            {
                hr = provider.AddPage(handlePropSheetPage);
            }
            return hr;
        }

        static bool TryGetResourceValue(System.Runtime.InteropServices.ComTypes.IDataObject dataObject, string resourceName, out string value)
        {
            value = string.Empty;

            FORMATETC formatEtc = new FORMATETC();
            formatEtc.cfFormat = (short)System.Windows.Forms.DataFormats.GetFormat(resourceName).Id;
            formatEtc.ptd = IntPtr.Zero;
            formatEtc.dwAspect = DVASPECT.DVASPECT_CONTENT;
            formatEtc.lindex = -1;
            formatEtc.tymed = TYMED.TYMED_HGLOBAL;

            STGMEDIUM stg = new STGMEDIUM();
            stg.pUnkForRelease = null;
            stg.tymed = TYMED.TYMED_HGLOBAL;
#pragma warning suppress 56523
            stg.unionmember = SafeNativeMethods.GlobalAlloc(SafeNativeMethods.GMEM_SHARE, 255);
            if (stg.unionmember == IntPtr.Zero)
            {
                return false;
            }
            try
            {
                dataObject.GetDataHere(ref formatEtc, ref stg);
            }
            catch (Exception ex)
            {
                if (Utilities.IsCriticalException(ex))
                {
                    throw;
                }
                return false;
            }

            value = Marshal.PtrToStringUni(stg.unionmember);

            SafeNativeMethods.ReleaseStgMedium(ref stg);
            return true;
        }

        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        SafeLocalAllocation GetDialogTemplate(short cX, short cY)
        {
            if (dlgTemplate != null)
                if (!dlgTemplate.IsInvalid)
                    if (!dlgTemplate.IsClosed)
                        return dlgTemplate;

            DialogTemplate dlg = new DialogTemplate();
            dlg.cx = cX;
            dlg.cy = cY;
            dlg.style = DS.DS_CONTROL;

            this.dlgTemplate = new SafeLocalAllocation(dlg);

            return dlgTemplate;
        }

        public int QueryPagesFor(System.Runtime.InteropServices.ComTypes.IDataObject dataObject)
        {
            return HRESULT.S_OK;
        }

        public int GetWatermarks(System.Runtime.InteropServices.ComTypes.IDataObject dataObject, ref IntPtr watermark, ref IntPtr header, ref IntPtr palette, ref int stretch)
        {
            // not implemented, implementation not required

            watermark = IntPtr.Zero;
            header = IntPtr.Zero;
            palette = IntPtr.Zero;
            stretch = 0;
            return 0;
        }

        protected virtual void OnShowPropertyPage(bool show)
        {
            if (this.ShowPropertyPage != null)
            {
                this.ShowPropertyPage(show);
            }
        }

        protected virtual bool OnBeforeApplyChanges()
        {
            bool valid = true;

            if (this.BeforeApplyChanges != null)
            {
                valid = this.BeforeApplyChanges();
            }

            // set DWL_MSGRESULT to FALSE when valid
            SetWindowLongWrapper(windowUnmanagedParent,
                                 SafeNativeMethods.DWL_MSGRESULT,
                                 valid ? IntPtr.Zero : (IntPtr)1);
            return true;
        }

        protected virtual bool OnApplyChanges()
        {
            bool succeeded = true;
            if (this.ApplyChanges != null)
            {
                succeeded = this.ApplyChanges();
            }
            SetWindowLongWrapper(windowUnmanagedParent,
                                 SafeNativeMethods.DWL_MSGRESULT,
                                 succeeded ? (IntPtr)PSNRET.NOERROR : (IntPtr)PSNRET.INVALID);            
            return true;
        }



        [CLSCompliantAttribute(false)]
        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        public bool PropPageDialogProc(IntPtr windowDialog, UInt32 message, IntPtr wordParameter, IntPtr longParameter)
        {
            if (this.windowUnmanagedParent == IntPtr.Zero)
            {
                this.windowUnmanagedParent = windowDialog;
            }

            switch (message)
            {
                case WM.INITDIALOG:
                    try
                    {
#pragma warning suppress 56523
                        IntPtr parent = SafeNativeMethods.GetParent(windowUnmanagedParent);
                        wsatControl = new WsatControl(windowUnmanagedParent, parent, this);

                        if (Utilities.OSMajor > 5)
                        {
                            wsatControl.Size = new System.Drawing.Size(425, wsatControl.Size.Height);
                        }

#pragma warning suppress 56523
                        SafeNativeMethods.SetParent(wsatControl.Handle, this.windowUnmanagedParent);
                        activeItem.Add(propSheet);
                    }
                    catch (WsatAdminException ex)
                    {
                        ShowErrorDialog(ex.Message);
                        return false;
                    }
                    return true;
                case WM.DESTROY:
                    return true;
                case WM.NOTIFY:
                    return HandleNotification(longParameter);
                case WM.SHOWWINDOW:
                    OnShowPropertyPage(wordParameter != IntPtr.Zero);
                    return true;
                default:
                    return false;
            }
        }

        bool HandleNotification(IntPtr longParameter)
        {
            NMHDR header = (NMHDR)Marshal.PtrToStructure(longParameter, typeof(NMHDR));

            if (header.hwndFrom != windowUnmanagedParent &&
                header.hwndFrom != SafeNativeMethods.GetParent(windowUnmanagedParent))
            {
                return false;
            }

            switch ((int)header.code)
            {
                case PSN.KILLACTIVE:
                    return OnBeforeApplyChanges();
                case PSN.APPLY:
                    return OnApplyChanges();
                case PSN.QUERYCANCEL:
                    return true;
                default:
                    return false;
            }
        }

        // this method is used to call the SetWindowLong function
        // if SetWindowLongPtr can not be found => we are not running on a 64b OS
        // thus we can use the SetWindowLong function.
        static IntPtr SetWindowLongWrapper(IntPtr hWnd, int nIndex, IntPtr dwNewLong)
        {
            IntPtr result;
            if (firstCallToSetWindowLongPtr)
            {
                firstCallToSetWindowLongPtr = false;
                try
                {
#pragma warning suppress 56523
                    result = SafeNativeMethods.SetWindowLongPtr(hWnd, nIndex, dwNewLong);
                    canCallSetWindowLongPtr = true;
                }
                catch (EntryPointNotFoundException)
                {
#pragma warning suppress 56523
                    int intResult = SafeNativeMethods.SetWindowLong(hWnd, nIndex, (int)dwNewLong);
                    result = new IntPtr(intResult);
                    canCallSetWindowLongPtr = false;
                }
            }
            else
            {
                if (canCallSetWindowLongPtr)
                {
#pragma warning suppress 56523
                    result = SafeNativeMethods.SetWindowLongPtr(hWnd, nIndex, dwNewLong);
                }
                else
                {
#pragma warning suppress 56523
                    int intResult = SafeNativeMethods.SetWindowLong(hWnd, nIndex, (int)dwNewLong);
                    result = new IntPtr(intResult);
                }
            }
            return result;
        }

        internal string MachineName
        {
            get
            {
                return Utilities.SafeCompare(machineName, "My Computer") ? "localhost" : machineName;
            }
        }

        internal string VirtualServer
        {
            get
            {
                return virtualServer;
            }
        }

        [SecurityPermissionAttribute(SecurityAction.Demand, Unrestricted = true, UnmanagedCode = true)]
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            // Check to see if Dispose has already been called.
            if (!this.disposed)
            {
                // If disposing equals true, dispose all managed 
                // and unmanaged resources.
                if (disposing)
                {
                    activeItem.Remove(propSheet);

                    // Dispose managed resources.
                    dlgTemplate.Dispose();
                    handlePropSheetPage.Dispose();
                    wsatControl.Dispose();
                }

                windowUnmanagedParent = IntPtr.Zero;
            }
            disposed = true;
        }

        // register MMC Property Page extension
        [ComRegisterFunctionAttribute]
        public static void RegisterWithMmc(Type type)
        {
            try
            {
                RegistryConfigurationProvider provider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WSATRegisterSnapinWsatPropertySheetKey, null);

                using (provider)
                {
                    provider.WriteString(WsatKeys.WSATRegisterSnapinWsatPropertySheetAboutEntry, WsatKeys.WSATRegisterSnapinWsatPropertySheetAboutValue);
                    provider.WriteString(WsatKeys.WSATRegisterSnapinWsatPropertySheetNameEntry, WsatKeys.WSATRegisterSnapinWsatPropertySheetNameValue);
                }

                provider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WSATRegisterNodeTypesCompServCompNodeKey, null);
                using (provider)
                {
                    provider.WriteString(WsatKeys.WSATRegisterNodeTypesWsatPropertySheetGUIDEntry, WsatKeys.WSATRegisterNodeTypesWsatPropertySheetGUIDValue);
                }

                // put binary path
                string assemblyLocation = System.Reflection.Assembly.GetExecutingAssembly().Location;
                provider = new RegistryConfigurationProvider(RegistryHive.LocalMachine, WsatKeys.WsatRegKey, null);
                using (provider)
                {
                    provider.WriteString(WsatKeys.WSATAssemblyPathEntry, assemblyLocation);
                }
            }
#pragma warning suppress 56500
            catch (Exception ex)
            {
                if (Utilities.IsCriticalException(ex))
                {
                    throw;
                }

                ShowErrorDialog(ex.Message);
            }
        }

        [ComUnregisterFunctionAttribute]
        public static void UnregisterWithMmc(Type type)
        {
            try
            {
                // unregister MMC Property Page extension
                RegistryHelper.DeleteKey(WsatKeys.WSATRegisterSnapinKey, "{" + WsatKeys.WsatPropertySheetGUID + "}");
                RegistryHelper.DeleteValue(WsatKeys.WSATRegisterNodeTypesCompServCompNodeKey, WsatKeys.WSATRegisterNodeTypesWsatPropertySheetGUIDEntry);

                // remove binary path
                RegistryHelper.DeleteValue(WsatKeys.WsatRegKey, WsatKeys.WSATAssemblyPathEntry);
            }
#pragma warning suppress 56500
            catch (Exception ex)
            {
                if (Utilities.IsCriticalException(ex))
                {
                    throw;
                }
                ShowErrorDialog(ex.Message);
            }
        }

        static void ShowErrorDialog(string msg)
        {
            MessageBox.Show(msg, SR.GetString(SR.ErrorMessageBoxTitle),
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error,
                            MessageBoxDefaultButton.Button1,
                            (MessageBoxOptions)0);
        } 
    }
}
