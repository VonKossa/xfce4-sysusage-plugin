#ifndef SYSUSAGE_PLUGIN_H
#define SYSUSAGE_PLUGIN_H

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

#define DEFAULT_BAR_WIDTH 64
#define DEFAULT_BAR_HEIGHT 10
#define DEFAULT_BAR_SPACING 4
#define DEFAULT_VERTICAL_BARS TRUE
#define DEFAULT_VERTICAL_TEXT_UNDER_BARS FALSE
#define DEFAULT_TEXT_PRIMARY_SIDE_LEFT TRUE
#define DEFAULT_TEXT_PRIMARY_SIDE_RIGHT TRUE
#define DEFAULT_TEXT_FONT "Sans 10"
#define DEFAULT_TEXT_COLOR "#FFFFFF"
#define DEFAULT_TEXT_PADDING 4
#define DEFAULT_BACKGROUND_COLOR "#E1DEDB"

#define DEFAULT_MEM_LABEL "Memory"
#define DEFAULT_SWAP_LABEL "Swap"
#define DEFAULT_SHOW_MEM_TEXT TRUE
#define DEFAULT_SHOW_SWAP_TEXT TRUE
#define DEFAULT_MEM_COLOR "#00AD00"
#define DEFAULT_SWAP_COLOR "#FF2C2C"

#define DEFAULT_NET_DEVICE ""
#define DEFAULT_NET_IN_LABEL "Incoming"
#define DEFAULT_NET_OUT_LABEL "Outgoing"
#define DEFAULT_SHOW_NET_IN_TEXT TRUE
#define DEFAULT_SHOW_NET_OUT_TEXT TRUE
#define DEFAULT_NET_IN_COLOR "#00AD00"
#define DEFAULT_NET_OUT_COLOR "#FF2C2C"
#define DEFAULT_NET_MAX_IN_MBIT 1000.0
#define DEFAULT_NET_MAX_OUT_MBIT 1000.0
#define DEFAULT_NET_INTERVAL_SECONDS 0.25

#define DEFAULT_SEPARATOR_SPACING 8
#define DEFAULT_SEPARATOR_COLOR "#3B5868"
#define DEFAULT_SEPARATOR_LINE_LENGTH 24
#define DEFAULT_SEPARATOR_LINE_WIDTH 1
#define DEFAULT_SEPARATOR_TRANSPARENT FALSE
#define DEFAULT_SEPARATOR_STYLE 0
#define DEFAULT_DISABLE_MEMUSAGE FALSE
#define DEFAULT_DISABLE_NETUSAGE FALSE

#ifndef SYSUSAGE_PLUGIN_VERSION
#define SYSUSAGE_PLUGIN_VERSION "2.14"
#endif

typedef struct
{
    /* Core widget graph and tooltip widgets owned by the plugin instance. */
    XfcePanelPlugin *plugin;
    GtkWidget *container;
    GtkWidget *mem_area;
    GtkWidget *separator_area;
    GtkWidget *net_area;
    GtkWidget *mem_tooltip_widget;
    GtkWidget *net_tooltip_widget;

    GdkRGBA mem_color;
    GdkRGBA swap_color;
    GdkRGBA net_in_color;
    GdkRGBA net_out_color;
    GdkRGBA background_color;
    GdkRGBA text_color;
    GdkRGBA separator_color;

    guint bar_width;
    guint bar_height;
    guint bar_spacing;
    gboolean vertical_bars;
    gboolean vertical_text_under_bars;
    gboolean text_primary_side_left;
    gboolean text_primary_side_right;
    gchar *text_font_family;
    guint text_padding;
    guint separator_spacing;
    guint separator_line_length;
    guint separator_line_width;
    guint vertical_under_text_slot_width;
    gboolean separator_transparent;
    guint separator_style;

    /* Runtime percentages (0..100) consumed by render functions. */
    gdouble mem_percent;
    gdouble swap_percent;
    gdouble net_in_percent;
    gdouble net_out_percent;

    gchar *mem_label;
    gchar *swap_label;
    gboolean show_mem_text;
    gboolean show_swap_text;
    gboolean disable_memusage;
    gboolean disable_netusage;

    gchar *net_device;
    gchar *net_in_label;
    gchar *net_out_label;
    gboolean show_net_in_text;
    gboolean show_net_out_text;
    gdouble net_interval_seconds;
    gdouble net_max_in_mbit;
    gdouble net_max_out_mbit;

    gchar *mem_tooltip_text;
    gchar *net_tooltip_text;

    /* Periodic sampling state for throughput calculations. */
    guint mem_timer_id;
    guint net_timer_id;
    guint64 prev_rx_bytes;
    guint64 prev_tx_bytes;
    gint64 prev_net_sample_us;
    gboolean has_prev_net_sample;
} SysusagePlugin;

/* render/layout */
void sysusage_plugin_update_size_request(SysusagePlugin *sysusage);
void sysusage_plugin_update_visibility(SysusagePlugin *sysusage);
gboolean sysusage_plugin_query_tooltip(GtkWidget *widget,
                                       gint x,
                                       gint y,
                                       gboolean keyboard_mode,
                                       GtkTooltip *tooltip,
                                       gpointer user_data);
gboolean sysusage_plugin_draw_mem(GtkWidget *widget, cairo_t *cr, gpointer user_data);
gboolean sysusage_plugin_draw_net(GtkWidget *widget, cairo_t *cr, gpointer user_data);
gboolean sysusage_plugin_draw_separator(GtkWidget *widget, cairo_t *cr, gpointer user_data);

/* metrics */
gboolean sysusage_plugin_read_mem_values(SysusagePlugin *sysusage);
gboolean sysusage_plugin_read_net_values(SysusagePlugin *sysusage);
void sysusage_plugin_update_mem_timer(SysusagePlugin *sysusage);
void sysusage_plugin_update_net_timer(SysusagePlugin *sysusage);

/* settings */
void sysusage_plugin_save(SysusagePlugin *sysusage);
void sysusage_plugin_load(SysusagePlugin *sysusage);

/* dialog/actions */
void sysusage_plugin_show_properties(SysusagePlugin *sysusage);
gboolean sysusage_plugin_button_press(GtkWidget *widget, const GdkEventButton *event, gpointer user_data);
void sysusage_plugin_configure(XfcePanelPlugin *plugin, SysusagePlugin *sysusage);
void sysusage_plugin_about(XfcePanelPlugin *plugin, SysusagePlugin *sysusage);

#endif
