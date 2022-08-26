#ifndef __CONTROLTABLEINIT_H
#define __CONTROLTABLEINIT_H        

// This class is used to initialize Control_Table global variable.
private ref class ControlTableInit
{
    static Object^ _staticLock = gcnew Object();
    static bool _isInitialized = false;

public:
    static void Init();
};
#endif //__CONTROLTABLEINIT_H
