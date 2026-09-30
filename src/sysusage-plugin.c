#include "sysusage-plugin.h"

static void
sysusage_plugin_setup_interactive_area(XfcePanelPlugin *plugin,
                                       GtkWidget *area,
                                       GCallback draw_cb,
                                       SysusagePlugin *sysusage)
{
    /*
     * Register each drawing area as an action widget so it participates in
     * panel context menu handling while still receiving custom draw/click
     * events from this plugin.
     */
    xfce_panel_plugin_add_action_widget(plugin, area);
    gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(area, "draw", draw_cb, sysusage);
    g_signal_connect(area, "button-press-event", G_CALLBACK(sysusage_plugin_button_press), sysusage);
}

static void
sysusage_plugin_free(gpointer user_data)
{
    SysusagePlugin *sysusage = user_data;

    /* Stop periodic samplers before releasing state they may touch. */
    if (sysusage->mem_timer_id != 0)
        g_source_remove(sysusage->mem_timer_id);
    if (sysusage->net_timer_id != 0)
        g_source_remove(sysusage->net_timer_id);

    g_free(sysusage->mem_label);
    g_free(sysusage->swap_label);
    g_free(sysusage->net_device);
    g_free(sysusage->net_in_label);
    g_free(sysusage->net_out_label);
    g_free(sysusage->mem_tooltip_text);
    g_free(sysusage->net_tooltip_text);
    g_free(sysusage->text_font_family);
    if (sysusage->mem_tooltip_widget != NULL)
        g_object_unref(sysusage->mem_tooltip_widget);
    if (sysusage->net_tooltip_widget != NULL)
        g_object_unref(sysusage->net_tooltip_widget);
    g_free(sysusage);
}

