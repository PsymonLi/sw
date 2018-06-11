ProtoObject: l2segment_pb2
Service: L2Segment
enabled : True
dolEnabled : True
objects:
    - object:
        name : L2Segment
        key_handle : L2SegmentKeyHandle
        max_objects : 500
        ignore:
        ignore_v2:
        create:
            api      : L2SegmentCreate
            request  : L2SegmentRequestMsg
            response : L2SegmentResponseMsg
            pre_cb   : callback://l2segment/PreCreateCb
            post_cb  : None
        update:
            api      : L2SegmentUpdate
            request  : L2SegmentRequestMsg
            response : L2SegmentResponseMsg
            pre_cb   : callback://l2segment/PreUpdateCb
            post_cb  : None
        delete:
            api      : L2SegmentDelete
            request  : L2SegmentDeleteRequestMsg
            response : L2SegmentDeleteResponseMsg
            pre_cb   : None
            post_cb  : None
        get:
            api      : L2SegmentGet
            request  : L2SegmentGetRequestMsg
            response : L2SegmentGetResponseMsg
            pre_cb   : None
            post_cb  : None
