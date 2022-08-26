//---------------------------------------------------------------------------
//
// <copyright file="RenderingBiasValidation.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// This file was generated, please do not edit it directly.
//
// Please see http://wiki/default.aspx/Microsoft.Projects.Avalon/MilCodeGen.html for more information.
//
//---------------------------------------------------------------------------

#if PRESENTATION_CORE
using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;
#else
using SR=System.Windows.SR;
using SRID=System.Windows.SRID;
#endif

namespace System.Windows.Media.Effects
{
    internal static partial class ValidateEnums
    {
        /// <summary>
        ///     Returns whether or not an enumeration instance a valid value.
        ///     This method is designed to be used with ValidateValueCallback, and thus
        ///     matches it's prototype.
        /// </summary>
        /// <param name="valueObject">
        ///     Enumeration value to validate.
        /// </param>    
        /// <returns> 'true' if the enumeration contains a valid value, 'false' otherwise. </returns>
        public static bool IsRenderingBiasValid(object valueObject)
        {
            RenderingBias value = (RenderingBias) valueObject;

            return (value == RenderingBias.Performance) || 
                   (value == RenderingBias.Quality);
        }                                
    }
}
