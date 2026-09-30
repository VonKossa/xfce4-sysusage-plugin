#include "sysusage-plugin.h"

static const gchar *
sysusage_plugin_read_entry_with_legacy(const XfceRc *rc,
                                       const gchar *new_key,
                                       const gchar *old_key,
                                       const gchar *default_value)
{
    /* Prefer the new key while keeping backward compatibility. */
    const gchar *legacy_value = xfce_rc_read_entry(rc, old_key, default_value);

    return xfce_rc_read_entry(rc, new_key, legacy_value);
}

static gboolean
sysusage_plugin_read_bool_with_legacy(const XfceRc *rc,
                                      const gchar *new_key,
                                      const gchar *old_key,
                                      gboolean default_value)
{
    const gboolean legacy_value = xfce_rc_read_bool_entry(rc, old_key, default_value);

    return xfce_rc_read_bool_entry(rc, new_key, legacy_value);
}

static gint
sysusage_plugin_read_int_with_legacy(const XfceRc *rc,
                                     const gchar *new_key,
                                     const gchar *old_key,
                                     gint default_value)
{
    const gint legacy_value = xfce_rc_read_int_entry(rc, old_key, default_value);

    return xfce_rc_read_int_entry(rc, new_key, legacy_value);
}

static void
sysusage_plugin_parse_color_or_default(GdkRGBA *out_color,
                                       const gchar *color_text,
                                       const gchar *default_color_text)
{
    g_return_if_fail(out_color != NULL);

    if (color_text == NULL || !gdk_rgba_parse(out_color, color_text))
        gdk_rgba_parse(out_color, default_color_text);
}

static void
sysusage_plugin_sanitize_loaded_settings(SysusagePlugin *sysusage)
{
    /*
     * Treat the rc file as untrusted input.  These bounds mirror the
     * properties dialog so hand-edited or stale settings cannot produce
     * impractical geometry, timers, or divide-by-zero scaling values.
     */
    sysusage->bar_height = (guint) CLAMP((gint) sysusage->bar_height, 2, 64);
    sysusage->bar_width = (guint) CLAMP((gint) sysusage->bar_width, 16, 512);
    sysusage->bar_spacing = (guint) CLAMP((gint) sysusage->bar_spacing, 0, 64);
    sysusage->separator_spacing = (guint) CLAMP((gint) sysusage->separator_spacing, 0, 64);
    sysusage->separator_line_length = (guint) CLAMP((gint) sysusage->separator_line_length, 1, 512);
    sysusage->separator_line_width = (guint) CLAMP((gint) sysusage->separator_line_width, 1, 32);
    sysusage->separator_style = (guint) CLAMP((gint) sysusage->separator_style, 0, 2);
    sysusage->text_padding = (guint) CLAMP((gint) sysusage->text_padding, 0, 64);

    sysusage->net_interval_seconds = CLAMP(sysusage->net_interval_seconds, 0.05, 5.0);
    sysusage->net_max_in_mbit = CLAMP(sysusage->net_max_in_mbit, 1.0, 100000.0);
    sysusage->net_max_out_mbit = CLAMP(sysusage->net_max_out_mbit, 1.0, 100000.0);

    /* Keep at least one section visible. */
    if (sysusage->disable_memusage && sysusage->disable_netusage)
        sysusage->disable_netusage = FALSE;
}

