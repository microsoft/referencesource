//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Net;
    using System.Runtime.Diagnostics;
    using System.Security.AccessControl;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Diagnostics;
    using System.Threading;   

    static class RoutingTable
    {
        class MessageQueueAndPath
        {
            MessageQueue messageQueue;
            Uri uri;

            internal MessageQueueAndPath(MessageQueue messageQueue, Uri uri)
            {
                this.messageQueue = messageQueue;
                this.uri = uri;
            }

            internal MessageQueue MessageQueue { get { return messageQueue; } }
            internal Uri Uri { get { return uri; } }
        }

        static UriPrefixTable<MessageQueueAndPath> namedPipeMessageQueues = new UriPrefixTable<MessageQueueAndPath>();
        static UriPrefixTable<MessageQueueAndPath> tcpMessageQueues = new UriPrefixTable<MessageQueueAndPath>(true);

#if DEBUG
        internal static void DumpTables(TransportType transportType)
        {
            UriPrefixTable<MessageQueueAndPath> table = transportType == TransportType.Tcp ? tcpMessageQueues : namedPipeMessageQueues;
            lock (table)
            {
                Debug.Print("RoutingTable dumping " + table.Count + " Route(s) for TransportType: " + transportType);
                int count = 0;
                foreach (KeyValuePair<BaseUriWithWildcard, MessageQueueAndPath> item in table.GetAll())
                {
                    bool activated = item.Value.MessageQueue.GetType().Equals(typeof(ActivatedMessageQueue));
                    Debug.Print("Registration #" + (++count).ToString(CultureInfo.CurrentUICulture));
                    Debug.Print("\tActivated:" + activated);
                    Debug.Print("\tCanDispatch:" + item.Value.MessageQueue.CanDispatch);
                    Debug.Print("\tBaseAddress:" + item.Key.BaseAddress);
                    Debug.Print("\tHostNameComparisonMode:" + item.Key.HostNameComparisonMode);
                    List<WorkerProcess> workers = item.Value.MessageQueue.SnapshotWorkers();
                    if (workers.Count == 0)
                    {
                        Debug.Print("\tNo WorkerProcess Active.");
                    }
                    else
                    {
                        Debug.Print("\t" + workers.Count + " WorkerProcess(es) Registered:");
                        foreach (WorkerProcess wp in workers)
                        {
                            Debug.Print("\t\tPid:" + wp.ProcessId);
                            if (activated)
                            {
                                Debug.Print("\t\tActive:" + wp.IsRegistered);
                                Debug.Print("\t\tQueueId:" + wp.QueueId);
                            }
                        }
                    }
                }
            }
        }
#endif
        internal static MessageQueue Lookup(Uri uri, IPAddress address, int port)
        {
            if (TD.RoutingTableLookupStartIsEnabled())
            {
                TD.RoutingTableLookupStart();
            }

            Uri wildCardUri = uri;
            UriPrefixTable<MessageQueueAndPath> table = namedPipeMessageQueues;
            if (address != null)
            {
                // Including port number to support TCP proxy (see MB56472). We only use it for wildcard matching below.
                // NOTE: we don't need to call TcpChannelListener.FixIpv6Hostname to fix the host name because it's ignored anyway.
                UriBuilder uriBuilder = new UriBuilder(uri.Scheme, uri.Host, port, uri.PathAndQuery);
                wildCardUri = uriBuilder.Uri;
                table = tcpMessageQueues;
            }

            MessageQueueAndPath found = null;
            bool success = table.TryLookupUri(wildCardUri, HostNameComparisonMode.StrongWildcard, out found);
            if (success && address != null)
            {
                success = ValidateAddress(address, ref found);
            }
            if (!success)
            {
                success = table.TryLookupUri(uri, HostNameComparisonMode.Exact, out found);
                if (success && address != null)
                {
                    success = ValidateAddress(address, ref found);
                }
            }
            if (!success)
            {
                success = table.TryLookupUri(wildCardUri, HostNameComparisonMode.WeakWildcard, out found);
                if (success && address != null)
                {
                    success = ValidateAddress(address, ref found);
                }
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTableLookup, SR.GetString(SR.TraceCodeRoutingTableLookup), new StringTraceRecord("Uri", uri.ToString()), null, null);
            }

            Debug.Print("RoutingTable.Lookup(" + uri + ") matched: " + (found == null ? "<NoMatch!>" : found.Uri.ToString()));
            if (TD.RoutingTableLookupStopIsEnabled())
            {
                TD.RoutingTableLookupStop();
            }
            return found == null ? null : found.MessageQueue;
        }

        static bool ValidateAddress(IPAddress address, ref MessageQueueAndPath found)
        {
            if (found.Uri.HostNameType != UriHostNameType.IPv4 && found.Uri.HostNameType != UriHostNameType.IPv6)
            {
                return true;
            }

            IPAddress foundAddress = IPAddress.Parse(found.Uri.DnsSafeHost);
            bool valid = (address.Equals(foundAddress) ||
                (foundAddress.Equals(IPAddress.Any) && foundAddress.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork) ||
                foundAddress.Equals(IPAddress.IPv6Any));
            found = valid ? found : null;
            return valid;
        }

        internal static ListenerExceptionStatus NamedPipeStart(MessageQueue messageQueue, BaseUriWithWildcard path)
        {
            int encodedSize = System.Text.Encoding.UTF8.GetByteCount(path.BaseAddress.AbsoluteUri);
            if (encodedSize > ListenerConstants.MaxUriSize)
            {
                if (DiagnosticUtility.ShouldTraceInformation)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTablePathTooLong, SR.GetString(SR.TraceCodeRoutingTablePathTooLong), new StringTraceRecord("Path", path.ToString()), null, null);
                }

                return ListenerExceptionStatus.PathTooLong;
            }
            lock (namedPipeMessageQueues)
            {
                if (namedPipeMessageQueues.IsRegistered(path))
                {
                    if (DiagnosticUtility.ShouldTraceInformation)
                    {
                        ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTableNamespaceConflict, SR.GetString(SR.TraceCodeRoutingTableNamespaceConflict), new StringTraceRecord("Path", path.ToString()), null, null);
                    }

                    return ListenerExceptionStatus.ConflictingRegistration;
                }

                TransportListener.Listen(path);
                namedPipeMessageQueues.RegisterUri(path.BaseAddress, path.HostNameComparisonMode, new MessageQueueAndPath(messageQueue, path.BaseAddress));
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTableRegisterSuccess, SR.GetString(SR.TraceCodeRoutingTableRegisterSuccess), new StringTraceRecord("Path", path.ToString()), null, null);
            }

            return ListenerExceptionStatus.Success;
        }

        internal static ListenerExceptionStatus Start(MessageQueue messageQueue, BaseUriWithWildcard path)
        {
            if (messageQueue.TransportType == TransportType.Tcp)
            {
                return TcpStart(messageQueue, path);
            }
            else
            {
                return NamedPipeStart(messageQueue, path);
            }
        }

        static IPEndPoint GetEndPoint(Uri path)
        {
            IPAddress address = IPAddress.Broadcast;
            if (path.HostNameType == UriHostNameType.IPv4 || path.HostNameType == UriHostNameType.IPv6)
            {
                address = IPAddress.Parse(path.DnsSafeHost);
            }
            return new IPEndPoint(address, path.Port);
        }

        internal static void Stop(MessageQueue messageQueue, BaseUriWithWildcard path)
        {
            if (messageQueue.TransportType == TransportType.Tcp)
            {
                IPEndPoint endPoint = GetEndPoint(path.BaseAddress);
                TransportListener.Stop(endPoint);
                tcpMessageQueues.UnregisterUri(path.BaseAddress, path.HostNameComparisonMode);
            }
            else
            {
                TransportListener.Stop(path);
                namedPipeMessageQueues.UnregisterUri(path.BaseAddress, path.HostNameComparisonMode);
            }
        }

        static ListenerExceptionStatus TcpStart(MessageQueue messageQueue, BaseUriWithWildcard path)
        {
            int encodedSize = System.Text.Encoding.UTF8.GetByteCount(path.BaseAddress.AbsoluteUri);
            if (encodedSize > ListenerConstants.MaxUriSize)
            {
                if (DiagnosticUtility.ShouldTraceInformation)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTablePathTooLong, SR.GetString(SR.TraceCodeRoutingTablePathTooLong), new StringTraceRecord("Path", path.ToString()), null, null);
                }

                return ListenerExceptionStatus.PathTooLong;
            }
            IPEndPoint endPoint = GetEndPoint(path.BaseAddress);
            lock (tcpMessageQueues)
            {
                if (tcpMessageQueues.IsRegistered(path))
                {
                    if (DiagnosticUtility.ShouldTraceInformation)
                    {
                        ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTableNamespaceConflict, SR.GetString(SR.TraceCodeRoutingTableNamespaceConflict), new StringTraceRecord("Path", path.ToString()), null, null);
                    }

                    return ListenerExceptionStatus.ConflictingRegistration;
                }

                TransportListener.Listen(endPoint);
                tcpMessageQueues.RegisterUri(path.BaseAddress, path.HostNameComparisonMode, new MessageQueueAndPath(messageQueue, path.BaseAddress));
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.RoutingTableRegisterSuccess, SR.GetString(SR.TraceCodeRoutingTableRegisterSuccess), new StringTraceRecord("Path", path.ToString()), null, null);
            }

            return ListenerExceptionStatus.Success;
        }
    }
}
