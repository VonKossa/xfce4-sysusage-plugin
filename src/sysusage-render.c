#include <math.h>

#include <pango/pangocairo.h>

#include "sysusage-plugin.h"

static void
sysusage_plugin_get_section_text_dimensions(GtkWidget *widget,
                                            const gchar *label_a,
                                            gboolean show_a,
                                            const gchar *label_b,
                                            gboolean show_b,
                                            const gchar *font_desc_text,
                                            gint *text_width,
                                            gint *text_height)
{
    const gchar *sample;
    PangoLayout *layout;
    PangoFontDescription *font_desc;

    *text_width = 0;
    *text_height = 0;

    if (!show_a && !show_b)
        return;

    /*
     * Measuring the longer label is sufficient because a section displays the
     * two labels in matching slots instead of laying them out inline.
     */
    if (show_a && show_b)
        sample = (g_utf8_strlen(label_a, -1) >= g_utf8_strlen(label_b, -1)) ? label_a : label_b;
    else if (show_a)
        sample = label_a;
    else
        sample = label_b;

    layout = gtk_widget_create_pango_layout(widget, sample);
    font_desc = pango_font_description_from_string((font_desc_text != NULL && font_desc_text[0] != '\0')
                                                   ? font_desc_text
                                                   : DEFAULT_TEXT_FONT);
    pango_layout_set_font_description(layout, font_desc);
    pango_layout_get_pixel_size(layout, text_width, text_height);

    pango_font_description_free(font_desc);
    g_object_unref(layout);
}

static void
sysusage_plugin_get_section_size(const SysusagePlugin *sysusage,
                                 GtkWidget *widget,
                                 const gchar *label_a,
                                 gboolean show_a,
                                 const gchar *label_b,
                                 gboolean show_b,
                                 gint forced_text_margin,
                                 gint forced_under_text_slot_width,
                                 gint *width,
                                 gint *height)
{
    gint text_width;
    gint text_height;

    sysusage_plugin_get_section_text_dimensions(widget,
                                                label_a,
                                                show_a,
                                                label_b,
                                                show_b,
                                                sysusage->text_font_family,
                                                &text_width,
                                                &text_height);

    if (sysusage->vertical_bars)
    {
        const gboolean show_text = show_a || show_b;
        const gint bar_group_width = (gint) ((sysusage->bar_height * 2) + sysusage->bar_spacing);

        *width = bar_group_width;
        *height = (gint) sysusage->bar_width;

        if (sysusage->vertical_text_under_bars)
        {
            /*
             * Optional vertical mode keeps labels horizontal below the bars.
             * Reserve two text slots so the labels do not overlap each other.
             * When both sections are visible, a forced slot width keeps their
             * bar centers aligned around the separator even if label widths differ.
             */
            if (show_text || forced_under_text_slot_width >= 0)
            {
                const gint text_slot_width = (forced_under_text_slot_width >= 0)
                                                ? forced_under_text_slot_width
                                                : MAX((gint) sysusage->bar_height, text_width);

                *width = MAX(*width, (text_slot_width * 2) + (gint) sysusage->bar_spacing);
                if (show_text)
                    *height += text_height + (gint) sysusage->text_padding;
            }
        }
        else
        {
            /*
             * Default vertical mode rotates labels beside the bars. Width
             * therefore needs extra side margin reserved for label glyphs.
             */
            const gint text_margin = (forced_text_margin >= 0)
                                    ? forced_text_margin
                                    : (text_height + (gint) sysusage->text_padding);

            if (show_text || forced_text_margin >= 0)
            {
                *width += text_margin * 2;
                *height = MAX(*height, text_width);
            }
        }
    }
    else
    {
        *width = (gint) sysusage->bar_width;
        *height = (gint) ((sysusage->bar_height * 2) + sysusage->bar_spacing);

        if (show_a)
            *height += text_height + (gint) sysusage->text_padding;
        if (show_b)
            *height += text_height + (gint) sysusage->text_padding;
    }
}

