//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Security;
    using System.Security.Permissions;

    class FirewallWrapper
    {
        const string FwMgrClassId = "{304CE942-6E39-40D8-943A-B913C40C9CD4}";
        const string FwOpenPortClassId = "{0CA545C6-37AD-4A6C-BF92-9F7610067EF5}";

        INetFirewallMgr manager = null;
        INetFirewallOpenPortsCollection openPorts = null;
        INetFirewallPolicy localPolicy = null;
        INetFirewallProfile currentProfile = null;

        [SecurityCritical]
        internal FirewallWrapper()
        {
            try
            {
                this.manager = (INetFirewallMgr)Activator.CreateInstance(Type.GetTypeFromCLSID(new Guid(FwMgrClassId)));
                this.localPolicy = this.manager.LocalPolicy;
                this.currentProfile = this.localPolicy.CurrentProfile;
                this.openPorts = this.currentProfile.GloballyOpenPorts;
            }
            catch (COMException)
            {
                this.manager = null;
                this.localPolicy = null;
                this.currentProfile = null;
                this.openPorts = null;
            }
            catch (MethodAccessException ex)
            {
                throw new WsatAdminException(WsatAdminErrorCode.FIREWALL_ACCESS_DENIED, SR.GetString(SR.FirewallAccessDenied), ex);
            }
        }

        bool IsHttpsPortOpened(int port)
        {
            foreach (INetFirewallOpenPort openPort in this.openPorts)
            {
                if (openPort.Port == port)
                {
                    return true;
                }
            }
            return false;
        }

        internal void AddHttpsPort(int portToAdd) 
        {
            //
            //if portToAdd is already opened, adding it anyway will remove the old entry
            //
            if (portToAdd < 0 || this.openPorts == null || IsHttpsPortOpened(portToAdd))
            {
                return;
            }

            try
            {
                INetFirewallOpenPort openPort = (INetFirewallOpenPort)Activator.CreateInstance(Type.GetTypeFromCLSID(new Guid(FwOpenPortClassId)));
                openPort.Enabled = true;
                openPort.IPVersion = NetFirewallIPVersion.Any;
                openPort.Name = SR.GetString(SR.HTTPSPortName);
                openPort.Port = portToAdd;
                openPort.Protocol = NetFirewallIPProtocol.Tcp;
                openPort.Scope = NetFirewallScope.All;
                this.openPorts.Add(openPort);
            }
            catch (COMException e)
            {
                throw new WsatAdminException(WsatAdminErrorCode.UNEXPECTED_FIREWALL_CONFIG_ERROR,
                                                SR.GetString(SR.UnexpectedFirewallError, e.Message), e);
            }
            catch (MethodAccessException e)
            {
                throw new WsatAdminException(WsatAdminErrorCode.FIREWALL_ACCESS_DENIED, SR.GetString(SR.FirewallAccessDenied), e);
            }
            catch (UnauthorizedAccessException e)
            {
                throw new WsatAdminException(WsatAdminErrorCode.FIREWALL_ACCESS_DENIED, SR.GetString(SR.FirewallAccessDenied), e);
            }
        }

        internal void RemoveHttpsPort(int portToRemove)
        {
            if (portToRemove < 0 || this.openPorts == null)
            {
                return;
            }

            List<INetFirewallOpenPort> ports = new List<INetFirewallOpenPort>();
            foreach (INetFirewallOpenPort port in this.openPorts)
            {
                if (port.Port == portToRemove && Utilities.SafeCompare(port.Name, SR.GetString(SR.HTTPSPortName)))
                {
                    ports.Add(port);
                    // continue to remove other ports under the WSAT port name to minimize security attack
                    // surface for the machine, but throw out an exception in the end
                }
            }

            bool accessDenied = false;
            foreach (INetFirewallOpenPort port in ports)
            {
                try
                {
                    this.openPorts.Remove(port.Port, port.Protocol);
                }
                catch (UnauthorizedAccessException)
                {
                    accessDenied = true;
                }
            }

            if (accessDenied)
            {
                // at least one port could not be removed due to permission denied
                throw new WsatAdminException(WsatAdminErrorCode.FIREWALL_ACCESS_DENIED,
                                                SR.GetString(SR.FirewallAccessDenied));
            }
        }
    }
}
