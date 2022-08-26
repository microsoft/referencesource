#ifndef __GLOBAL_INIT_H
#define __GLOBAL_INIT_H

// This class is used to initialize the global arrays below.
private ref class GlobalInit
{
    static Object^ _staticLock = gcnew Object();
    static bool _isInitialized = false;

public:
    static void Init();
};
#endif // __GLOBAL_INIT_H
