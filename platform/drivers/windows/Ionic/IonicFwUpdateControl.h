#ifndef _IONICFWUPDATECONTROL_H_
#define _IONICFWUPDATECONTROL_H_

typedef struct ionic* PIONIC_ADAPTER;

NDIS_STATUS FwCtrlDevRegister(PIONIC_ADAPTER Adapter);

VOID FwCtrlDevUnregister(PIONIC_ADAPTER Adapter);

#endif

