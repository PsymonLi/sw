/*

CURRENT MODEL:

SIP:SPORT

S0: P1[DIP:DPORT]
S1: P2[P1:P2]
S2: P3; STAG:SPORT
S3: P1_1[DIP:DPORT]    R
S4: P2_1[P1:P2]
S5: P3_1[SIP:SPORT]
S6: SET1               R1
S7: SET2

S0: P1[DTAG:DPORT]
S1: P2[P1:P2]
S2: P3[STAG:SPORT]
S3: P1_1[DTAG:DPORT]   R
S4: P2_1[P1:P2]
S5: P3_1[SIP:SPORT]
S6: SET1               R1
S7: SET2

PROPOSED MODEL:

SIP:SPORT              DIP:DPORT

S0: P1[XXX:SPORT]      P2[XXX:DPORT]
S1: P1[XXX:SPORT]      P2[XXX:DPORT]
S2: P1[XXX:SPORT]      P2[XXX:DPORT]
S3: P1[XXX:SPORT]      P2[XXX:DPORT]
S4: P1[XXX:SPORT]      P2[XXX:DPORT]
S5: P1[XXX:SPORT]      P2[XXX:DPORT]
S6: NA                 NA                    S1[SIP:SPORT; DIP:DPORT]
S7: NA                 NA                    S2

S0: P3[P1:PX; RS]      NA
S1: P3[P1:PX; RS]      RS
S2: P3[P1:PX; RS]      RS
S3: P3[P1:PX; RS]      RS
S4: P3[P1:PX; RS]      RS
S5: P3[P1:PX; RS]      RS
S6: NA                 RS                    S1[P1:PX]
S7: NA                 NA                    S2

*/

action rfc_action_p1(pad,id50, id49, id48,
                         id47, id46, id45, id44, id43, id42, id41, id40,
                         id39, id38, id37, id36, id35, id34, id33, id32,
                         id31, id30, id29, id28, id27, id26, id25, id24,
                         id23, id22, id21, id20, id19, id18, id17, id16,
                         id15, id14, id13, id12, id11, id10, id09, id08,
                         id07, id06, id05, id04, id03, id02, id01, id00
                    )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the policy index to PHV
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV4) {
        modify_field(txdma_control.policy_index,             // Policy Index =
                     ((rx_to_tx_hdr.sacl_base_addr0 -        // (Policy base -
                       scratch_metadata.sacl_region_addr) /  // Region base)/
                       SACL_IPV4_BLOCK_SIZE));               // Block Size
    }

    /* Get the P1 classid by indexing into the classid array */
    modify_field(scratch_metadata.field10, (id00 >> ((txdma_control.rfc_index %
                 SACL_P1_ENTRIES_PER_CACHE_LINE)*SACL_P1_CLASSID_WIDTH)));

    // Write the P1 classid to PHV
    modify_field(txdma_control.rfc_p1_classid, scratch_metadata.field10);

    // Initialize the correct table base and index based on the recirc count
    if ((txdma_control.recirc_count & 0x1) == 1) {
        // P2 table index = DIP:DPORT
        modify_field(scratch_metadata.field20, (rx_to_tx_hdr.dip_classid0 <<
                                                SACL_PROTO_DPORT_CLASSID_WIDTH)|
                                                rx_to_tx_hdr.dport_classid0);
    } else {
        // P2 table index = DTAG:DPORT
        modify_field(scratch_metadata.field20, (txdma_control.dtag_classid <<
                                                SACL_PROTO_DPORT_CLASSID_WIDTH) |
                                                rx_to_tx_hdr.dport_classid0);
    }

    // Write P2 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P2 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,               // P2 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +              // Region Base +
                 SACL_P2_TABLE_OFFSET +                      // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P2_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));                    // Index Bytes
}

@pragma stage 0
@pragma hbm_table
@pragma raw_index_table
table rfc_p1 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p1;
    }
}

action rfc_action_p2(pad,id50, id49, id48,
                         id47, id46, id45, id44, id43, id42, id41, id40,
                         id39, id38, id37, id36, id35, id34, id33, id32,
                         id31, id30, id29, id28, id27, id26, id25, id24,
                         id23, id22, id21, id20, id19, id18, id17, id16,
                         id15, id14, id13, id12, id11, id10, id09, id08,
                         id07, id06, id05, id04, id03, id02, id01, id00
                    )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the policy index to PHV
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV6) {
        modify_field(txdma_control.policy_index,             // Policy Index =
                     ((rx_to_tx_hdr.sacl_base_addr0 -        // (Policy base -
                       scratch_metadata.sacl_region_addr) /  // Region base)/
                       SACL_IPV6_BLOCK_SIZE));               // Block Size
    }

    /* Get the P2 classid by indexing into the classid array */
    modify_field(scratch_metadata.field10, (id00 >> ((txdma_control.rfc_index %
                 SACL_P2_ENTRIES_PER_CACHE_LINE)*SACL_P2_CLASSID_WIDTH)));

    // P3 table index = P1:P2
    modify_field(scratch_metadata.field20, ((txdma_control.rfc_p1_classid <<
                                            SACL_P2_CLASSID_WIDTH) |
                                            scratch_metadata.field10));

    // Write P3 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P3 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,               // P3 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +              // Region Base +
                 SACL_P3_TABLE_OFFSET +                      // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P3_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));                    // Index Bytes
}

@pragma stage 1
@pragma hbm_table
@pragma raw_index_table
table rfc_p2 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p2;
    }
}

action rfc_action_p3(pad,id50, id49, id48,
                         id47, id46, id45, id44, id43, id42, id41, id40,
                         id39, id38, id37, id36, id35, id34, id33, id32,
                         id31, id30, id29, id28, id27, id26, id25, id24,
                         id23, id22, id21, id20, id19, id18, id17, id16,
                         id15, id14, id13, id12, id11, id10, id09, id08,
                         id07, id06, id05, id04, id03, id02, id01, id00
                    )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the counter hbm region address from table constant
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV4) {
        modify_field(txdma_control.sacl_cntr_regn_addr, \
                     scratch_metadata.sacl_cntr_regn_addr);
    }

    /* Get the P3 rule_index by indexing into the rule_index array */
    modify_field(scratch_metadata.field10,id00>>(txdma_control.rfc_index%
                 SACL_P3_ENTRIES_PER_CACHE_LINE)*SACL_P3_ENTRY_WIDTH);

    // Write rule id to the PHV
    modify_field(txdma_control.rule_index, scratch_metadata.field10);

    // Write result table lookup address to PHV
    modify_field(txdma_control.sacl_rslt_tbl_addr,      // RSLT Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +         // Region Base +
                 SACL_RSLT_TABLE_OFFSET +               // Table Base +
                 (((scratch_metadata.field10) /
                  SACL_RSLT_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));               // Index Bytes

    // P1 table index = STAG:SPORT
    modify_field(scratch_metadata.field20, ((txdma_control.stag_classid <<
                                            SACL_SPORT_CLASSID_WIDTH) |
                                            rx_to_tx_hdr.sport_classid0));
    // Write P1 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P1 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,          // P1 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +         // Region Base +
                 SACL_P1_TABLE_OFFSET +                 // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P1_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));               // Index Bytes
}

