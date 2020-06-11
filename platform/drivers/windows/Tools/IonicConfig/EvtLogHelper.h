#pragma once
#include <winevt.h>

#define EVTLOG_IONIC_QUERY     L"<QueryList> \
                                  <Query Path='System'> \
                                      <Select>*[System[Provider[@Name='Ionic']]]</Select> \
                                  </Query> \
                              </QueryList>"
#define  EVTLOG_IONICLBFO_QUERY L"<QueryList> \
                                  <Query Path='System'> \
                                      <Select>*[System[Provider[@Name='Ionic']]]</Select> \
                                  </Query> \
                                  <Query Path='System'> \
                                      <Select>*[System[Provider[@EventSourceName='MsLbfoProvider']]]</Select> \
                                  </Query> \
                              </QueryList>"

HRESULT EvtLogHelperGetEvtLogs(LPCWSTR Query, std::wostream& outfile);