void
sysusage_plugin_update_visibility(SysusagePlugin *sysusage)
{
    const gboolean show_mem = !sysusage->disable_memusage;
    const gboolean show_net = !sysusage->disable_netusage;
    const gboolean show_separator = show_mem && show_net;

    gtk_widget_set_no_show_all(sysusage->mem_area, !show_mem);
    gtk_widget_set_no_show_all(sysusage->net_area, !show_net);
    gtk_widget_set_no_show_all(sysusage->separator_area, !show_separator);

    /*
     * no-show-all protects hidden sections from gtk_widget_show_all(), while
     * set_visible applies the current state immediately after settings change.
     */
    gtk_widget_set_visible(sysusage->mem_area, show_mem);
    gtk_widget_set_visible(sysusage->net_area, show_net);
    gtk_widget_set_visible(sysusage->separator_area, show_separator);
}

void
sysusage_plugin_update_size_request(SysusagePlugin *sysusage)
{
    gint mem_w = 0;
    gint mem_h = 0;
    gint net_w = 0;
    gint net_h = 0;
    const gboolean show_mem = !sysusage->disable_memusage;
    const gboolean show_net = !sysusage->disable_netusage;
    const gboolean show_separator = show_mem && show_net;
    gint shared_vertical_text_margin = -1;
    gint shared_under_text_slot_width = -1;

    sysusage->vertical_under_text_slot_width = 0;

    if (show_separator && sysusage->vertical_bars && sysusage->vertical_text_under_bars)
    {
        /*
         * Keep under-bar text slots equally wide in both sections so the bars
         * remain centered relative to the separator, matching side-text mode.
         */
        gint mem_text_width = 0;
        gint mem_text_height = 0;
        gint net_text_width = 0;
        gint net_text_height = 0;

        sysusage_plugin_get_section_text_dimensions(sysusage->mem_area,
                                                    sysusage->mem_label,
                                                    sysusage->show_mem_text,
                                                    sysusage->swap_label,
                                                    sysusage->show_swap_text,
                                                    sysusage->text_font_family,
                                                    &mem_text_width,
                                                    &mem_text_height);
        sysusage_plugin_get_section_text_dimensions(sysusage->net_area,
                                                    sysusage->net_in_label,
                                                    sysusage->show_net_in_text,
                                                    sysusage->net_out_label,
                                                    sysusage->show_net_out_text,
                                                    sysusage->text_font_family,
                                                    &net_text_width,
                                                    &net_text_height);

        (void) mem_text_height;
        (void) net_text_height;

        shared_under_text_slot_width = MAX((gint) sysusage->bar_height, MAX(mem_text_width, net_text_width));
        sysusage->vertical_under_text_slot_width = (guint) shared_under_text_slot_width;
    }
    else if (show_separator && sysusage->vertical_bars && !sysusage->vertical_text_under_bars)
    {
        /*
         * Keep both sections aligned by using the same label margin on each
         * side when both are visible in vertical mode.
         */
        gint mem_text_width = 0;
        gint mem_text_height = 0;
        gint net_text_width = 0;
        gint net_text_height = 0;
        gint mem_margin = 0;
        gint net_margin = 0;

        sysusage_plugin_get_section_text_dimensions(sysusage->mem_area,
                                                    sysusage->mem_label,
                                                    sysusage->show_mem_text,
                                                    sysusage->swap_label,
                                                    sysusage->show_swap_text,
                                                    sysusage->text_font_family,
                                                    &mem_text_width,
                                                    &mem_text_height);
        sysusage_plugin_get_section_text_dimensions(sysusage->net_area,
                                                    sysusage->net_in_label,
                                                    sysusage->show_net_in_text,
                                                    sysusage->net_out_label,
                                                    sysusage->show_net_out_text,
                                                    sysusage->text_font_family,
                                                    &net_text_width,
                                                    &net_text_height);

        (void) mem_text_width;
        (void) net_text_width;

        if (sysusage->show_mem_text || sysusage->show_swap_text)
            mem_margin = mem_text_height + (gint) sysusage->text_padding;
        if (sysusage->show_net_in_text || sysusage->show_net_out_text)
            net_margin = net_text_height + (gint) sysusage->text_padding;

        shared_vertical_text_margin = MAX(mem_margin, net_margin);
    }

    if (show_mem)
    {
        sysusage_plugin_get_section_size(sysusage,
                                         sysusage->mem_area,
                                         sysusage->mem_label,
                                         sysusage->show_mem_text,
                                         sysusage->swap_label,
                                         sysusage->show_swap_text,
                                         shared_vertical_text_margin,
                                         shared_under_text_slot_width,
                                         &mem_w,
                                         &mem_h);
    }

    if (show_net)
    {
        sysusage_plugin_get_section_size(sysusage,
                                         sysusage->net_area,
                                         sysusage->net_in_label,
                                         sysusage->show_net_in_text,
                                         sysusage->net_out_label,
                                         sysusage->show_net_out_text,
                                         shared_vertical_text_margin,
                                         shared_under_text_slot_width,
                                         &net_w,
                                         &net_h);
    }

    gint separator_w = 0;
    gint separator_h = 0;
    gint plugin_w;
    gint plugin_h;

    gtk_widget_set_size_request(sysusage->mem_area, mem_w, mem_h);
    gtk_widget_set_size_request(sysusage->net_area, net_w, net_h);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(sysusage->container), GTK_ORIENTATION_HORIZONTAL);

    if (show_separator)
    {
        if (sysusage->separator_style == 2)
        {
            /* Handle style uses fixed motif geometry independent of line controls. */
            separator_w = 8;
            separator_h = MAX(MAX(mem_h, net_h), 18);
        }
        else
        {
            separator_w = (gint) MAX(sysusage->separator_line_width, 1);
            separator_h = MAX(MAX(mem_h, net_h), (gint) sysusage->separator_line_length);
        }
        gtk_widget_set_size_request(sysusage->separator_area, separator_w, separator_h);
    }
    else
    {
        gtk_widget_set_size_request(sysusage->separator_area, 0, 0);
    }

    gtk_box_set_spacing(GTK_BOX(sysusage->container), show_separator ? (gint) sysusage->separator_spacing : 0);

    plugin_w = mem_w + net_w + separator_w;
    if (show_separator)
        plugin_w += (gint) sysusage->separator_spacing * 2;
    plugin_h = MAX(MAX(mem_h, net_h), separator_h);

    gtk_widget_set_size_request(GTK_WIDGET(sysusage->container), plugin_w, plugin_h);
    gtk_widget_set_size_request(GTK_WIDGET(sysusage->plugin), plugin_w, plugin_h);
    gtk_widget_queue_resize(GTK_WIDGET(sysusage->container));
    gtk_widget_queue_resize(GTK_WIDGET(sysusage->plugin));
}