@pragma stage 2
@pragma hbm_table
@pragma raw_index_table
table rfc_p3 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p3;
    }
}

action sacl_action_result(alg_type31, stateful31, priority31, action31,
                          alg_type30, stateful30, priority30, action30,
                          alg_type29, stateful29, priority29, action29,
                          alg_type28, stateful28, priority28, action28,
                          alg_type27, stateful27, priority27, action27,
                          alg_type26, stateful26, priority26, action26,
                          alg_type25, stateful25, priority25, action25,
                          alg_type24, stateful24, priority24, action24,
                          alg_type23, stateful23, priority23, action23,
                          alg_type22, stateful22, priority22, action22,
                          alg_type21, stateful21, priority21, action21,
                          alg_type20, stateful20, priority20, action20,
                          alg_type19, stateful19, priority19, action19,
                          alg_type18, stateful18, priority18, action18,
                          alg_type17, stateful17, priority17, action17,
                          alg_type16, stateful16, priority16, action16,
                          alg_type15, stateful15, priority15, action15,
                          alg_type14, stateful14, priority14, action14,
                          alg_type13, stateful13, priority13, action13,
                          alg_type12, stateful12, priority12, action12,
                          alg_type11, stateful11, priority11, action11,
                          alg_type10, stateful10, priority10, action10,
                          alg_type09, stateful09, priority09, action09,
                          alg_type08, stateful08, priority08, action08,
                          alg_type07, stateful07, priority07, action07,
                          alg_type06, stateful06, priority06, action06,
                          alg_type05, stateful05, priority05, action05,
                          alg_type04, stateful04, priority04, action04,
                          alg_type03, stateful03, priority03, action03,
                          alg_type02, stateful02, priority02, action02,
                          alg_type01, stateful01, priority01, action01,
                          alg_type00, stateful00, priority00, action00
                         )
{
    modify_field(scratch_metadata.field4, alg_type31);
    modify_field(scratch_metadata.field1, stateful31);
    modify_field(scratch_metadata.field10, priority31);
    modify_field(scratch_metadata.field1, action31);
    modify_field(scratch_metadata.field4, alg_type30);
    modify_field(scratch_metadata.field1, stateful30);
    modify_field(scratch_metadata.field10, priority30);
    modify_field(scratch_metadata.field1, action30);
    modify_field(scratch_metadata.field4, alg_type29);
    modify_field(scratch_metadata.field1, stateful29);
    modify_field(scratch_metadata.field10, priority29);
    modify_field(scratch_metadata.field1, action29);
    modify_field(scratch_metadata.field4, alg_type28);
    modify_field(scratch_metadata.field1, stateful28);
    modify_field(scratch_metadata.field10, priority28);
    modify_field(scratch_metadata.field1, action28);
    modify_field(scratch_metadata.field4, alg_type27);
    modify_field(scratch_metadata.field1, stateful27);
    modify_field(scratch_metadata.field10, priority27);
    modify_field(scratch_metadata.field1, action27);
    modify_field(scratch_metadata.field4, alg_type26);
    modify_field(scratch_metadata.field1, stateful26);
    modify_field(scratch_metadata.field10, priority26);
    modify_field(scratch_metadata.field1, action26);
    modify_field(scratch_metadata.field4, alg_type25);
    modify_field(scratch_metadata.field1, stateful25);
    modify_field(scratch_metadata.field10, priority25);
    modify_field(scratch_metadata.field1, action25);
    modify_field(scratch_metadata.field4, alg_type24);
    modify_field(scratch_metadata.field1, stateful24);
    modify_field(scratch_metadata.field10, priority24);
    modify_field(scratch_metadata.field1, action24);
    modify_field(scratch_metadata.field4, alg_type23);
    modify_field(scratch_metadata.field1, stateful23);
    modify_field(scratch_metadata.field10, priority23);
    modify_field(scratch_metadata.field1, action23);
    modify_field(scratch_metadata.field4, alg_type22);
    modify_field(scratch_metadata.field1, stateful22);
    modify_field(scratch_metadata.field10, priority22);
    modify_field(scratch_metadata.field1, action22);
    modify_field(scratch_metadata.field4, alg_type21);
    modify_field(scratch_metadata.field1, stateful21);
    modify_field(scratch_metadata.field10, priority21);
    modify_field(scratch_metadata.field1, action21);
    modify_field(scratch_metadata.field4, alg_type20);
    modify_field(scratch_metadata.field1, stateful20);
    modify_field(scratch_metadata.field10, priority20);
    modify_field(scratch_metadata.field1, action20);
    modify_field(scratch_metadata.field4, alg_type19);
    modify_field(scratch_metadata.field1, stateful19);
    modify_field(scratch_metadata.field10, priority19);
    modify_field(scratch_metadata.field1, action19);
    modify_field(scratch_metadata.field4, alg_type18);
    modify_field(scratch_metadata.field1, stateful18);
    modify_field(scratch_metadata.field10, priority18);
    modify_field(scratch_metadata.field1, action18);
    modify_field(scratch_metadata.field4, alg_type17);
    modify_field(scratch_metadata.field1, stateful17);
    modify_field(scratch_metadata.field10, priority17);
    modify_field(scratch_metadata.field1, action17);
    modify_field(scratch_metadata.field4, alg_type16);
    modify_field(scratch_metadata.field1, stateful16);
    modify_field(scratch_metadata.field10, priority16);
    modify_field(scratch_metadata.field1, action16);
    modify_field(scratch_metadata.field4, alg_type15);
    modify_field(scratch_metadata.field1, stateful15);
    modify_field(scratch_metadata.field10, priority15);
    modify_field(scratch_metadata.field1, action15);
    modify_field(scratch_metadata.field4, alg_type14);
    modify_field(scratch_metadata.field1, stateful14);
    modify_field(scratch_metadata.field10, priority14);
    modify_field(scratch_metadata.field1, action14);
    modify_field(scratch_metadata.field4, alg_type13);
    modify_field(scratch_metadata.field1, stateful13);
    modify_field(scratch_metadata.field10, priority13);
    modify_field(scratch_metadata.field1, action13);
    modify_field(scratch_metadata.field4, alg_type12);
    modify_field(scratch_metadata.field1, stateful12);
    modify_field(scratch_metadata.field10, priority12);
    modify_field(scratch_metadata.field1, action12);
    modify_field(scratch_metadata.field4, alg_type11);
    modify_field(scratch_metadata.field1, stateful11);
    modify_field(scratch_metadata.field10, priority11);
    modify_field(scratch_metadata.field1, action11);
    modify_field(scratch_metadata.field4, alg_type10);
    modify_field(scratch_metadata.field1, stateful10);
    modify_field(scratch_metadata.field10, priority10);
    modify_field(scratch_metadata.field1, action10);
    modify_field(scratch_metadata.field4, alg_type09);
    modify_field(scratch_metadata.field1, stateful09);
    modify_field(scratch_metadata.field10, priority09);
    modify_field(scratch_metadata.field1, action09);
    modify_field(scratch_metadata.field4, alg_type08);
    modify_field(scratch_metadata.field1, stateful08);
    modify_field(scratch_metadata.field10, priority08);
    modify_field(scratch_metadata.field1, action08);
    modify_field(scratch_metadata.field4, alg_type07);
    modify_field(scratch_metadata.field1, stateful07);
    modify_field(scratch_metadata.field10, priority07);
    modify_field(scratch_metadata.field1, action07);
    modify_field(scratch_metadata.field4, alg_type06);
    modify_field(scratch_metadata.field1, stateful06);
    modify_field(scratch_metadata.field10, priority06);
    modify_field(scratch_metadata.field1, action06);
    modify_field(scratch_metadata.field4, alg_type05);
    modify_field(scratch_metadata.field1, stateful05);
    modify_field(scratch_metadata.field10, priority05);
    modify_field(scratch_metadata.field1, action05);
    modify_field(scratch_metadata.field4, alg_type04);
    modify_field(scratch_metadata.field1, stateful04);
    modify_field(scratch_metadata.field10, priority04);
    modify_field(scratch_metadata.field1, action04);
    modify_field(scratch_metadata.field4, alg_type03);
    modify_field(scratch_metadata.field1, stateful03);
    modify_field(scratch_metadata.field10, priority03);
    modify_field(scratch_metadata.field1, action03);
    modify_field(scratch_metadata.field4, alg_type02);
    modify_field(scratch_metadata.field1, stateful02);
    modify_field(scratch_metadata.field10, priority02);
    modify_field(scratch_metadata.field1, action02);
    modify_field(scratch_metadata.field4, alg_type01);
    modify_field(scratch_metadata.field1, stateful01);
    modify_field(scratch_metadata.field10, priority01);
    modify_field(scratch_metadata.field1, action01);
    modify_field(scratch_metadata.field4, alg_type00);
    modify_field(scratch_metadata.field1, stateful00);
    modify_field(scratch_metadata.field10, priority00);
    modify_field(scratch_metadata.field1, action00);

    // Write the counter hbm region address from table constant
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV6) {
        modify_field(txdma_control.sacl_cntr_regn_addr, \
                     scratch_metadata.sacl_cntr_regn_addr);
    }

    /* Get the result entry by indexing into the result array */
    modify_field(scratch_metadata.field4,alg_type00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.stateful,stateful00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.field10,priority00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.field1,action00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);

    // Is the device configured for 'any deny is deny' ?
    if ((scratch_metadata.flag == FW_ACTION_XPOSN_ANY_DENY) and
        (txdma_control.stag_count == 0) and
        (txdma_control.dtag_count == 0) and
        ((txdma_control.recirc_count & 0x1) == 1)) {
        // If so, is the current policy action a deny ?
        if (scratch_metadata.field1 == SACL_RSLT_ENTRY_ACTION_DENY) {
            // Is this a higher priority deny than before ?
            if ((txdma_to_p4e.sacl_drop != SACL_RSLT_ENTRY_ACTION_DENY) or
                (txdma_control.rule_priority > scratch_metadata.field10)) {
                // Then overwrite the result in the PHV with the current one
                modify_field(txdma_control.rule_priority, scratch_metadata.field10);
                modify_field(txdma_to_p4e.sacl_drop, scratch_metadata.field1);
                modify_field(txdma_to_p4e.sacl_alg_type, scratch_metadata.field4);
                modify_field(txdma_to_p4e.sacl_stateful, scratch_metadata.stateful);
                modify_field(txdma_to_p4e.sacl_root_num, txdma_control.root_count);
                modify_field(txdma_control.final_rule_index, txdma_control.rule_index);
                modify_field(txdma_control.final_policy_index, txdma_control.policy_index);
            }
        } else {
            // Is this a higher priority allow than before ?
            if ((txdma_to_p4e.sacl_drop != SACL_RSLT_ENTRY_ACTION_DENY) and
                (txdma_control.rule_priority > scratch_metadata.field10)) {
                // Then overwrite the result in the PHV with the current one
                modify_field(txdma_control.rule_priority, scratch_metadata.field10);
                modify_field(txdma_to_p4e.sacl_drop, scratch_metadata.field1);
                modify_field(txdma_to_p4e.sacl_alg_type, scratch_metadata.field4);
                modify_field(txdma_to_p4e.sacl_stateful, scratch_metadata.stateful);
                modify_field(txdma_to_p4e.sacl_root_num, txdma_control.root_count);
                modify_field(txdma_control.final_rule_index, txdma_control.rule_index);
                modify_field(txdma_control.final_policy_index, txdma_control.policy_index);
            }
        }
    } else {
        // If the priority of the current result is higher than whats in the PHV
        if (txdma_control.rule_priority > scratch_metadata.field10) {
            // Then overwrite the result in the PHV with the current one
            modify_field(txdma_control.rule_priority, scratch_metadata.field10);
            modify_field(txdma_to_p4e.sacl_drop, scratch_metadata.field1);
            modify_field(txdma_to_p4e.sacl_alg_type, scratch_metadata.field4);
            modify_field(txdma_to_p4e.sacl_stateful, scratch_metadata.stateful);
            modify_field(txdma_to_p4e.sacl_root_num, txdma_control.root_count);
            modify_field(txdma_control.final_rule_index, txdma_control.rule_index);
            modify_field(txdma_control.final_policy_index, txdma_control.policy_index);
        }
    }
}

