//---------------------------------------------------------------------------
//
// <copyright file="DataBindOperation.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Analogous to DispatcherOperation - one unit of cross-thread work.
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Threading;

namespace MS.Internal.Data
{
    internal class DataBindOperation
    {
        public DataBindOperation(DispatcherOperationCallback method, object arg, int cost=1)
        {
            _method = method;
            _arg = arg;
            _cost = cost;
        }

        public int Cost
        {
            get { return _cost; }
            set { _cost = value; }
        }

        public void Invoke()
        {
            _method(_arg);
        }

        DispatcherOperationCallback _method;
        object _arg;
        int _cost;
    }
}
