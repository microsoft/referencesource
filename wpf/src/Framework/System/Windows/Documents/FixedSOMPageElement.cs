/*++

    File: FixedSOMPageElement.cs
    
    Copyright (C) 2005 Microsoft Corporation. All rights reserved.                                                        
                                                                              
    Description:
       Abstract class that provides a common base class for all containers directly accessible from the page level

    History:
       05/17/2005: agurcan - Created
                
--*/

namespace System.Windows.Documents
{
    using System.Collections.Generic;
    using System.Diagnostics;
    
    internal abstract class FixedSOMPageElement :FixedSOMContainer
    {
        //--------------------------------------------------------------------
        //
        // Constructors
        //
        //---------------------------------------------------------------------
        
        #region Constructors
        public FixedSOMPageElement(FixedSOMPage page)
        {
            _page = page;
        }
        #endregion Constructors        

        //--------------------------------------------------------------------
        //
        // Public properties
        //
        //---------------------------------------------------------------------
        
        #region Public properties
        public FixedSOMPage FixedSOMPage
        {
            get
            {
                return _page;
            }
        }

        public abstract bool IsRTL
        {
            get;
        }
        #endregion Constructors        
        


        //--------------------------------------------------------------------
        //
        // Protected Fields
        //
        //---------------------------------------------------------------------

        #region Protected Fields
        protected FixedSOMPage _page;
        #endregion Protected Fields
        
    }
}

