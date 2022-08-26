// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Value Type:  ContravarianceAdapter
**
** Purpose: To convert an IEnumerable<Base> to an IEnumerable<Derived>
**
===========================================================*/
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    internal struct ContravarianceAdapter<Base, Derived> : IEnumerable<Derived> where Derived : Base
    {
        private IEnumerable<Base> _enumerable;

        internal ContravarianceAdapter(IEnumerable<Base> collection)
        {
            _enumerable = collection;
        }

        public IEnumerator<Derived> GetEnumerator()
        {
            return new ContravariantEnumerator(_enumerable.GetEnumerator());
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return new ContravariantEnumerator(_enumerable.GetEnumerator());
        }

        internal struct ContravariantEnumerator : IEnumerator<Derived>
        {
            IEnumerator<Base> _enumerator;

            internal ContravariantEnumerator(IEnumerator<Base> enumerator)
            {
                _enumerator = enumerator;
            }

            public bool MoveNext()
            {
                return _enumerator.MoveNext();
            }

            public Derived Current
            {
                get { return (Derived)_enumerator.Current; }
            }

            Object IEnumerator.Current
            {
                get { return ((IEnumerator)_enumerator).Current; }
            }

            public void Reset()
            {
                _enumerator.Reset();
            }

            public void Dispose()
            {
                _enumerator.Dispose();
            }
        }
    }
}