gboolean
sysusage_plugin_query_tooltip(GtkWidget *widget,
                              gint x,
                              gint y,
                              gboolean keyboard_mode,
                              GtkTooltip *tooltip,
                              gpointer user_data)
{
    SysusagePlugin *sysusage = user_data;
    GtkAllocation mem_alloc;
    GtkAllocation net_alloc;
    gint local_x;
    gint local_y;

    (void) keyboard_mode;

    if (!gtk_widget_translate_coordinates(widget, sysusage->container, x, y, &local_x, &local_y))
        return FALSE;

    gtk_widget_get_allocation(sysusage->mem_area, &mem_alloc);
    gtk_widget_get_allocation(sysusage->net_area, &net_alloc);

    if (local_x >= mem_alloc.x && local_x < (mem_alloc.x + mem_alloc.width)
        && local_y >= mem_alloc.y && local_y < (mem_alloc.y + mem_alloc.height))
    {
        if (sysusage->mem_tooltip_text != NULL)
        {
            gtk_tooltip_set_custom(tooltip, sysusage->mem_tooltip_widget);
            return TRUE;
        }
        return FALSE;
    }

    if (local_x >= net_alloc.x && local_x < (net_alloc.x + net_alloc.width)
        && local_y >= net_alloc.y && local_y < (net_alloc.y + net_alloc.height))
    {
        if (sysusage->net_tooltip_text != NULL)
        {
            gtk_tooltip_set_custom(tooltip, sysusage->net_tooltip_widget);
            return TRUE;
        }
        return FALSE;
    }

    return FALSE;
}