@pragma stage 3
@pragma hbm_table
@pragma raw_index_table
table sacl_result {
    reads {
        txdma_control.sacl_rslt_tbl_addr : exact;
    }
    actions {
        sacl_action_result;
    }
}

action rfc_action_p1_1(pad,id50, id49, id48,
                           id47, id46, id45, id44, id43, id42, id41, id40,
                           id39, id38, id37, id36, id35, id34, id33, id32,
                           id31, id30, id29, id28, id27, id26, id25, id24,
                           id23, id22, id21, id20, id19, id18, id17, id16,
                           id15, id14, id13, id12, id11, id10, id09, id08,
                           id07, id06, id05, id04, id03, id02, id01, id00
                      )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the policy index to PHV
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV4) {
        modify_field(txdma_control.policy_index,             // Policy Index =
                     ((rx_to_tx_hdr.sacl_base_addr0 -        // (Policy base -
                       scratch_metadata.sacl_region_addr) /  // Region base)/
                       SACL_IPV4_BLOCK_SIZE));               // Block Size
    }

    /* Get the P1 classid by indexing into the classid array */
    modify_field(scratch_metadata.field10, (id00 >> ((txdma_control.rfc_index %
                 SACL_P1_ENTRIES_PER_CACHE_LINE)*SACL_P1_CLASSID_WIDTH)));

    // Write the P1 classid to PHV
    modify_field(txdma_control.rfc_p1_classid, scratch_metadata.field10);

    // Initialize the correct table base and index based on the recirc count
    if ((txdma_control.recirc_count & 0x1) == 1) {
        // P2 table index = DIP:DPORT
        modify_field(scratch_metadata.field20, (rx_to_tx_hdr.dip_classid0 <<
                                                SACL_PROTO_DPORT_CLASSID_WIDTH)|
                                                rx_to_tx_hdr.dport_classid0);
    } else {
        // P2 table index = DTAG:DPORT
        modify_field(scratch_metadata.field20, (txdma_control.dtag_classid <<
                                                SACL_PROTO_DPORT_CLASSID_WIDTH) |
                                                rx_to_tx_hdr.dport_classid0);
    }

    // Write P2 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P2 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,              // P2 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +             // Region Base +
                 SACL_P2_TABLE_OFFSET +                     // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P2_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));                   // Index Bytes
}

