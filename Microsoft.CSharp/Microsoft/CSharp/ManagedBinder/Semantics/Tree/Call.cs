// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRCALL : EXPR
    {
        private EXPR OptionalArguments;
        public EXPR GetOptionalArguments() { return OptionalArguments; }
        public void SetOptionalArguments(EXPR value) { OptionalArguments = value; }

        private EXPRMEMGRP MemberGroup;
        public EXPRMEMGRP GetMemberGroup() { return MemberGroup; }
        public void SetMemberGroup(EXPRMEMGRP value) { MemberGroup = value; }

        public MethWithInst mwi;

        public PREDEFMETH PredefinedMethod;

        public NullableCallLiftKind nubLiftKind;
        public EXPR pConversions;
        public EXPR castOfNonLiftedResultToLiftedType;
    }
}
