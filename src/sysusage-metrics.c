#include <stdio.h>
#include <string.h>

#include "sysusage-plugin.h"
#include "sysusage-logic.h"

static gdouble
sysusage_plugin_kib_to_gib(gulong kib)
{
    return (gdouble) kib / (1024.0 * 1024.0);
}

static gboolean
sysusage_plugin_read_uint64_file(const gchar *path,
                                 guint64 *value)
{
    FILE *fp;
    unsigned long long parsed = 0ULL;

    g_return_val_if_fail(value != NULL, FALSE);

    fp = fopen(path, "r");
    if (fp == NULL)
        return FALSE;

    if (fscanf(fp, "%llu", &parsed) != 1)
    {
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    *value = (guint64) parsed;
    return TRUE;
}

static void
sysusage_plugin_set_tooltip_and_queue_draw(gchar **tooltip_text,
                                           GtkWidget *tooltip_widget,
                                           GtkWidget *draw_area,
                                           const gchar *new_text)
{
    g_return_if_fail(tooltip_text != NULL);
    g_return_if_fail(GTK_IS_LABEL(tooltip_widget));
    g_return_if_fail(GTK_IS_WIDGET(draw_area));

    g_free(*tooltip_text);
    *tooltip_text = g_strdup(new_text);
    gtk_label_set_text(GTK_LABEL(tooltip_widget), *tooltip_text);
    gtk_widget_queue_draw(draw_area);
}

void
sysusage_plugin_update_mem_timer(SysusagePlugin *sysusage)
{
    if (sysusage->mem_timer_id != 0)
    {
        g_source_remove(sysusage->mem_timer_id);
        sysusage->mem_timer_id = 0;
    }

    if (!sysusage->disable_memusage)
        sysusage->mem_timer_id = g_timeout_add_seconds(1, (GSourceFunc) sysusage_plugin_read_mem_values, sysusage);
}

gboolean
sysusage_plugin_read_mem_values(SysusagePlugin *sysusage)
{
    /* Disabled sections do not need periodic sampling. */
    if (sysusage->disable_memusage)
        return G_SOURCE_REMOVE;

    /* Read Linux memory counters and derive percentages for the two bars. */
    FILE *fp = fopen("/proc/meminfo", "r");
    char key[64] = { 0 };
    gulong value = 0;
    char unit[16] = { 0 };
    gulong mem_total = 0;
    gulong mem_available = 0;
    gulong swap_total = 0;
    gulong swap_free = 0;
    gulong mem_used = 0;
    gulong swap_used = 0;

    if (fp == NULL)
        return G_SOURCE_CONTINUE;

    while (fscanf(fp, "%63s %lu %15s", key, &value, unit) == 3)
    {
        if (g_strcmp0(key, "MemTotal:") == 0)
            mem_total = value;
        else if (g_strcmp0(key, "MemAvailable:") == 0)
            mem_available = value;
        else if (g_strcmp0(key, "SwapTotal:") == 0)
            swap_total = value;
        else if (g_strcmp0(key, "SwapFree:") == 0)
            swap_free = value;
    }

    fclose(fp);

    sysusage_logic_calculate_mem_swap_percent(mem_total,
                                              mem_available,
                                              swap_total,
                                              swap_free,
                                              &sysusage->mem_percent,
                                              &sysusage->swap_percent,
                                              &mem_used,
                                              &swap_used);

    g_free(sysusage->mem_tooltip_text);
    sysusage->mem_tooltip_text = g_strdup_printf("%s: %.1f%% (%.2f / %.2f GiB)\n%s: %.1f%% (%.2f / %.2f GiB)",
                                                  sysusage->mem_label,
                                                  sysusage->mem_percent,
                                                  sysusage_plugin_kib_to_gib(mem_used),
                                                  sysusage_plugin_kib_to_gib(mem_total),
                                                  sysusage->swap_label,
                                                  sysusage->swap_percent,
                                                  sysusage_plugin_kib_to_gib(swap_used),
                                                  sysusage_plugin_kib_to_gib(swap_total));
    gtk_label_set_text(GTK_LABEL(sysusage->mem_tooltip_widget), sysusage->mem_tooltip_text);

    gtk_widget_queue_draw(sysusage->mem_area);
    return G_SOURCE_CONTINUE;
}

void
sysusage_plugin_update_net_timer(SysusagePlugin *sysusage)
{
    guint interval_ms;

    if (sysusage->net_timer_id != 0)
    {
        g_source_remove(sysusage->net_timer_id);
        sysusage->net_timer_id = 0;
    }

    if (sysusage->disable_netusage)
    {
        sysusage->has_prev_net_sample = FALSE;
        return;
    }

    /* Clamp to a practical range to avoid excessively fast/slow timers. */
    interval_ms = (guint) CLAMP((gint) (sysusage->net_interval_seconds * 1000.0), 50, 60000);
    sysusage->net_timer_id = g_timeout_add(interval_ms, (GSourceFunc) sysusage_plugin_read_net_values, sysusage);
}

gboolean
sysusage_plugin_read_net_values(SysusagePlugin *sysusage)
{
    if (sysusage->disable_netusage)
    {
        sysusage->has_prev_net_sample = FALSE;
        return G_SOURCE_REMOVE;
    }

    gchar *rx_path;
    gchar *tx_path;
    guint64 rx_bytes = 0;
    guint64 tx_bytes = 0;
    gint64 now_us;
    gdouble in_mbit = 0.0;
    gdouble out_mbit = 0.0;

    if (sysusage->net_device == NULL || sysusage->net_device[0] == '\0')
    {
        /* No interface selected: render empty bars and explain in tooltip. */
        sysusage->net_in_percent = 0.0;
        sysusage->net_out_percent = 0.0;
        sysusage->has_prev_net_sample = FALSE;
        sysusage_plugin_set_tooltip_and_queue_draw(&sysusage->net_tooltip_text,
                                                   sysusage->net_tooltip_widget,
                                                   sysusage->net_area,
                                                   "Network device not set");
        return G_SOURCE_CONTINUE;
    }

    if (!sysusage_logic_net_device_name_is_valid(sysusage->net_device))
    {
        sysusage->net_in_percent = 0.0;
        sysusage->net_out_percent = 0.0;
        sysusage->has_prev_net_sample = FALSE;
        sysusage_plugin_set_tooltip_and_queue_draw(&sysusage->net_tooltip_text,
                                                   sysusage->net_tooltip_widget,
                                                   sysusage->net_area,
                                                   "Invalid network device name");
        return G_SOURCE_CONTINUE;
    }

    rx_path = g_strdup_printf("/sys/class/net/%s/statistics/rx_bytes", sysusage->net_device);
    tx_path = g_strdup_printf("/sys/class/net/%s/statistics/tx_bytes", sysusage->net_device);

    if (!sysusage_plugin_read_uint64_file(rx_path, &rx_bytes) || !sysusage_plugin_read_uint64_file(tx_path, &tx_bytes))
    {
        gchar *tooltip = g_strdup_printf("Unable to read counters for device: %s", sysusage->net_device);

        sysusage_plugin_set_tooltip_and_queue_draw(&sysusage->net_tooltip_text,
                                                   sysusage->net_tooltip_widget,
                                                   sysusage->net_area,
                                                   tooltip);
        g_free(tooltip);
        sysusage->net_in_percent = 0.0;
        sysusage->net_out_percent = 0.0;
        sysusage->has_prev_net_sample = FALSE;
        g_free(rx_path);
        g_free(tx_path);
        return G_SOURCE_CONTINUE;
    }

    g_free(rx_path);
    g_free(tx_path);

    now_us = g_get_monotonic_time();

    if (sysusage->has_prev_net_sample)
    {
        /*
         * Throughput uses byte deltas over monotonic elapsed time.  Counter
         * wrap or interface replacement is treated as a zero delta so the UI
         * shows a quiet sample instead of a spurious full-scale spike.
         */
        gdouble dt = ((gdouble) (now_us - sysusage->prev_net_sample_us)) / 1000000.0;

        sysusage_logic_calculate_net_rates(sysusage->prev_rx_bytes,
                                           sysusage->prev_tx_bytes,
                                           rx_bytes,
                                           tx_bytes,
                                           dt,
                                           &in_mbit,
                                           &out_mbit);
    }

    sysusage->prev_rx_bytes = rx_bytes;
    sysusage->prev_tx_bytes = tx_bytes;
    sysusage->prev_net_sample_us = now_us;
    sysusage->has_prev_net_sample = TRUE;

    /* Percentages are display scaling only; tooltips keep the raw Mbit/s. */
    if (sysusage->net_max_in_mbit > 0.0)
        sysusage->net_in_percent = CLAMP((in_mbit / sysusage->net_max_in_mbit) * 100.0, 0.0, 100.0);
    else
        sysusage->net_in_percent = 0.0;

    if (sysusage->net_max_out_mbit > 0.0)
        sysusage->net_out_percent = CLAMP((out_mbit / sysusage->net_max_out_mbit) * 100.0, 0.0, 100.0);
    else
        sysusage->net_out_percent = 0.0;

    gchar *tooltip = g_strdup_printf("Device: %s\nIncoming: %.2f Mbit/s\nOutgoing: %.2f Mbit/s",
                                     sysusage->net_device,
                                     in_mbit,
                                     out_mbit);

    sysusage_plugin_set_tooltip_and_queue_draw(&sysusage->net_tooltip_text,
                                               sysusage->net_tooltip_widget,
                                               sysusage->net_area,
                                               tooltip);
    g_free(tooltip);

    return G_SOURCE_CONTINUE;
}
