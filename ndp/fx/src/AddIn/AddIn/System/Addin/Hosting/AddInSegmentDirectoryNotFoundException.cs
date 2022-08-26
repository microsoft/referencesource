// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Globalization;
using System.Runtime.Serialization;

namespace System.AddIn.Hosting
{
   [Serializable]
   public class AddInSegmentDirectoryNotFoundException : Exception 
   {
       public AddInSegmentDirectoryNotFoundException(String message)
           : base(message) { }

       public AddInSegmentDirectoryNotFoundException(String message, Exception innerException)
           : base(message, innerException) { }

       protected AddInSegmentDirectoryNotFoundException(SerializationInfo info, StreamingContext context)
           : base(info, context) { }

       public AddInSegmentDirectoryNotFoundException() { }
   }
}

