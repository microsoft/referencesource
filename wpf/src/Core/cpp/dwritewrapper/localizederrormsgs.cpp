#include "LocalizedErrorMsgs.h"

namespace MS { namespace Internal { namespace Text { namespace TextInterface
{
    System::String^ LocalizedErrorMsgs::EnumeratorNotStarted::get()
    {
    
        System::Threading::Monitor::Enter(_staticLockForLocalizedExceptionMsgs);
        try
        {
            return _localizedExceptionMsgEnumeratorNotStarted;
        }
        finally
        {
            System::Threading::Monitor::Exit(_staticLockForLocalizedExceptionMsgs);
        }
    }

    void LocalizedErrorMsgs::EnumeratorNotStarted::set(System::String^ msg)
    {
        System::Threading::Monitor::Enter(_staticLockForLocalizedExceptionMsgs);
        try
        {
            _localizedExceptionMsgEnumeratorNotStarted = msg;
        }
        finally
        {
            System::Threading::Monitor::Exit(_staticLockForLocalizedExceptionMsgs);
        }
    }    

    System::String^ LocalizedErrorMsgs::EnumeratorReachedEnd::get()
    {        
        System::Threading::Monitor::Enter(_staticLockForLocalizedExceptionMsgs);
        try
        {
            return _localizedExceptionMsgEnumeratorReachedEnd;
        }
        finally
        {
            System::Threading::Monitor::Exit(_staticLockForLocalizedExceptionMsgs);
        }
    }

    void LocalizedErrorMsgs::EnumeratorReachedEnd::set(System::String^ msg)
    {
        System::Threading::Monitor::Enter(_staticLockForLocalizedExceptionMsgs);
        try
        {
            _localizedExceptionMsgEnumeratorReachedEnd = msg;
        }
        finally
        {
            System::Threading::Monitor::Exit(_staticLockForLocalizedExceptionMsgs);
        }
    }

}}}}//MS::Internal::Text::TextInterface