void
sysusage_plugin_save(SysusagePlugin *sysusage)
{
    /* Persist current state to the plugin rc file. */
    gchar *filename = xfce_panel_plugin_save_location(sysusage->plugin, TRUE);

    if (filename == NULL)
        return;

    XfceRc *rc = xfce_rc_simple_open(filename, FALSE);
    if (rc != NULL)
    {
        gchar *mem_color = gdk_rgba_to_string(&sysusage->mem_color);
        gchar *swap_color = gdk_rgba_to_string(&sysusage->swap_color);
        gchar *net_in_color = gdk_rgba_to_string(&sysusage->net_in_color);
        gchar *net_out_color = gdk_rgba_to_string(&sysusage->net_out_color);
        gchar *background_color = gdk_rgba_to_string(&sysusage->background_color);
        gchar *text_color = gdk_rgba_to_string(&sysusage->text_color);
        gchar *separator_color = gdk_rgba_to_string(&sysusage->separator_color);

        xfce_rc_write_entry(rc, "sys_mem_color", mem_color);
        xfce_rc_write_entry(rc, "sys_swap_color", swap_color);
        xfce_rc_write_entry(rc, "sys_net_in_color", net_in_color);
        xfce_rc_write_entry(rc, "sys_net_out_color", net_out_color);
        xfce_rc_write_entry(rc, "sys_background_color", background_color);
        xfce_rc_write_entry(rc, "sys_text_color", text_color);
        xfce_rc_write_entry(rc, "sys_text_font", sysusage->text_font_family);
        xfce_rc_write_entry(rc, "sys_text_font_family", sysusage->text_font_family);
        xfce_rc_write_entry(rc, "sys_separator_color", separator_color);

        xfce_rc_write_entry(rc, "sys_mem_label", sysusage->mem_label);
        xfce_rc_write_entry(rc, "sys_swap_label", sysusage->swap_label);
        xfce_rc_write_entry(rc, "sys_net_device", sysusage->net_device);
        xfce_rc_write_entry(rc, "sys_net_in_label", sysusage->net_in_label);
        xfce_rc_write_entry(rc, "sys_net_out_label", sysusage->net_out_label);

        xfce_rc_write_bool_entry(rc, "sys_show_mem_text", sysusage->show_mem_text);
        xfce_rc_write_bool_entry(rc, "sys_show_swap_text", sysusage->show_swap_text);
        xfce_rc_write_bool_entry(rc, "sys_show_net_in_text", sysusage->show_net_in_text);
        xfce_rc_write_bool_entry(rc, "sys_show_net_out_text", sysusage->show_net_out_text);
        xfce_rc_write_bool_entry(rc, "sys_text_primary_side_left", sysusage->text_primary_side_left);
        xfce_rc_write_bool_entry(rc, "sys_text_primary_side_right", sysusage->text_primary_side_right);
        xfce_rc_write_bool_entry(rc, "sys_vertical_bars", sysusage->vertical_bars);
        xfce_rc_write_bool_entry(rc, "sys_vertical_text_under_bars", sysusage->vertical_text_under_bars);
        xfce_rc_write_bool_entry(rc, "sys_separator_transparent", sysusage->separator_transparent);
        xfce_rc_write_bool_entry(rc, "sys_disable_memusage", sysusage->disable_memusage);
        xfce_rc_write_bool_entry(rc, "sys_disable_netusage", sysusage->disable_netusage);

        xfce_rc_write_int_entry(rc, "sys_text_padding", (gint) sysusage->text_padding);
        xfce_rc_write_int_entry(rc, "sys_bar_width", (gint) sysusage->bar_width);
        xfce_rc_write_int_entry(rc, "sys_bar_height", (gint) sysusage->bar_height);
        xfce_rc_write_int_entry(rc, "sys_bar_spacing", (gint) sysusage->bar_spacing);
        xfce_rc_write_int_entry(rc, "sys_separator_spacing", (gint) sysusage->separator_spacing);
        xfce_rc_write_int_entry(rc, "sys_separator_line_length", (gint) sysusage->separator_line_length);
        xfce_rc_write_int_entry(rc, "sys_separator_line_width", (gint) sysusage->separator_line_width);
        xfce_rc_write_int_entry(rc, "sys_separator_style", (gint) sysusage->separator_style);

        gchar *net_interval = g_strdup_printf("%.2f", sysusage->net_interval_seconds);
        gchar *net_max_in = g_strdup_printf("%.2f", sysusage->net_max_in_mbit);
        gchar *net_max_out = g_strdup_printf("%.2f", sysusage->net_max_out_mbit);

        xfce_rc_write_entry(rc, "sys_net_interval_seconds", net_interval);
        xfce_rc_write_entry(rc, "sys_net_max_in_mbit", net_max_in);
        xfce_rc_write_entry(rc, "sys_net_max_out_mbit", net_max_out);

        g_free(mem_color);
        g_free(swap_color);
        g_free(net_in_color);
        g_free(net_out_color);
        g_free(background_color);
        g_free(text_color);
        g_free(separator_color);
        g_free(net_interval);
        g_free(net_max_in);
        g_free(net_max_out);
        xfce_rc_close(rc);
    }

    g_free(filename);
}