static void
sysusage_plugin_draw_rounded_rect(cairo_t *cr,
                                  gdouble x,
                                  gdouble y,
                                  gdouble width,
                                  gdouble height,
                                  gdouble corner_radius)
{
    gdouble rounded_radius;

    if (width <= 0.0 || height <= 0.0)
        return;

    rounded_radius = MIN(corner_radius, MIN(width / 2.0, height / 2.0));

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - rounded_radius, y + rounded_radius, rounded_radius, -(G_PI / 2.0), 0.0);
    cairo_arc(cr, x + width - rounded_radius, y + height - rounded_radius, rounded_radius, 0.0, (G_PI / 2.0));
    cairo_arc(cr, x + rounded_radius, y + height - rounded_radius, rounded_radius, (G_PI / 2.0), G_PI);
    cairo_arc(cr, x + rounded_radius, y + rounded_radius, rounded_radius, G_PI, (3.0 * G_PI / 2.0));
    cairo_close_path(cr);
}

typedef struct
{
    PangoLayout *layout;
    PangoRectangle ink_rect;
    PangoRectangle logical_rect;
    gint width;
    gint height;
} SysusageTextLayout;

static void
sysusage_plugin_text_layout_init(SysusageTextLayout *text_layout,
                                 cairo_t *cr,
                                 const gchar *text,
                                 const PangoFontDescription *font_desc)
{
    text_layout->layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(text_layout->layout, font_desc);
    pango_layout_set_text(text_layout->layout, text != NULL ? text : "", -1);
    pango_layout_get_pixel_extents(text_layout->layout,
                                   &text_layout->ink_rect,
                                   &text_layout->logical_rect);
    pango_layout_get_pixel_size(text_layout->layout, &text_layout->width, &text_layout->height);
}

static void
sysusage_plugin_text_layout_clear(SysusageTextLayout *text_layout)
{
    g_clear_object(&text_layout->layout);
    text_layout->ink_rect = (PangoRectangle) {0, 0, 0, 0};
    text_layout->logical_rect = (PangoRectangle) {0, 0, 0, 0};
    text_layout->width = 0;
    text_layout->height = 0;
}

static gdouble
sysusage_plugin_text_visual_width(const SysusageTextLayout *text_layout)
{
    return (gdouble) text_layout->width;
}

static gdouble
sysusage_plugin_centered_text_x(gdouble center_x,
                                const SysusageTextLayout *text_layout)
{
    return center_x
           - (gdouble) text_layout->ink_rect.x
           - ((gdouble) text_layout->ink_rect.width / 2.0);
}

static gdouble
sysusage_plugin_text_left_x(gdouble x,
                           const SysusageTextLayout *text_layout)
{
    return x - (gdouble) text_layout->ink_rect.x;
}

static gdouble
sysusage_plugin_text_top_y(gdouble y,
                           const SysusageTextLayout *text_layout)
{
    return y - (gdouble) text_layout->ink_rect.y;
}

static gdouble
sysusage_plugin_text_bottom_y(gdouble y,
                              const SysusageTextLayout *text_layout)
{
    return y - (gdouble) text_layout->ink_rect.y - (gdouble) text_layout->ink_rect.height;
}

static gdouble
sysusage_plugin_rotated_text_left_x(gdouble x,
                                   const SysusageTextLayout *text_layout)
{
    return x - (gdouble) text_layout->ink_rect.y - (gdouble) text_layout->ink_rect.height;
}

static gdouble
sysusage_plugin_rotated_text_right_x(gdouble x,
                                    const SysusageTextLayout *text_layout)
{
    return x - (gdouble) text_layout->ink_rect.y;
}

static void
sysusage_plugin_show_text_layout(cairo_t *cr,
                                 const SysusageTextLayout *text_layout,
                                 gdouble x,
                                 gdouble y)
{
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, text_layout->layout);
}

