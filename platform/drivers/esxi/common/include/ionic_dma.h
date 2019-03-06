/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 *
 * This software is provided to you under the terms of the GNU
 * General Public License (GPL) Version 2.0, available from the file
 * [ionic_dma.h] in the main directory of this source tree.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * ionic_dma.h --
 *
 * Implement all DMA related functions
 */

#ifndef _IONIC_DMA_H_
#define _IONIC_DMA_H_

#include <vmkapi.h>

#define IONIC_DMA_MAX_SEGMENT        (800 * 1024 * 1024)


VMK_ReturnStatus
ionic_dma_engine_create(const char *const name,
                        vmk_ModuleID module_id,
                        vmk_Device device,
                        vmk_Bool is_coherent,
                        vmk_ByteCountSmall sg_elem_size,
                        vmk_ByteCountSmall sg_elem_alignment,
                        vmk_DMAEngine *const dma_engine);

VMK_ReturnStatus
ionic_dma_engine_destroy(vmk_DMAEngine dma_engine);

vmk_IOA
ionic_dma_map_ma(vmk_DMAEngine dma_engine,
                 vmk_DMADirection dir,
                 vmk_MA ma,
                 vmk_ByteCountSmall length);

vmk_IOA
ionic_dma_map_va(vmk_DMAEngine dma_engine,
                 vmk_DMADirection dir,
                 vmk_VA va,
                 vmk_ByteCountSmall length);

VMK_ReturnStatus
ionic_dma_unmap(vmk_DMAEngine dma_engine,
                vmk_DMADirection dir,
                vmk_ByteCountSmall length,
                vmk_IOA ioa);

void *
ionic_dma_alloc_align(vmk_HeapID heap_id,
                      vmk_DMAEngine dma_engine,
                      vmk_uint32 size,
                      vmk_uint32 align,
                      vmk_IOA *ioa);

void *
ionic_dma_zalloc_align(vmk_HeapID heap_id,
                       vmk_DMAEngine dma_engine,
                       vmk_uint32 size,
                       vmk_uint32 align,
                       vmk_IOA *ioa);

void
ionic_dma_free(vmk_HeapID heap_id,
               vmk_DMAEngine dma_engine,
               vmk_uint32 size,
               void *va,
               vmk_IOA ioa);

#endif /* End of _IONIC_DMA_H_ */ 