static void
sysusage_plugin_construct(XfcePanelPlugin *plugin)
{
    static gsize i18n_initialized = 0;
    SysusagePlugin *sysusage = g_new0(SysusagePlugin, 1);
    if (g_once_init_enter(&i18n_initialized))
    {
        xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
        g_once_init_leave(&i18n_initialized, 1);
    }

    /*
     * The plugin is composed of three drawing areas laid out left-to-right:
     * memory section, separator section, and network section.
     */
    sysusage->plugin = plugin;
    sysusage->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DEFAULT_SEPARATOR_SPACING);
    sysusage->mem_area = gtk_drawing_area_new();
    sysusage->separator_area = gtk_drawing_area_new();
    sysusage->net_area = gtk_drawing_area_new();

    gdk_rgba_parse(&sysusage->mem_color, DEFAULT_MEM_COLOR);
    gdk_rgba_parse(&sysusage->swap_color, DEFAULT_SWAP_COLOR);
    gdk_rgba_parse(&sysusage->net_in_color, DEFAULT_NET_IN_COLOR);
    gdk_rgba_parse(&sysusage->net_out_color, DEFAULT_NET_OUT_COLOR);
    gdk_rgba_parse(&sysusage->background_color, DEFAULT_BACKGROUND_COLOR);
    gdk_rgba_parse(&sysusage->text_color, DEFAULT_TEXT_COLOR);
    gdk_rgba_parse(&sysusage->separator_color, DEFAULT_SEPARATOR_COLOR);

    sysusage->bar_width = DEFAULT_BAR_WIDTH;
    sysusage->bar_height = DEFAULT_BAR_HEIGHT;
    sysusage->bar_spacing = DEFAULT_BAR_SPACING;
    sysusage->separator_spacing = DEFAULT_SEPARATOR_SPACING;
    sysusage->separator_line_length = DEFAULT_SEPARATOR_LINE_LENGTH;
    sysusage->separator_line_width = DEFAULT_SEPARATOR_LINE_WIDTH;
    sysusage->separator_transparent = DEFAULT_SEPARATOR_TRANSPARENT;
    sysusage->separator_style = DEFAULT_SEPARATOR_STYLE;
    sysusage->vertical_bars = DEFAULT_VERTICAL_BARS;
    sysusage->vertical_text_under_bars = DEFAULT_VERTICAL_TEXT_UNDER_BARS;
    sysusage->text_primary_side_left = DEFAULT_TEXT_PRIMARY_SIDE_LEFT;
    sysusage->text_primary_side_right = DEFAULT_TEXT_PRIMARY_SIDE_RIGHT;
    sysusage->text_font_family = g_strdup(DEFAULT_TEXT_FONT);
    sysusage->text_padding = DEFAULT_TEXT_PADDING;

    sysusage->mem_label = g_strdup(DEFAULT_MEM_LABEL);
    sysusage->swap_label = g_strdup(DEFAULT_SWAP_LABEL);
    sysusage->show_mem_text = DEFAULT_SHOW_MEM_TEXT;
    sysusage->show_swap_text = DEFAULT_SHOW_SWAP_TEXT;
    sysusage->disable_memusage = DEFAULT_DISABLE_MEMUSAGE;
    sysusage->disable_netusage = DEFAULT_DISABLE_NETUSAGE;

    sysusage->net_device = g_strdup(DEFAULT_NET_DEVICE);
    sysusage->net_in_label = g_strdup(DEFAULT_NET_IN_LABEL);
    sysusage->net_out_label = g_strdup(DEFAULT_NET_OUT_LABEL);
    sysusage->show_net_in_text = DEFAULT_SHOW_NET_IN_TEXT;
    sysusage->show_net_out_text = DEFAULT_SHOW_NET_OUT_TEXT;
    sysusage->net_interval_seconds = DEFAULT_NET_INTERVAL_SECONDS;
    sysusage->net_max_in_mbit = DEFAULT_NET_MAX_IN_MBIT;
    sysusage->net_max_out_mbit = DEFAULT_NET_MAX_OUT_MBIT;
    sysusage->mem_tooltip_text = g_strdup(_("Loading memory usage..."));
    sysusage->net_tooltip_text = g_strdup(_("Loading network usage..."));

    sysusage->mem_tooltip_widget = gtk_label_new(sysusage->mem_tooltip_text);
    sysusage->net_tooltip_widget = gtk_label_new(sysusage->net_tooltip_text);
    g_object_ref_sink(sysusage->mem_tooltip_widget);
    g_object_ref_sink(sysusage->net_tooltip_widget);

    sysusage_plugin_load(sysusage);
    /* Size depends on orientation, text visibility and section toggles. */
    sysusage_plugin_update_size_request(sysusage);

    gtk_container_add(GTK_CONTAINER(plugin), sysusage->container);
    gtk_box_pack_start(GTK_BOX(sysusage->container), sysusage->mem_area, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sysusage->container), sysusage->separator_area, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sysusage->container), sysusage->net_area, FALSE, FALSE, 0);

    sysusage_plugin_update_visibility(sysusage);

    sysusage_plugin_setup_interactive_area(plugin,
                                           sysusage->mem_area,
                                           G_CALLBACK(sysusage_plugin_draw_mem),
                                           sysusage);
    sysusage_plugin_setup_interactive_area(plugin,
                                           sysusage->separator_area,
                                           G_CALLBACK(sysusage_plugin_draw_separator),
                                           sysusage);
    sysusage_plugin_setup_interactive_area(plugin,
                                           sysusage->net_area,
                                           G_CALLBACK(sysusage_plugin_draw_net),
                                           sysusage);

    g_signal_connect(plugin, "configure-plugin", G_CALLBACK(sysusage_plugin_configure), sysusage);
    g_signal_connect(plugin, "about", G_CALLBACK(sysusage_plugin_about), sysusage);
    g_signal_connect(plugin, "query-tooltip", G_CALLBACK(sysusage_plugin_query_tooltip), sysusage);

    gtk_widget_set_has_tooltip(GTK_WIDGET(plugin), TRUE);

    xfce_panel_plugin_menu_show_configure(plugin);
    xfce_panel_plugin_menu_show_about(plugin);

    sysusage_plugin_update_mem_timer(sysusage);
    /* Net interval is user-configurable, so timer setup is delegated. */
    sysusage_plugin_update_net_timer(sysusage);

    if (!sysusage->disable_memusage)
        sysusage_plugin_read_mem_values(sysusage);
    if (!sysusage->disable_netusage)
        sysusage_plugin_read_net_values(sysusage);

    g_object_set_data_full(G_OBJECT(plugin), "sysusage-data", sysusage, sysusage_plugin_free);

    gtk_widget_show_all(GTK_WIDGET(plugin));
    /* show_all() can reveal child areas hidden by loaded settings. */
    sysusage_plugin_update_visibility(sysusage);
}

XFCE_PANEL_PLUGIN_REGISTER(sysusage_plugin_construct);
