namespace System.Drawing {
    using System;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Drawing.Printing;
    using System.Runtime.Versioning;

    internal static class IntSecurity {
        private static readonly UIPermission AllWindows = new UIPermission(UIPermissionWindow.AllWindows);
        private static readonly UIPermission SafeSubWindows = new UIPermission(UIPermissionWindow.SafeSubWindows);

        public static readonly CodeAccessPermission UnmanagedCode = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);

        public static readonly CodeAccessPermission ObjectFromWin32Handle = UnmanagedCode;
        public static readonly CodeAccessPermission Win32HandleManipulation = UnmanagedCode;

        public static readonly PrintingPermission NoPrinting = new PrintingPermission(PrintingPermissionLevel.NoPrinting);
        public static readonly PrintingPermission SafePrinting = new PrintingPermission(PrintingPermissionLevel.SafePrinting);
        public static readonly PrintingPermission DefaultPrinting = new PrintingPermission(PrintingPermissionLevel.DefaultPrinting);
        public static readonly PrintingPermission AllPrinting = new PrintingPermission(PrintingPermissionLevel.AllPrinting);

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        internal static void DemandReadFileIO(string fileName) {
            string full = fileName;
            
            full = UnsafeGetFullPath(fileName);

            new FileIOPermission(FileIOPermissionAccess.Read, full).Demand();
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        internal static void DemandWriteFileIO(string fileName) {
            string full = fileName;

            full = UnsafeGetFullPath(fileName);

            new FileIOPermission(FileIOPermissionAccess.Write, full).Demand();
        }

        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        internal static string UnsafeGetFullPath(string fileName) {
            string full = fileName;

            FileIOPermission fiop = new FileIOPermission(PermissionState.None);
            fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
            fiop.Assert();

            try {
                full = Path.GetFullPath(fileName);
            } finally {
                CodeAccessPermission.RevertAssert();
            }

            return full;
        }

        static PermissionSet allPrintingAndUnmanagedCode;
        public static PermissionSet AllPrintingAndUnmanagedCode {
            get {
                if (allPrintingAndUnmanagedCode == null) {
                    PermissionSet temp = new PermissionSet(PermissionState.None);
                    temp.SetPermission(IntSecurity.UnmanagedCode);
                    temp.SetPermission(IntSecurity.AllPrinting);
                    allPrintingAndUnmanagedCode = temp;
                }
                return allPrintingAndUnmanagedCode;
            }
        }

        internal static bool HasPermission(PrintingPermission permission) {
            try {
                permission.Demand();
                return true;
            }
            catch (SecurityException) {
                return false;
            }
        }
    }
}
