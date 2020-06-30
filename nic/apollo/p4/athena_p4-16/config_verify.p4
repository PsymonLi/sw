control config_verify(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }

    @name(".config1_epoch_verify")
    action config1_epoch_verify(bit<16>  epoch) {
      if(metadata.cntrl.epoch1_value != epoch) {
	metadata.cntrl.flow_miss = TRUE;
      }
      /* Implementation to be decided */
      /*
      if(hdr.p4i_to_p4e_header.dnat_epoch != 0) {
	if (hdr.p4i_to_p4e_header.dnat_epoch > metadata.cntrl.epoch1_value) {
	    metadata.cntrl.flow_miss = TRUE;
      }
      */
    }

    @name(".config2_epoch_verify")
    action config2_epoch_verify(bit<16>  epoch) {
      if(metadata.cntrl.epoch2_value != epoch) {
	metadata.cntrl.flow_miss = TRUE;
      }
    }



    @capi_bitfields_struct
    @name(".config1") table config1 {
        key = {
            metadata.cntrl.epoch1_id        : exact;
        }
        actions = {
            config1_epoch_verify;
        }
        size = SESSION_TABLE_SIZE;
        placement = HBM;
        default_action = config1_epoch_verify;
        stage = 1;
    }

    @capi_bitfields_struct
    @name(".config2") table config2 {
        key = {
            metadata.cntrl.epoch2_id        : exact;
        }
        actions = {
            config2_epoch_verify;
        }
        size = SESSION_TABLE_SIZE;
        placement = HBM;
        default_action = config2_epoch_verify;
        stage = 1;
    }
    
   
    apply {
      if (metadata.cntrl.flow_miss == FALSE) {
	  if(metadata.cntrl.epoch1_id_valid == TRUE) {
	    config1.apply();
	  }
	  
	  if(metadata.cntrl.epoch2_id_valid == TRUE) {
            config2.apply();
	  }

      }
    }

}
