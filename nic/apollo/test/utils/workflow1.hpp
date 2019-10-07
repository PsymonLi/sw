//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all workflows
///
//----------------------------------------------------------------------------

#ifndef __TEST_UTILS_WORKFLOW1_HPP__
#define __TEST_UTILS_WORKFLOW1_HPP__

#include "nic/apollo/test/utils/api_base.hpp"
#include "nic/apollo/test/utils/batch.hpp"

namespace api_test {

/// \defgroup WORKFLOW Workflow
/// This group implements the workflows which will be called by test cases
/// for example vnic, vpc, subnet etc. This module invokes the feeder class
/// to generate the config objects and execute those on the hardware.
/// @{

/// \brief WF_TMP_1: [ Create SetMax ] - Read
/// workflow for objects for which delete's not supported. cleanup not done
/// on purpose as this will be the only testcase supported in the binary
/// this workflow will be deleted later
/// \anchor WF_TMP_1
template <typename feeder_T>
inline void workflow_tmp_1(feeder_T &feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_OK);
}

/// \brief WF_1: [ Create SetMax - Delete SetMax ] - Read
/// Create and delete max objects in the same batch.
/// The operation should be de-duped by framework and is
/// a NO-OP from hardware perspective
/// \anchor WF_1
template <typename feeder_T>
inline void workflow_1(feeder_T& feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    many_delete<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_2: [ Create SetMax - Delete SetMax - Create SetMax ] - Read
/// Create, delete max objects and create max objects again in the same batch
/// \anchor WF_2
template <typename feeder_T>
inline void workflow_2(feeder_T& feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    many_delete<feeder_T>(feeder);
    many_create<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_OK);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_3: [ Create Set1, Set2 - Delete Set1 - Create Set3 ] - Read
/// Create, delete some and create another set of nodes in the same batch
/// \anchor WF_3
template <typename feeder_T>
inline void workflow_3(feeder_T& feeder1, feeder_T& feeder2,
                       feeder_T& feeder3) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_create<feeder_T>(feeder2);
    many_delete<feeder_T>(feeder1);
    many_create<feeder_T>(feeder3);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder2);
    many_read<feeder_T>(feeder3);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder2);
    many_delete<feeder_T>(feeder3);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder2, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder3, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_4: [ Create SetMax ] - Read - [ Delete SetMax ] - Read
/// Create and delete mirror sessions in two batches.
/// The hardware should create and delete mirror sessions correctly.
/// Validate using reads at each batch end
/// \anchor WF_4
template <typename feeder_T>
inline void workflow_4(feeder_T& feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_5: [ Create Set1, Set2 ] - Read - [ Delete Set1 - Create Set3 ] - Read
/// Create and delete mix and match of mirror sessions in two batches
/// \anchor WF_5
template <typename feeder_T>
inline void workflow_5(feeder_T& feeder1, feeder_T& feeder2,
                       feeder_T& feeder3) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_create<feeder_T>(feeder2);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);
    many_read<feeder_T>(feeder2);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    many_create<feeder_T>(feeder3);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder2);
    many_read<feeder_T>(feeder3);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder2);
    many_delete<feeder_T>(feeder3);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder2, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder3, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_6: [ Create SetMax - Update SetMax - Update SetMax - Delete SetMax ] - Read
/// Create update, update and delete max mirror sessions in the same batch
/// NO-OP kind of result from hardware perspective
/// \anchor WF_6
template <typename feeder_T>
inline void workflow_6(feeder_T& feeder1, feeder_T& feeder1A,
                       feeder_T& feeder1B) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_update<feeder_T>(feeder1A);
    many_update<feeder_T>(feeder1B);
    many_delete<feeder_T>(feeder1B);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1B, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_7: [ Create SetMax - Delete SetMax - Create SetMax - Update SetMax -
///         Update SetMax ] - Read
/// Create delete create update and update max mirror sessions in the same batch
/// Last update should be retained
/// \anchor WF_7
template <typename feeder_T>
inline void workflow_7(feeder_T& feeder1, feeder_T& feeder1A,
                       feeder_T& feeder1B) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_delete<feeder_T>(feeder1);
    many_create<feeder_T>(feeder1);
    many_update<feeder_T>(feeder1A);
    many_update<feeder_T>(feeder1B);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1B);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1B);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1B, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_8: [ Create SetMax - Update SetMax ] - Read - [ Update SetMax ] - Read -
//       [ Delete SetMax ] - Read
/// Create update in a batch, update in a batch delete in a batch max mirror sessions
/// checking multiple updates, each in different batch
/// \anchor WF_8
template <typename feeder_T>
inline void workflow_8(feeder_T& feeder1, feeder_T& feeder1A,
                       feeder_T& feeder1B) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_update<feeder_T>(feeder1A);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1A);

    bctxt = batch_start();
    many_update<feeder_T>(feeder1B);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1B);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder1B);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1B, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_9: [ Create SetMax ] - Read - [ Update SetMax - Delete SetMax ] - Read
