//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Provides a stack based class for updating the currently compiled project
//
//-------------------------------------------------------------------------------------------------

#pragma once

class UpdateProjectBeingCompiled 
{
public:

    UpdateProjectBeingCompiled( 
        _In_ Compiler* pCompiler,
        _In_opt_ CompilerProject* pProject ) : m_pCompiler(pCompiler)
    {
        ThrowIfNull(pCompiler); 
        m_pPreviousProject = pCompiler->GetProjectBeingCompiled();
        pCompiler->SetProjectBeingCompiled(pProject);
    }

    ~UpdateProjectBeingCompiled()
    {
        m_pCompiler->SetProjectBeingCompiled(m_pPreviousProject);
    }

    CompilerProject* GetPrevious() const
    {
        return m_pPreviousProject;
    }

private:
    // Do not support copy or default initialization
    UpdateProjectBeingCompiled();
    UpdateProjectBeingCompiled(const UpdateProjectBeingCompiled&);
    UpdateProjectBeingCompiled& operator=(const UpdateProjectBeingCompiled&);

    Compiler* m_pCompiler;

    // Project previously being compiled.  Can be NULL
    CompilerProject* m_pPreviousProject;
};

