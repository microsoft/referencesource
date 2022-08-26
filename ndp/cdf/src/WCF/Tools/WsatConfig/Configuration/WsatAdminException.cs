//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;

    class WsatAdminException : Exception
    {
        WsatAdminErrorCode errCode;

        internal WsatAdminException(WsatAdminErrorCode errCode, string errMessage)
            : base(errMessage)
        {
            this.errCode = errCode;
        }

        internal WsatAdminException(WsatAdminErrorCode errCode, string errMessage, Exception innerException)
            : base(errMessage, innerException)
        {
            this.errCode = errCode;
        }

        internal WsatAdminErrorCode ErrorCode
        {
            get { return this.errCode; }
        }
    }
}
