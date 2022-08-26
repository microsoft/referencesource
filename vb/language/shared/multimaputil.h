//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Template Utilities for the std::multimap class
//
//-------------------------------------------------------------------------------------------------

#pragma once

class MultimapUtil
{
public:

    template <typename TMultimap, typename TKey>
    static 
    bool ContainsKey( _In_ const TMultimap& map, _In_ TKey& key )
    {
        return map.find(key) != map.end();
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Insert the pair of values into the multi-map 
    //
    //-------------------------------------------------------------------------------------------------
    template <typename TMultimap, typename TKey,typename TValue>
    static 
    typename TMultimap::iterator InsertPair( 
        _Inout_ TMultimap& map, 
        _In_ const TKey& key,
        _In_ const TValue& value)
    {
        TMultimap::value_type p(key,value);
        return map.insert(p);
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Find the key / value pair inside the multi-map.  If the Key/Pair is not found this will
    // return multimap<TKey,TValue>::end()
    //
    //-------------------------------------------------------------------------------------------------
    template <typename TMultimap, typename TKey,typename TValue>
    static 
    typename TMultimap::const_iterator FindPair( 
        _In_ const TMultimap& map, 
        _In_ const TKey& key,
        _In_ const TValue& value)
    {
        auto ret = map.equal_range(key);
        for ( auto it = ret.first; it != ret.second; ++it )
        {
            if ( it->second == value )
            {
                return it;
            }
        }

        return map.end();
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Determines if the key / value pair are present in the multimap
    //
    //-------------------------------------------------------------------------------------------------
    template <typename TMultimap, typename TKey,typename TValue>
    static 
    bool ContainsPair( 
        _In_ const TMultimap& map, 
        _In_ const TKey& key,
        _In_ const TValue& value)
    {
        auto it = MultimapUtil::FindPair(map, key, value);
        return it != map.end();
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Try and find the first value for the specified key
    //
    //-------------------------------------------------------------------------------------------------
    template <typename TMultimap, typename TKey,typename TValue>
    static 
    bool TryGetFirstValue( 
        _In_ const TMultimap& map, 
        _In_ const TKey& key,
        _Out_ TValue& value)
    {
        auto it = map.find(key);
        if ( it == map.end() )
        {
            return false;
        }

        value = it->second;
        return true;
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Remove the first value with the specified key.  If the Key is not present, no action will be 
    // taken on the multimap
    //
    //-------------------------------------------------------------------------------------------------
    template <typename TMultimap, typename TKey>
    static 
    void RemoveFirstValue(
        _Inout_ TMultimap& map,
        _In_ const TKey& key)
    {
        auto it = map.find(key);
        if ( it != map.end() )
        {
            map.erase(it);
        }
    }


};
