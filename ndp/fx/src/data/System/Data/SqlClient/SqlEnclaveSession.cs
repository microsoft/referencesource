//------------------------------------------------------------------------------
// <copyright file="SqlCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">nivithla</owner>
// <owner current="true" primary="false">nivithla</owner>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Data.SqlClient {

    /// <summary>
    /// Encapsulates the state of a secure session between SqlClient and an enclave inside SQL Server, which can be used for computations on encrypted columns protected with Always Encrypted.
    /// </summary>
    public class SqlEnclaveSession {
        
        private static readonly string _sessionKeyName = "SessionKey";
        private static readonly string _className = "EnclaveSession";

        private readonly byte[] _sessionKey;
        
        /// <summary>
        /// A session id
        /// </summary>
        public long SessionId { get; }

        /// <summary>
        /// The symmetric key SqlClient uses to encrypt all the information it sends to the enclave using the session. 
        /// </summary>
        public byte[] GetSessionKey() {
            return Clone(_sessionKey);
        }

        /// <summary>
        /// Deep copy the array into a new array
        /// </summary>
        /// <param name="arrayToClone"></param>
        /// <returns></returns>
        private byte[] Clone(byte[] arrayToClone) {

            byte[] returnValue = new byte[arrayToClone.Length];

            for (int i = 0; i < arrayToClone.Length; i++) {
                returnValue[i] = arrayToClone[i];
            }

            return returnValue;
        }

        /// <summary>
        /// Creates a new session
        /// </summary>
        /// <param name="sessionKey">The symmetric key used to encrypt all the information sent using the session.</param>
        /// <param name="sessionId">The session id.</param>
        /// <param name="counter">The counter that helps prevent replay attacks and is incremented each time the session is retrieved from the cache.</param>
        public SqlEnclaveSession(byte[] sessionKey, long sessionId/*, long counter*/) {
            if (null == sessionKey)  { throw SQL.NullArgumentInConstructorInternal(_sessionKeyName, _className); }
            if (0 == sessionKey.Length) { throw SQL.EmptyArgumentInConstructorInternal(_sessionKeyName, _className); }

            _sessionKey = sessionKey;
            SessionId = sessionId;
        }
    }
}
