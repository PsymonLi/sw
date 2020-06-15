/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 */

/*
 * ionic_completion.c --
 *
 * Implement completion related functions 
 */

#include "ionic_log.h"
#include "ionic_completion.h"
#include "ionic_locks.h"
#include "ionic_utilities.h"
//static vmk_uint32 compl_counter = 0;

/*
 ******************************************************************************
 *
 * ionic_completion_create --
 *
 *    Create Completion and its resources
 *
 *  Parameters:
 *     module_id   - IN (module that init this completion)
 *     heap_id     - IN (heap to allocate init resources this completion)
 *     domain      - IN (domain for lock init for this completion)
 *     name        - IN (name for this completion)
 *     completion  - IN (completion struct to initialize)
 *
 *  Results:
 *     VMK_ReturnStatus
 *
 *  Side-effects:
 *     None
 *
 ******************************************************************************
 */

VMK_ReturnStatus
ionic_completion_create(vmk_ModuleID module_id,                   // IN
                        vmk_HeapID heap_id,                       // IN
                        vmk_LockDomainID domain,                  // IN
                        const char *name,                         // IN
                        struct ionic_completion *completion)      // IN
{
        VMK_ReturnStatus status;

        VMK_ASSERT(completion);

        vmk_Memset(completion, 0, sizeof(struct ionic_completion));
// TODO: Add compl_counter at the end of name
        status = ionic_spinlock_create(name,
                                       module_id,
                                       heap_id,
                                       domain,
                                       VMK_SPINLOCK,
                                       IONIC_LOCK_RANK_HIGH,
                                       &completion->lock);

        if (status != VMK_OK) {
                ionic_err("ionic_spinlock_create() failed, status: %s",
                          vmk_StatusToString(status));
        }

        return status;
}


/*
 ******************************************************************************
 *
 * ionic_completion_destroy --
 *
 *    Destroy completion resources
 *
 *  Parameters:
 *     completion - IN (completion to free)
 *
 *  Results:
 *     None
 *
 *  Side-effects:
 *     None
 *
 ******************************************************************************
 */

void
ionic_completion_destroy(struct ionic_completion *completion)     // IN
{
        VMK_ASSERT(completion);

        if (completion &&
            (completion->validation_id == IONIC_EN_COMPL_VALIDATION_ID) &&
            (completion->lock)) {
                ionic_spinlock_destroy(completion->lock);
                completion->lock = NULL;
                completion->validation_id = 0;
        }
}


/*
 ******************************************************************************
 *
 * ionic_completion_init --
 *
 *    Initialize Completion
 *
 *  Parameters:
 *     completion - IN (completion struct to initialize)
 *
 *  Results:
 *     None
 *
 *  Side-effects:
 *     None
 *
 ******************************************************************************
 */

void
ionic_completion_init(struct ionic_completion *completion)        // IN
{
        VMK_ASSERT(completion);
        IONIC_INIT_WORLD_EVENT_ID(&completion->event_id);
        completion->done = VMK_FALSE;
        completion->validation_id = IONIC_EN_COMPL_VALIDATION_ID;
}


/*
 ******************************************************************************
 *
 * ionic_complete --
 *
 *    Wake up process waiting for completion event
 *
 *  Parameters:
 *     completion - IN (completion struct)
 *
 *  Results:
 *     None
 *
 *  Side-effects:
 *     None
 *
 ******************************************************************************
 */

void
ionic_complete(struct ionic_completion *completion)               // IN
{
        VMK_ASSERT(completion);

        if (completion &&
            (completion->validation_id == IONIC_EN_COMPL_VALIDATION_ID) &&
            (completion->lock)) {
                vmk_SpinlockLockIgnoreDeathPending(completion->lock);
                completion->done = VMK_TRUE;
                vmk_SpinlockUnlock(completion->lock);
                vmk_WorldWakeup(completion->event_id);
        }
}


/*
 ******************************************************************************
 *
 * ionic_wait_for_completion_timeout --
 *
 *    Wait indefinitely for completion event to happen
 *
 *  Parameters:
 *     completion  - IN (completion struct)
 *     timeout_ms  - IN (timeout in ms)
 *
 *  Results:
 *    Is completion generates before timeout
 *
 *  Side-effects:
 *     None
 *
 ******************************************************************************
 */

vmk_Bool
ionic_wait_for_completion_timeout(struct ionic_completion *completion,    // IN
                                  vmk_uint32 timeout_ms)                  // IN
{
        VMK_ReturnStatus status = VMK_OK;
        vmk_Bool is_timeout = VMK_TRUE;
        VMK_ASSERT(completion);

        vmk_SpinlockLockIgnoreDeathPending(completion->lock);
        if (!completion->done) {
                status = vmk_WorldWait(completion->event_id,
                                       completion->lock,
                                       timeout_ms,
                                       "wait_for_completion");

                if (status == VMK_OK && completion->done) {
                        is_timeout = VMK_FALSE;
                } else if (status == VMK_WAIT_INTERRUPTED && !completion->done) {
                        is_timeout = VMK_TRUE;
                } else if (status == VMK_TIMEOUT && !completion->done) {
                        is_timeout = VMK_TRUE;
                }

        } else {
                vmk_SpinlockUnlock(completion->lock);
                is_timeout = VMK_FALSE;
        }

        /*
         * vmk_WorldWait() can return the following results:
         *  - VMK_OK            - all good
         *  - VMK_BAD_PARAM     - this means that we have a bug in caller function
         *  - VMK_TIMEOUT       - timeout on an unlimited wait is a bug
         *  - VMK_DEATH_PENDING - shouldn't happen, means that there is a bug
         *  - VMK_WAIT_INTERRUPTED - we can't have this status here (check code above)
         */

        if (status != VMK_OK) {
                ionic_err("ionic_wait_for_completion(): vmk_WorldWait"
                          "- failed (%s)", vmk_StatusToString(status));
                VMK_ASSERT(0);
        }
        
        return is_timeout;
}