static void
sysusage_plugin_draw_vertical_section(const GtkAllocation *allocation,
                                      cairo_t *cr,
                                      SysusagePlugin *sysusage,
                                      gdouble percent_a,
                                      gdouble percent_b,
                                      gboolean show_a,
                                      gboolean show_b,
                                      const GdkRGBA *color_a,
                                      const GdkRGBA *color_b,
                                      const SysusageTextLayout *a_layout,
                                      const SysusageTextLayout *b_layout,
                                      gdouble text_padding)
{
    /* Geometry for two adjacent vertical bars centered in the allocation. */
    const gdouble bar_w = (gdouble) sysusage->bar_height;
    const gdouble bar_h = (gdouble) sysusage->bar_width;
    const gboolean show_under_text = sysusage->vertical_text_under_bars && (show_a || show_b);
    const gdouble text_width_a = show_a ? sysusage_plugin_text_visual_width(a_layout) : 0.0;
    const gdouble text_width_b = show_b ? sysusage_plugin_text_visual_width(b_layout) : 0.0;
    const gdouble text_slot_w = show_under_text
                                      ? (sysusage->vertical_under_text_slot_width > 0
                                             ? (gdouble) sysusage->vertical_under_text_slot_width
                                             : MAX(bar_w, MAX(text_width_a, text_width_b)))
                                      : bar_w;
    const gdouble total_w = show_under_text
                                ? ((text_slot_w * 2.0) + (gdouble) sysusage->bar_spacing)
                                : ((bar_w * 2.0) + (gdouble) sysusage->bar_spacing);
    const gdouble text_height_a = show_a ? a_layout->height : 0.0;
    const gdouble text_height_b = show_b ? b_layout->height : 0.0;
    const gdouble text_height = show_under_text ? MAX(text_height_a, text_height_b) : 0.0;
    const gdouble total_h = show_under_text ? (bar_h + text_padding + text_height) : bar_h;
    const gdouble x_offset = MAX(0.0, (((gdouble) allocation->width) - total_w) / 2.0);
    const gdouble y_offset = MAX(0.0, (((gdouble) allocation->height) - total_h) / 2.0);
    const gdouble a_x = show_under_text ? (x_offset + ((text_slot_w - bar_w) / 2.0)) : x_offset;
    const gdouble b_x = show_under_text
                            ? (x_offset + text_slot_w + (gdouble) sysusage->bar_spacing + ((text_slot_w - bar_w) / 2.0))
                            : (x_offset + bar_w + (gdouble) sysusage->bar_spacing);
    const gdouble fill_a = (bar_h * percent_a) / 100.0;
    const gdouble fill_b = (bar_h * percent_b) / 100.0;
    const gdouble bg_radius = bar_w / 2.0;
    const gdouble a_text_x = sysusage->text_primary_side_left
                             ? sysusage_plugin_rotated_text_left_x(a_x - text_padding, a_layout)
                             : sysusage_plugin_rotated_text_right_x(a_x + bar_w + text_padding, a_layout);
    const gdouble b_text_x = sysusage->text_primary_side_right
                             ? sysusage_plugin_rotated_text_left_x(b_x - text_padding, b_layout)
                             : sysusage_plugin_rotated_text_right_x(b_x + bar_w + text_padding, b_layout);

    cairo_set_source_rgba(cr,
                          sysusage->background_color.red,
                          sysusage->background_color.green,
                          sysusage->background_color.blue,
                          sysusage->background_color.alpha);
    sysusage_plugin_draw_rounded_rect(cr, a_x, y_offset, bar_w, bar_h, bg_radius);
    sysusage_plugin_draw_rounded_rect(cr, b_x, y_offset, bar_w, bar_h, bg_radius);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, color_a->red, color_a->green, color_a->blue, color_a->alpha);
    cairo_save(cr);
    sysusage_plugin_draw_rounded_rect(cr, a_x, y_offset, bar_w, bar_h, bg_radius);
    cairo_clip(cr);
    cairo_rectangle(cr, a_x, y_offset + (bar_h - fill_a), bar_w, fill_a);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_set_source_rgba(cr, color_b->red, color_b->green, color_b->blue, color_b->alpha);
    cairo_save(cr);
    sysusage_plugin_draw_rounded_rect(cr, b_x, y_offset, bar_w, bar_h, bg_radius);
    cairo_clip(cr);
    cairo_rectangle(cr, b_x, y_offset + (bar_h - fill_b), bar_w, fill_b);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_set_source_rgba(cr,
                          sysusage->text_color.red,
                          sysusage->text_color.green,
                          sysusage->text_color.blue,
                          sysusage->text_color.alpha);

    if (show_under_text)
    {
        const gdouble text_y = y_offset + bar_h + text_padding;

        if (show_a)
        {
            const gdouble a_text_center = x_offset + (text_slot_w / 2.0);
            sysusage_plugin_show_text_layout(cr,
                                             a_layout,
                                             sysusage_plugin_centered_text_x(a_text_center, a_layout),
                                             sysusage_plugin_text_top_y(text_y, a_layout));
        }

        if (show_b)
        {
            const gdouble b_text_center = x_offset
                                          + text_slot_w
                                          + (gdouble) sysusage->bar_spacing
                                          + (text_slot_w / 2.0);
            sysusage_plugin_show_text_layout(cr,
                                             b_layout,
                                             sysusage_plugin_centered_text_x(b_text_center, b_layout),
                                             sysusage_plugin_text_top_y(text_y, b_layout));
        }
        return;
    }

    if (show_a)
    {
        cairo_save(cr);
        cairo_translate(cr, a_text_x, y_offset + bar_h);
        cairo_rotate(cr, -G_PI / 2.0);
        sysusage_plugin_show_text_layout(cr, a_layout, 0.0, 0.0);
        cairo_restore(cr);
    }

    if (show_b)
    {
        cairo_save(cr);
        cairo_translate(cr, b_text_x, y_offset + bar_h);
        cairo_rotate(cr, -G_PI / 2.0);
        sysusage_plugin_show_text_layout(cr, b_layout, 0.0, 0.0);
        cairo_restore(cr);
    }
}

