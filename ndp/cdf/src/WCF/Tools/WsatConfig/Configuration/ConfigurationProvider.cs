//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------
//
// Provide an abstract base class for providers of configuration
// (E.g. local registry, remote registry or the cluster registry)
//
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;

    abstract class ConfigurationProvider : IDisposable
    {   
        internal abstract ConfigurationProvider OpenKey(string subKey);

        // readers
        internal abstract uint ReadUInt32(string name, uint defaultValue);
        //internal abstract ushort ReadUInt16(string name, ushort defaultValue);
        internal abstract string ReadString(string name, string defaultValue);
        internal abstract string[] ReadMultiString(string name, string[] defaultValue);

        // writers
        internal abstract void WriteUInt32(string name, uint value);
        internal abstract void WriteString(string name, string value);
        internal abstract void WriteMultiString(string name, string[] value);

        internal abstract void AdjustRegKeyPermission();

        public abstract void Dispose();
    }
}
