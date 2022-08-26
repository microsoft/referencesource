//------------------------------------------------------------------------------
// <copyright file="SystemIcons.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;
    using System.Diagnostics.Contracts;
    using System;
    using System.Runtime.Versioning;

    /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons"]/*' />
    /// <devdoc>
    ///     Icon objects for Windows system-wide icons.
    /// </devdoc>
    public sealed class SystemIcons {
        private static Icon _application ;
        private static Icon _asterisk    ;
        private static Icon _error       ;
        private static Icon _exclamation ;
        private static Icon _hand        ;
        private static Icon _information ;
        private static Icon _question    ;
        private static Icon _warning     ;
        private static Icon _winlogo     ;
        private static Icon _shield      ;
        
        // not creatable...
        //
        private SystemIcons() {
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Application"]/*' />
        /// <devdoc>
        ///     Icon is the default Application icon.  (WIN32:  IDI_APPLICATION)
        /// </devdoc>
        public static Icon Application {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_application == null)
                    _application = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_APPLICATION ));
                return _application;
            }
        }
        
        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Asterisk"]/*' />
        /// <devdoc>
        ///     Icon is the system Asterisk icon.  (WIN32:  IDI_ASTERISK)
        /// </devdoc>
        public static Icon Asterisk {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_asterisk== null)
                    _asterisk = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_ASTERISK ));
                return _asterisk;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Error"]/*' />
        /// <devdoc>
        ///     Icon is the system Error icon.  (WIN32:  IDI_ERROR)
        /// </devdoc>
        public static Icon Error {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_error == null)
                    _error = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_ERROR ));
                return _error;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Exclamation"]/*' />
        /// <devdoc>
        ///     Icon is the system Exclamation icon.  (WIN32:  IDI_EXCLAMATION)
        /// </devdoc>
        public static Icon Exclamation {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);
                
                if (_exclamation == null)
                    _exclamation = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_EXCLAMATION ));
                return _exclamation;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Hand"]/*' />
        /// <devdoc>
        ///     Icon is the system Hand icon.  (WIN32:  IDI_HAND)
        /// </devdoc>
        public static Icon Hand {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_hand == null)
                    _hand = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_HAND ));
                return _hand;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Information"]/*' />
        /// <devdoc>
        ///     Icon is the system Information icon.  (WIN32:  IDI_INFORMATION)
        /// </devdoc>
        public static Icon Information {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_information == null)
                    _information = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_INFORMATION ));
                return _information;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Question"]/*' />
        /// <devdoc>
        ///     Icon is the system Question icon.  (WIN32:  IDI_QUESTION)
        /// </devdoc>
        public static Icon Question {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);
                
                if (_question == null)
                    _question = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_QUESTION ));
                return _question;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Warning"]/*' />
        /// <devdoc>
        ///     Icon is the system Warning icon.  (WIN32:  IDI_WARNING)
        /// </devdoc>
        public static Icon Warning {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);
                
                if (_warning == null)
                    _warning = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_WARNING ));
                return _warning;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.WinLogo"]/*' />
        /// <devdoc>
        ///     Icon is the Windows Logo icon.  (WIN32:  IDI_WINLOGO)
        /// </devdoc>
        public static Icon WinLogo {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);
                
                if (_winlogo == null)
                    _winlogo = new Icon( SafeNativeMethods.LoadIcon( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_WINLOGO ));
                return _winlogo;
            }
        }

        /// <include file='doc\SystemIcons.uex' path='docs/doc[@for="SystemIcons.Shield"]/*' />
        /// <devdoc>
        ///     Icon is the Windows Shield Icon.
        /// </devdoc>
        public static Icon Shield {
            get {
                Contract.Ensures(Contract.Result<Icon>() != null);

                if (_shield == null) {
                    try {
                        // IDI_SHIELD is defined in OS Vista and above
                        if(Environment.OSVersion.Version.Major >= 6) {
                            // we hard-code size here, to prevent breaking change
                            // the size of _shield before this change is always 32 * 32  
                            IntPtr hIcon = IntPtr.Zero;
                            int result = SafeNativeMethods.LoadIconWithScaleDown( NativeMethods.NullHandleRef, SafeNativeMethods.IDI_SHIELD, 32, 32, ref hIcon );

                            if(result == 0)
                                _shield = new Icon( hIcon );
                        }
                    }
                    catch {
                        // we don't want to throw exception here.
                        // If there is an exception, we will load an icon from file ShieldIcon.ico
                    }
                }
                if (_shield == null) {
                    _shield = new Icon(typeof(SystemIcons), "ShieldIcon.ico");
                }
                return _shield;
            }
        }
    }
}
