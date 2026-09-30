#include "sysusage-plugin.h"

static void
sysusage_plugin_sync_disable_toggles(GtkToggleButton *mem_toggle,
                                     GtkToggleButton *net_toggle)
{
    /* Avoid allowing both sections to be disabled simultaneously in the UI. */
    const gboolean mem_disabled = gtk_toggle_button_get_active(mem_toggle);
    const gboolean net_disabled = gtk_toggle_button_get_active(net_toggle);

    gtk_widget_set_sensitive(GTK_WIDGET(mem_toggle), !net_disabled);
    gtk_widget_set_sensitive(GTK_WIDGET(net_toggle), !mem_disabled);
}

static void
sysusage_plugin_apply_greyed_toggle_css(GtkWidget **toggles,
                                        guint n_toggles)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        "checkbutton.sysusage-greyed-toggle:disabled > check,"
        "checkbutton.sysusage-greyed-toggle:insensitive > check {"
        "  background-image: none;"
        "  box-shadow: none;"
        "  background-color: rgba(220, 220, 220, 1.0);"
        "  border-color: rgba(150, 150, 150, 1.0);"
        "}"
        "checkbutton.sysusage-greyed-toggle:disabled > check:checked,"
        "checkbutton.sysusage-greyed-toggle:insensitive > check:checked {"
        "  background-image: none;"
        "  box-shadow: none;"
        "  background-color: rgba(200, 200, 200, 1.0);"
        "}";
    guint i;

    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    for (i = 0; i < n_toggles; ++i)
    {
        gtk_style_context_add_class(gtk_widget_get_style_context(toggles[i]), "sysusage-greyed-toggle");
        gtk_style_context_add_provider(gtk_widget_get_style_context(toggles[i]),
                                       GTK_STYLE_PROVIDER(provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    g_object_unref(provider);
}

static void
sysusage_plugin_apply_disable_toggle_css(GtkWidget *mem_toggle,
                                         GtkWidget *net_toggle)
{
    GtkWidget *toggles[] = { mem_toggle, net_toggle };

    sysusage_plugin_apply_greyed_toggle_css(toggles, G_N_ELEMENTS(toggles));
}


typedef struct
{
    GtkWidget *horizontal_label;
    GtkWidget *horizontal_check;
    GtkWidget *vertical_text_under_bars_label;
    GtkWidget *vertical_text_under_bars_check;
    GtkWidget *left_text_side_label;
    GtkWidget *left_text_side_check;
    GtkWidget *right_text_side_label;
    GtkWidget *right_text_side_check;
} SysusageTextPlacementControls;

static void
sysusage_plugin_update_text_placement_sensitivity(const SysusageTextPlacementControls *controls)
{
    const gboolean horizontal_bars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->horizontal_check));
    const gboolean left_side_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->left_text_side_check));
    const gboolean right_side_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->right_text_side_check));
    const gboolean any_side_text = left_side_text || right_side_text;
    gboolean text_under_vertical_bars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->vertical_text_under_bars_check));
    const gboolean under_controls_sensitive = !horizontal_bars && !any_side_text;

    if (!under_controls_sensitive && text_under_vertical_bars)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls->vertical_text_under_bars_check), FALSE);
        text_under_vertical_bars = FALSE;
    }

    const gboolean horizontal_controls_sensitive = !text_under_vertical_bars;
    const gboolean side_controls_sensitive = horizontal_bars || !text_under_vertical_bars;

    gtk_widget_set_sensitive(controls->horizontal_label, horizontal_controls_sensitive);
    gtk_widget_set_sensitive(controls->horizontal_check, horizontal_controls_sensitive);
    gtk_widget_set_sensitive(controls->vertical_text_under_bars_label, under_controls_sensitive);
    gtk_widget_set_sensitive(controls->vertical_text_under_bars_check, under_controls_sensitive);
    gtk_widget_set_sensitive(controls->left_text_side_label, side_controls_sensitive);
    gtk_widget_set_sensitive(controls->left_text_side_check, side_controls_sensitive);
    gtk_widget_set_sensitive(controls->right_text_side_label, side_controls_sensitive);
    gtk_widget_set_sensitive(controls->right_text_side_check, side_controls_sensitive);
}

static void
sysusage_plugin_text_placement_toggled(GtkToggleButton *toggle,
                                       gpointer user_data)
{
    const SysusageTextPlacementControls *controls = user_data;

    (void) toggle;

    sysusage_plugin_update_text_placement_sensitivity(controls);
}

static void
sysusage_plugin_disable_mem_toggled(GtkToggleButton *toggle,
                                    gpointer user_data)
{
    GtkToggleButton *net_toggle = GTK_TOGGLE_BUTTON(user_data);

    sysusage_plugin_sync_disable_toggles(toggle, net_toggle);
}

static void
sysusage_plugin_disable_net_toggled(GtkToggleButton *toggle,
                                    gpointer user_data)
{
    GtkToggleButton *mem_toggle = GTK_TOGGLE_BUTTON(user_data);

    sysusage_plugin_sync_disable_toggles(mem_toggle, toggle);
}

