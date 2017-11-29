
#include "nic/utils/fsm/fsm.hpp"
#include <iostream>

namespace hal {
namespace utils {

fsm_transition_t::fsm_transition_t(uint32_t event, fsm_transition_func function,
                                   uint32_t next_state) {
    this->event = event;
    this->func = function;
    this->next_state = next_state;
}

/* State Machine definition could be allocated globally once and HAL restart
 * will re-allocate SM definition. To make sure persistent state machine works
 * correctly always get the current state machine by using a function pointer.
 */
fsm_state_machine_t::fsm_state_machine_t(get_sm_func sm_func, uint32_t init_state,
                                     uint32_t end_state, uint32_t timeout_event,
                                     fsm_state_ctx ctx, get_timer_func timer_func) {
    this->sm_get_func_ = sm_func;
    this->current_state_ = init_state;
    this->init_state_ = init_state;
    this->end_state_ = end_state;
    this->timeout_event_ = timeout_event;
    this->timeout_ = 0;
    this->cur_state_time_ctx_ = nullptr;
    this->timer_get_func_ = timer_func;
    this->ctx_ = ctx;
    this->_reset_next_event();

    fsm_state_t current_state(this->get_state());
    fsm_state_machine_def_t* sm = sm_func();
    auto result = sm->find(current_state);

    if (result == sm->end()) {
        assert(0);
    }
    fsm_state_t state = result->first;
    if (state.entry_func) {
        state.entry_func(NULL);
    }

    if (this->timer_get_func_ && state.timeout) {
        this->cur_state_time_ctx_ =
            this->timer_get_func_()->add_timer(state.timeout, this);
    }
}

void fsm_state_machine_t::_process_event_internal(uint32_t event,
                                                fsm_event_data data) {
    fsm_state_t current_state(this->get_state());
    fsm_state_machine_def_t* sm = this->sm_get_func_();

    auto result = sm->find(current_state);
    if (result == sm->end()) {
        std::cout << "State not found...\n";
        assert(0);
    }

    fsm_state_t state = result->first;
    std::vector<fsm_transition_t> transitions = result->second;
    for (fsm_transition_t t : transitions) {
        if (t.event == event) {
            if (t.func != nullptr) {
                bool ret = t.func(this->ctx_, data);
                if (!ret) {
                    HAL_TRACE_ERR("{}: Transition failed for event {}",
                                  __func__, event);
                    return;
                }
            }
            /* State is the same, don't do anything. */
            if (state.state_id == t.next_state) {
                return;
            }
            if (state.exit_func) {
                state.exit_func(this->ctx_);
            }
            /* Cancel the old timer if its set. */
            if (this->timer_get_func_ && this->cur_state_time_ctx_) {
                this->timer_get_func_()->delete_timer(
                    this->cur_state_time_ctx_);
                this->cur_state_time_ctx_ = nullptr;
            }
            fsm_state_t newState(t.next_state);
            auto result = sm->find(newState);
            if (result == sm->end()) {
                /*  Assert, state not defined in SM */
                assert(0);
            }
            current_state = result->first;
            /* Set the timeout default to Statically defined state machine.
             * In entry function caller can override the value.
             */
            this->timeout_ = current_state.timeout;
            if (current_state.entry_func) {
                current_state.entry_func(this->ctx_);
            }
            if (this->timer_get_func_ && this->timeout_) {
                this->cur_state_time_ctx_ =
                    this->timer_get_func_()->add_timer(this->timeout_, this);
            }
            this->set_state(t.next_state);
            break;
        }
    }

    return;
}

void fsm_state_machine_t::stop_state_timer() {
    if (this->cur_state_time_ctx_) {
        this->timer_get_func_()->delete_timer(this->cur_state_time_ctx_);
        this->reset_timer();
    }
}

void fsm_state_machine_t::reset_timer() {
    this->cur_state_time_ctx_ = nullptr;
}

void fsm_state_machine_t::process_event(uint32_t event, fsm_event_data data) {
    /* TODO: Define max cycles to run! */
    while (true) {
        this->_process_event_internal(event, data);
        if (this->_next_event_set()) {
            event = std::get<0>(this->next_event_);
            data = std::get<1>(this->next_event_);
            this->_reset_next_event();
        } else {
            break;
        }
    }
}

void fsm_state_machine_t::set_current_state_timeout(uint32_t timeout) {
    this->timeout_ = timeout;
}

uint32_t fsm_state_machine_t::get_current_state_timeout() { return this->timeout_; }

uint32_t fsm_state_machine_t::get_timeout_event() { return this->timeout_event_; }

void fsm_state_machine_t::throw_event(uint32_t event, fsm_event_data data) {
    assert(!this->_next_event_set());
    this->_set_next_event(event, data);
}

}  // namespace utils
}  // namespace hal
