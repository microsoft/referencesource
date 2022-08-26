//---------------------------------------------------------------------------
//
// <copyright file="SecurityCriticalData.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description:
//              This is a helper struct to facilate the storage of Security critical data ( aka "Plutonium")
//              It's primary purpose is to do put a [SecurityCritical] on all access to the data.     
//
//              What is "critical data" ? This is any data created that required an Assert for it's creation. 
//              As an example - the creation of an HwndWrapper during Dispatcher.Attach. 
//              The current implementation requires the consumer to use the data member only if IsValid is true
//
// History:
//  10/25/05 : akaza Created. 
//
//---------------------------------------------------------------------------
using System ; 
using System.Security ;

#if WINDOWS_BASE
    using MS.Internal.WindowsBase;
#elif PRESENTATION_CORE
    using MS.Internal.PresentationCore;
#elif PRESENTATIONFRAMEWORK
    using MS.Internal.PresentationFramework;
#elif DRT
    using MS.Internal.Drt;
#else
#error Attempt to use FriendAccessAllowedAttribute from an unknown assembly.
using MS.Internal.YourAssemblyName;
#endif

namespace MS.Internal 
{
    [FriendAccessAllowed]
    internal class SecurityCriticalDataClass<T>
    {
        /// <SecurityNote>
        /// Critical - as this accesses _value which is Critical.
        /// Safe - as the caller already got the critical value.
        /// </SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        internal SecurityCriticalDataClass(T value) 
        { 
            _value = value;
        }

        // <SecurityNote>
        //    Critical "by definition" - this class is intended only to store critical data. 
        // </SecurityNote>
        internal T Value 
        { 
            [SecurityCritical]
            get 
            {     
                return _value; 
            } 
        }


        /// <SecurityNote>
        /// Critical - by definition as this is a wrapper for Critical data.
        /// </SecurityNote>
        [SecurityCritical]
        private T _value;
    }
}