static void
sysusage_plugin_update_separator_control_sensitivity(GtkComboBox *style_combo,
                                                     GtkWidget *length_label,
                                                     GtkWidget *length_spin,
                                                     GtkWidget *width_label,
                                                     GtkWidget *width_spin)
{
    const gboolean handle_style = (gtk_combo_box_get_active(style_combo) == 2);
    const gboolean sensitive = !handle_style;

    gtk_widget_set_sensitive(length_label, sensitive);
    gtk_widget_set_sensitive(length_spin, sensitive);
    gtk_widget_set_sensitive(width_label, sensitive);
    gtk_widget_set_sensitive(width_spin, sensitive);
}

static void
sysusage_plugin_separator_style_changed(GtkComboBox *style_combo,
                                        gpointer user_data)
{
    GtkWidget **controls = user_data;

    sysusage_plugin_update_separator_control_sensitivity(style_combo,
                                                         controls[0],
                                                         controls[1],
                                                         controls[2],
                                                         controls[3]);
}

static gint
sysusage_plugin_compare_strings(gconstpointer a,
                                gconstpointer b)
{
    return g_strcmp0((const gchar *) a, (const gchar *) b);
}

static gboolean
sysusage_plugin_string_list_contains(GList *strings,
                                     const gchar *needle)
{
    GList *iter;

    if (needle == NULL || needle[0] == '\0')
        return FALSE;

    for (iter = strings; iter != NULL; iter = iter->next)
    {
        if (g_strcmp0((const gchar *) iter->data, needle) == 0)
            return TRUE;
    }

    return FALSE;
}

static gboolean
sysusage_plugin_net_interface_name_is_listable(const gchar *name)
{
    const gchar *p;

    if (name == NULL || name[0] == '\0')
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

static void
sysusage_plugin_populate_net_device_combo(GtkComboBoxText *combo,
                                          const gchar *selected_device)
{
    GDir *net_dir;
    GError *error = NULL;
    GList *interfaces = NULL;
    GList *iter;
    const gchar *entry_name;
    gint selected_index = -1;
    gint index = 0;

    gtk_combo_box_text_remove_all(combo);

    net_dir = g_dir_open("/sys/class/net", 0, &error);
    if (net_dir != NULL)
    {
        while ((entry_name = g_dir_read_name(net_dir)) != NULL)
        {
            if (sysusage_plugin_net_interface_name_is_listable(entry_name))
                interfaces = g_list_prepend(interfaces, g_strdup(entry_name));
        }
        g_dir_close(net_dir);
    }
    else
    {
        g_clear_error(&error);
    }

    if (selected_device != NULL
        && selected_device[0] != '\0'
        && !sysusage_plugin_string_list_contains(interfaces, selected_device))
    {
        interfaces = g_list_prepend(interfaces, g_strdup(selected_device));
    }

    interfaces = g_list_sort(interfaces, sysusage_plugin_compare_strings);

    for (iter = interfaces; iter != NULL; iter = iter->next)
    {
        const gchar *name = iter->data;

        gtk_combo_box_text_append_text(combo, name);
        if (g_strcmp0(name, selected_device) == 0)
            selected_index = index;
        index++;
    }

    if (selected_index >= 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), selected_index);
    else if (gtk_combo_box_get_has_entry(GTK_COMBO_BOX(combo)))
    {
        GtkWidget *entry = gtk_bin_get_child(GTK_BIN(combo));

        gtk_entry_set_text(GTK_ENTRY(entry), selected_device != NULL ? selected_device : "");
    }

    g_list_free_full(interfaces, g_free);
}

static void
sysusage_plugin_net_device_refresh_clicked(GtkButton *button,
                                           gpointer user_data)
{
    GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(user_data);
    GtkWidget *entry = gtk_bin_get_child(GTK_BIN(combo));
    gchar *current_device;

    (void) button;

    current_device = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    sysusage_plugin_populate_net_device_combo(combo, current_device);
    g_free(current_device);
}

typedef struct
{
    GtkWidget *mem_button;
    GtkWidget *swap_button;
    GtkWidget *net_in_button;
    GtkWidget *net_out_button;
    GtkWidget *background_button;
    GtkWidget *text_button;
    GtkWidget *separator_button;

    GtkWidget *height_spin;
    GtkWidget *width_spin;
    GtkWidget *spacing_spin;
    GtkWidget *separator_spacing_spin;
    GtkWidget *separator_length_spin;
    GtkWidget *separator_width_spin;
    GtkWidget *separator_transparent_check;
    GtkWidget *separator_style_combo;
    GtkWidget *horizontal_check;
    GtkWidget *vertical_text_under_bars_check;
    GtkWidget *left_text_side_check;
    GtkWidget *right_text_side_check;
    GtkWidget *font_button;
    GtkWidget *text_padding_spin;

    GtkWidget *mem_name_entry;
    GtkWidget *swap_name_entry;
    GtkWidget *mem_text_check;
    GtkWidget *swap_text_check;
    GtkWidget *disable_memusage_check;

    GtkWidget *net_device_entry;
    GtkWidget *net_in_name_entry;
    GtkWidget *net_out_name_entry;
    GtkWidget *net_in_text_check;
    GtkWidget *net_out_text_check;
    GtkWidget *net_interval_spin;
    GtkWidget *net_max_in_spin;
    GtkWidget *net_max_out_spin;
    GtkWidget *disable_netusage_check;
} SysusagePropertiesWidgets;