static void
sysusage_plugin_draw_horizontal_section(const GtkAllocation *allocation,
                                        cairo_t *cr,
                                        SysusagePlugin *sysusage,
                                        gdouble percent_a,
                                        gdouble percent_b,
                                        gboolean show_a,
                                        gboolean show_b,
                                        const GdkRGBA *color_a,
                                        const GdkRGBA *color_b,
                                        const SysusageTextLayout *a_layout,
                                        const SysusageTextLayout *b_layout,
                                        gdouble text_padding)
{
    /* Geometry for two stacked horizontal bars centered in the allocation. */
    const gdouble total_h = (gdouble) ((sysusage->bar_height * 2) + sysusage->bar_spacing);
    const gdouble x_offset = MAX(0.0, (((gdouble) allocation->width) - ((gdouble) sysusage->bar_width)) / 2.0);
    const gdouble y_offset = MAX(0.0, (((gdouble) allocation->height) - total_h) / 2.0);
    const gdouble a_bar_y = y_offset;
    const gdouble b_bar_y = a_bar_y + (gdouble) sysusage->bar_height + (gdouble) sysusage->bar_spacing;
    const gdouble a_text_above_y = sysusage_plugin_text_bottom_y(a_bar_y - text_padding, a_layout);
    const gdouble b_text_above_y = sysusage_plugin_text_bottom_y(b_bar_y - text_padding, b_layout);
    const gdouble a_text_below_y = sysusage_plugin_text_top_y(a_bar_y
                                                              + (gdouble) sysusage->bar_height
                                                              + text_padding,
                                                              a_layout);
    const gdouble b_text_below_y = sysusage_plugin_text_top_y(b_bar_y
                                                              + (gdouble) sysusage->bar_height
                                                              + text_padding,
                                                              b_layout);
    const gdouble fill_a = ((gdouble) sysusage->bar_width * percent_a) / 100.0;
    const gdouble fill_b = ((gdouble) sysusage->bar_width * percent_b) / 100.0;
    const gdouble bg_radius = (gdouble) sysusage->bar_height / 2.0;

    cairo_set_source_rgba(cr,
                          sysusage->background_color.red,
                          sysusage->background_color.green,
                          sysusage->background_color.blue,
                          sysusage->background_color.alpha);
    sysusage_plugin_draw_rounded_rect(cr, x_offset, a_bar_y, (gdouble) sysusage->bar_width, (gdouble) sysusage->bar_height, bg_radius);
    sysusage_plugin_draw_rounded_rect(cr, x_offset, b_bar_y, (gdouble) sysusage->bar_width, (gdouble) sysusage->bar_height, bg_radius);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, color_a->red, color_a->green, color_a->blue, color_a->alpha);
    cairo_save(cr);
    sysusage_plugin_draw_rounded_rect(cr, x_offset, a_bar_y, (gdouble) sysusage->bar_width, (gdouble) sysusage->bar_height, bg_radius);
    cairo_clip(cr);
    cairo_rectangle(cr, x_offset, a_bar_y, fill_a, (gdouble) sysusage->bar_height);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_set_source_rgba(cr, color_b->red, color_b->green, color_b->blue, color_b->alpha);
    cairo_save(cr);
    sysusage_plugin_draw_rounded_rect(cr, x_offset, b_bar_y, (gdouble) sysusage->bar_width, (gdouble) sysusage->bar_height, bg_radius);
    cairo_clip(cr);
    cairo_rectangle(cr, x_offset, b_bar_y, fill_b, (gdouble) sysusage->bar_height);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_set_source_rgba(cr,
                          sysusage->text_color.red,
                          sysusage->text_color.green,
                          sysusage->text_color.blue,
                          sysusage->text_color.alpha);

    if (show_a)
    {
        const gdouble text_y = sysusage->text_primary_side_left ? a_text_above_y : a_text_below_y;
        sysusage_plugin_show_text_layout(cr,
                                         a_layout,
                                         sysusage_plugin_text_left_x(x_offset, a_layout),
                                         text_y);
    }

    if (show_b)
    {
        const gdouble text_y = sysusage->text_primary_side_right ? b_text_above_y : b_text_below_y;
        sysusage_plugin_show_text_layout(cr,
                                         b_layout,
                                         sysusage_plugin_text_left_x(x_offset, b_layout),
                                         text_y);
    }
}

