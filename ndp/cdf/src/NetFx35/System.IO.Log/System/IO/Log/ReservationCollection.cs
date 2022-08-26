//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading;


    public abstract class ReservationCollection : ICollection<long>
    {
        Dictionary<long, int> reservations;
        List<long> reservationKeys;
        int reservationCount;
        bool keysSorted;
        int version;

        protected ReservationCollection()
        {
            this.reservations = new Dictionary<long, int>();
            this.reservationKeys = new List<long>();
            this.reservationCount = 0;
            this.keysSorted = true;
            this.version = 0;
        }

        ~ReservationCollection()
        {
            Clear();
        }

        public int Count
        {
            get
            {
                return this.reservationCount;
            }
        }

        public bool IsReadOnly
        {
            get
            {
                return false;
            }
        }

        List<long> ReservationKeys
        {
            get { return this.reservationKeys; }
        }

        Dictionary<long, int> Reservations
        {
            get { return this.reservations; }
        }

        object SyncRoot
        {
            get { return this.reservations; }
        }

        int Version
        {
            get { return this.version; }
        }

        public void Add(long size)
        {
            if (size < 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));

            bool throwing = true;
            MakeReservation(size);
            try
            {
                ReservationMade(size);
                throwing = false;
            }
            finally
            {
                try
                {
                    if (throwing)
                    {
                        FreeReservation(size);
                    }
                }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
                catch (Exception e)
                {
                    // The undo operation failed - IO.Log is now in an inconsistent state
                    DiagnosticUtility.InvokeFinalHandler(e);
                }
            }
        }

        public void Clear()
        {
            Dictionary<long, int> reservationsToClear;

            // Finalizers can run on on object even if the constructor didn't run.
            if (this.reservations == null || this.reservationKeys == null)
            {
                return;
            }

            lock (SyncRoot)
            {
                reservationsToClear = new Dictionary<long, int>(this.reservations);

                this.reservations.Clear();
                this.reservationKeys.Clear();
                this.reservationCount = 0;
                this.version = 0;
                this.keysSorted = true;
            }

            foreach (KeyValuePair<long, int> entry in reservationsToClear)
            {
                for (int i = 0; i < entry.Value; i++)
                {
                    try
                    {
                        FreeReservation(entry.Key);
                    }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
                    catch (Exception e)
                    {
                        // We should always be able to free a reservation
                        DiagnosticUtility.InvokeFinalHandler(e);
                    }
                }
            }
        }

        public bool Contains(long size)
        {
            lock (SyncRoot)
            {
                return this.reservations.ContainsKey(size);
            }
        }

        public void CopyTo(long[] array, int arrayIndex)
        {
            lock (SyncRoot)
            {
                if (array == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("array"));
                }
                if (array.Rank != 1)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.ArgumentInvalid(SR.Argument_ArrayIsMultiDimensional));
                }
                if (arrayIndex < array.GetLowerBound(0))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("arrayIndex"));
                }
                if (array.Length == 0 && arrayIndex == 0)
                {
                    return;
                }

                if ((arrayIndex >= array.Length) ||
                    (this.reservationCount > array.Length - arrayIndex))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_CopyNotEnoughSpace));
                }

                SortReservationKeys();

                foreach (long size in this.reservationKeys)
                {
                    int count = this.reservations[size];
                    for (int i = 0; i < count; i++)
                    {
                        array[arrayIndex] = size;
                        arrayIndex++;
                    }
                }
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        public IEnumerator<long> GetEnumerator()
        {
            lock (SyncRoot)
            {
                SortReservationKeys();

                return new Enumerator(this);
            }
        }

        public bool Remove(long item)
        {
            bool success = false;
            lock (SyncRoot)
            {
                if (this.reservations.ContainsKey(item))
                {
                    ReservationFreed(item);
                    success = true;
                }
            }

            if (success)
            {
                bool throwing = true;
                try
                {
                    FreeReservation(item);
                    throwing = false;
                }
                finally
                {
                    try
                    {
                        if (throwing)
                        {
                            ReservationMade(item);
                        }
                    }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
                    catch (Exception e)
                    {
                        // The undo operation failed - IO.Log is now in an inconsistent state
                        DiagnosticUtility.InvokeFinalHandler(e);
                    }
                }
            }

            return success;
        }

        protected abstract void MakeReservation(long size);
        protected abstract void FreeReservation(long size);

        protected void ReservationMade(long size)
        {
            if (size < 0)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("size"));

            lock (SyncRoot)
            {
                int count;
                if (!this.reservations.TryGetValue(size, out count))
                {
                    this.reservationKeys.Add(size);
                    this.keysSorted = false;

                    count = 0;
                }

                this.reservations[size] = count + 1;
                this.reservationCount++;
                this.version++;
            }
        }

        protected void ReservationFreed(long size)
        {
            lock (SyncRoot)
            {
                int count;
                if (!this.reservations.TryGetValue(size, out count))
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_ReservationNotFound));

                count--;
                if (count == 0)
                {
                    int index;
                    if (this.keysSorted)
                        index = this.reservationKeys.BinarySearch(size);
                    else
                        index = this.reservationKeys.IndexOf(size);

                    this.reservationKeys.RemoveAt(index);
                    this.reservations.Remove(size);
                }
                else
                {
                    this.reservations[size] = count;
                }

                this.reservationCount--;
                this.version++;
            }
        }

        protected long GetBestMatchingReservation(long size)
        {
            lock (SyncRoot)
            {
                SortReservationKeys();

                int index = this.reservationKeys.BinarySearch(size);
                if (index < 0)
                {
                    index = ~index;
                    if (index == this.reservationKeys.Count)
                        return -1;
                }

                long reservation = this.reservationKeys[index];
                ReservationFreed(reservation);

                if (reservation < size)
                {
                    // An internal consistency check has failed. The indicates a bug in IO.Log's internal processing
                    // Rather than proceeding with non-deterministic execution and risking the loss or corruption of
                    // log records, we failfast the process.
                    DiagnosticUtility.FailFast("Binary search returned an inappropriate reservation");
                }

                return reservation;
            }
        }

        void SortReservationKeys()
        {
            if (!this.keysSorted)
            {
                this.reservationKeys.Sort();
                this.keysSorted = true;
            }
        }

        class Enumerator : IEnumerator<long>
        {
            const long NOT_STARTED = -1;
            const long FINISHED = -2;

            ReservationCollection parent;
            long current;
            int keyIndex;
            int numberRemaining;
            int version;
            bool disposed;

            public Enumerator(ReservationCollection parent)
            {
                this.parent = parent;
                this.version = parent.Version;
                this.current = NOT_STARTED;
                this.keyIndex = 0;
                this.numberRemaining = 0;
                this.disposed = false;
            }

            object IEnumerator.Current
            {
                get
                {
                    return this.Current;
                }
            }

            public long Current
            {
                get
                {
                    if (this.disposed)
                    {
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                    }

                    // IEnumerable interface contract for "current" member can throw InvalidOperationException. Suppressing this warning.           
                    if (this.current == NOT_STARTED)
                    {
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.InvalidOperation_EnumNotStarted));
                    }

                    if (this.current == FINISHED)
                    {
#pragma warning suppress 56503
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.InvalidOperation_EnumEnded));
                    }

                    return this.current;
                }
            }

            public bool MoveNext()
            {
                if (this.disposed)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }

                lock (this.parent.SyncRoot)
                {
                    if (this.parent.Version != this.version)
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.InvalidOperation_EnumFailedVersion));
                    }

                    if (this.numberRemaining == 0)
                    {
                        if (this.keyIndex == this.parent.ReservationKeys.Count)
                        {
                            this.current = FINISHED;
                            return false;
                        }

                        this.current = this.parent.ReservationKeys[this.keyIndex];
                        this.keyIndex++;

                        this.numberRemaining = this.parent.Reservations[this.current];
                    }

                    this.numberRemaining--;
                    return true;
                }
            }

            public void Reset()
            {
                if (this.disposed)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }

                lock (this.parent.SyncRoot)
                {

                    if (this.parent.Version != this.version)
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                                        Error.InvalidOperation(SR.InvalidOperation_EnumFailedVersion));
                    }

                    this.version = this.parent.Version;
                    this.current = NOT_STARTED;
                    this.keyIndex = 0;
                    this.numberRemaining = 0;
                }
            }


            public void Dispose()
            {
                this.disposed = true;
            }
        }
    }
}
