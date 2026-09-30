#ifndef __SYSUSAGE_LOGIC_H__
#define __SYSUSAGE_LOGIC_H__

#include <glib.h>

gboolean sysusage_logic_net_device_name_is_valid(const gchar *name);

void sysusage_logic_calculate_mem_swap_percent(gulong mem_total,
                                               gulong mem_available,
                                               gulong swap_total,
                                               gulong swap_free,
                                               gdouble *mem_percent,
                                               gdouble *swap_percent,
                                               gulong *mem_used,
                                               gulong *swap_used);

void sysusage_logic_calculate_net_rates(guint64 prev_rx_bytes,
                                        guint64 prev_tx_bytes,
                                        guint64 rx_bytes,
                                        guint64 tx_bytes,
                                        gdouble dt_seconds,
                                        gdouble *in_mbit,
                                        gdouble *out_mbit);

#endif
