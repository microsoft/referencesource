// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       BaseTransportHeaders.cs
//
//  Summary:    Base class for special-cased transport headers implementations.
//
//==========================================================================


using System;
using System.Collections;
using System.Globalization;
using System.Net;


namespace System.Runtime.Remoting.Channels
{

    [Serializable]
    internal class BaseTransportHeaders : ITransportHeaders
    {
        // IMPORTANT: If a new wellknown header is added, MapHeaderNameToIndex,
        //   GetValueFromHeaderIndex, and SetValueFromHeaderIndex methods must
        //   be updated (as well as WellknownHeaderCount)!!!

        internal const int WellknownHeaderCount = 4;

        private Object _connectionId; // 0) CommonTransportKeys.ConnectionId
        private Object _ipAddress;    // 1) CommonTransportKeys.IPAddress
    
        private String _requestUri;   // 2) CommonTransportKeys.RequestUri
        private String _contentType;  // 3) "Content-Type"


        // transport headers structure is for non well-known headers
        private ITransportHeaders _otherHeaders;


        public BaseTransportHeaders()
        {
            _otherHeaders = null;
        }


        public String RequestUri
        {
            get { return _requestUri; }
            set { _requestUri = value; }
        } // RequestUri


        public String ContentType
        {
            get { return _contentType; }
            set { _contentType = value; }
        } // ContentType


        public Object ConnectionId
        {
            set { _connectionId = value; }
        }

        public IPAddress IPAddress
        {
            set { _ipAddress = value; }
        }        


        //
        // ITransportHeaders implementation
        //

        public Object this[Object key]
        {
            get 
            {
                String strKey = key as String;
                if (strKey != null)
                {
                    int index = MapHeaderNameToIndex(strKey);
                    if (index != -1)
                        return GetValueFromHeaderIndex(index);
                }
            
                if (_otherHeaders != null)
                    return _otherHeaders[key];

                return null;
            } // get

            set
            {
                bool bSet = false;
            
                String strKey = key as String;
                if (strKey != null)
                {
                    int index = MapHeaderNameToIndex(strKey);
                    if (index != -1)     
                    {
                        SetValueFromHeaderIndex(index, value);
                        bSet = true;
                    }
                }

                if (!bSet)
                {
                    if (_otherHeaders == null)
                        _otherHeaders = new TransportHeaders();
                    _otherHeaders[key] = value;
                }
            } // set
        } // Object this[Object key]


        public IEnumerator GetEnumerator() 
        {
            return new BaseTransportHeadersEnumerator(this);
        } // GetEnumerator


        internal IEnumerator GetOtherHeadersEnumerator()
        {
            if (_otherHeaders == null)
                return null;

            return _otherHeaders.GetEnumerator();            
        } // GetOtherHeadersEnumerator



        internal int MapHeaderNameToIndex(String headerName)
        {
            // 0) CommonTransportKeys.ConnectionId
            // 1) CommonTransportKeys.IPAddress
            // 2) CommonTransportKeys.RequestUri
            // 3) "Content-Type"
            
            if (String.Compare(headerName, CommonTransportKeys.ConnectionId, StringComparison.OrdinalIgnoreCase) == 0)
                return 0;
            else
            if (String.Compare(headerName, CommonTransportKeys.IPAddress, StringComparison.OrdinalIgnoreCase) == 0)
                return 1;
            else
            if (String.Compare(headerName, CommonTransportKeys.RequestUri, StringComparison.OrdinalIgnoreCase) == 0)
                return 2;
            else
            if (String.Compare(headerName, "Content-Type", StringComparison.OrdinalIgnoreCase) == 0)
                return 3;

            return -1;
        } // MapHeaderNameToIndex


