/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#ifndef _LIBMCTP_LOG_H
#define _LIBMCTP_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

void set_log_level (int log_level);
int get_log_level (void);

void mctp_log(int level, const char *pname, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

void mctp_prlog(int level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define mctpd_err(fmt, ...) mctp_log(MCTP_LOG_ERR, "mctpd", fmt, ##__VA_ARGS__)
#define mctpd_warn(fmt, ...) mctp_log(MCTP_LOG_WARNING, "mctpd", fmt, ##__VA_ARGS__)
#define mctpd_info(fmt, ...) mctp_log(MCTP_LOG_INFO, "mctpd", fmt, ##__VA_ARGS__)
#define mctpd_debug(fmt, ...) mctp_log(MCTP_LOG_DEBUG, "mctpd", fmt, ##__VA_ARGS__)

#define mctp_prerr(fmt, ...) mctp_prlog(MCTP_LOG_ERR, fmt, ##__VA_ARGS__)
#define mctp_prwarn(fmt, ...) mctp_prlog(MCTP_LOG_WARNING, fmt, ##__VA_ARGS__)
#define mctp_prinfo(fmt, ...) mctp_prlog(MCTP_LOG_INFO, fmt, ##__VA_ARGS__)
#define mctp_prdebug(fmt, ...) mctp_prlog(MCTP_LOG_DEBUG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* _LIBMCTP_LOG_H */
