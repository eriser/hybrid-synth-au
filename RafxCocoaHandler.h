//
//  RafxCocoaHandler.h
//  RAFXResonantLPF_GUI
//
//  Created by Will Pirkle on 11/9/14.
//
//

#ifndef __RAFXResonantLPF_GUI__RafxCocoaHandler__
#define __RAFXResonantLPF_GUI__RafxCocoaHandler__
/// \cond ignore

// #include <iostream>
class CRafxCocoaHandler
{
public:
    CRafxCocoaHandler();
    ~CRafxCocoaHandler();
    
    void* m_pRafxVSTEditor;
    
    void open(void* pWnd);
};

#endif /* defined(__RAFXResonantLPF_GUI__RafxCocoaHandler__) */