static gboolean
sysusage_plugin_draw_section(GtkWidget *widget,
                             cairo_t *cr,
                             SysusagePlugin *sysusage,
                             gdouble percent_a,
                             gdouble percent_b,
                             const gchar *label_a,
                             const gchar *label_b,
                             gboolean show_a,
                             gboolean show_b,
                             const GdkRGBA *color_a,
                             const GdkRGBA *color_b)
{
    /* Shared rendering path used by both mem and net sections. */
    SysusageTextLayout a_layout;
    SysusageTextLayout b_layout;
    PangoFontDescription *font_desc = pango_font_description_from_string((sysusage->text_font_family != NULL
                                                                           && sysusage->text_font_family[0] != '\0')
                                                                              ? sysusage->text_font_family
                                                                              : DEFAULT_TEXT_FONT);
    const gdouble text_padding = (gdouble) sysusage->text_padding;
    const GtkAllocation allocation = {
        0,
        0,
        gtk_widget_get_allocated_width(widget),
        gtk_widget_get_allocated_height(widget)
    };

    sysusage_plugin_text_layout_init(&a_layout, cr, label_a, font_desc);
    sysusage_plugin_text_layout_init(&b_layout, cr, label_b, font_desc);

    if (sysusage->vertical_bars)
        sysusage_plugin_draw_vertical_section(&allocation,
                                              cr,
                                              sysusage,
                                              percent_a,
                                              percent_b,
                                              show_a,
                                              show_b,
                                              color_a,
                                              color_b,
                                              &a_layout,
                                              &b_layout,
                                              text_padding);
    else
        sysusage_plugin_draw_horizontal_section(&allocation,
                                                cr,
                                                sysusage,
                                                percent_a,
                                                percent_b,
                                                show_a,
                                                show_b,
                                                color_a,
                                                color_b,
                                                &a_layout,
                                                &b_layout,
                                                text_padding);

    sysusage_plugin_text_layout_clear(&a_layout);
    sysusage_plugin_text_layout_clear(&b_layout);
    pango_font_description_free(font_desc);
    return FALSE;
}

