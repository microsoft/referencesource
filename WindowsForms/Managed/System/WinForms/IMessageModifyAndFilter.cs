namespace System.Windows.Forms {
    using System;
    
    // this is used by Application.cs to detect if we should respect changes to 
    // the message as well as whether or not we should filter the message.
    internal interface IMessageModifyAndFilter : IMessageFilter {
    }
}
        
