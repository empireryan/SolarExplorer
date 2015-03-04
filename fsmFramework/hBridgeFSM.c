/**************************************************************************
*@File hBridgeFSM.C                                                      *
*@author Ryan Rodriguez                                                   *
*@date 12/27/14                                                           *
*@brief This module implements the hBridge state machine                 *
*This module is the software implementation of the hBridge state machine, *
*and is intended for research purposes at this moment.                    *
**************************************************************************/

#include "fsm.h"
#include "hBridgeFSM.h"
#include <stdio.h>
#include <ctype.h>
#include "stdbool.h"

///////////
//Events //
///////////

/* signals used by the hBridge FSM */
enum
{
    NEG_VDC,    //q = -1
    ZERO_VDC,   //q = 0
    VDC,        //q = 1
    NO_EVENT   //qDot = 0
};

/**
* Begin State Defintiions
*/

void hBridgeCtor(hBridge *self)
{
    _FsmCtor_(&self->super_, &hBridge_initial);
}

void hBridge_initial(hBridge *self, Event *e)
{
    /* ... initialization of hBridge attributes */
    printf("hBridge initialized");
    _FsmTran_((Fsm *)self, &hBridge_default);
}

void hBridge_default(hBridge *self, Event *e)
{
    switch (e->signal)
    {

        case NEG_VDC:
            printf("H-bridge to negVDC");
            _FsmTran_((Fsm *)self, &hBridge_negVDC);
            break;

        case ZERO_VDC:
            printf("H-bridge to zero");
            _FsmTran_((Fsm *)self, &hBridge_Zero);
            break;

        case VDC:
            printf("H-bridge to VDC");
            _FsmTran_((Fsm *)self, &hBridge_VDC);
            break;

        case NO_EVENT:
            printf("defaultNOEVENT");
            _FsmTran_((Fsm *)self, &hBridge_Zero);
            break;
    }
}


void hBridge_VDC(hBridge *self, Event *e)
{
    switch (e->signal)
    {
        case NEG_VDC:
            printf("H-bridge to negVDC");
            _FsmTran_((Fsm *)self, &hBridge_negVDC);
            break;

        case ZERO_VDC:
            printf("H-bridge to zero");
            _FsmTran_((Fsm *)self, &hBridge_Zero);
            break;

        default:
            printf("hBridge_VDC");
            _FsmTran_((Fsm *)self, &hBridge_VDC);
            break;
    }
}

void hBridge_Zero(hBridge *self, Event *e)
{
    switch (e->signal)
    {
        case VDC:
            printf("H-bridge to VDC");
            _FsmTran_((Fsm *)self, &hBridge_VDC);
            break;

        case NEG_VDC:
            printf("H-bridge to negVDC");
            _FsmTran_((Fsm *)self, &hBridge_negVDC);
            break;

        default:
            printf("hBridge_Zero");
            _FsmTran_((Fsm *)self, &hBridge_Zero);
            break;

    }
}

void hBridge_negVDC(hBridge *self, Event *e)
{
    switch (e->signal)
    {
        case VDC:
            printf("H-bridge to VDC");
            _FsmTran_((Fsm *)self, &hBridge_VDC);
            break;

        case ZERO_VDC:
            printf("H-bridge to zero");
            _FsmTran_((Fsm *)self, &hBridge_Zero);
            break;

        default:
            printf("hBridge_negVDC");
            _FsmTran_((Fsm *)self, &hBridge_negVDC);
            break;
    }
}
/**
* End State Definitions
*/

/**
* @brief Used to determine the event that should be passed to the FsmDispatch function
*
* Using the hBridgeEvent class, we utilize the data variable 'code' to switch the signal
* of the Event super-class. Next, we take the updated Event signal and dispatch it to
* the current state function pointed to by Fsm of the class hBridge.
*
*
* @param self [description]
* @param e    [description]
*/

char hBridgeTransitionFunction(hBridge self, hBridgeEvent *e)
{
    //After dereferencing, self is an hBridge object
    //
    //hBridgeEvent e is a wrapper structure containing members Char code, and Event super_.
    //  Code is just a data variable used to switch the signal attribute of the super_ Event.
    //  In our case, code should be a tuple of Il and Vc, used to switch the signal of the Event structure.
    //  The super_ Event is then passed to the dispatch function which calls the state function.
    //  The state function reads the signal and responds with the appropriate transition.

    //First, get the event pointed to by hBridge event
    //Next, get the signal pointed to by the event in hBridgeEvent

    void    *funptr = self.super_.state__;
    //void    *funptr = self->super_.state__;

    if(funptr == &hBridge_default){
        switch (e->code)                  //This switch uses the data attribute 'code' of the hBridge Event
        {
            case '.' : return -1;          // terminate the test
            case '+' : e->super_.signal = VDC; break;
            case '-' : e->super_.signal = NEG_VDC; break;
            case '0' : e->super_.signal = ZERO_VDC; break;

            default : e->super_.signal = NO_EVENT; break;
        }
    }
    else if(funptr == &hBridge_VDC){
        switch (e->code)                  //This switch uses the data attribute 'code' of the hBridge Event
        {
            case '.' : return -1;          // terminate the test
            case '+' : e->super_.signal = VDC; break;
            case '-' : e->super_.signal = NEG_VDC; break;
            case '0' : e->super_.signal = ZERO_VDC; break;

            default : e->super_.signal = NO_EVENT; break;
        }
    }
    else if(funptr == &hBridge_Zero){
        switch (e->code)                  //This switch uses the data attribute 'code' of the hBridge Event
        {
            case '.' : return -1;          // terminate the test
            case '+' : e->super_.signal = VDC; break;
            case '-' : e->super_.signal = NEG_VDC; break;
            case '0' : e->super_.signal = ZERO_VDC; break;

            default : e->super_.signal = NO_EVENT; break;
        }
    }
    else if(funptr == &hBridge_negVDC){
        switch (e->code)                  //This switch uses the data attribute 'code' of the hBridge Event
        {
            case '.' : return -1;          // terminate the test
            case '+' : e->super_.signal = VDC; break;
            case '-' : e->super_.signal = NEG_VDC; break;
            case '0' : e->super_.signal = ZERO_VDC; break;

            default : e->super_.signal = NO_EVENT; break;
        }
    }
    else
        ;

    return 0;
}

