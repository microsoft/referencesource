#include "Win32Inc.hpp"

// Required to compile pure assembly : From http://msdn.microsoft.com/en-us/library/ms384352(VS.71).aspx
#ifdef __cplusplus
extern "C" { 
#endif
int _fltused=1; 
#ifdef __cplusplus
}
#endif
