//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------    

namespace System.Messaging.Interop
{
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;

    [System.Runtime.InteropServices.ComVisible(false)]
    internal static class NativeMethods
    {
        //Message Acknowledge constants.
        public const int ACKNOWLEDGE_NEGATIVE_ARRIVAL = 0x04;
        public const int ACKNOWLEDGE_NEGATIVE_RECEIVE = 0x08;
        public const int ACKNOWLEDGE_NONE = 0x00;
        public const int ACKNOWLEDGE_POSITIVE_ARRIVAL = 0x01;
        public const int ACKNOWLEDGE_POSITIVE_RECEIVE = 0x02;
        public const int ACKNOWLEDGE_FULL_REACH_QUEUE = ACKNOWLEDGE_NEGATIVE_ARRIVAL |
                                                        ACKNOWLEDGE_POSITIVE_ARRIVAL;
        public const int ACKNOWLEDGE_FULL_RECEIVE = ACKNOWLEDGE_NEGATIVE_ARRIVAL |
                                                    ACKNOWLEDGE_NEGATIVE_RECEIVE | ACKNOWLEDGE_POSITIVE_RECEIVE;
        public const int ACKNOWLEDGE_NOTACKNOWLEDGE_REACH_QUEUE = ACKNOWLEDGE_NEGATIVE_ARRIVAL;
        public const int ACKNOWLEDGE_NOTACKNOWLEDGE_RECEIVE = ACKNOWLEDGE_NEGATIVE_ARRIVAL |
                                                              ACKNOWLEDGE_NEGATIVE_RECEIVE;

        // Algorithm classes.
        private const int ALG_CLASS_DATA_ENCRYPT = (3 << 13);
        private const int ALG_CLASS_HASH = (4 << 13);

        // Hash sub ids.
        private const int ALG_SID_MD2 = 1;
        private const int ALG_SID_MD4 = 2;
        private const int ALG_SID_MD5 = 3;
        private const int ALG_SID_SHA = 4;
        private const int ALG_SID_MAC = 5;
        private const int ALG_SID_SHA256 = 12; // 0xC
        private const int ALG_SID_SHA384 = 13; // 0xD
        private const int ALG_SID_SHA512 = 14; // 0xE
        private const int ALG_SID_RIPEMD = 6;
        private const int ALG_SID_RIPEMD160 = 7;
        private const int ALG_SID_SSL3SHAMD5 = 8;

        // RC2 sub-ids.
        private const int ALG_SID_RC2 = 2;
        private const int ALG_SID_RC4 = 1;

        // Algorithm types.
        private const int ALG_TYPE_ANY = 0;
        private const int ALG_TYPE_BLOCK = (3 << 9);
        private const int ALG_TYPE_STREAM = (4 << 9);

