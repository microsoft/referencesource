//------------------------------------------------------------------------------
// <copyright file="IDeviceContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing
{
    using System.Security.Permissions;
    
    /// <include file='doc\IDeviceContext.uex' path='docs/doc[@for="IDeviceContext"]/*' />
    /// <devdoc>
    ///       This interface defines methods for obtaining a display/window device context handle (Win32 hdc).
    ///       Note: Display and window dc handles are obtained and released using BeginPaint/EndPaint and
    ///       GetDC/ReleaseDC; this interface is intended to be used with the last method only.  
    ///       
    ///       Warning to implementors: Creating and releasing non-display dc handles using this interface needs
    ///       special care, for instance using other Win32 functions like CreateDC or CreateCompatibleDC require 
    ///       DeleteDC instead of ReleaseDC to properly free the dc handle.  
    ///       
    ///       See the DeviceContext class for an implemenation of this interface, it uses the Dispose method
    ///       for freeing non-display dc handles.
    ///       
    ///       This is a low-level API that is expected to be used with TextRenderer or PInvoke calls.
    ///       
    ///       
    ///       SECREVIEW : This interface has an inheritance demand to protect implementors from creating malicious
    ///                   IDeviceContext objects that can be used in libraries like TextRenderer to do harmful
    ///                   operations using our securitiy credentials.  For instance, the implementation could be
    ///                   wrapping a screen dc or a restricted full-window dc where they can overwrite the caption.
    ///                   Also, GetHdc can return a bogus value that we have no way of validating and even though
    ///                   GDI & GDI+ usually fail when an invalid hdc is used, we don't know for sure what could
    ///                   happen (defense in depth).
    ///                   
    ///                   It also has a LinkDemand for UnamagedCode since we need to have that demand in the Graphics
    ///                   class which implements this interface, the interface needs to have it to avoid bypassing
    ///                   the demand by calling on the interface.  Grahpics.GetHdc needs the demand because it 
    ///                   blocks the Graphics object; DOS attacks can happen if bad code, for instance a pluggable 
    ///                   component, gets a ref to a Graphics object that is also used internally, like the one from 
    ///                   PaintEventArguments.  An exception is thrown when calling upon a  method on the blocked object.
    ///                   
    ///                   These methods will be running under the implementors credentials so no harm can be done by 
    ///                   calling them from our API.
    /// </devdoc>
    public interface IDeviceContext : IDisposable
    {
        [SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
        IntPtr GetHdc();

        [SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
        void ReleaseHdc();
	}
}