@pragma stage 3
@pragma hbm_table
@pragma raw_index_table
table rfc_p1_1 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p1_1;
    }
}

action rfc_action_p2_1(pad,id50, id49, id48,
                           id47, id46, id45, id44, id43, id42, id41, id40,
                           id39, id38, id37, id36, id35, id34, id33, id32,
                           id31, id30, id29, id28, id27, id26, id25, id24,
                           id23, id22, id21, id20, id19, id18, id17, id16,
                           id15, id14, id13, id12, id11, id10, id09, id08,
                           id07, id06, id05, id04, id03, id02, id01, id00
                      )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the policy index to PHV
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV6) {
        modify_field(txdma_control.policy_index,             // Policy Index =
                     ((rx_to_tx_hdr.sacl_base_addr0 -        // (Policy base -
                       scratch_metadata.sacl_region_addr) /  // Region base)/
                       SACL_IPV6_BLOCK_SIZE));               // Block Size
    }

    /* Get the P2 classid by indexing into the classid array */
    modify_field(scratch_metadata.field10, (id00 >> ((txdma_control.rfc_index %
                 SACL_P2_ENTRIES_PER_CACHE_LINE)*SACL_P2_CLASSID_WIDTH)));

    // P3 table index = P1:P2
    modify_field(scratch_metadata.field20, ((txdma_control.rfc_p1_classid <<
                                            SACL_P2_CLASSID_WIDTH) |
                                            scratch_metadata.field10));

    // Write P3 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P3 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,              // P3 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +             // Region Base +
                 SACL_P3_TABLE_OFFSET +                     // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P3_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));                   // Index Bytes
}

@pragma stage 4
@pragma hbm_table
@pragma raw_index_table
table rfc_p2_1 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p2_1;
    }
}

action rfc_action_p3_1(pad,id50, id49, id48,
                           id47, id46, id45, id44, id43, id42, id41, id40,
                           id39, id38, id37, id36, id35, id34, id33, id32,
                           id31, id30, id29, id28, id27, id26, id25, id24,
                           id23, id22, id21, id20, id19, id18, id17, id16,
                           id15, id14, id13, id12, id11, id10, id09, id08,
                           id07, id06, id05, id04, id03, id02, id01, id00
                      )
{
    modify_field(scratch_metadata.field10, id00);
    modify_field(scratch_metadata.field10, id01);
    modify_field(scratch_metadata.field10, id02);
    modify_field(scratch_metadata.field10, id03);
    modify_field(scratch_metadata.field10, id04);
    modify_field(scratch_metadata.field10, id05);
    modify_field(scratch_metadata.field10, id06);
    modify_field(scratch_metadata.field10, id07);
    modify_field(scratch_metadata.field10, id08);
    modify_field(scratch_metadata.field10, id09);
    modify_field(scratch_metadata.field10, id10);
    modify_field(scratch_metadata.field10, id11);
    modify_field(scratch_metadata.field10, id12);
    modify_field(scratch_metadata.field10, id13);
    modify_field(scratch_metadata.field10, id14);
    modify_field(scratch_metadata.field10, id15);
    modify_field(scratch_metadata.field10, id16);
    modify_field(scratch_metadata.field10, id17);
    modify_field(scratch_metadata.field10, id18);
    modify_field(scratch_metadata.field10, id19);
    modify_field(scratch_metadata.field10, id20);
    modify_field(scratch_metadata.field10, id21);
    modify_field(scratch_metadata.field10, id22);
    modify_field(scratch_metadata.field10, id23);
    modify_field(scratch_metadata.field10, id24);
    modify_field(scratch_metadata.field10, id25);
    modify_field(scratch_metadata.field10, id26);
    modify_field(scratch_metadata.field10, id27);
    modify_field(scratch_metadata.field10, id28);
    modify_field(scratch_metadata.field10, id29);
    modify_field(scratch_metadata.field10, id30);
    modify_field(scratch_metadata.field10, id31);
    modify_field(scratch_metadata.field10, id32);
    modify_field(scratch_metadata.field10, id33);
    modify_field(scratch_metadata.field10, id34);
    modify_field(scratch_metadata.field10, id35);
    modify_field(scratch_metadata.field10, id36);
    modify_field(scratch_metadata.field10, id37);
    modify_field(scratch_metadata.field10, id38);
    modify_field(scratch_metadata.field10, id39);
    modify_field(scratch_metadata.field10, id40);
    modify_field(scratch_metadata.field10, id41);
    modify_field(scratch_metadata.field10, id42);
    modify_field(scratch_metadata.field10, id43);
    modify_field(scratch_metadata.field10, id44);
    modify_field(scratch_metadata.field10, id45);
    modify_field(scratch_metadata.field10, id46);
    modify_field(scratch_metadata.field10, id47);
    modify_field(scratch_metadata.field10, id48);
    modify_field(scratch_metadata.field10, id49);
    modify_field(scratch_metadata.field10, id50);
    modify_field(scratch_metadata.field2,  pad);

    // Write the counter hbm region address from table constant
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV6) {
        modify_field(txdma_control.sacl_cntr_regn_addr, \
                     scratch_metadata.sacl_cntr_regn_addr);
    }

    /* Get the P3 rule_index by indexing into the rule_index array */
    modify_field(scratch_metadata.field10,id00>>(txdma_control.rfc_index%
                 SACL_P3_ENTRIES_PER_CACHE_LINE)*SACL_P3_ENTRY_WIDTH);

    // Write rule id to the PHV
    modify_field(txdma_control.rule_index, scratch_metadata.field10);

    // Write result table lookup address to PHV
    modify_field(txdma_control.sacl_rslt_tbl_addr,      // RSLT Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +         // Region Base +
                 SACL_RSLT_TABLE_OFFSET +               // Table Base +
                 (((scratch_metadata.field10) /
                  SACL_RSLT_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));               // Index Bytes

    // P1 table index = SIP:SPORT
    modify_field(scratch_metadata.field20, ((rx_to_tx_hdr.sip_classid0 <<
                                             SACL_SPORT_CLASSID_WIDTH) |
                                            rx_to_tx_hdr.sport_classid0));
    // Write P1 table index to PHV
    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

    // Write P1 table lookup address to PHV
    modify_field(txdma_control.rfc_table_addr,      // P1 Lookup Addr =
                 rx_to_tx_hdr.sacl_base_addr0 +     // Region Base +
                 SACL_P1_TABLE_OFFSET +             // Table Base +
                 (((scratch_metadata.field20) /
                  SACL_P1_ENTRIES_PER_CACHE_LINE) *
                  SACL_CACHE_LINE_SIZE));           // Index Bytes
}

