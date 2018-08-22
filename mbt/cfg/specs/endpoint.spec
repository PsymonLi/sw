ProtoObject: endpoint_pb2
Service: Endpoint
enabled : False
dolEnabled : False
objects:
    - object :
        name : Endpoint
        key_handle : EndpointKeyHandle
        ignore:
            - op : Get
            - op : Update
            - op : Delete
            - op : Create
        create:
            api      : EndpointCreate
            request  : EndpointRequestMsg
            response : EndpointResponseMsg
            pre_cb   : callback://endpoint/PreCreateCb
            post_cb  : callback://endpoint/PostCreateCb
        update:
            api      : EndpointUpdate
            request  : EndpointRequestMsg
            response : EndpointResponseMsg
            pre_cb   : None
            post_cb  : None
        delete:
            api      : EndpointDelete
            request  : EndpointDeleteRequestMsg
            response : EndpointDeleteResponseMsg
            pre_cb   : None
            post_cb  : None
        get:
            api      : EndpointGet
            request  : EndpointGetRequestMsg
            response : EndpointGetResponseMsg
            pre_cb   : None
            post_cb  : None
