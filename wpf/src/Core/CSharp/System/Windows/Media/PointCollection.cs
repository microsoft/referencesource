//---------------------------------------------------------------------------
//
// <copyright file="PointCollection.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Point collection partial class. 
//
//
// History:  
//  11/09/05 : marka - Created
//
//---------------------------------------------------------------------------

using System.Windows;
using System.Windows.Media.Media3D;
using MS.Internal.PresentationCore; 
using System;
using System.IO; 
using MS.Internal.Media; 

namespace System.Windows.Media
{
    public partial class PointCollection
    {
        ///<summary>
        /// Deserialize this object from BAML
        ///</summary>
        [FriendAccessAllowed] // Built into Core, also used by Framework.
        internal static object DeserializeFrom(BinaryReader reader)
        {
            // Get the size.
            uint count = reader.ReadUInt32() ; 
            
            PointCollection collection = new PointCollection( (int) count) ; 
            
            for ( uint i = 0; i < count ; i ++ ) 
            {

                Point point = new Point(
                                             XamlSerializationHelper.ReadDouble( reader ), 
                                             XamlSerializationHelper.ReadDouble( reader ) ) ; 

                collection.Add( point );                 
            }

            return collection ; 
        }
     
        
    }
}