void
sysusage_plugin_load(SysusagePlugin *sysusage)
{
    /* Load persisted settings and fall back to legacy keys when needed. */
    gchar *filename = xfce_panel_plugin_lookup_rc_file(sysusage->plugin);

    if (filename == NULL)
        return;

    XfceRc *rc = xfce_rc_simple_open(filename, TRUE);
    if (rc != NULL)
    {
        const gchar *mem_color = sysusage_plugin_read_entry_with_legacy(rc, "sys_mem_color", "mem_color", DEFAULT_MEM_COLOR);
        const gchar *swap_color = sysusage_plugin_read_entry_with_legacy(rc, "sys_swap_color", "swap_color", DEFAULT_SWAP_COLOR);
        const gchar *net_in_color = sysusage_plugin_read_entry_with_legacy(rc, "sys_net_in_color", "net_in_color", mem_color);
        const gchar *net_out_color = sysusage_plugin_read_entry_with_legacy(rc, "sys_net_out_color", "net_out_color", swap_color);
        const gchar *background_color = sysusage_plugin_read_entry_with_legacy(rc,
                                                                                "sys_background_color",
                                                                                "background_color",
                                                                                DEFAULT_BACKGROUND_COLOR);
        const gchar *text_color = sysusage_plugin_read_entry_with_legacy(rc, "sys_text_color", "text_color", DEFAULT_TEXT_COLOR);
        const gchar *text_font = xfce_rc_read_entry(rc, "sys_text_font", NULL);
        const gchar *text_font_family = xfce_rc_read_entry(rc, "sys_text_font_family", DEFAULT_TEXT_FONT);
        const gchar *separator_color = sysusage_plugin_read_entry_with_legacy(rc,
                                                                               "sys_separator_color",
                                                                               "separator_color",
                                                                               DEFAULT_SEPARATOR_COLOR);
        const gchar *mem_label = sysusage_plugin_read_entry_with_legacy(rc, "sys_mem_label", "mem_label", DEFAULT_MEM_LABEL);
        const gchar *swap_label = sysusage_plugin_read_entry_with_legacy(rc, "sys_swap_label", "swap_label", DEFAULT_SWAP_LABEL);
        const gchar *net_device = sysusage_plugin_read_entry_with_legacy(rc, "sys_net_device", "net_device", DEFAULT_NET_DEVICE);
        const gchar *net_in_label = sysusage_plugin_read_entry_with_legacy(rc, "sys_net_in_label", "net_in_label", NULL);
        const gchar *net_out_label = sysusage_plugin_read_entry_with_legacy(rc, "sys_net_out_label", "net_out_label", NULL);
        const gchar *legacy_net_label = xfce_rc_read_entry(rc, "net_label", DEFAULT_NET_IN_LABEL);

        sysusage->bar_width = (guint) sysusage_plugin_read_int_with_legacy(rc, "sys_bar_width", "bar_width", DEFAULT_BAR_WIDTH);
        sysusage->bar_height = (guint) sysusage_plugin_read_int_with_legacy(rc, "sys_bar_height", "bar_height", DEFAULT_BAR_HEIGHT);
        sysusage->bar_spacing = (guint) sysusage_plugin_read_int_with_legacy(rc, "sys_bar_spacing", "bar_spacing", DEFAULT_BAR_SPACING);
        sysusage->separator_spacing = (guint) sysusage_plugin_read_int_with_legacy(rc,
                                                                                    "sys_separator_spacing",
                                                                                    "separator_spacing",
                                                                                    DEFAULT_SEPARATOR_SPACING);
        sysusage->separator_line_length = (guint) sysusage_plugin_read_int_with_legacy(rc,
                                                                                         "sys_separator_line_length",
                                                                                         "separator_line_length",
                                                                                         DEFAULT_SEPARATOR_LINE_LENGTH);
        sysusage->separator_line_width = (guint) sysusage_plugin_read_int_with_legacy(rc,
                                                                                        "sys_separator_line_width",
                                                                                        "separator_line_width",
                                                                                        DEFAULT_SEPARATOR_LINE_WIDTH);
        sysusage->separator_style = (guint) xfce_rc_read_int_entry(rc,
                                                                    "sys_separator_style",
                                                                    DEFAULT_SEPARATOR_STYLE);
        sysusage->show_mem_text = sysusage_plugin_read_bool_with_legacy(rc,
                                                                         "sys_show_mem_text",
                                                                         "show_mem_text",
                                                                         DEFAULT_SHOW_MEM_TEXT);
        sysusage->show_swap_text = sysusage_plugin_read_bool_with_legacy(rc,
                                                                          "sys_show_swap_text",
                                                                          "show_swap_text",
                                                                          DEFAULT_SHOW_SWAP_TEXT);
        sysusage->show_net_in_text = sysusage_plugin_read_bool_with_legacy(rc,
                                                                            "sys_show_net_in_text",
                                                                            "show_net_in_text",
                                                                            DEFAULT_SHOW_NET_IN_TEXT);
        sysusage->show_net_out_text = sysusage_plugin_read_bool_with_legacy(rc,
                                                                             "sys_show_net_out_text",
                                                                             "show_net_out_text",
                                                                             DEFAULT_SHOW_NET_OUT_TEXT);

        const gboolean legacy_text_primary_side = xfce_rc_read_bool_entry(rc, "text_primary_side", DEFAULT_TEXT_PRIMARY_SIDE_LEFT);
        sysusage->text_primary_side_left = sysusage_plugin_read_bool_with_legacy(rc,
                                                                                  "sys_text_primary_side_left",
                                                                                  "text_primary_side_left",
                                                                                  legacy_text_primary_side);
        sysusage->text_primary_side_right = sysusage_plugin_read_bool_with_legacy(rc,
                                                                                   "sys_text_primary_side_right",
                                                                                   "text_primary_side_right",
                                                                                   legacy_text_primary_side);

        sysusage->text_padding = (guint) sysusage_plugin_read_int_with_legacy(rc,
                                                                               "sys_text_padding",
                                                                               "text_padding",
                                                                               DEFAULT_TEXT_PADDING);
        sysusage->vertical_bars = sysusage_plugin_read_bool_with_legacy(rc, "sys_vertical_bars", "vertical_bars", DEFAULT_VERTICAL_BARS);
        sysusage->vertical_text_under_bars = xfce_rc_read_bool_entry(rc,
                                                                     "sys_vertical_text_under_bars",
                                                                     DEFAULT_VERTICAL_TEXT_UNDER_BARS);
        sysusage->separator_transparent = xfce_rc_read_bool_entry(rc, "sys_separator_transparent", DEFAULT_SEPARATOR_TRANSPARENT);
        sysusage->disable_memusage = sysusage_plugin_read_bool_with_legacy(rc,
                                                                            "sys_disable_memusage",
                                                                            "disable_memusage",
                                                                            DEFAULT_DISABLE_MEMUSAGE);
        sysusage->disable_netusage = sysusage_plugin_read_bool_with_legacy(rc,
                                                                            "sys_disable_netusage",
                                                                            "disable_netusage",
                                                                            DEFAULT_DISABLE_NETUSAGE);

        sysusage->net_interval_seconds = g_ascii_strtod(sysusage_plugin_read_entry_with_legacy(rc,
                                                                                                 "sys_net_interval_seconds",
                                                                                                 "net_interval_seconds",
                                                                                                 "0.25"),
                                                        NULL);
        sysusage->net_max_in_mbit = g_ascii_strtod(sysusage_plugin_read_entry_with_legacy(rc,
                                                                                            "sys_net_max_in_mbit",
                                                                                            "net_max_in_mbit",
                                                                                            "1000.0"),
                                                   NULL);
        sysusage->net_max_out_mbit = g_ascii_strtod(sysusage_plugin_read_entry_with_legacy(rc,
                                                                                             "sys_net_max_out_mbit",
                                                                                             "net_max_out_mbit",
                                                                                             "1000.0"),
                                                    NULL);

        sysusage_plugin_sanitize_loaded_settings(sysusage);

        sysusage_plugin_parse_color_or_default(&sysusage->mem_color, mem_color, DEFAULT_MEM_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->swap_color, swap_color, DEFAULT_SWAP_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->net_in_color, net_in_color, DEFAULT_NET_IN_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->net_out_color, net_out_color, DEFAULT_NET_OUT_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->background_color, background_color, DEFAULT_BACKGROUND_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->text_color, text_color, DEFAULT_TEXT_COLOR);
        sysusage_plugin_parse_color_or_default(&sysusage->separator_color, separator_color, DEFAULT_SEPARATOR_COLOR);

        g_free(sysusage->mem_label);
        g_free(sysusage->swap_label);
        g_free(sysusage->net_device);
        g_free(sysusage->net_in_label);
        g_free(sysusage->net_out_label);
        g_free(sysusage->text_font_family);
        sysusage->mem_label = g_strdup(mem_label);
        sysusage->swap_label = g_strdup(swap_label);
        sysusage->net_device = g_strdup(net_device);
        sysusage->net_in_label = g_strdup((net_in_label != NULL && net_in_label[0] != '\0') ? net_in_label : legacy_net_label);
        sysusage->net_out_label = g_strdup((net_out_label != NULL && net_out_label[0] != '\0') ? net_out_label : legacy_net_label);
        sysusage->text_font_family = g_strdup((text_font != NULL && text_font[0] != '\0')
                                                  ? text_font
                                                  : ((text_font_family != NULL && text_font_family[0] != '\0')
                                                         ? text_font_family
                                                         : DEFAULT_TEXT_FONT));

        xfce_rc_close(rc);
    }

    g_free(filename);
}