static void
sysusage_plugin_properties_collect_widgets(GtkDialog *dialog,
                                           SysusagePropertiesWidgets *widgets)
{
    /* Collect all controls from dialog object data for batch processing. */
    widgets->mem_button = g_object_get_data(G_OBJECT(dialog), "mem-color");
    widgets->swap_button = g_object_get_data(G_OBJECT(dialog), "swap-color");
    widgets->net_in_button = g_object_get_data(G_OBJECT(dialog), "net-in-color");
    widgets->net_out_button = g_object_get_data(G_OBJECT(dialog), "net-out-color");
    widgets->background_button = g_object_get_data(G_OBJECT(dialog), "background-color");
    widgets->text_button = g_object_get_data(G_OBJECT(dialog), "text-color");
    widgets->separator_button = g_object_get_data(G_OBJECT(dialog), "separator-color");

    widgets->height_spin = g_object_get_data(G_OBJECT(dialog), "height-spin");
    widgets->width_spin = g_object_get_data(G_OBJECT(dialog), "width-spin");
    widgets->spacing_spin = g_object_get_data(G_OBJECT(dialog), "spacing-spin");
    widgets->separator_spacing_spin = g_object_get_data(G_OBJECT(dialog), "separator-spacing-spin");
    widgets->separator_length_spin = g_object_get_data(G_OBJECT(dialog), "separator-length-spin");
    widgets->separator_width_spin = g_object_get_data(G_OBJECT(dialog), "separator-width-spin");
    widgets->separator_transparent_check = g_object_get_data(G_OBJECT(dialog), "separator-transparent-check");
    widgets->separator_style_combo = g_object_get_data(G_OBJECT(dialog), "separator-style-combo");
    widgets->horizontal_check = g_object_get_data(G_OBJECT(dialog), "horizontal-check");
    widgets->vertical_text_under_bars_check = g_object_get_data(G_OBJECT(dialog), "vertical-text-under-bars-check");
    widgets->left_text_side_check = g_object_get_data(G_OBJECT(dialog), "left-text-side-check");
    widgets->right_text_side_check = g_object_get_data(G_OBJECT(dialog), "right-text-side-check");
    widgets->font_button = g_object_get_data(G_OBJECT(dialog), "font-button");
    widgets->text_padding_spin = g_object_get_data(G_OBJECT(dialog), "text-padding-spin");

    widgets->mem_name_entry = g_object_get_data(G_OBJECT(dialog), "mem-name-entry");
    widgets->swap_name_entry = g_object_get_data(G_OBJECT(dialog), "swap-name-entry");
    widgets->mem_text_check = g_object_get_data(G_OBJECT(dialog), "mem-text-check");
    widgets->swap_text_check = g_object_get_data(G_OBJECT(dialog), "swap-text-check");
    widgets->disable_memusage_check = g_object_get_data(G_OBJECT(dialog), "disable-memusage-check");

    widgets->net_device_entry = g_object_get_data(G_OBJECT(dialog), "net-device-entry");
    widgets->net_in_name_entry = g_object_get_data(G_OBJECT(dialog), "net-in-name-entry");
    widgets->net_out_name_entry = g_object_get_data(G_OBJECT(dialog), "net-out-name-entry");
    widgets->net_in_text_check = g_object_get_data(G_OBJECT(dialog), "net-in-text-check");
    widgets->net_out_text_check = g_object_get_data(G_OBJECT(dialog), "net-out-text-check");
    widgets->net_interval_spin = g_object_get_data(G_OBJECT(dialog), "net-interval-spin");
    widgets->net_max_in_spin = g_object_get_data(G_OBJECT(dialog), "net-max-in-spin");
    widgets->net_max_out_spin = g_object_get_data(G_OBJECT(dialog), "net-max-out-spin");
    widgets->disable_netusage_check = g_object_get_data(G_OBJECT(dialog), "disable-netusage-check");
}

static void
sysusage_plugin_set_string_or_default(gchar **target,
                                      const gchar *value,
                                      const gchar *default_value)
{
    g_free(*target);

    if (value == NULL || value[0] == '\0')
        *target = g_strdup(default_value);
    else
        *target = g_strdup(value);
}

