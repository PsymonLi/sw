ProtoObject: nw_pb2
Service: Network
enabled: True
ignore:
    - op : Get
    - op : Update
create:
    api      : NetworkCreate
    request  : NetworkRequestMsg
    response : NetworkResponseMsg
    pre_cb   : None
    post_cb  : callback://network/PostCreateCb
update:
    api      : NetworkUpdate
    request  : NetworkRequestMsg
    response : NetworkResponseMsg
    pre_cb   : None
    post_cb  : callback://network/PostUpdateCb
delete:
    api      : NetworkDelete
    request  : NetworkDeleteRequestMsg
    response : NetworkDeleteResponseMsg
    pre_cb   : None
    post_cb  : None
get:
    api      : NetworkGet
    request  : NetworkGetRequestMsg
    response : NetworkGetResponseMsg
    pre_cb   : None
    post_cb  : callback://network/PostGetCb