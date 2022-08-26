//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// File: FloatUtil.cs
//
// Description: This file contains the implementation of FloatUtil, which
//              provides "fuzzy" comparison functionality for floats and
//              float-based classes and structs in our code.
//
// History:
//  04/28/2003 : Microsoft - Created this header
//  05/20/2003 : Microsoft - Move to Shared.
//
//---------------------------------------------------------------------------

using System;

namespace MS.Internal
{
    internal static class FloatUtil
    {
        internal static float FLT_EPSILON  =   1.192092896e-07F;
        internal static float FLT_MAX_PRECISION = 0xffffff;
        internal static float INVERSE_FLT_MAX_PRECISION = 1.0F / FLT_MAX_PRECISION;

        /// <summary>
        /// AreClose
        /// </summary>
        public static bool AreClose(float a, float b)
        {
            if(a == b) return true;
            // This computes (|a-b| / (|a| + |b| + 10.0f)) < FLT_EPSILON
            float eps = ((float)Math.Abs(a) + (float)Math.Abs(b) + 10.0f) * FLT_EPSILON;
            float delta = a - b;
            return(-eps < delta) && (eps > delta);
        }

        /// <summary>
        /// IsOne
        /// </summary>
        public static bool IsOne(float a)
        {
            return (float)Math.Abs(a-1.0f) < 10.0f * FLT_EPSILON;
        }

        /// <summary>
        /// IsZero
        /// </summary>
        public static bool IsZero(float a)
        {
            return (float)Math.Abs(a) < 10.0f * FLT_EPSILON;
        }

        /// <summary>
        /// IsCloseToDivideByZero
        /// </summary>
        public static bool IsCloseToDivideByZero(float numerator, float denominator)
        {
            // When updating this, please also update code in Arithmetic.h
            return Math.Abs(denominator) <= Math.Abs(numerator) * INVERSE_FLT_MAX_PRECISION;
        }

    }
}