static void
sysusage_plugin_apply_visual_settings(SysusagePlugin *sysusage,
                                      const SysusagePropertiesWidgets *widgets)
{
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->mem_button), &sysusage->mem_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->swap_button), &sysusage->swap_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->net_in_button), &sysusage->net_in_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->net_out_button), &sysusage->net_out_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->background_button), &sysusage->background_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->text_button), &sysusage->text_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widgets->separator_button), &sysusage->separator_color);

    sysusage->bar_height = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->height_spin));
    sysusage->bar_width = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->width_spin));
    sysusage->bar_spacing = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->spacing_spin));
    sysusage->separator_spacing = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->separator_spacing_spin));
    sysusage->separator_line_length = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->separator_length_spin));
    sysusage->separator_line_width = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->separator_width_spin));
    sysusage->vertical_bars = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->horizontal_check));
    sysusage->vertical_text_under_bars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->vertical_text_under_bars_check));
    sysusage->text_primary_side_left = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->left_text_side_check));
    sysusage->text_primary_side_right = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->right_text_side_check));
    if (sysusage->vertical_text_under_bars && (!sysusage->vertical_bars || sysusage->text_primary_side_left || sysusage->text_primary_side_right))
        sysusage->vertical_text_under_bars = FALSE;
    const gchar *font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(widgets->font_button));
    sysusage_plugin_set_string_or_default(&sysusage->text_font_family, font_name, DEFAULT_TEXT_FONT);
    sysusage->text_padding = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets->text_padding_spin));
}

static void
sysusage_plugin_apply_separator_settings(SysusagePlugin *sysusage,
                                         const SysusagePropertiesWidgets *widgets)
{
    sysusage->separator_transparent = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->separator_transparent_check));
    sysusage->separator_style = (guint) gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->separator_style_combo));
}

static void
sysusage_plugin_apply_toggle_settings(SysusagePlugin *sysusage,
                                      const SysusagePropertiesWidgets *widgets)
{
    sysusage->show_mem_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->mem_text_check));
    sysusage->show_swap_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->swap_text_check));
    sysusage->show_net_in_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->net_in_text_check));
    sysusage->show_net_out_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->net_out_text_check));
    sysusage->disable_memusage = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->disable_memusage_check));
    sysusage->disable_netusage = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->disable_netusage_check));

    if (sysusage->disable_memusage && sysusage->disable_netusage)
        sysusage->disable_netusage = FALSE;
}

static void
sysusage_plugin_apply_name_settings(SysusagePlugin *sysusage,
                                    const SysusagePropertiesWidgets *widgets)
{
    sysusage_plugin_set_string_or_default(&sysusage->mem_label,
                                          gtk_entry_get_text(GTK_ENTRY(widgets->mem_name_entry)),
                                          DEFAULT_MEM_LABEL);
    sysusage_plugin_set_string_or_default(&sysusage->swap_label,
                                          gtk_entry_get_text(GTK_ENTRY(widgets->swap_name_entry)),
                                          DEFAULT_SWAP_LABEL);
    sysusage_plugin_set_string_or_default(&sysusage->net_device,
                                          gtk_entry_get_text(GTK_ENTRY(widgets->net_device_entry)),
                                          DEFAULT_NET_DEVICE);
    sysusage_plugin_set_string_or_default(&sysusage->net_in_label,
                                          gtk_entry_get_text(GTK_ENTRY(widgets->net_in_name_entry)),
                                          DEFAULT_NET_IN_LABEL);
    sysusage_plugin_set_string_or_default(&sysusage->net_out_label,
                                          gtk_entry_get_text(GTK_ENTRY(widgets->net_out_name_entry)),
                                          DEFAULT_NET_OUT_LABEL);
}

static void
sysusage_plugin_apply_net_scaling_settings(SysusagePlugin *sysusage,
                                           const SysusagePropertiesWidgets *widgets)
{
    sysusage->net_interval_seconds = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->net_interval_spin));
    sysusage->net_max_in_mbit = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->net_max_in_spin));
    sysusage->net_max_out_mbit = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->net_max_out_spin));
}

static void
sysusage_plugin_apply_properties_changes(SysusagePlugin *sysusage,
                                         const SysusagePropertiesWidgets *widgets)
{
    /* Apply user edits, then refresh layout, timers and sampled values. */
    sysusage_plugin_apply_visual_settings(sysusage, widgets);
    sysusage_plugin_apply_separator_settings(sysusage, widgets);
    sysusage_plugin_apply_toggle_settings(sysusage, widgets);
    sysusage_plugin_apply_name_settings(sysusage, widgets);
    sysusage_plugin_apply_net_scaling_settings(sysusage, widgets);

    sysusage->has_prev_net_sample = FALSE;
    sysusage_plugin_update_visibility(sysusage);
    sysusage_plugin_update_size_request(sysusage);
    sysusage_plugin_update_mem_timer(sysusage);
    sysusage_plugin_update_net_timer(sysusage);
    sysusage_plugin_save(sysusage);
    if (!sysusage->disable_memusage)
        sysusage_plugin_read_mem_values(sysusage);
    if (!sysusage->disable_netusage)
        sysusage_plugin_read_net_values(sysusage);

    gtk_widget_queue_draw(sysusage->separator_area);
}

