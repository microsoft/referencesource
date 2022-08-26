#pragma once

// Wrapper for a Fusion Activation Context (ACTCTX)
class ActivationContext
{
    ActivationContext(const ActivationContext&);
    void operator =(const ActivationContext&);
public:
    ActivationContext() :
        m_hActCtx(INVALID_HANDLE_VALUE),
        m_actCtxCookie(0),
        m_fAutoDeactivate(true)
    {
    }

    ~ActivationContext()
    {
        if(m_hActCtx != INVALID_HANDLE_VALUE)
        {
            // If we don't deactivate the context, we can't release it either.
            if(m_fAutoDeactivate)
            {
                DeActivate();
                ReleaseActCtx(m_hActCtx);
            }
        }
    }

    bool Create(LPCWSTR manifestModulePath)
    {
        if (m_hActCtx != INVALID_HANDLE_VALUE || !manifestModulePath || !*manifestModulePath)
        {
            ASSERT(false);
            return false;
        }

        ACTCTXW actctx = { sizeof(actctx) };
        actctx.lpSource = manifestModulePath;
        actctx.lpResourceName = MAKEINTRESOURCEW(2);
        actctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;

        m_hActCtx = CreateActCtxW(&actctx);
        return m_hActCtx != INVALID_HANDLE_VALUE;
    }

    bool Activate(bool autoDeactivate = true)
    {
        if(m_hActCtx == INVALID_HANDLE_VALUE)
        {
            ASSERT(false);
            return false;
        }
        m_fAutoDeactivate = autoDeactivate;
        return ActivateActCtx(m_hActCtx, &m_actCtxCookie) != false;
    }

    bool DeActivate()
    {
        if(m_hActCtx == INVALID_HANDLE_VALUE || m_actCtxCookie == 0)
        {
            ASSERT(false);
            return false;
        }
        ULONG_PTR cookie = m_actCtxCookie;
        m_actCtxCookie = 0;
        return DeactivateActCtx( 0, cookie) != false;
    }
    
private:
    HANDLE m_hActCtx;
    ULONG_PTR m_actCtxCookie;
    bool m_fAutoDeactivate;
};