@pragma stage 5
@pragma hbm_table
@pragma raw_index_table
table rfc_p3_1 {
    reads {
        txdma_control.rfc_table_addr : exact;
    }
    actions {
        rfc_action_p3_1;
    }
}

action sacl_action_result_1(alg_type31, stateful31, priority31, action31,
                            alg_type30, stateful30, priority30, action30,
                            alg_type29, stateful29, priority29, action29,
                            alg_type28, stateful28, priority28, action28,
                            alg_type27, stateful27, priority27, action27,
                            alg_type26, stateful26, priority26, action26,
                            alg_type25, stateful25, priority25, action25,
                            alg_type24, stateful24, priority24, action24,
                            alg_type23, stateful23, priority23, action23,
                            alg_type22, stateful22, priority22, action22,
                            alg_type21, stateful21, priority21, action21,
                            alg_type20, stateful20, priority20, action20,
                            alg_type19, stateful19, priority19, action19,
                            alg_type18, stateful18, priority18, action18,
                            alg_type17, stateful17, priority17, action17,
                            alg_type16, stateful16, priority16, action16,
                            alg_type15, stateful15, priority15, action15,
                            alg_type14, stateful14, priority14, action14,
                            alg_type13, stateful13, priority13, action13,
                            alg_type12, stateful12, priority12, action12,
                            alg_type11, stateful11, priority11, action11,
                            alg_type10, stateful10, priority10, action10,
                            alg_type09, stateful09, priority09, action09,
                            alg_type08, stateful08, priority08, action08,
                            alg_type07, stateful07, priority07, action07,
                            alg_type06, stateful06, priority06, action06,
                            alg_type05, stateful05, priority05, action05,
                            alg_type04, stateful04, priority04, action04,
                            alg_type03, stateful03, priority03, action03,
                            alg_type02, stateful02, priority02, action02,
                            alg_type01, stateful01, priority01, action01,
                            alg_type00, stateful00, priority00, action00
                           )
{
    modify_field(scratch_metadata.field4, alg_type31);
    modify_field(scratch_metadata.field1, stateful31);
    modify_field(scratch_metadata.field10, priority31);
    modify_field(scratch_metadata.field1, action31);
    modify_field(scratch_metadata.field4, alg_type30);
    modify_field(scratch_metadata.field1, stateful30);
    modify_field(scratch_metadata.field10, priority30);
    modify_field(scratch_metadata.field1, action30);
    modify_field(scratch_metadata.field4, alg_type29);
    modify_field(scratch_metadata.field1, stateful29);
    modify_field(scratch_metadata.field10, priority29);
    modify_field(scratch_metadata.field1, action29);
    modify_field(scratch_metadata.field4, alg_type28);
    modify_field(scratch_metadata.field1, stateful28);
    modify_field(scratch_metadata.field10, priority28);
    modify_field(scratch_metadata.field1, action28);
    modify_field(scratch_metadata.field4, alg_type27);
    modify_field(scratch_metadata.field1, stateful27);
    modify_field(scratch_metadata.field10, priority27);
    modify_field(scratch_metadata.field1, action27);
    modify_field(scratch_metadata.field4, alg_type26);
    modify_field(scratch_metadata.field1, stateful26);
    modify_field(scratch_metadata.field10, priority26);
    modify_field(scratch_metadata.field1, action26);
    modify_field(scratch_metadata.field4, alg_type25);
    modify_field(scratch_metadata.field1, stateful25);
    modify_field(scratch_metadata.field10, priority25);
    modify_field(scratch_metadata.field1, action25);
    modify_field(scratch_metadata.field4, alg_type24);
    modify_field(scratch_metadata.field1, stateful24);
    modify_field(scratch_metadata.field10, priority24);
    modify_field(scratch_metadata.field1, action24);
    modify_field(scratch_metadata.field4, alg_type23);
    modify_field(scratch_metadata.field1, stateful23);
    modify_field(scratch_metadata.field10, priority23);
    modify_field(scratch_metadata.field1, action23);
    modify_field(scratch_metadata.field4, alg_type22);
    modify_field(scratch_metadata.field1, stateful22);
    modify_field(scratch_metadata.field10, priority22);
    modify_field(scratch_metadata.field1, action22);
    modify_field(scratch_metadata.field4, alg_type21);
    modify_field(scratch_metadata.field1, stateful21);
    modify_field(scratch_metadata.field10, priority21);
    modify_field(scratch_metadata.field1, action21);
    modify_field(scratch_metadata.field4, alg_type20);
    modify_field(scratch_metadata.field1, stateful20);
    modify_field(scratch_metadata.field10, priority20);
    modify_field(scratch_metadata.field1, action20);
    modify_field(scratch_metadata.field4, alg_type19);
    modify_field(scratch_metadata.field1, stateful19);
    modify_field(scratch_metadata.field10, priority19);
    modify_field(scratch_metadata.field1, action19);
    modify_field(scratch_metadata.field4, alg_type18);
    modify_field(scratch_metadata.field1, stateful18);
    modify_field(scratch_metadata.field10, priority18);
    modify_field(scratch_metadata.field1, action18);
    modify_field(scratch_metadata.field4, alg_type17);
    modify_field(scratch_metadata.field1, stateful17);
    modify_field(scratch_metadata.field10, priority17);
    modify_field(scratch_metadata.field1, action17);
    modify_field(scratch_metadata.field4, alg_type16);
    modify_field(scratch_metadata.field1, stateful16);
    modify_field(scratch_metadata.field10, priority16);
    modify_field(scratch_metadata.field1, action16);
    modify_field(scratch_metadata.field4, alg_type15);
    modify_field(scratch_metadata.field1, stateful15);
    modify_field(scratch_metadata.field10, priority15);
    modify_field(scratch_metadata.field1, action15);
    modify_field(scratch_metadata.field4, alg_type14);
    modify_field(scratch_metadata.field1, stateful14);
    modify_field(scratch_metadata.field10, priority14);
    modify_field(scratch_metadata.field1, action14);
    modify_field(scratch_metadata.field4, alg_type13);
    modify_field(scratch_metadata.field1, stateful13);
    modify_field(scratch_metadata.field10, priority13);
    modify_field(scratch_metadata.field1, action13);
    modify_field(scratch_metadata.field4, alg_type12);
    modify_field(scratch_metadata.field1, stateful12);
    modify_field(scratch_metadata.field10, priority12);
    modify_field(scratch_metadata.field1, action12);
    modify_field(scratch_metadata.field4, alg_type11);
    modify_field(scratch_metadata.field1, stateful11);
    modify_field(scratch_metadata.field10, priority11);
    modify_field(scratch_metadata.field1, action11);
    modify_field(scratch_metadata.field4, alg_type10);
    modify_field(scratch_metadata.field1, stateful10);
    modify_field(scratch_metadata.field10, priority10);
    modify_field(scratch_metadata.field1, action10);
    modify_field(scratch_metadata.field4, alg_type09);
    modify_field(scratch_metadata.field1, stateful09);
    modify_field(scratch_metadata.field10, priority09);
    modify_field(scratch_metadata.field1, action09);
    modify_field(scratch_metadata.field4, alg_type08);
    modify_field(scratch_metadata.field1, stateful08);
    modify_field(scratch_metadata.field10, priority08);
    modify_field(scratch_metadata.field1, action08);
    modify_field(scratch_metadata.field4, alg_type07);
    modify_field(scratch_metadata.field1, stateful07);
    modify_field(scratch_metadata.field10, priority07);
    modify_field(scratch_metadata.field1, action07);
    modify_field(scratch_metadata.field4, alg_type06);
    modify_field(scratch_metadata.field1, stateful06);
    modify_field(scratch_metadata.field10, priority06);
    modify_field(scratch_metadata.field1, action06);
    modify_field(scratch_metadata.field4, alg_type05);
    modify_field(scratch_metadata.field1, stateful05);
    modify_field(scratch_metadata.field10, priority05);
    modify_field(scratch_metadata.field1, action05);
    modify_field(scratch_metadata.field4, alg_type04);
    modify_field(scratch_metadata.field1, stateful04);
    modify_field(scratch_metadata.field10, priority04);
    modify_field(scratch_metadata.field1, action04);
    modify_field(scratch_metadata.field4, alg_type03);
    modify_field(scratch_metadata.field1, stateful03);
    modify_field(scratch_metadata.field10, priority03);
    modify_field(scratch_metadata.field1, action03);
    modify_field(scratch_metadata.field4, alg_type02);
    modify_field(scratch_metadata.field1, stateful02);
    modify_field(scratch_metadata.field10, priority02);
    modify_field(scratch_metadata.field1, action02);
    modify_field(scratch_metadata.field4, alg_type01);
    modify_field(scratch_metadata.field1, stateful01);
    modify_field(scratch_metadata.field10, priority01);
    modify_field(scratch_metadata.field1, action01);
    modify_field(scratch_metadata.field4, alg_type00);
    modify_field(scratch_metadata.field1, stateful00);
    modify_field(scratch_metadata.field10, priority00);
    modify_field(scratch_metadata.field1, action00);

    // Write the counter hbm region address from table constant
    if (rx_to_tx_hdr.iptype == IPTYPE_IPV6) {
        modify_field(txdma_control.sacl_cntr_regn_addr, \
                     scratch_metadata.sacl_cntr_regn_addr);
    }

    /* Get the result entry by indexing into the result array */
    modify_field(scratch_metadata.field4,alg_type00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.stateful,stateful00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.field10,priority00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);
    modify_field(scratch_metadata.field1,action00>>(txdma_control.rule_index%
                 SACL_RSLT_ENTRIES_PER_CACHE_LINE)*SACL_RSLT_ENTRY_WIDTH);

    // If the priority of the current result is higher than whats in the PHV
    if (txdma_control.rule_priority > scratch_metadata.field10) {
        // Then overwrite the result in the PHV with the current one
        modify_field(txdma_control.rule_priority, scratch_metadata.field10);
        modify_field(txdma_to_p4e.sacl_drop, scratch_metadata.field1);
        modify_field(txdma_to_p4e.sacl_alg_type, scratch_metadata.field4);
        modify_field(txdma_to_p4e.sacl_stateful, scratch_metadata.stateful);
        modify_field(txdma_to_p4e.sacl_root_num, txdma_control.root_count);
        modify_field(txdma_control.final_rule_index, txdma_control.rule_index);
        modify_field(txdma_control.final_policy_index, txdma_control.policy_index);
    }
}