static void
sysusage_plugin_properties_response(GtkDialog *dialog,
                                    gint response,
                                    gpointer user_data)
{
    if (response == GTK_RESPONSE_OK)
    {
        SysusagePlugin *sysusage = user_data;
        SysusagePropertiesWidgets widgets;

        sysusage_plugin_properties_collect_widgets(dialog, &widgets);
        sysusage_plugin_apply_properties_changes(sysusage, &widgets);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static GtkWidget *
sysusage_plugin_new_grid(void)
{
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
    return grid;
}

static GtkWidget *
sysusage_plugin_new_preference_label(const gchar *text)
{
    GtkWidget *label = gtk_label_new(text);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);

    return label;
}

void
sysusage_plugin_show_properties(SysusagePlugin *sysusage)
{
    /* Build a notebook-based properties dialog with general/mem/net tabs. */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Sysusage Properties"),
                                                     NULL,
                                                     GTK_DIALOG_MODAL,
                                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                     _("_OK"), GTK_RESPONSE_OK,
                                                     NULL);
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(sysusage->plugin));
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *notebook = gtk_notebook_new();

    GtkWidget *general_grid = sysusage_plugin_new_grid();
    GtkWidget *mem_grid = sysusage_plugin_new_grid();
    GtkWidget *net_grid = sysusage_plugin_new_grid();

    GtkWidget *background_color_label = sysusage_plugin_new_preference_label(_("Bar background color:"));
    GtkWidget *text_color_label = sysusage_plugin_new_preference_label(_("Text color:"));
    GtkWidget *separator_color_label = sysusage_plugin_new_preference_label(_("Separator line color:"));
    GtkWidget *height_label = sysusage_plugin_new_preference_label(_("Bar thickness:"));
    GtkWidget *width_label = sysusage_plugin_new_preference_label(_("Bar length:"));
    GtkWidget *spacing_label = sysusage_plugin_new_preference_label(_("Bar spacing:"));
    GtkWidget *separator_spacing_label = sysusage_plugin_new_preference_label(_("Section separator spacing:"));
    GtkWidget *separator_length_label = sysusage_plugin_new_preference_label(_("Separator line length:"));
    GtkWidget *separator_width_label = sysusage_plugin_new_preference_label(_("Separator line width:"));
    GtkWidget *separator_transparent_label = sysusage_plugin_new_preference_label(_("Transparent separator:"));
    GtkWidget *separator_style_label = sysusage_plugin_new_preference_label(_("Separator style:"));
    GtkWidget *horizontal_label = sysusage_plugin_new_preference_label(_("Horizontal bars:"));
    GtkWidget *vertical_text_under_bars_label = sysusage_plugin_new_preference_label(_("Text under vertical bars:"));
    GtkWidget *left_text_side_label = sysusage_plugin_new_preference_label(_("Left bar text side (left/upper):"));
    GtkWidget *right_text_side_label = sysusage_plugin_new_preference_label(_("Right bar text side (left/upper):"));
    GtkWidget *font_label = sysusage_plugin_new_preference_label(_("Text font:"));
    GtkWidget *text_padding_label = sysusage_plugin_new_preference_label(_("Text distance to bar:"));

    GtkWidget *background_color_button = gtk_color_button_new();
    GtkWidget *text_color_button = gtk_color_button_new();
    GtkWidget *separator_color_button = gtk_color_button_new();
    GtkWidget *height_spin = gtk_spin_button_new_with_range(2, 64, 1);
    GtkWidget *width_spin = gtk_spin_button_new_with_range(16, 512, 1);
    GtkWidget *spacing_spin = gtk_spin_button_new_with_range(0, 64, 1);
    GtkWidget *separator_spacing_spin = gtk_spin_button_new_with_range(0, 64, 1);
    GtkWidget *separator_length_spin = gtk_spin_button_new_with_range(1, 512, 1);
    GtkWidget *separator_width_spin = gtk_spin_button_new_with_range(1, 32, 1);
    GtkWidget *separator_transparent_check = gtk_check_button_new();
    GtkWidget *separator_style_combo = gtk_combo_box_text_new();
    GtkWidget *horizontal_check = gtk_check_button_new();
    GtkWidget *vertical_text_under_bars_check = gtk_check_button_new();
    GtkWidget *left_text_side_check = gtk_check_button_new();
    GtkWidget *right_text_side_check = gtk_check_button_new();
    GtkWidget *font_button = gtk_font_button_new();
    GtkWidget *text_padding_spin = gtk_spin_button_new_with_range(0, 64, 1);

    GtkWidget *disable_memusage_label = sysusage_plugin_new_preference_label(_("Turn off Memusage:"));
    GtkWidget *mem_color_label = sysusage_plugin_new_preference_label(_("Memory bar color:"));
    GtkWidget *swap_color_label = sysusage_plugin_new_preference_label(_("Swap bar color:"));
    GtkWidget *mem_name_label = sysusage_plugin_new_preference_label(_("Memory bar text:"));
    GtkWidget *swap_name_label = sysusage_plugin_new_preference_label(_("Swap bar text:"));
    GtkWidget *mem_color_button = gtk_color_button_new();
    GtkWidget *swap_color_button = gtk_color_button_new();
    GtkWidget *mem_name_entry = gtk_entry_new();
    GtkWidget *swap_name_entry = gtk_entry_new();
    GtkWidget *mem_text_check = gtk_check_button_new();
    GtkWidget *swap_text_check = gtk_check_button_new();
    GtkWidget *disable_memusage_check = gtk_check_button_new();

    GtkWidget *disable_netusage_label = sysusage_plugin_new_preference_label(_("Turn off Netusage:"));
    GtkWidget *net_in_color_label = sysusage_plugin_new_preference_label(_("Network incoming color:"));
    GtkWidget *net_out_color_label = sysusage_plugin_new_preference_label(_("Network outgoing color:"));
    GtkWidget *net_device_label = sysusage_plugin_new_preference_label(_("Network device:"));
    GtkWidget *net_in_name_label = sysusage_plugin_new_preference_label(_("Incoming bar name:"));
    GtkWidget *net_out_name_label = sysusage_plugin_new_preference_label(_("Outgoing bar name:"));
    GtkWidget *net_interval_label = sysusage_plugin_new_preference_label(_("Update interval (s):"));
    GtkWidget *net_max_in_label = sysusage_plugin_new_preference_label(_("Maximum incoming (Mbit/s):"));
    GtkWidget *net_max_out_label = sysusage_plugin_new_preference_label(_("Maximum outgoing (Mbit/s):"));

    GtkWidget *net_in_color_button = gtk_color_button_new();
    GtkWidget *net_out_color_button = gtk_color_button_new();
    GtkWidget *net_device_combo = gtk_combo_box_text_new_with_entry();
    GtkWidget *net_device_entry = gtk_bin_get_child(GTK_BIN(net_device_combo));
    GtkWidget *net_device_refresh_button = gtk_button_new_with_label(_("Refresh"));
    GtkWidget *net_in_name_entry = gtk_entry_new();
    GtkWidget *net_out_name_entry = gtk_entry_new();
    GtkWidget *net_in_text_check = gtk_check_button_new();
    GtkWidget *net_out_text_check = gtk_check_button_new();
    GtkWidget *net_interval_spin = gtk_spin_button_new_with_range(0.05, 5.0, 0.05);
    GtkWidget *net_max_in_spin = gtk_spin_button_new_with_range(1.0, 100000.0, 1.0);
    GtkWidget *net_max_out_spin = gtk_spin_button_new_with_range(1.0, 100000.0, 1.0);
    GtkWidget *disable_netusage_check = gtk_check_button_new();

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_color_button), &sysusage->background_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(text_color_button), &sysusage->text_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(separator_color_button), &sysusage->separator_color);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(height_spin), sysusage->bar_height);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_spin), sysusage->bar_width);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spacing_spin), sysusage->bar_spacing);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_spacing_spin), sysusage->separator_spacing);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_length_spin), sysusage->separator_line_length);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_width_spin), sysusage->separator_line_width);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(separator_transparent_check), sysusage->separator_transparent);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator_style_combo), _("Line"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator_style_combo), _("Dots"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator_style_combo), _("Handle"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(separator_style_combo), (gint) CLAMP((gint) sysusage->separator_style, 0, 2));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(horizontal_check), !sysusage->vertical_bars);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vertical_text_under_bars_check), sysusage->vertical_text_under_bars);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(left_text_side_check), sysusage->text_primary_side_left);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(right_text_side_check), sysusage->text_primary_side_right);
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(font_button), sysusage->text_font_family);
    gtk_font_button_set_use_size(GTK_FONT_BUTTON(font_button), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(text_padding_spin), sysusage->text_padding);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(mem_color_button), &sysusage->mem_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(swap_color_button), &sysusage->swap_color);
    gtk_entry_set_text(GTK_ENTRY(mem_name_entry), sysusage->mem_label);
    gtk_entry_set_text(GTK_ENTRY(swap_name_entry), sysusage->swap_label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mem_text_check), sysusage->show_mem_text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(swap_text_check), sysusage->show_swap_text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_memusage_check), sysusage->disable_memusage);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(net_in_color_button), &sysusage->net_in_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(net_out_color_button), &sysusage->net_out_color);
    sysusage_plugin_populate_net_device_combo(GTK_COMBO_BOX_TEXT(net_device_combo), sysusage->net_device);
    gtk_widget_set_hexpand(net_device_combo, TRUE);
    gtk_widget_set_tooltip_text(net_device_combo, _("Select a detected interface or type one manually"));
    gtk_widget_set_tooltip_text(net_device_refresh_button, _("Refresh detected network interfaces"));
    gtk_entry_set_text(GTK_ENTRY(net_in_name_entry), sysusage->net_in_label);
    gtk_entry_set_text(GTK_ENTRY(net_out_name_entry), sysusage->net_out_label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(net_in_text_check), sysusage->show_net_in_text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(net_out_text_check), sysusage->show_net_out_text);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(net_interval_spin), sysusage->net_interval_seconds);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(net_max_in_spin), sysusage->net_max_in_mbit);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(net_max_out_spin), sysusage->net_max_out_mbit);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_netusage_check), sysusage->disable_netusage);

    g_signal_connect(disable_memusage_check, "toggled", G_CALLBACK(sysusage_plugin_disable_mem_toggled), disable_netusage_check);
    g_signal_connect(disable_netusage_check, "toggled", G_CALLBACK(sysusage_plugin_disable_net_toggled), disable_memusage_check);
    g_signal_connect(net_device_refresh_button,
                     "clicked",
                     G_CALLBACK(sysusage_plugin_net_device_refresh_clicked),
                     net_device_combo);
    sysusage_plugin_apply_disable_toggle_css(disable_memusage_check, disable_netusage_check);
    sysusage_plugin_sync_disable_toggles(GTK_TOGGLE_BUTTON(disable_memusage_check), GTK_TOGGLE_BUTTON(disable_netusage_check));

    GtkWidget *greyed_toggles[] = {
        horizontal_check,
        vertical_text_under_bars_check,
        left_text_side_check,
        right_text_side_check
    };
    sysusage_plugin_apply_greyed_toggle_css(greyed_toggles, G_N_ELEMENTS(greyed_toggles));

    SysusageTextPlacementControls *text_placement_controls = g_new(SysusageTextPlacementControls, 1);
    text_placement_controls->horizontal_label = horizontal_label;
    text_placement_controls->horizontal_check = horizontal_check;
    text_placement_controls->vertical_text_under_bars_label = vertical_text_under_bars_label;
    text_placement_controls->vertical_text_under_bars_check = vertical_text_under_bars_check;
    text_placement_controls->left_text_side_label = left_text_side_label;
    text_placement_controls->left_text_side_check = left_text_side_check;
    text_placement_controls->right_text_side_label = right_text_side_label;
    text_placement_controls->right_text_side_check = right_text_side_check;
    g_object_set_data_full(G_OBJECT(dialog),
                           "text-placement-controls",
                           text_placement_controls,
                           g_free);
    g_signal_connect(horizontal_check,
                     "toggled",
                     G_CALLBACK(sysusage_plugin_text_placement_toggled),
                     text_placement_controls);
    g_signal_connect(vertical_text_under_bars_check,
                     "toggled",
                     G_CALLBACK(sysusage_plugin_text_placement_toggled),
                     text_placement_controls);
    g_signal_connect(left_text_side_check,
                     "toggled",
                     G_CALLBACK(sysusage_plugin_text_placement_toggled),
                     text_placement_controls);
    g_signal_connect(right_text_side_check,
                     "toggled",
                     G_CALLBACK(sysusage_plugin_text_placement_toggled),
                     text_placement_controls);
    sysusage_plugin_update_text_placement_sensitivity(text_placement_controls);

    GtkWidget **separator_style_controls = g_new(GtkWidget *, 4);
    separator_style_controls[0] = separator_length_label;
    separator_style_controls[1] = separator_length_spin;
    separator_style_controls[2] = separator_width_label;
    separator_style_controls[3] = separator_width_spin;
    g_object_set_data_full(G_OBJECT(separator_style_combo),
                           "separator-style-controls",
                           separator_style_controls,
                           g_free);
    g_signal_connect(separator_style_combo,
                     "changed",
                     G_CALLBACK(sysusage_plugin_separator_style_changed),
                     separator_style_controls);
    sysusage_plugin_update_separator_control_sensitivity(GTK_COMBO_BOX(separator_style_combo),
                                                         separator_length_label,
                                                         separator_length_spin,
                                                         separator_width_label,
                                                         separator_width_spin);

    gtk_grid_attach(GTK_GRID(general_grid), background_color_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), background_color_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), text_color_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), text_color_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_color_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_color_button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), height_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), height_spin, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), width_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), width_spin, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), spacing_label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), spacing_spin, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_spacing_label, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_spacing_spin, 1, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_length_label, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_length_spin, 1, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_width_label, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_width_spin, 1, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_transparent_label, 0, 9, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_transparent_check, 1, 9, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_style_label, 0, 10, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), separator_style_combo, 1, 10, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), horizontal_label, 0, 11, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), horizontal_check, 1, 11, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), vertical_text_under_bars_label, 0, 12, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), vertical_text_under_bars_check, 1, 12, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), left_text_side_label, 0, 13, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), left_text_side_check, 1, 13, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), right_text_side_label, 0, 14, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), right_text_side_check, 1, 14, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), font_label, 0, 15, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), font_button, 1, 15, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), text_padding_label, 0, 16, 1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), text_padding_spin, 1, 16, 1, 1);

    gtk_grid_attach(GTK_GRID(mem_grid), disable_memusage_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), disable_memusage_check, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), mem_color_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), mem_color_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), swap_color_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), swap_color_button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), mem_name_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), mem_text_check, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), mem_name_entry, 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), swap_name_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), swap_text_check, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(mem_grid), swap_name_entry, 2, 4, 1, 1);

    gtk_grid_attach(GTK_GRID(net_grid), disable_netusage_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), disable_netusage_check, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_in_color_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_in_color_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_out_color_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_out_color_button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_device_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_device_combo, 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_device_refresh_button, 3, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_in_name_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_in_text_check, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_in_name_entry, 2, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_out_name_label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_out_text_check, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_out_name_entry, 2, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_interval_label, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_interval_spin, 2, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_max_in_label, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_max_in_spin, 2, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_max_out_label, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(net_grid), net_max_out_spin, 2, 8, 1, 1);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_grid, gtk_label_new(_("General")));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mem_grid, gtk_label_new(_("Memusage")));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), net_grid, gtk_label_new(_("Netusage")));

    gtk_container_add(GTK_CONTAINER(content), notebook);

    g_object_set_data(G_OBJECT(dialog), "mem-color", mem_color_button);
    g_object_set_data(G_OBJECT(dialog), "swap-color", swap_color_button);
    g_object_set_data(G_OBJECT(dialog), "net-in-color", net_in_color_button);
    g_object_set_data(G_OBJECT(dialog), "net-out-color", net_out_color_button);
    g_object_set_data(G_OBJECT(dialog), "background-color", background_color_button);
    g_object_set_data(G_OBJECT(dialog), "text-color", text_color_button);
    g_object_set_data(G_OBJECT(dialog), "separator-color", separator_color_button);

    g_object_set_data(G_OBJECT(dialog), "height-spin", height_spin);
    g_object_set_data(G_OBJECT(dialog), "width-spin", width_spin);
    g_object_set_data(G_OBJECT(dialog), "spacing-spin", spacing_spin);
    g_object_set_data(G_OBJECT(dialog), "separator-spacing-spin", separator_spacing_spin);
    g_object_set_data(G_OBJECT(dialog), "separator-length-spin", separator_length_spin);
    g_object_set_data(G_OBJECT(dialog), "separator-width-spin", separator_width_spin);
    g_object_set_data(G_OBJECT(dialog), "separator-transparent-check", separator_transparent_check);
    g_object_set_data(G_OBJECT(dialog), "separator-style-combo", separator_style_combo);
    g_object_set_data(G_OBJECT(dialog), "horizontal-check", horizontal_check);
    g_object_set_data(G_OBJECT(dialog), "vertical-text-under-bars-check", vertical_text_under_bars_check);
    g_object_set_data(G_OBJECT(dialog), "left-text-side-check", left_text_side_check);
    g_object_set_data(G_OBJECT(dialog), "right-text-side-check", right_text_side_check);
    g_object_set_data(G_OBJECT(dialog), "font-button", font_button);
    g_object_set_data(G_OBJECT(dialog), "text-padding-spin", text_padding_spin);

    g_object_set_data(G_OBJECT(dialog), "mem-name-entry", mem_name_entry);
    g_object_set_data(G_OBJECT(dialog), "swap-name-entry", swap_name_entry);
    g_object_set_data(G_OBJECT(dialog), "mem-text-check", mem_text_check);
    g_object_set_data(G_OBJECT(dialog), "swap-text-check", swap_text_check);
    g_object_set_data(G_OBJECT(dialog), "disable-memusage-check", disable_memusage_check);

    g_object_set_data(G_OBJECT(dialog), "net-device-entry", net_device_entry);
    g_object_set_data(G_OBJECT(dialog), "net-in-name-entry", net_in_name_entry);
    g_object_set_data(G_OBJECT(dialog), "net-out-name-entry", net_out_name_entry);
    g_object_set_data(G_OBJECT(dialog), "net-in-text-check", net_in_text_check);
    g_object_set_data(G_OBJECT(dialog), "net-out-text-check", net_out_text_check);
    g_object_set_data(G_OBJECT(dialog), "net-interval-spin", net_interval_spin);
    g_object_set_data(G_OBJECT(dialog), "net-max-in-spin", net_max_in_spin);
    g_object_set_data(G_OBJECT(dialog), "net-max-out-spin", net_max_out_spin);
    g_object_set_data(G_OBJECT(dialog), "disable-netusage-check", disable_netusage_check);

    if (GTK_IS_WINDOW(toplevel))
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(toplevel));

    g_signal_connect(dialog, "response", G_CALLBACK(sysusage_plugin_properties_response), sysusage);
    gtk_widget_show_all(dialog);
}

