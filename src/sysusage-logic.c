#include <string.h>

#include "sysusage-logic.h"

#define SYSUSAGE_NET_DEVICE_NAME_MAX 15

gboolean
sysusage_logic_net_device_name_is_valid(const gchar *name)
{
    const gchar *p;

    if (name == NULL || name[0] == '\0')
        return FALSE;

    if (strlen(name) > SYSUSAGE_NET_DEVICE_NAME_MAX)
        return FALSE;

    if (g_strcmp0(name, ".") == 0 || g_strcmp0(name, "..") == 0)
        return FALSE;

    for (p = name; *p != '\0'; p++)
    {
        if (*p == '/' || *p == ':' || g_ascii_isspace(*p))
            return FALSE;
    }

    return TRUE;
}

void
sysusage_logic_calculate_mem_swap_percent(gulong mem_total,
                                          gulong mem_available,
                                          gulong swap_total,
                                          gulong swap_free,
                                          gdouble *mem_percent,
                                          gdouble *swap_percent,
                                          gulong *mem_used,
                                          gulong *swap_used)
{
    gulong local_mem_used = 0;
    gulong local_swap_used = 0;

    g_return_if_fail(mem_percent != NULL);
    g_return_if_fail(swap_percent != NULL);

    if (mem_total > 0)
    {
        local_mem_used = (mem_available <= mem_total) ? (mem_total - mem_available) : 0;
        *mem_percent = CLAMP((((gdouble) local_mem_used / (gdouble) mem_total) * 100.0), 0.0, 100.0);
    }
    else
    {
        *mem_percent = 0.0;
    }

    if (swap_total > 0)
    {
        local_swap_used = (swap_free <= swap_total) ? (swap_total - swap_free) : 0;
        *swap_percent = CLAMP((((gdouble) local_swap_used / (gdouble) swap_total) * 100.0), 0.0, 100.0);
    }
    else
    {
        *swap_percent = 0.0;
    }

    if (mem_used != NULL)
        *mem_used = local_mem_used;

    if (swap_used != NULL)
        *swap_used = local_swap_used;
}

void
sysusage_logic_calculate_net_rates(guint64 prev_rx_bytes,
                                   guint64 prev_tx_bytes,
                                   guint64 rx_bytes,
                                   guint64 tx_bytes,
                                   gdouble dt_seconds,
                                   gdouble *in_mbit,
                                   gdouble *out_mbit)
{
    guint64 delta_rx;
    guint64 delta_tx;

    g_return_if_fail(in_mbit != NULL);
    g_return_if_fail(out_mbit != NULL);

    *in_mbit = 0.0;
    *out_mbit = 0.0;

    if (dt_seconds <= 0.0)
        return;

    delta_rx = (rx_bytes >= prev_rx_bytes) ? (rx_bytes - prev_rx_bytes) : 0;
    delta_tx = (tx_bytes >= prev_tx_bytes) ? (tx_bytes - prev_tx_bytes) : 0;

    *in_mbit = (((gdouble) delta_rx) * 8.0) / (dt_seconds * 1000.0 * 1000.0);
    *out_mbit = (((gdouble) delta_tx) * 8.0) / (dt_seconds * 1000.0 * 1000.0);
}