@pragma stage 6
@pragma hbm_table
@pragma raw_index_table
table sacl_result_1 {
    reads {
        txdma_control.sacl_rslt_tbl_addr : exact;
    }
    actions {
        sacl_action_result_1;
    }
}

action setup_rfc1()
{
    // No Op. This being here helps key maker. Remove at your own peril!
    modify_field(scratch_metadata.field10, rx_to_tx_hdr.dtag0_classid0);

    if ((txdma_control.recirc_count & 0x1) == 0) {
        // Done for combination. Initialize for the next combination.

        // Find the next STAG
        if (txdma_control.stag_count == 0) {
            modify_field(scratch_metadata.field10, rx_to_tx_hdr.stag1_classid0);
        } else {
        if (txdma_control.stag_count == 1) {
            modify_field(scratch_metadata.field10, rx_to_tx_hdr.stag2_classid0);
        } else {
        if (txdma_control.stag_count == 2) {
            modify_field(scratch_metadata.field10, rx_to_tx_hdr.stag3_classid0);
        } else {
        if (txdma_control.stag_count == 3) {
            modify_field(scratch_metadata.field10, rx_to_tx_hdr.stag4_classid0);
        } else {
            modify_field(scratch_metadata.field10, SACL_CLASSID_DEFAULT);
        }}}}

        // If the next STAG is invalid
        if (scratch_metadata.field10 == SACL_CLASSID_DEFAULT) {
            // Find the next DTAG
            if (txdma_control.dtag_count == 0) {
                modify_field(scratch_metadata.field10, rx_to_tx_hdr.dtag1_classid0);
            } else {
            if (txdma_control.dtag_count == 1) {
                modify_field(scratch_metadata.field10, rx_to_tx_hdr.dtag2_classid0);
            } else {
            if (txdma_control.dtag_count == 2) {
                modify_field(scratch_metadata.field10, rx_to_tx_hdr.dtag3_classid0);
            } else {
            if (txdma_control.dtag_count == 3) {
                modify_field(scratch_metadata.field10, rx_to_tx_hdr.dtag4_classid0);
            } else {
                modify_field(scratch_metadata.field10, SACL_CLASSID_DEFAULT);
            }}}}

            // If the next DTAG is invalid
            if (scratch_metadata.field10 == SACL_CLASSID_DEFAULT) {
                // Done for policy root. Initialize for the next policy root.

                // Reinitialized TAG classids to the first
                modify_field(txdma_control.stag_count, 0);
                modify_field(txdma_control.dtag_count, 0);

                // Load the new root and its class ids
                if (txdma_control.root_count == 0) {
                    modify_field(scratch_metadata.sip_classid, rx_to_tx_hdr.sip_classid1);
                    modify_field(scratch_metadata.dip_classid, rx_to_tx_hdr.dip_classid1);
                    modify_field(scratch_metadata.sport_classid, rx_to_tx_hdr.sport_classid1);
                    modify_field(scratch_metadata.dport_classid, rx_to_tx_hdr.dport_classid1);
                    modify_field(scratch_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr1);
                } else {
                if (txdma_control.root_count == 1) {
                    modify_field(scratch_metadata.sip_classid, rx_to_tx_hdr.sip_classid2);
                    modify_field(scratch_metadata.dip_classid, rx_to_tx_hdr.dip_classid2);
                    modify_field(scratch_metadata.sport_classid, rx_to_tx_hdr.sport_classid2);
                    modify_field(scratch_metadata.dport_classid, rx_to_tx_hdr.dport_classid2);
                    modify_field(scratch_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr2);
                } else {
                if (txdma_control.root_count == 2) {
                    modify_field(scratch_metadata.sip_classid, rx_to_tx_hdr.sip_classid3);
                    modify_field(scratch_metadata.dip_classid, rx_to_tx_hdr.dip_classid3);
                    modify_field(scratch_metadata.sport_classid, rx_to_tx_hdr.sport_classid3);
                    modify_field(scratch_metadata.dport_classid, rx_to_tx_hdr.dport_classid3);
                    modify_field(scratch_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr3);
                } else {
                if (txdma_control.root_count == 3) {
                    modify_field(scratch_metadata.sip_classid, rx_to_tx_hdr.sip_classid4);
                    modify_field(scratch_metadata.dip_classid, rx_to_tx_hdr.dip_classid4);
                    modify_field(scratch_metadata.sport_classid, rx_to_tx_hdr.sport_classid4);
                    modify_field(scratch_metadata.dport_classid, rx_to_tx_hdr.dport_classid4);
                    modify_field(scratch_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr4);
                } else {
                if (txdma_control.root_count == 4) {
                    modify_field(scratch_metadata.sip_classid, rx_to_tx_hdr.sip_classid5);
                    modify_field(scratch_metadata.dip_classid, rx_to_tx_hdr.dip_classid5);
                    modify_field(scratch_metadata.sport_classid, rx_to_tx_hdr.sport_classid5);
                    modify_field(scratch_metadata.dport_classid, rx_to_tx_hdr.dport_classid5);
                    modify_field(scratch_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr5);
                } else {
                    // No more RFC Lookups
                    modify_field(scratch_metadata.sip_classid, 0);
                    modify_field(scratch_metadata.dip_classid, 0);
                    modify_field(scratch_metadata.sport_classid, 0);
                    modify_field(scratch_metadata.dport_classid, 0);
                    modify_field(scratch_metadata.sacl_base_addr, 0);
                }}}}}

                // Update new root and class ids in PHV
                modify_field(rx_to_tx_hdr.sip_classid0, scratch_metadata.sip_classid);
                modify_field(rx_to_tx_hdr.dip_classid0, scratch_metadata.dip_classid);
                modify_field(rx_to_tx_hdr.sport_classid0, scratch_metadata.sport_classid);
                modify_field(rx_to_tx_hdr.dport_classid0, scratch_metadata.dport_classid);
                modify_field(rx_to_tx_hdr.sacl_base_addr0, scratch_metadata.sacl_base_addr);

                // If we have a valid new root
                if (scratch_metadata.sacl_base_addr != 0) {
                    // Increment root count and setup P1 lookup for the new root
                    modify_field(txdma_control.root_count, txdma_control.root_count + 1);

                    // P1 table index = SIP:SPORT
                    modify_field(scratch_metadata.field20, ((scratch_metadata.sip_classid <<
                                                             SACL_SPORT_CLASSID_WIDTH) |
                                                            scratch_metadata.sport_classid));
                    // Write P1 table index to PHV
                    modify_field(txdma_control.rfc_index, scratch_metadata.field20);

                    // Write P1 table lookup address to PHV
                    modify_field(txdma_control.rfc_table_addr,      // P1 Lookup Addr =
                                 scratch_metadata.sacl_base_addr +  // Region Base +
                                 SACL_P1_TABLE_OFFSET +             // Table Base +
                                 (((scratch_metadata.field20) /
                                  SACL_P1_ENTRIES_PER_CACHE_LINE) *
                                  SACL_CACHE_LINE_SIZE));           // Index Bytes
                } else {
                    // Advance root count to signal all roots done
                    modify_field(txdma_control.root_count, SACL_ROOT_COUNT_MAX);
                }

                // Signal that the root has changed.
                modify_field(txdma_control.root_change, TRUE);
            } else {
                // The next DTAG is valid. Start again with the first STAG.
                modify_field(txdma_control.stag_count, 0);
                modify_field(txdma_control.stag_classid, rx_to_tx_hdr.stag0_classid0);
                modify_field(txdma_control.dtag_count, txdma_control.dtag_count + 1);
                modify_field(txdma_control.dtag_classid, scratch_metadata.field10);
            }
        } else {
            // The next STAG is valid.
            modify_field(txdma_control.stag_count, txdma_control.stag_count + 1);
            modify_field(txdma_control.stag_classid, scratch_metadata.field10);
        }
    }
}

