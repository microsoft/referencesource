// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Runtime.Serialization;

namespace System.AddIn.MiniReflection
{
   [Serializable]
   internal class GenericsNotImplementedException : Exception {

       protected GenericsNotImplementedException(SerializationInfo info, StreamingContext context)
           : base(info, context) { }

       internal GenericsNotImplementedException() { }
   }
}
