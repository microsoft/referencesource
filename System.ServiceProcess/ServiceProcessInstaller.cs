//------------------------------------------------------------------------------
// <copyright file="ServiceProcessInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;

namespace System.ServiceProcess 
{
    using System.ServiceProcess.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;   
    using System;
    using System.Collections;
    using System.Configuration.Install;    
    using System.Windows.Forms;
    using System.Globalization;

    /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller"]/*' />
    /// <devdoc>
    ///    <para>Installs an executable containing classes 
    ///       that extend <see cref='System.ServiceProcess.ServiceBase'/> . This class is called by the installation utility, InstallUtil.exe, when installing a service application.</para>
    /// </devdoc>
    public class ServiceProcessInstaller : ComponentInstaller 
    {

        private ServiceAccount serviceAccount = ServiceAccount.User;
        private bool haveLoginInfo;
        private string password;
        private string username;
        private static bool helpPrinted;

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.HelpText"]/*' />
        /// <devdoc>
        ///    <para>Indicates help text displayed for service installation options.</para>
        /// </devdoc>
        public override string HelpText 
        {
            get 
            {
                if (helpPrinted)
                    return base.HelpText;
                else 
                {
                    helpPrinted = true;
                    return Res.GetString(Res.HelpText) + "\r\n" + base.HelpText;
                }
            }
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.Password"]/*' />
        /// <devdoc>
        ///    <para> Indicates the password associated with the user account under which
        ///       the service application will run.</para>
        /// </devdoc>
        [Browsable(false)]
        public string Password 
        {
            get 
            {
                if (!haveLoginInfo) 
                {
                    GetLoginInfo();
                }

                return password;
            }
            set 
            {
                haveLoginInfo = false;
                password = value;
            }
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.Account"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the account type under which the service will run.</para>
        /// </devdoc>
        [
        DefaultValue(ServiceAccount.User),
        ServiceProcessDescription(Res.ServiceProcessInstallerAccount)
        ]
        public ServiceAccount Account 
        {
            get 
            {
                if (!haveLoginInfo) 
                {
                    GetLoginInfo();
                }

                return serviceAccount;
            }
            set 
            {
                haveLoginInfo = false;
                serviceAccount = value;
            }
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.Username"]/*' />
        /// <devdoc>
        ///    <para> Indicates the user account under which
        ///       the service application will run.</para>
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        Browsable(false)
        ]
        public string Username 
        {
            get 
            {
                if (!haveLoginInfo) 
                {
                    GetLoginInfo();
                }

                return username;
            }
            set 
            {
                haveLoginInfo = false;
                username = value;
            }
        }

        
        
        // Consider, V2, jruiz: implement this. How do you set the working directory
        // for a service?
        //public string InitialWorkingDirectory {
        //    get {                
        //    }
        //    set {
        //    }
        //}
        

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.AccountHasRight"]/*' />
        /// <devdoc>
        /// Enumerates through the rights of the given account and checks whether the given right
        /// is in the list.
        /// </devdoc>
        private static bool AccountHasRight(IntPtr policyHandle, byte[] accountSid, string rightName) 
        {
            IntPtr pRights = (IntPtr)0;
            int rightsCount = 0;
            
            // This function gives us back a pointer to the start of an array of LSA_UNICODE_STRING structs (in pRights).
            int status = NativeMethods.LsaEnumerateAccountRights(policyHandle, accountSid, out pRights, out rightsCount);
            
            if (status == NativeMethods.STATUS_OBJECT_NAME_NOT_FOUND) 
            {
                // this means that the accountSid has no specific rights 
                return false;
            }
            
            if (status != 0) 
            {
                throw new Win32Exception(SafeNativeMethods.LsaNtStatusToWinError(status));
            }
            
            bool found = false;
            try 
            {
                // look through the rights and see if the desired one is present.
                IntPtr pCurRights = pRights;
                for (int i = 0; i < rightsCount; i++) 
                {
                    // Get this element in the array they gave us
                    NativeMethods.LSA_UNICODE_STRING_withPointer uStr = new NativeMethods.LSA_UNICODE_STRING_withPointer();
                    Marshal.PtrToStructure(pCurRights, uStr); // copy the buffer portion to an array & create a string from that
                    char[] rightChars = new char[uStr.length / sizeof(char)];                    
                    Marshal.Copy(uStr.pwstr, rightChars, 0, rightChars.Length);
                    string right = new string(rightChars, 0, rightChars.Length);
                    // see if this is the one we're looking for
                    if (string.Compare(right, rightName, StringComparison.Ordinal) == 0) 
                    {
                        found = true;
                        break;
                    }
                    // move to the next element in the array
                    pCurRights = (IntPtr)((long)pCurRights + Marshal.SizeOf(typeof(NativeMethods.LSA_UNICODE_STRING)));
                }
            }
            finally 
            {
                // make sure we free the memory they allocated for us
                SafeNativeMethods.LsaFreeMemory(pRights);
            }
            
            return found;
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        /// <para>Implements the base class <see cref='System.Configuration.Install.ComponentInstaller.CopyFromComponent' qualify='true'/> method with no <see cref='System.ServiceProcess.ServiceProcessInstaller'/> class-specific behavior.</para>
        /// </devdoc>
        public override void CopyFromComponent(IComponent comp) 
        {
            // we don't have any service-specific information.
        }

        private byte[] GetAccountSid(string accountName) 
        {
            //Lookup SID
            byte[] newSid = new byte[256];
            int[] sidLen = new int[] {newSid.Length};
            char[] domName = new char[1024];
            int[] domNameLen = new int[] {domName.Length};
            int[] peUse = new int[1];
            bool success;

            // PS 79686: Handle account name in the form ".\Account" for local machine.
            if ( accountName.Substring(0,2) == ".\\" )
            {
                // Replace the "." with the local computer name.
                System.Text.StringBuilder compName = new System.Text.StringBuilder( NativeMethods.MAX_COMPUTERNAME_LENGTH + 1 );
                int nameLen = NativeMethods.MAX_COMPUTERNAME_LENGTH + 1;
                success = NativeMethods.GetComputerName( compName, ref nameLen );
                if ( !success )
                    throw new Win32Exception();

                accountName = compName + accountName.Substring( 1 );
            }

            success = NativeMethods.LookupAccountName(null, accountName,  newSid, sidLen, domName, domNameLen, peUse);
            if ( !success )
                throw new Win32Exception();

            byte[] sid = new byte[sidLen[0]];
            System.Array.Copy(newSid, 0, sid, 0, sidLen[0]);
            return sid;
        }
        
        // This function contains all the logic to get the username and password
        // from some combination of command line arguments, hard-coded values,
        // and dialog box responses.  This function is called the first time
        // the Username, Password, or RunUnderSystemAccout property is retrieved.
        private void GetLoginInfo() 
        {
            // if we're in design mode we won't have a context, etc.
            // PS 79665: changed from test for this.DesignMode flag to explicit test for the condition that
            // was causing the AV. -- jonfisch
            if (Context != null && !this.DesignMode) 
            {
                if (haveLoginInfo)
                    return;

                haveLoginInfo = true;
    
                // ask for the account to run under if necessary
                if (serviceAccount == ServiceAccount.User) 
                {
                    if (Context.Parameters.ContainsKey("username")) 
                    {
                        username = Context.Parameters["username"];
                    }
                    if (Context.Parameters.ContainsKey("password")) 
                    {
                        password = Context.Parameters["password"];
                    }
                    if (username == null || username.Length == 0 || password == null) 
                    {
                        //display the dialog if we are not under unattended setup
                        if (!Context.Parameters.ContainsKey("unattended")) 
                        {
                            using (ServiceInstallerDialog dlg = new ServiceInstallerDialog()) 
                            {
                                if (username != null)
                                    dlg.Username = username;
                                dlg.ShowDialog();
                                switch (dlg.Result) 
                                {
                                    case ServiceInstallerDialogResult.Canceled:
                                        throw new InvalidOperationException(Res.GetString(Res.UserCanceledInstall, Context.Parameters["assemblypath"]));
                                    case ServiceInstallerDialogResult.UseSystem:
                                        username = null;
                                        password = null;
                                        serviceAccount = ServiceAccount.LocalSystem;
                                        break;
                                    case ServiceInstallerDialogResult.OK:
                                        username = dlg.Username;
                                        password = dlg.Password;
                                        break;
                                }
                            }
                        }
                        else 
                        {
                            throw new InvalidOperationException(Res.GetString(Res.UnattendedCannotPrompt, Context.Parameters["assemblypath"]));
                        }
                    }
                }               
            }
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.GrantAccountRight"]/*' />
        /// <devdoc>
        /// Grants the named right to the given account.
        /// </devdoc>
        private static void GrantAccountRight(IntPtr policyHandle, byte[] accountSid, string rightName) 
        {
            //Add Account Rights
            NativeMethods.LSA_UNICODE_STRING accountRights = new NativeMethods.LSA_UNICODE_STRING();
            accountRights.buffer = rightName;
            accountRights.length = (short)(accountRights.buffer.Length * sizeof(char));
            accountRights.maximumLength = accountRights.length;
            int result = NativeMethods.LsaAddAccountRights(policyHandle, accountSid, accountRights, 1);
            if ( result != 0 )
                throw new Win32Exception( SafeNativeMethods.LsaNtStatusToWinError( result ) );
        }
        
        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>Writes service application information to the registry. 
        ///       This method is meant to be used by installation tools, which call the appropriate methods automatically.</para>
        /// </devdoc>
        public override void Install(IDictionary stateSaver) 
        {            
            try 
            {
                ServiceInstaller.CheckEnvironment();    		
                try 
                {
                 
                    if (!haveLoginInfo) 
                    {
                        try 
                        {
                            GetLoginInfo();  // if the user hits "cancel" this will throw
                        } 
                        catch 
                        {
                            stateSaver["hadServiceLogonRight"] = true;  // this prevents rollback from trying to remove logonRights for the user
                            throw;
                        }
                    }
                }
                finally 
                {
                    // save out that information (but not the password)
                    stateSaver["Account"] = Account;
                    if (Account == ServiceAccount.User)
                        stateSaver["Username"] = Username;
                }
            
                if (Account == ServiceAccount.User) 
                {
                    // grant the right to run as a service to the given username. If we don't do this,
                    // the service will be unable to start under that account.
                    IntPtr policyHandle = OpenSecurityPolicy();
                    bool hasServiceLogonRight = true;  // we use 'true' here for the default because later in rollback we do
                                                       //   "if not hasServiceLogon revoke logon priviledge"
                    try 
                    {
                        byte[] sid = GetAccountSid(Username);
                        hasServiceLogonRight = AccountHasRight(policyHandle, sid, "SeServiceLogonRight");
                        if (!hasServiceLogonRight)
                            GrantAccountRight(policyHandle, sid, "SeServiceLogonRight");
                    }
                    finally 
                    {
                        stateSaver["hadServiceLogonRight"] = hasServiceLogonRight;
                        SafeNativeMethods.LsaClose(policyHandle);                    
                    }
                }
            }
            finally 
            {
                // now install all the contained services. They will use the Username and Password properties to do
                // their installation.
                base.Install(stateSaver);
            }
        }

        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.OpenSecurityPolicy"]/*' />
        /// <devdoc>
        /// Returns an LSA handle to the local machine's security policy. Call LsaClose when finished.
        /// </devdoc>
        private IntPtr OpenSecurityPolicy() 
        {
            // PS 79669: This code was incorrectly using a hardcoded byte array of size 33 (which breaks on 64 bit).
            // Changed it to use actual structure. -- jonfisch
            NativeMethods.LSA_OBJECT_ATTRIBUTES attribs = new NativeMethods.LSA_OBJECT_ATTRIBUTES();
            GCHandle attribsHandle = GCHandle.Alloc(attribs, GCHandleType.Pinned);
            try 
            {
                IntPtr policyHandle;
                int result = 0;
                IntPtr attribsPointer =  attribsHandle.AddrOfPinnedObject();
                result = NativeMethods.LsaOpenPolicy(null, attribsPointer, NativeMethods.POLICY_CREATE_ACCOUNT | NativeMethods.POLICY_LOOKUP_NAMES, out policyHandle);
                if ( result != 0 )
                    throw new Win32Exception( SafeNativeMethods.LsaNtStatusToWinError( result ) );

                return policyHandle;
            }
            finally 
            {
                attribsHandle.Free();
            }
        }
        
        private static void RemoveAccountRight(IntPtr policyHandle, byte[] accountSid, string rightName) 
        {
            NativeMethods.LSA_UNICODE_STRING accountRights = new NativeMethods.LSA_UNICODE_STRING();
            accountRights.buffer = rightName;
            accountRights.length = (short) (accountRights.buffer.Length * sizeof(char));
            accountRights.maximumLength = accountRights.length;
            int result = NativeMethods.LsaRemoveAccountRights(policyHandle, accountSid, false, accountRights, 1);
            if ( result != 0 )
                throw new Win32Exception( SafeNativeMethods.LsaNtStatusToWinError( result ) );
        }
        
        /// <include file='doc\ServiceProcessInstaller.uex' path='docs/doc[@for="ServiceProcessInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para>Rolls back service application information that was 
        ///       written to the registry by the installation procedure. This method is meant to be used by installation tools, which process
        ///       the appropriate methods automatically.</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState) 
        {
            try 
            {
                // remove the SeServiceLogonRight from the account if we added it.
                if (((ServiceAccount) savedState["Account"]) == ServiceAccount.User && !((bool) savedState["hadServiceLogonRight"])) 
                {
                    string username = (string) savedState["Username"];
                    IntPtr policyHandle = OpenSecurityPolicy();
                    try 
                    {
                        byte[] sid = GetAccountSid(username);
                        RemoveAccountRight(policyHandle, sid, "SeServiceLogonRight");
                    }
                    finally 
                    {
                        SafeNativeMethods.LsaClose(policyHandle);
                    }
                }
            }
            finally 
            {
                base.Rollback(savedState);
            }
        }

    }

}