/// Create in a batch, update and delete in a batch
/// Delete checking after Update
/// \anchor WF_9
template <typename feeder_T>
inline void workflow_9(feeder_T& feeder1, feeder_T& feeder1A) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_update<feeder_T>(feeder1A);
    many_delete<feeder_T>(feeder1A);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1A, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_10: [ Create Set1, Set2, Set3 - Delete Set1 - Update Set2 ] - Read -
///        [ Update Set3 - Delete Set2 - Create Set4] - Read
/// Create and delete mix and match of mirror sessions in two batches
/// \anchor WF_10
template <typename feeder_T>
inline void workflow_10(feeder_T& feeder1, feeder_T& feeder2,
                        feeder_T& feeder2A, feeder_T& feeder3,
                        feeder_T& feeder3A, feeder_T& feeder4) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    many_create<feeder_T>(feeder2);
    many_create<feeder_T>(feeder3);
    many_delete<feeder_T>(feeder1);
    many_update<feeder_T>(feeder2A);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder2A);
    many_read<feeder_T>(feeder3);

    bctxt = batch_start();
    many_update<feeder_T>(feeder3A);
    many_delete<feeder_T>(feeder2A);
    many_create<feeder_T>(feeder4);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder3A);
    many_read<feeder_T>(feeder2, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder4);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder4);
    many_delete<feeder_T>(feeder3A);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder4, sdk::SDK_RET_ENTRY_NOT_FOUND);
    many_read<feeder_T>(feeder3A, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_1: [ Create SetMax ] - [ Create SetMax ] - Read
/// Create maximum number of mirror sessions in two batches
/// \anchor WF_N_1
template <typename feeder_T>
inline void workflow_neg_1(feeder_T& feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder);

    bctxt = batch_start();
    many_create<feeder_T>(feeder);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_2: [ Create SetMax+1 ] - Read
/// Create more than maximum number of mirror sessions supported.
/// \anchor WF_N_2
template <typename feeder_T>
inline void workflow_neg_2(feeder_T& feeder) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_3: Read NonEx - [ Delete NonExMax ] - [ Update NonExMax ] - Read
/// Read of a non-existing mirror session should return entry not found.
/// \anchor WF_N_3
template <typename feeder_T>
inline void workflow_neg_3(feeder_T& feeder) {
    // trigger
    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);

    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_delete<feeder_T>(feeder);
    batch_commit_fail(bctxt);

    // trigger
    bctxt = batch_start();
    many_update<feeder_T>(feeder);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_4: [ Create Set1 ] - Read - [ Delete Set1, Set2 ] - Read
/// Invalid batch shouldn't affect entries of previous batch
/// \anchor WF_N_4
template <typename feeder_T>
inline void workflow_neg_4(feeder_T& feeder1, feeder_T& feeder2) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    many_delete<feeder_T>(feeder2);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder1);
    many_read<feeder_T>(feeder2, sdk::SDK_RET_ENTRY_NOT_FOUND);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder2, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_5: [ Create SetMax ] - Read - [ Delete SetMax - Update SetMax ] - Read
/// Create max mirror sessions in a batch,
/// delete and update in a batch which fails resulting same old state as create.
/// \anchor WF_N_5
template <typename feeder_T>
inline void workflow_neg_5(feeder_T& feeder1, feeder_T& feeder1A) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    many_update<feeder_T>(feeder1A);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder1);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_6: [ Create SetMax ] - Read - [ Update SetMax + 1 ] - Read
/// Updation of more than max mirror sessions should fail leaving old state unchanged
/// \anchor WF_N_6
template <typename feeder_T>
inline void workflow_neg_6(feeder_T& feeder1, feeder_T& feeder1A) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_update<feeder_T>(feeder1A);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder1);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_7: [ Create Set1 ] - Read - [ Update Set1 - Update Set2 ] - Read
/// Create set1 in a batch, update set1 and set2 in next batch
/// fails leaving set1 unchanged
/// \anchor WF_N_7
template <typename feeder_T>
inline void workflow_neg_7(feeder_T& feeder1, feeder_T& feeder1A,
                           feeder_T& feeder2) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_update<feeder_T>(feeder1A);
    many_update<feeder_T>(feeder2);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder1);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// \brief WF_N_8: [ Create Set1 ] - Read - [ Delete Set1 - Update Set2 ] - Read
/// Create set1 in a batch, delete set1 and update set2 in next batch
/// fails leaving set1 unchanged
/// \anchor WF_N_8
template <typename feeder_T>
inline void workflow_neg_8(feeder_T& feeder1, feeder_T& feeder2) {
    // trigger
    pds_batch_ctxt_t bctxt = batch_start();
    many_create<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1);

    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    many_update<feeder_T>(feeder2);
    batch_commit_fail(bctxt);

    many_read<feeder_T>(feeder1);

    // cleanup
    bctxt = batch_start();
    many_delete<feeder_T>(feeder1);
    batch_commit(bctxt);

    many_read<feeder_T>(feeder1, sdk::SDK_RET_ENTRY_NOT_FOUND);
}

/// @}

}    // end namespace

#endif    // __TEST_UTILS_WORKFLOW1_HPP__
