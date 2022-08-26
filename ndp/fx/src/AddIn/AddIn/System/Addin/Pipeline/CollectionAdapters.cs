// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  CollectionAdapters
**
** Purpose: Provides methods to convert from IList<T> to IListContract<T>
** and vice versa.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.AddIn.Contract;
using System.AddIn;

namespace System.AddIn.Pipeline
{
    public static class CollectionAdapters
    {
        // Create a IListContract wrapper for an IList.  This is typically done
        // by an addin adapter that wants to pass a list of objects to the host, 
        // or vice versa. 
        public static IListContract<TContract> ToIListContract<TView, TContract>(
            IList<TView> collection, 
            Converter<TView, TContract> viewContractAdapter, 
            Converter<TContract, TView> contractViewAdapter)
        {
            if (collection == null)
                return null;
            return new ListContractAdapter<TView, TContract>(collection, viewContractAdapter, contractViewAdapter);
        }

        public static IListContract<T> ToIListContract<T>(IList<T> collection)
        {
            if (collection == null)
                return null;
            Converter<T, T> c = new Converter<T, T>(IdentityConverter<T>);
            return ToIListContract(collection, c, c);
        }

        // Create an IList that wraps a IContractList.  The returned IList will
        // have a lifetimeToken for the remote IContractList.  Its finalizer
        // will revoke the lifetimeToken.
        public static IList<TView> ToIList<TContract, TView>(
            IListContract<TContract> collection, 
            Converter<TContract, TView> contractViewAdapter,
            Converter<TView, TContract> viewContractAdapter)
        {
            if (collection == null)
                return null;
            return new ContractListAdapter<TContract, TView>(collection, contractViewAdapter, viewContractAdapter);
        }

        public static IList<T> ToIList<T>(IListContract<T> collection)
        {
            if (collection == null)
                return null;
            Converter<T,T> c = new Converter<T,T>(IdentityConverter<T>);
            return ToIList(collection, c, c);
        }

        private static T IdentityConverter<T>(T item)
        {
            return item;
        }
    }
}

