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
   public class InvalidPipelineStoreException : Exception 
   {
       public InvalidPipelineStoreException(String message)
           : base(message) { }

       public InvalidPipelineStoreException(String message, Exception innerException)
           : base(message, innerException) { }

       protected InvalidPipelineStoreException(SerializationInfo info, StreamingContext context)
           : base(info, context) { }

       public InvalidPipelineStoreException() : base(Res.InvalidPipelineStoreExceptionMessage)
       { }
   }
}

