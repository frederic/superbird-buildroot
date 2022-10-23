#ifndef TX_UEVENT_CALLBACK_H
#define TX_UEVENT_CALLBACK_H

class TxUEventCallback {
public:
    virtual ~TxUEventCallback();
    virtual void onTxEvent(char *hpdState, int outputState) = 0;

protected:
    TxUEventCallback();
};

#endif