        // Algorithm identifier definitions.
        public const int CALG_MD2 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD2);
        public const int CALG_MD4 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD4);
        public const int CALG_MD5 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5);
        public const int CALG_SHA = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA);
        public const int CALG_MAC = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MAC);
        public const int CALG_SHA256 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA256);
        public const int CALG_SHA384 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA384);
        public const int CALG_SHA512 = (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA512);
        public const int CALG_RC2 = (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2);
        public const int CALG_RC4 = (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_RC4);

        //Stream constants
        public const int LOCK_WRITE = 0x1;
        public const int LOCK_EXCLUSIVE = 0x2;
        public const int LOCK_ONLYONCE = 0x4;
        public const int STATFLAG_DEFAULT = 0x0;
        public const int STATFLAG_NONAME = 0x1;
        public const int STATFLAG_NOOPEN = 0x2;
        public const int STGC_DEFAULT = 0x0;
        public const int STGC_OVERWRITE = 0x1;
        public const int STGC_ONLYIFCURRENT = 0x2;
        public const int STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE = 0x4;
        public const int STREAM_SEEK_SET = 0x0;
        public const int STREAM_SEEK_CUR = 0x1;
        public const int STREAM_SEEK_END = 0x2;

        public const int E_UNEXPECTED = unchecked((int)0x8000FFFF);
        public const int E_NOTIMPL = unchecked((int)0x80004001);
        public const int E_OUTOFMEMORY = unchecked((int)0x8007000E);
        public const int E_INVALIDARG = unchecked((int)0x80070057);
        public const int E_NOINTERFACE = unchecked((int)0x80004002);
        public const int E_POINTER = unchecked((int)0x80004003);
        public const int E_HANDLE = unchecked((int)0x80070006);
        public const int E_ABORT = unchecked((int)0x80004004);
        public const int E_FAIL = unchecked((int)0x80004005);

        public static Guid IID_IUnknown = new Guid("{00000000-0000-0000-C000-000000000046}");

        //Management Properties constants.
        public const int MANAGEMENT_BASE = 0;
        public const int MANAGEMENT_ACTIVEQUEUES = (MANAGEMENT_BASE + 1);   /* VT_LPWSTR | VT_VECTOR */
        public const int MANAGEMENT_PRIVATEQ = (MANAGEMENT_BASE + 2);      /* VT_LPWSTR | VT_VECTOR  */
        public const int MANAGEMENT_DSSERVER = (MANAGEMENT_BASE + 3);      /* VT_LPWSTR        */
        public const int MANAGEMENT_CONNECTED = (MANAGEMENT_BASE + 4);     /* VT_LPWSTR        */
        public const int MANAGEMENT_TYPE = (MANAGEMENT_BASE + 5);    /* VT_LPWSTR        */

        //Machine Properties constants.
        public const int MACHINE_BASE = 200;
        public const int MACHINE_SITE_ID = MACHINE_BASE + 1;  /* VT_CLSID            */
        public const int MACHINE_ID = MACHINE_BASE + 2;  /* VT_CLSID            */
        public const int MACHINE_PATHNAME = MACHINE_BASE + 3;  /* VT_LPWSTR           */
        public const int MACHINE_CONNECTION = MACHINE_BASE + 4;  /* VT_LPWSTR|VT_VECTOR */
        public const int MACHINE_ENCRYPTION_PK = MACHINE_BASE + 5;  /* VT_BLOB             */

        //Max constants.
        public const int MAX_MESSAGE_ID_SIZE = 20;
        public const int MAX_LABEL_LEN = 124;

        //Message Authentication level constants.
        public const int MESSAGE_AUTHENTICATION_LEVEL_NONE = 0;
        public const int MESSAGE_AUTHENTICATION_LEVEL_ALWAYS = 1;
        public const int MESSAGE_AUTHENTICATION_LEVEL_MSMQ10 = 2;
        public const int MESSAGE_AUTHENTICATION_LEVEL_MSMQ20 = 4;

        //Message Class constants
        public const int MESSAGE_CLASS_ACCESS_DENIED = (1 << 15) | 0x04;
        public const int MESSAGE_CLASS_BAD_DESTINATION_QUEUE = (1 << 15);
        public const int MESSAGE_CLASS_BAD_ENCRYPTION = (1 << 15) | 0x07;
        public const int MESSAGE_CLASS_BAD_SIGNATURE = (1 << 15) | 0x06;
        public const int MESSAGE_CLASS_COULD_NOT_ENCRYPT = (1 << 15) | 0x08;
        public const int MESSAGE_CLASS_HOP_COUNT_EXCEEDED = (1 << 15) | 0x05;
        public const int MESSAGE_CLASS_NORMAL = 0x00;
        public const int MESSAGE_CLASS_NOT_TRANSACTIONAL_QUEUE = (1 << 15) | 0x09;
        public const int MESSAGE_CLASS_NOT_TRANSACTIONAL_MESSAGE = (1 << 15) | 0x0A;
        public const int MESSAGE_CLASS_PURGED = (1 << 15) | 0x01;
        public const int MESSAGE_CLASS_QUEUE_DELETED = (1 << 15) | (1 << 14);
        public const int MESSAGE_CLASS_QUEUE_EXCEED_QUOTA = (1 << 15) | 0x03;
        public const int MESSAGE_CLASS_QUEUE_PURGED = (1 << 15) | (1 << 14) | 0x01;
        public const int MESSAGE_CLASS_REACH_QUEUE = 0x02;
        public const int MESSAGE_CLASS_REACH_QUEUE_TIMEOUT = (1 << 15) | 0x02;
        public const int MESSAGE_CLASS_RECEIVE = (1 << 14);
        public const int MESSAGE_CLASS_RECEIVE_TIMEOUT = (1 << 15) | (1 << 14) | 0x02;
        public const int MESSAGE_CLASS_REPORT = 0x01;

        //Message Delivery constants.
        public const int MESSAGE_DELIVERY_EXPRESS = 0;
        public const int MESSAGE_DELIVERY_RECOVERABLE = 1;

        //Message Journal constants.
        public const int MESSAGE_JOURNAL_NONE = 0;
        public const int MESSAGE_JOURNAL_DEADLETTER = 1;
        public const int MESSAGE_JOURNAL_JOURNAL = 2;

        //Message Privacy Level constants.
        public const int MESSAGE_PRIVACY_LEVEL_NONE = 0;
        public const int MESSAGE_PRIVACY_LEVEL_BODY = 1;

        //Message PropertyId constants.
        public const int MESSAGE_PROPID_BASE = 0;
        public const int MESSAGE_PROPID_ACKNOWLEDGE = (MESSAGE_PROPID_BASE + 6);            /* VT_UI1           */
        public const int MESSAGE_PROPID_ADMIN_QUEUE = (MESSAGE_PROPID_BASE + 17);           /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_ADMIN_QUEUE_LEN = (MESSAGE_PROPID_BASE + 18);       /* VT_UI4           */
        public const int MESSAGE_PROPID_APPSPECIFIC = (MESSAGE_PROPID_BASE + 8);            /* VT_UI4           */
        public const int MESSAGE_PROPID_ARRIVEDTIME = (MESSAGE_PROPID_BASE + 32);           /* VT_UI4           */
        public const int MESSAGE_PROPID_AUTHENTICATED = (MESSAGE_PROPID_BASE + 25);         /* VT_UI1           */
        public const int MESSAGE_PROPID_AUTH_LEVEL = (MESSAGE_PROPID_BASE + 24);            /* VT_UI4           */
        public const int MESSAGE_PROPID_BODY = (MESSAGE_PROPID_BASE + 9);                   /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_BODY_SIZE = (MESSAGE_PROPID_BASE + 10);             /* VT_UI4           */
        public const int MESSAGE_PROPID_BODY_TYPE = (MESSAGE_PROPID_BASE + 42);             /* VT_UI4           */
        public const int MESSAGE_PROPID_CLASS = (MESSAGE_PROPID_BASE + 1);                  /* VT_UI2           */
        public const int MESSAGE_PROPID_CONNECTOR_TYPE = (MESSAGE_PROPID_BASE + 38);        /* VT_CLSID         */
        public const int MESSAGE_PROPID_CORRELATIONID = (MESSAGE_PROPID_BASE + 3);          /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_DELIVERY = (MESSAGE_PROPID_BASE + 5);               /* VT_UI1           */
        public const int MESSAGE_PROPID_DEST_QUEUE = (MESSAGE_PROPID_BASE + 33);            /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_DEST_QUEUE_LEN = (MESSAGE_PROPID_BASE + 34);        /* VT_UI4           */
        public const int MESSAGE_PROPID_DEST_SYMM_KEY = (MESSAGE_PROPID_BASE + 43);         /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_DEST_SYMM_KEY_LEN = (MESSAGE_PROPID_BASE + 44);     /* VT_UI4           */
        public const int MESSAGE_PROPID_ENCRYPTION_ALG = (MESSAGE_PROPID_BASE + 27);        /* VT_UI4           */
        public const int MESSAGE_PROPID_EXTENSION = (MESSAGE_PROPID_BASE + 35);             /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_EXTENSION_LEN = (MESSAGE_PROPID_BASE + 36);         /* VT_UI4           */
        public const int MESSAGE_PROPID_FIRST_IN_XACT = (MESSAGE_PROPID_BASE + 50);  /* VT_UI1           */
        public const int MESSAGE_PROPID_HASH_ALG = (MESSAGE_PROPID_BASE + 26);              /* VT_UI4           */
        public const int MESSAGE_PROPID_JOURNAL = (MESSAGE_PROPID_BASE + 7);                /* VT_UI1           */
        public const int MESSAGE_PROPID_LABEL = (MESSAGE_PROPID_BASE + 11);                 /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_LABEL_LEN = (MESSAGE_PROPID_BASE + 12);             /* VT_UI4           */
        public const int MESSAGE_PROPID_LAST_IN_XACT = (MESSAGE_PROPID_BASE + 51);  /* VT_UI1           */
        public const int MESSAGE_PROPID_MSGID = (MESSAGE_PROPID_BASE + 2);                  /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_PRIORITY = (MESSAGE_PROPID_BASE + 4);               /* VT_UI1           */
        public const int MESSAGE_PROPID_PRIV_LEVEL = (MESSAGE_PROPID_BASE + 23);            /* VT_UI4           */
        public const int MESSAGE_PROPID_PROV_NAME = (MESSAGE_PROPID_BASE + 48);             /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_PROV_NAME_LEN = (MESSAGE_PROPID_BASE + 49);         /* VT_UI4           */
        public const int MESSAGE_PROPID_PROV_TYPE = (MESSAGE_PROPID_BASE + 47);             /* VT_UI4           */
        public const int MESSAGE_PROPID_RESP_QUEUE = (MESSAGE_PROPID_BASE + 15);            /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_RESP_QUEUE_LEN = (MESSAGE_PROPID_BASE + 16);        /* VT_UI4           */
        public const int MESSAGE_PROPID_SECURITY_CONTEXT = (MESSAGE_PROPID_BASE + 37);      /* VT_UI4           */
        public const int MESSAGE_PROPID_SENDERID = (MESSAGE_PROPID_BASE + 20);              /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_SENDERID_LEN = (MESSAGE_PROPID_BASE + 21);          /* VT_UI4           */
        public const int MESSAGE_PROPID_SENDERID_TYPE = (MESSAGE_PROPID_BASE + 22);         /* VT_UI4           */
        public const int MESSAGE_PROPID_SENDER_CERT = (MESSAGE_PROPID_BASE + 28);           /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_SENDER_CERT_LEN = (MESSAGE_PROPID_BASE + 29);       /* VT_UI4           */
        public const int MESSAGE_PROPID_SENTTIME = (MESSAGE_PROPID_BASE + 31);              /* VT_UI4           */
        public const int MESSAGE_PROPID_SIGNATURE = (MESSAGE_PROPID_BASE + 45);             /* VT_UI1|VT_VECTOR */
        public const int MESSAGE_PROPID_SIGNATURE_LEN = (MESSAGE_PROPID_BASE + 46);         /* VT_UI4           */
        public const int MESSAGE_PROPID_SRC_MACHINE_ID = (MESSAGE_PROPID_BASE + 30);        /* VT_CLSID         */
        public const int MESSAGE_PROPID_TIME_TO_BE_RECEIVED = (MESSAGE_PROPID_BASE + 14);   /* VT_UI4           */
        public const int MESSAGE_PROPID_TIME_TO_REACH_QUEUE = (MESSAGE_PROPID_BASE + 13);   /* VT_UI4           */
        public const int MESSAGE_PROPID_TRACE = (MESSAGE_PROPID_BASE + 41);                 /* VT_UI1           */
        public const int MESSAGE_PROPID_VERSION = (MESSAGE_PROPID_BASE + 19);               /* VT_UI4           */
        public const int MESSAGE_PROPID_XACT_STATUS_QUEUE = (MESSAGE_PROPID_BASE + 39);     /* VT_LPWSTR        */
        public const int MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN = (MESSAGE_PROPID_BASE + 40); /* VT_UI4           */
        public const int MESSAGE_PROPID_XACTID = (MESSAGE_PROPID_BASE + 52); /* VT_UI1|VT_VECTOR */

        public const int MESSAGE_PROPID_LOOKUPID = (MESSAGE_PROPID_BASE + 60);    /* VT_UI8           */

        //Message SenderId types
        public const int MESSAGE_SENDERID_TYPE_NONE = 0;
        public const int MESSAGE_SENDERID_TYPE_SID = 1;

        //Message Trace constants.
        public const int MESSAGE_TRACE_NONE = 0;
        public const int MESSAGE_TRACE_SEND_ROUTE_TO_REPORT_QUEUE = 1;

        // Chryptographic Provider Types
        public const int PROV_RSA_FULL = 1;
        public const int PROV_RSA_SIG = 2;
        public const int PROV_DSS = 3;
        public const int PROV_FORTEZZA = 4;
        public const int PROV_MS_EXCHANGE = 5;
        public const int PROV_SSL = 6;
        public const int PROV_STT_MER = 7;
        public const int PROV_STT_ACQ = 8;
        public const int PROV_STT_BRND = 9;
        public const int PROV_STT_ROOT = 10;
        public const int PROV_STT_ISS = 11;

        //Queue Access constants.
        public const int QUEUE_ACCESS_RECEIVE = 1;
        public const int QUEUE_ACCESS_SEND = 2;
        public const int QUEUE_ACCESS_PEEK = 32;
        public const int QUEUE_ACCESS_ADMIN = 128;

        //Queue Action constants
        public const int QUEUE_ACTION_RECEIVE = 0x00000000;
        public const int QUEUE_ACTION_PEEK_CURRENT = unchecked((int)0x80000000);
        public const int QUEUE_ACTION_PEEK_NEXT = unchecked((int)0x80000001);

        //Lookup Action constants
        internal const int LOOKUP_PEEK_MASK = 0x40000010;
        internal const int LOOKUP_RECEIVE_MASK = 0x40000020;

        //Queue Authenticate constants.
        public const int QUEUE_AUTHENTICATE_NONE = 0;
        public const int QUEUE_AUTHENTICATE_AUTHENTICATE = 1;

        //Queue Journal constants.
        public const int QUEUE_JOURNAL_NONE = 0;
        public const int QUEUE_JOURNAL_JOURNAL = 1;

        //Queue Privacy level constants
        public const int QUEUE_PRIVACY_LEVEL_NONE = 0;
        public const int QUEUE_PRIVACY_LEVEL_OPTIONAL = 1;
        public const int QUEUE_PRIVACY_LEVEL_BODY = 2;

        //Queue PropertyId constants.
        public const int QUEUE_PROPID_BASE = 100;
        public const int QUEUE_PROPID_INSTANCE = QUEUE_PROPID_BASE + 1;           /* VT_CLSID     */
        public const int QUEUE_PROPID_TYPE = QUEUE_PROPID_BASE + 2;               /* VT_CLSID     */
        public const int QUEUE_PROPID_PATHNAME = QUEUE_PROPID_BASE + 3;           /* VT_LPWSTR    */
        public const int QUEUE_PROPID_JOURNAL = QUEUE_PROPID_BASE + 4;            /* VT_UI1       */
        public const int QUEUE_PROPID_QUOTA = QUEUE_PROPID_BASE + 5;              /* VT_UI4       */
        public const int QUEUE_PROPID_BASEPRIORITY = QUEUE_PROPID_BASE + 6;       /* VT_I2        */
        public const int QUEUE_PROPID_JOURNAL_QUOTA = QUEUE_PROPID_BASE + 7;      /* VT_UI4       */
        public const int QUEUE_PROPID_LABEL = QUEUE_PROPID_BASE + 8;              /* VT_LPWSTR    */
        public const int QUEUE_PROPID_CREATE_TIME = QUEUE_PROPID_BASE + 9;        /* VT_I4        */
        public const int QUEUE_PROPID_MODIFY_TIME = QUEUE_PROPID_BASE + 10;       /* VT_I4        */
        public const int QUEUE_PROPID_AUTHENTICATE = QUEUE_PROPID_BASE + 11;      /* VT_UI1       */
        public const int QUEUE_PROPID_PRIV_LEVEL = QUEUE_PROPID_BASE + 12;        /* VT_UI4       */
        public const int QUEUE_PROPID_TRANSACTION = QUEUE_PROPID_BASE + 13;       /* VT_UI1       */
        //public const int QUEUE_PROPID_PATHNAME_DNS  = QUEUE_PROPID_BASE + 24; /* VT_LPWSTR */ 
        public const int QUEUE_PROPID_MULTICAST_ADDRESS = QUEUE_PROPID_BASE + 25; /* VT_LPWSTR /
        //public const int QUEUE_PROPID_ADS_PATH  = QUEUE_PROPID_BASE + 26; //needed to add queue to DL /* VT_LPWSTR    */ 


        //Queue Shared Mode constants.
        public const int QUEUE_SHARED_MODE_DENY_NONE = 0;
        public const int QUEUE_SHARED_MODE_DENY_RECEIVE = 1;

        //Queue Transaction constants.
        public const int QUEUE_TRANSACTION_NONE = 0;
        public const int QUEUE_TRANSACTION_MTS = 1;
        public const int QUEUE_TRANSACTION_XA = 2;
        public const int QUEUE_TRANSACTION_SINGLE = 3;

        //Queue Transactional Mode constants.
        public const int QUEUE_TRANSACTIONAL_NONE = 0;
        public const int QUEUE_TRANSACTIONAL_TRANSACTIONAL = 1;

        //Security constants
        public const int MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL = unchecked((int)0xc00e0023);
        public const int MQ_OK = 0;

        public const int TRUSTEE_IS_SID = 0;
        public const int TRUSTEE_IS_NAME = 1;
        public const int TRUSTEE_IS_USER = 1;
        public const int TRUSTEE_IS_GROUP = 2;
        public const int TRUSTEE_IS_DOMAIN = 3;
        public const int TRUSTEE_IS_ALIAS = 4;
        public const int TRUSTEE_IS_WELL_KNOWN_GROUP = 5;
        public const int DACL_SECURITY_INFORMATION = 4;
        public const int GRANT_ACCESS = 1;
        public const int SET_ACCESS = 2;
        public const int DENY_ACCESS = 3;
        public const int REVOKE_ACCESS = 4;
        public const int NO_MULTIPLE_TRUSTEE = 0;
        public const int ERROR_SUCCESS = 0;
        public const int SECURITY_DESCRIPTOR_REVISION = 1;

        // This call is here because we don't want to invent a separate MessageQueuePermission
        // for this call, and there's no suitable existing permission. 
        [DllImport(ExternDll.Mqrt, EntryPoint = "MQGetSecurityContextEx", CharSet = CharSet.Unicode)]
        private static extern int IntMQGetSecurityContextEx(IntPtr lpCertBuffer, int dwCertBufferLength, out SecurityContextHandle phSecurityContext);
        public static int MQGetSecurityContextEx(out SecurityContextHandle securityContext)
        {
            try
            {
                return IntMQGetSecurityContextEx(IntPtr.Zero, 0, out securityContext);
            }
            catch (DllNotFoundException)
            {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Ole32, PreserveSig = false)]
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern object OleLoadFromStream(IStream stream, [In] ref Guid iid);

        [DllImport(ExternDll.Ole32, PreserveSig = false)]
        public static extern void OleSaveToStream(IPersistStream persistStream, IStream stream);

        [StructLayout(LayoutKind.Sequential)]
        public class SECURITY_DESCRIPTOR
        {
            public byte revision = 0;
            public byte size = 0;
            public short control = 0;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr owner = (IntPtr)0;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr Group = (IntPtr)0;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr Sacl = (IntPtr)0;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr Dacl = (IntPtr)0;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ExplicitAccess
        {
            public int grfAccessPermissions;
            public int grfAccessMode;
            public int grfInheritance;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr pMultipleTrustees;
            public int MultipleTrusteeOperation;
            public int TrusteeForm;
            public int TrusteeType;

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources")]
            public IntPtr data;
        }
    }
}