        internal String MapHeaderIndexToName(int index)
        {
            // 0) CommonTransportKeys.ConnectionId
            // 1) CommonTransportKeys.IPAddress
            // 2) CommonTransportKeys.RequestUri
            // 3) "Content-Type"

            switch (index)
            {
            case 0: return CommonTransportKeys.ConnectionId;
            case 1: return CommonTransportKeys.IPAddress;            
            case 2: return CommonTransportKeys.RequestUri;
            case 3: return "Content-Type";
            
            default: return null;
            }
            
        } // MapHeaderNameToIndex


        internal Object GetValueFromHeaderIndex(int index)
        {
            // NOTE: If this method returns the null, then that indicates the header has no
            //   value (i.e. isn't in the "dictionary"). For the purposes of iteration, this
            //   means that the header should be skipped.
        
            // 0) CommonTransportKeys.ConnectionId
            // 1) CommonTransportKeys.IPAddress
            // 2) CommonTransportKeys.RequestUri
            // 3) "Content-Type"

            switch (index)
            {
            case 0: return _connectionId;
            case 1: return _ipAddress;            
            case 2: return _requestUri;
            case 3: return _contentType;
            
            default: return null;
            }
            
        } // MapHeaderIndexToValue


        internal void SetValueFromHeaderIndex(int index, Object value)
        {
            // NOTE: If this method returns the null, then that indicates the header has no
            //   value (i.e. isn't in the "dictionary"). For the purposes of iteration, this
            //   means that the header should be skipped.
        
            // 0) CommonTransportKeys.ConnectionId
            // 1) CommonTransportKeys.IPAddress
            // 2) CommonTransportKeys.RequestUri
            // 3) "Content-Type"

            switch (index)
            {
            case 0: _connectionId = value; break;
            case 1: _ipAddress = value; break;        
            case 2: _requestUri = (String)value; break;
            case 3: _contentType = (String)value; break;

            default: 
            {
                InternalRemotingServices.RemotingAssert(false, "someone forgot to update this method"); 
                break;
            }
            
            } // switch (index)
            
        } // MapHeaderIndexToValue
        

    
    } // class BaseTransportHeaders



    internal class BaseTransportHeadersEnumerator : IEnumerator
    {
        private BaseTransportHeaders _headers;
        private bool _bStarted;
        private int _currentIndex;
        private IEnumerator _otherHeadersEnumerator;
    
        public BaseTransportHeadersEnumerator(BaseTransportHeaders headers)
        {
            _headers = headers;
            Reset();
        } // BaseTransportHeadersEnumerator

    
        public bool MoveNext()
        {
            if (_currentIndex != -1)
            {
                if (_bStarted)
                    _currentIndex++;
                else
                    _bStarted = true;

                while (_currentIndex != -1)
                {
                    if (_currentIndex >= BaseTransportHeaders.WellknownHeaderCount)
                    {
                        _otherHeadersEnumerator = _headers.GetOtherHeadersEnumerator();
                        _currentIndex = -1;
                    }
                    else
                    {
                        if (_headers.GetValueFromHeaderIndex(_currentIndex) != null)
                            return true;
                
                        _currentIndex++;
                    }
                }
            }
            
            if (_otherHeadersEnumerator != null)
            {
                if (!_otherHeadersEnumerator.MoveNext())
                {
                    _otherHeadersEnumerator = null;
                    return false;                   
                }
                else
                    return true;                    
            }

            return false;
        } // MoveNext
        
        public void Reset()
        {
            _bStarted = false;
            _currentIndex = 0;
            _otherHeadersEnumerator = null;
        } // Reset

        public Object Current  
        {
            get 
            {
                if (!_bStarted)
                    return null;
            
                if (_currentIndex != -1)
                {
                    return 
                        new DictionaryEntry(
                            _headers.MapHeaderIndexToName(_currentIndex),
                            _headers.GetValueFromHeaderIndex(_currentIndex));
                }

                if (_otherHeadersEnumerator != null)
                {
                    return _otherHeadersEnumerator.Current;
                }

                return null;
            }
        } // Current
        
    } // class BaseTransportHeadersEnumerator




} // namespace System.Runtime.Remoting.Channels
