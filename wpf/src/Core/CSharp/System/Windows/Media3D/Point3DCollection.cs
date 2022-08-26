//---------------------------------------------------------------------------
//
// <copyright file="Point3DCollection.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: 3D point collection partial class. 
//
//              See spec at http://avalon/medialayer/Specifications/Avalon3D%20API%20Spec.mht 
//
// History:  
//  11/03/05 : marka - Created
//
//---------------------------------------------------------------------------

using System.Windows;
using System.Windows.Media.Media3D;
using MS.Internal.PresentationCore; 
using System;
using System.IO; 
using MS.Internal.Media; 

namespace System.Windows.Media.Media3D
{
    /// <summary>
    /// Point3D - 3D point representation. 
    /// Defaults to (0,0,0).
    /// </summary>
    public partial class Point3DCollection
    {


        ///<summary>
        ///  Deserialize this object from  BAML binary format.
        ///</summary>
        [FriendAccessAllowed] // Built into Core, also used by Framework.
        internal static object DeserializeFrom(BinaryReader reader)
        {
            // Get the size.
            uint count = reader.ReadUInt32() ; 
            
            Point3DCollection collection = new Point3DCollection( (int) count) ; 
            
            for ( uint i = 0; i < count ; i ++ ) 
            {

                Point3D point = new Point3D(
                                             XamlSerializationHelper.ReadDouble( reader ), 
                                             XamlSerializationHelper.ReadDouble( reader ) , 
                                             XamlSerializationHelper.ReadDouble( reader ) ) ; 

                collection.Add( point );                 
            }

            return collection ; 
        }
     
        
    }
}