gboolean
sysusage_plugin_button_press(GtkWidget *widget,
                             const GdkEventButton *event,
                             gpointer user_data)
{
    (void) widget;

    if (event->button == GDK_BUTTON_PRIMARY)
    {
        SysusagePlugin *sysusage = user_data;

        sysusage_plugin_show_properties(sysusage);
        return TRUE;
    }

    return FALSE;
}

void
sysusage_plugin_configure(XfcePanelPlugin *plugin,
                          SysusagePlugin *sysusage)
{
    (void) plugin;
    sysusage_plugin_show_properties(sysusage);
}

void
sysusage_plugin_about(XfcePanelPlugin *plugin,
                      SysusagePlugin *sysusage)
{
    static const gchar *authors[] = {
        "OpenAI Codex",
        "Tomas Johansson",
        NULL
    };
    GtkWidget *dialog = gtk_about_dialog_new();
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(plugin));

    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "xfce4-sysusage-plugin");
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), "xfce4-sysusage-plugin");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), SYSUSAGE_PLUGIN_VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Graphical representation of memory, swap and network usage"));
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://github.com/tomas-ld/xfce4-sysusage-plugin");
    gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), _("Website"));
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), XFCE_LICENSE_GPL);
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), TRUE);

    if (GTK_IS_WINDOW(toplevel))
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(toplevel));

    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_widget_show(dialog);

    (void) sysusage;
}
