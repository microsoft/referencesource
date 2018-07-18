// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Dynamic;

namespace Microsoft.CSharp.RuntimeBinder
{
    internal sealed class CSharpInvokeConstructorBinder : DynamicMetaObjectBinder, ICSharpInvokeOrInvokeMemberBinder
    {
        public CSharpCallFlags Flags { get { return m_flags; } }
        private CSharpCallFlags m_flags;

        public Type CallingContext { get { return m_callingContext; } }
        private Type m_callingContext;

        public IList<CSharpArgumentInfo> ArgumentInfo { get { return m_argumentInfo.AsReadOnly(); } }
        private List<CSharpArgumentInfo> m_argumentInfo;
        
        public bool StaticCall { get { return true; } }
        public IList<Type> TypeArguments { get { return new Type[0]; } }
        public string Name { get { return ".ctor"; } }

        bool ICSharpInvokeOrInvokeMemberBinder.ResultDiscarded { get { return false; } }

        private RuntimeBinder m_binder;

        public CSharpInvokeConstructorBinder(
            CSharpCallFlags flags,
            Type callingContext,
            IEnumerable<CSharpArgumentInfo> argumentInfo)
        {
            m_flags = flags;
            m_callingContext = callingContext;
            m_argumentInfo = BinderHelper.ToList(argumentInfo);
            m_binder = RuntimeBinder.GetInstance();
        }
        
        public sealed override DynamicMetaObject Bind(DynamicMetaObject target, DynamicMetaObject[] args)
        {
            return BinderHelper.Bind(this, m_binder, BinderHelper.Cons(target, args), m_argumentInfo, null);
        }
    }
}
