// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __SDK_LINKMGR_STATE_HPP__
#define __SDK_LINKMGR_STATE_HPP__

#include "lib/slab/slab.hpp"
#include "port.hpp"
#include "linkmgr_internal.hpp"

using sdk::lib::slab;

namespace sdk {
namespace linkmgr {

class linkmgr_state {
public:
    static linkmgr_state *factory(void);
    // get APIs for port related state
    slab *port_slab(void) const { return port_slab_; }

    /// \brief  return the port bitmap of uplink link status
    /// \return port bitmap of uplink link status
    uint64_t port_bmap(void) { return port_bmap_; }

    /// \brief set the port bitmap of uplink link status
    void set_port_bmap(uint64_t port_bmap) {
        this->port_bmap_ = port_bmap;
    }

    /// \brief  return the port bitmap mask of uplink link status
    /// \return port bitmap mask of uplink link status
    uint64_t port_bmap_mask(void) { return port_bmap_mask_; }

    /// \brief set the port bitmap mask of uplink link status
    void set_port_bmap_mask(uint64_t port_bmap_mask) {
        this->port_bmap_mask_ = port_bmap_mask;
    }

    /// \brief  returns the aacs server port number
    /// \return aacs port number
    int aacs_server_port_num(void) { return aacs_server_port_num_; }

    /// \brief set the aacs port number
    void set_aacs_server_port_num(int port) { aacs_server_port_num_ = port; }

    /// \brief  returns the handle for transceiver poll timer
    /// \return transceiver poll timer
    sdk::event_thread::timer_t *xcvr_poll_timer_handle(void) {
        return &xcvr_poll_timer_handle_;
    }

    /// \brief  returns the handle for link status poll timer
    /// \return link status poll timer
    sdk::event_thread::timer_t *link_poll_timer_handle(void) {
        return &link_poll_timer_handle_;
    }

    /// \brief  returns true if link status poll is enabled
    /// \return true if link status poll is enabled, else false
    bool link_poll_en(void) { return link_poll_en_; }

    /// \brief enable/disable link status poll
    void set_link_poll_en(bool en) { link_poll_en_ = en; }

    /// \brief  returns true if port state is restored from memory
    /// \return true if port state is restored from memory, else false
    bool port_restore_state(void) { return port_restore_state_; }

    /// \brief set true if port state is restored from memory
    void set_port_restore_state(bool restore_state) {
        port_restore_state_ = restore_state;
    }

    /// \brief  return pointer to port struct memory
    /// \return pointer to port struct memory
    void *mem(void) { return mem_; }

    /// \brief set port struct memory
    void set_mem(void *mem) { mem_ = mem; }

    /// \brief     returns pointer to port struct
    /// \param[in] index 0-based logical port number to index into array
    /// \return    pointer to port struct
    port *port_p(uint32_t index) { return port_[index]; }

    /// \brief     sets pointer to port struct in array
    /// \param[in] index 0-based logical port number to index into array
    /// \param[in] port_p pointer to port struct
    void set_port_p(uint32_t index, port *port_p) { port_[index] = port_p; }

private:
    sdk_ret_t init(void);

private:
    slab       *port_slab_;
    uint64_t   port_bmap_;                                 ///< bitmap of uplinks physical link status
    uint64_t   port_bmap_mask_;                            ///< bitmap mask of uplinks physical link status
    int        aacs_server_port_num_;                      ///< aacs server port for serdes
    bool       link_poll_en_;                              ///< enable/disable link status poll
    bool       port_restore_state_;                        ///< port state restored from memory
    void       *mem_;                                      ///< port struct memory
    port       *port_[LINKMGR_MAX_PORTS];                  ///< array of port structs
    sdk::event_thread::timer_t xcvr_poll_timer_handle_;    ///< xcvr poll timer handle
    sdk::event_thread::timer_t link_poll_timer_handle_;    ///< link poll timer handle
};

extern linkmgr_state *g_linkmgr_state;

}    // namespace linkmgr
}    // namespace sdk

#endif    // __SDK_LINKMGR_STATE_HPP__