gboolean
sysusage_plugin_draw_mem(GtkWidget *widget,
                         cairo_t *cr,
                         gpointer user_data)
{
    SysusagePlugin *sysusage = user_data;

    return sysusage_plugin_draw_section(widget,
                                        cr,
                                        sysusage,
                                        sysusage->mem_percent,
                                        sysusage->swap_percent,
                                        sysusage->mem_label,
                                        sysusage->swap_label,
                                        sysusage->show_mem_text,
                                        sysusage->show_swap_text,
                                        &sysusage->mem_color,
                                        &sysusage->swap_color);
}

gboolean
sysusage_plugin_draw_net(GtkWidget *widget,
                         cairo_t *cr,
                         gpointer user_data)
{
    SysusagePlugin *sysusage = user_data;

    return sysusage_plugin_draw_section(widget,
                                        cr,
                                        sysusage,
                                        sysusage->net_in_percent,
                                        sysusage->net_out_percent,
                                        sysusage->net_in_label,
                                        sysusage->net_out_label,
                                        sysusage->show_net_in_text,
                                        sysusage->show_net_out_text,
                                        &sysusage->net_in_color,
                                        &sysusage->net_out_color);
}

gboolean
sysusage_plugin_draw_separator(GtkWidget *widget,
                               cairo_t *cr,
                               gpointer user_data)
{
    SysusagePlugin *sysusage = user_data;

    if (sysusage->disable_memusage || sysusage->disable_netusage || sysusage->separator_transparent)
        return FALSE;
    const gint width = gtk_widget_get_allocated_width(widget);
    const gint height = gtk_widget_get_allocated_height(widget);
    const gdouble requested_length = (gdouble) MAX(sysusage->separator_line_length, 1);
    const gdouble line_length = MIN(requested_length, (gdouble) height);
    const gdouble y_start = (((gdouble) height) - line_length) / 2.0;
    const gdouble y_end = y_start + line_length;
    const gdouble line_width = (gdouble) MAX(sysusage->separator_line_width, 1);

    cairo_set_source_rgba(cr,
                          sysusage->separator_color.red,
                          sysusage->separator_color.green,
                          sysusage->separator_color.blue,
                          sysusage->separator_color.alpha);

    const gdouble x = ((gdouble) width) / 2.0;
    if (sysusage->separator_style == 1)
    {
        const gdouble radius = MAX(1.0, line_width / 2.0);
        const gdouble step = 8.0;
        gdouble y = y_start + radius;

        while (y <= (y_end - radius))
        {
            cairo_arc(cr, x, y, radius, 0.0, 2.0 * G_PI);
            cairo_fill(cr);
            y += step;
        }
    }
    else if (sysusage->separator_style == 2)
    {
        /*
         * Mode A: fixed handle motif.
         * Keep geometry constant so length/width controls do not affect look.
         */
        const gdouble handle_w = 4.0;
        const gdouble handle_h = 2.0;
        const gdouble handle_gap = 2.0;
        const gdouble total_h = (handle_h * 3.0) + (handle_gap * 2.0);
        const gdouble y0 = floor((((gdouble) height) - total_h) / 2.0);
        gint i;

        for (i = 0; i < 3; i++)
        {
            const gdouble y = y0 + ((gdouble) i * (handle_h + handle_gap));
            cairo_rectangle(cr, floor(x - (handle_w / 2.0)), y, handle_w, handle_h);
            cairo_fill(cr);
        }
    }
    else
    {
        cairo_move_to(cr, x, y_start);
        cairo_line_to(cr, x, y_end);
        cairo_set_line_width(cr, line_width);
        cairo_stroke(cr);
    }
    return FALSE;
}
