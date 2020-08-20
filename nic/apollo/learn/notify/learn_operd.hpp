//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// operd helper utilities for learn
///
//----------------------------------------------------------------------------

#ifndef __LEARN_NOTIFY_LEARN_OPERD_HPP__
#define __LEARN_NOTIFY_LEARN_OPERD_HPP__

#include "nic/apollo/learn/learn_ctxt.hpp"

namespace learn {
namespace mode_notify {

/// \brief        generate learn packet event and write to operd region
/// \param[in]    ctxt    learn context containing endpoint data
void generate_learn_event(learn_ctxt_t *ctxt);

}    // namepsace mode_notify
}    // namespace learn

#endif    //__LEARN_NOTIFY_LEARN_OPERD_HPP__