@pragma stage 6
table setup_rfc1 {
    actions {
        setup_rfc1;
    }
}

action setup_rfc2()
{
    // If a root change has been signalled
    if (txdma_control.root_change == TRUE) {
        if (txdma_control.root_count == 1) {
            modify_field(rx_to_tx_hdr.stag0_classid0, rx_to_tx_hdr.stag0_classid1);
            modify_field(rx_to_tx_hdr.stag1_classid0, rx_to_tx_hdr.stag1_classid1);
            modify_field(rx_to_tx_hdr.stag2_classid0, rx_to_tx_hdr.stag2_classid1);
            modify_field(rx_to_tx_hdr.stag3_classid0, rx_to_tx_hdr.stag3_classid1);
            modify_field(rx_to_tx_hdr.stag4_classid0, rx_to_tx_hdr.stag4_classid1);
            modify_field(rx_to_tx_hdr.dtag0_classid0, rx_to_tx_hdr.dtag0_classid1);
            modify_field(rx_to_tx_hdr.dtag1_classid0, rx_to_tx_hdr.dtag1_classid1);
            modify_field(rx_to_tx_hdr.dtag2_classid0, rx_to_tx_hdr.dtag2_classid1);
            modify_field(rx_to_tx_hdr.dtag3_classid0, rx_to_tx_hdr.dtag3_classid1);
            modify_field(rx_to_tx_hdr.dtag4_classid0, rx_to_tx_hdr.dtag4_classid1);
            modify_field(txdma_control.stag_classid,  rx_to_tx_hdr.stag0_classid1);
            modify_field(txdma_control.dtag_classid,  rx_to_tx_hdr.dtag0_classid1);
        } else {
        if (txdma_control.root_count == 2) {
            modify_field(rx_to_tx_hdr.stag0_classid0, rx_to_tx_hdr.stag0_classid2);
            modify_field(rx_to_tx_hdr.stag1_classid0, rx_to_tx_hdr.stag1_classid2);
            modify_field(rx_to_tx_hdr.stag2_classid0, rx_to_tx_hdr.stag2_classid2);
            modify_field(rx_to_tx_hdr.stag3_classid0, rx_to_tx_hdr.stag3_classid2);
            modify_field(rx_to_tx_hdr.stag4_classid0, rx_to_tx_hdr.stag4_classid2);
            modify_field(rx_to_tx_hdr.dtag0_classid0, rx_to_tx_hdr.dtag0_classid2);
            modify_field(rx_to_tx_hdr.dtag1_classid0, rx_to_tx_hdr.dtag1_classid2);
            modify_field(rx_to_tx_hdr.dtag2_classid0, rx_to_tx_hdr.dtag2_classid2);
            modify_field(rx_to_tx_hdr.dtag3_classid0, rx_to_tx_hdr.dtag3_classid2);
            modify_field(rx_to_tx_hdr.dtag4_classid0, rx_to_tx_hdr.dtag4_classid2);
            modify_field(txdma_control.stag_classid,  rx_to_tx_hdr.stag0_classid2);
            modify_field(txdma_control.dtag_classid,  rx_to_tx_hdr.dtag0_classid2);
        } else {
        if (txdma_control.root_count == 3) {
            modify_field(rx_to_tx_hdr.stag0_classid0, rx_to_tx_hdr.stag0_classid3);
            modify_field(rx_to_tx_hdr.stag1_classid0, rx_to_tx_hdr.stag1_classid3);
            modify_field(rx_to_tx_hdr.stag2_classid0, rx_to_tx_hdr.stag2_classid3);
            modify_field(rx_to_tx_hdr.stag3_classid0, rx_to_tx_hdr.stag3_classid3);
            modify_field(rx_to_tx_hdr.stag4_classid0, rx_to_tx_hdr.stag4_classid3);
            modify_field(rx_to_tx_hdr.dtag0_classid0, rx_to_tx_hdr.dtag0_classid3);
            modify_field(rx_to_tx_hdr.dtag1_classid0, rx_to_tx_hdr.dtag1_classid3);
            modify_field(rx_to_tx_hdr.dtag2_classid0, rx_to_tx_hdr.dtag2_classid3);
            modify_field(rx_to_tx_hdr.dtag3_classid0, rx_to_tx_hdr.dtag3_classid3);
            modify_field(rx_to_tx_hdr.dtag4_classid0, rx_to_tx_hdr.dtag4_classid3);
            modify_field(txdma_control.stag_classid,  rx_to_tx_hdr.stag0_classid3);
            modify_field(txdma_control.dtag_classid,  rx_to_tx_hdr.dtag0_classid3);
        } else {
        if (txdma_control.root_count == 4) {
            modify_field(rx_to_tx_hdr.stag0_classid0, rx_to_tx_hdr.stag0_classid4);
            modify_field(rx_to_tx_hdr.stag1_classid0, rx_to_tx_hdr.stag1_classid4);
            modify_field(rx_to_tx_hdr.stag2_classid0, rx_to_tx_hdr.stag2_classid4);
            modify_field(rx_to_tx_hdr.stag3_classid0, rx_to_tx_hdr.stag3_classid4);
            modify_field(rx_to_tx_hdr.stag4_classid0, rx_to_tx_hdr.stag4_classid4);
            modify_field(rx_to_tx_hdr.dtag0_classid0, rx_to_tx_hdr.dtag0_classid4);
            modify_field(rx_to_tx_hdr.dtag1_classid0, rx_to_tx_hdr.dtag1_classid4);
            modify_field(rx_to_tx_hdr.dtag2_classid0, rx_to_tx_hdr.dtag2_classid4);
            modify_field(rx_to_tx_hdr.dtag3_classid0, rx_to_tx_hdr.dtag3_classid4);
            modify_field(rx_to_tx_hdr.dtag4_classid0, rx_to_tx_hdr.dtag4_classid4);
            modify_field(txdma_control.stag_classid,  rx_to_tx_hdr.stag0_classid4);
            modify_field(txdma_control.dtag_classid,  rx_to_tx_hdr.dtag0_classid4);
        } else {
        if (txdma_control.root_count == 5) {
            modify_field(rx_to_tx_hdr.stag0_classid0, rx_to_tx_hdr.stag0_classid5);
            modify_field(rx_to_tx_hdr.stag1_classid0, rx_to_tx_hdr.stag1_classid5);
            modify_field(rx_to_tx_hdr.stag2_classid0, rx_to_tx_hdr.stag2_classid5);
            modify_field(rx_to_tx_hdr.stag3_classid0, rx_to_tx_hdr.stag3_classid5);
            modify_field(rx_to_tx_hdr.stag4_classid0, rx_to_tx_hdr.stag4_classid5);
            modify_field(rx_to_tx_hdr.dtag0_classid0, rx_to_tx_hdr.dtag0_classid5);
            modify_field(rx_to_tx_hdr.dtag1_classid0, rx_to_tx_hdr.dtag1_classid5);
            modify_field(rx_to_tx_hdr.dtag2_classid0, rx_to_tx_hdr.dtag2_classid5);
            modify_field(rx_to_tx_hdr.dtag3_classid0, rx_to_tx_hdr.dtag3_classid5);
            modify_field(rx_to_tx_hdr.dtag4_classid0, rx_to_tx_hdr.dtag4_classid5);
            modify_field(txdma_control.stag_classid,  rx_to_tx_hdr.stag0_classid5);
            modify_field(txdma_control.dtag_classid,  rx_to_tx_hdr.dtag0_classid5);
        } else {
            modify_field(rx_to_tx_hdr.stag0_classid0, 0);
            modify_field(rx_to_tx_hdr.stag1_classid0, 0);
            modify_field(rx_to_tx_hdr.stag2_classid0, 0);
            modify_field(rx_to_tx_hdr.stag3_classid0, 0);
            modify_field(rx_to_tx_hdr.stag4_classid0, 0);
            modify_field(rx_to_tx_hdr.dtag0_classid0, 0);
            modify_field(rx_to_tx_hdr.dtag1_classid0, 0);
            modify_field(rx_to_tx_hdr.dtag2_classid0, 0);
            modify_field(rx_to_tx_hdr.dtag3_classid0, 0);
            modify_field(rx_to_tx_hdr.dtag4_classid0, 0);
            modify_field(txdma_control.stag_classid,  0);
            modify_field(txdma_control.dtag_classid,  0);
            modify_field(txdma_predicate.rfc_enable, FALSE);
        }}}}}

        // Reset root changed
        modify_field(txdma_control.root_change, FALSE);
    }
}

@pragma stage 7
table setup_rfc2 {
    actions {
        setup_rfc2;
    }
}

control sacl_rfc {
    if (txdma_predicate.rfc_enable == TRUE) {
        apply(rfc_p1);
        apply(rfc_p2);
        apply(rfc_p3);
        apply(sacl_result);
        apply(rfc_p1_1);
        apply(rfc_p2_1);
        apply(rfc_p3_1);
        apply(sacl_result_1);

        apply(setup_rfc1);
        apply(setup_rfc2);
    }
}
