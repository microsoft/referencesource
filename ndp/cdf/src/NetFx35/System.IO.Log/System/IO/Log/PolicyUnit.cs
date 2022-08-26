//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Globalization;
    
    public struct PolicyUnit
    {
        PolicyUnitType type;
        long value;
        
        public PolicyUnit(long value, PolicyUnitType type)
        {
            if (type == PolicyUnitType.Percentage)
            {
                if ((value < 0) || (value > 100))
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));
            }
            else if (type == PolicyUnitType.Extents)
            {
                if (value < 0)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));                
            }
            else
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("type"));
            }

            this.type = type;
            this.value = value;
        }

        public PolicyUnitType Type
        {
            get
            {
                return this.type;
            }
        }

        public long Value
        {
            get
            {
                return this.value;
            }
        }

        public override int GetHashCode()
        {
            return (this.type.GetHashCode() ^
                    this.value.GetHashCode());
        }

        public override bool Equals(object obj)
        {
            if (!(obj is PolicyUnit)) return false;

            PolicyUnit other = (PolicyUnit)(obj);

            return this == other;
        }

        public static bool operator ==(PolicyUnit left, PolicyUnit right)
        {
            return ((left.type == right.type) &&
                    (left.value == right.value));
        }

        public static bool operator !=(PolicyUnit left, PolicyUnit right)
        {
            return !(left == right);
        }

        public static PolicyUnit Percentage(long value)
        {
            return new PolicyUnit(value, PolicyUnitType.Percentage);
        }

        public static PolicyUnit Extents(long value)
        {
            return new PolicyUnit(value, PolicyUnitType.Extents);
        }
        
        public override string ToString()
        {
            if (this.type == PolicyUnitType.Percentage)
            {
                return SR.GetString(SR.PolicyUnit_Percent, this.value);
            }
            else
            {
                return SR.GetString(SR.PolicyUnit_Extents, this.value);
            }
        }
    }
}
