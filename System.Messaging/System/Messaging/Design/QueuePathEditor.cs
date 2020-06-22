//------------------------------------------------------------------------------
// <copyright file="QueuePathEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Design
{
    using System;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Messaging;

    /// <include file='doc\QueuePathEditor.uex' path='docs/doc[@for="QueuePathEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    // permissions demanded by parent class
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name = "FullTrust")]
    public class QueuePathEditor : UITypeEditor
    {

        /// <include file='doc\QueuePathEditor.uex' path='docs/doc[@for="QueuePathEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            if (provider != null)
            {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                if (edSvc != null)
                {
                    QueuePathDialog dialog = new QueuePathDialog(provider);
                    MessageQueue queue = null;
                    if (value is MessageQueue)
                        queue = (MessageQueue)value;
                    else if (value is string)
                        queue = new MessageQueue((string)value);
                    else if (value != null)
                        return value;

                    if (queue != null)
                        dialog.SelectQueue(queue);

                    IDesignerHost host = (IDesignerHost)provider.GetService(typeof(IDesignerHost));
                    DesignerTransaction trans = null;
                    if (host != null)
                        trans = host.CreateTransaction();

                    try
                    {
                        if ((context == null || context.OnComponentChanging()) && edSvc.ShowDialog(dialog) == DialogResult.OK)
                        {
                            if (dialog.Path != String.Empty)
                            {
                                if (context.Instance is MessageQueue || context.Instance is MessageQueueInstaller)
                                    value = dialog.Path;
                                else
                                {
                                    value = MessageQueueConverter.GetFromCache(dialog.Path);
                                    if (value == null)
                                    {
                                        value = new MessageQueue(dialog.Path);
                                        MessageQueueConverter.AddToCache((MessageQueue)value);
                                        if (context != null)
                                            context.Container.Add((IComponent)value);
                                    }
                                }

                                context.OnComponentChanged();
                            }
                        }
                    }
                    finally
                    {
                        if (trans != null)
                        {
                            trans.Commit();
                        }
                    }
                }
            }

            return value;
        }

        /// <include file='doc\QueuePathEditor.uex' path='docs/doc[@for="QueuePathEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }
    }
}

