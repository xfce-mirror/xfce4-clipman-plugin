/*
 * Copyright (c) 2004 Eduard Roccatello (eduard@xfce.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#define MAXHISTORY 100
#define MENUTXT_LEN 30
#define CLIPMAN_TIMER_INTERVAL 500
#define PLUGIN_NAME "clipboard"
#define DEFHISTORY 6

/* for xml store */
#define BUFFERS_XML_KEY "Buffers"

typedef GString* pGString; /* pointer to GString */

typedef struct
{
	GtkWidget   *ebox;
	GtkWidget   *button;
    GtkWidget   *img;
    GtkWidget   *menu;
    GString     **content;
    guint        iter;
    gint         timeId;
    gboolean     killing;
    GtkTooltips *tooltip;
    gint         content_size;
} t_clipman;

typedef struct
{
    t_clipman  *clip;
    guint       idx;
} t_action;

typedef struct
{
    t_clipman  *clip;
    GtkWidget  *sb_clips; /* spin button for number of clips to store */
} t_options;

static GtkClipboard *primaryClip, *defaultClip;

static void resetTimer (gpointer data);
static gboolean isThere (t_clipman *clip, gchar *txt);
static gchar* filterLFCR (gchar *txt);

static gboolean isThere (t_clipman *clipman, gchar *txt)
{
    gint i;
    for (i=0; i<clipman->content_size; i++) {
        if (strcmp(clipman->content[i]->str, txt) == 0)
            return TRUE;
    }
    return FALSE;
}

static gchar* filterLFCR (gchar *txt)
{
    guint i = 0;
    while (txt[i] != '\0') {
        if (txt[i] == '\n' || txt[i] == '\r' || txt[i] == '\t')
            txt[i] = ' ';
        i++;
    }
    txt = g_strstrip(txt);
    return txt;
}

static void
item_pressed_cb (GtkWidget *widget, GdkEventButton *ev, gpointer user_data)
{
    t_action *act = (t_action *)user_data;
    if (ev->button != 3) {
        gtk_clipboard_set_text(defaultClip, act->clip->content[act->idx]->str, -1);
        gtk_clipboard_set_text(primaryClip, act->clip->content[act->idx]->str, -1);
    } else {
        if (confirm(N_("Do you want to remove it from the history?"), "gtk-clear", NULL)) {
            gtk_clipboard_set_text(defaultClip, "", -1);
            gtk_clipboard_set_text(primaryClip, "", -1);
            g_string_assign(act->clip->content[act->idx], "");
            act->clip->iter = act->idx;
        }
        gtk_menu_popdown(GTK_MENU(act->clip->menu));
    }
    gtk_widget_destroy (act->clip->menu);
}

static void
drag_data_get_cb (GtkWidget *widget, GdkDragContext *dg, GtkSelectionData *seldata, gint i, gint t, gpointer user_data)
{
    t_action *action = user_data;
    gint idx = action->idx;
    gchar *text = action->clip->content[idx]->str;
    gtk_selection_data_set(seldata, gdk_atom_intern("STRING", FALSE), 8, text, strlen(text));
}


static void
clearClipboard (GtkWidget *widget, gpointer data)
{
    gint i;
    t_clipman *clipman = (t_clipman *)data;

    /* Clear History */
    for (i=0; i<clipman->content_size; i++)
        g_string_assign(clipman->content[i], "");

    /* Clear Clipboard */
    gtk_clipboard_set_text(defaultClip, "", -1);
    gtk_clipboard_set_text(primaryClip, "", -1);

    /* Set iterator to the first element of the array */
    clipman->iter = 0;

}

static void
clicked_cb(GtkWidget *button, gpointer data)
{
    GtkMenu    *menu = NULL;
    GtkWidget  *mi;
    GtkTargetEntry *te;
    t_clipman  *clipman = data;
    t_action   *action = NULL;
    gboolean    hasOne = FALSE;
    gint        i;                  /* an index */
    guint       last;               /* latest item inserted */
    guint       num = 0;            /* just a counter */

    te = g_new0(GtkTargetEntry,1);
    te->target="UTF8_STRING";
    te->flags=0;
    te->info=0;

    menu = GTK_MENU(gtk_menu_new());

    mi = gtk_menu_item_new_with_label (N_("Clipboard History"));
    gtk_widget_show (mi);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    /*
        Append cliboard history items.
        I need to have an circular array scan. Latest item inserted in the array
        is at iter-1 position so we need a special trick to scan the array
    */

    if (clipman->iter!=0)
        last=clipman->iter-1;
    else
        last=clipman->content_size-1;

    for (i=last;i>=0;i--){
        if (clipman->content[i]->str != NULL && (strcmp(clipman->content[i]->str, "") != 0)) {
            mi = gtk_menu_item_new_with_label (g_strdup_printf("%d. %s", ++num, filterLFCR(g_strndup(clipman->content[i]->str, MENUTXT_LEN))));
            gtk_drag_source_set(mi, GDK_BUTTON1_MASK, te, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
            gtk_widget_show (mi);
            action = g_new(t_action, 1);
            action->clip = clipman;
            action->idx = i;
            g_signal_connect (G_OBJECT (mi), "drag_data_get", G_CALLBACK (drag_data_get_cb), (gpointer)action);
            g_signal_connect (G_OBJECT(mi), "button_press_event", G_CALLBACK(item_pressed_cb), (gpointer)action);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
            hasOne = TRUE;
        }
    }

    if (last!=clipman->content_size-1) {
        for (i=clipman->content_size-1;i>last;i--) {
            if (clipman->content[i]->str != NULL && (strcmp(clipman->content[i]->str, "") != 0)) {
                mi = gtk_menu_item_new_with_label (g_strdup_printf("%d. %s", ++num, filterLFCR(g_strndup(clipman->content[i]->str, 20))));
                gtk_widget_show (mi);
                action = g_new(t_action, 1);
                action->clip = clipman;
                action->idx = i;
                g_signal_connect (G_OBJECT(mi), "button_press_event", G_CALLBACK(item_pressed_cb), (gpointer)action);
                g_signal_connect (G_OBJECT (mi), "drag_data_get", G_CALLBACK (drag_data_get_cb), (gpointer)action);
                gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
            }
        }
    }

    /* If the clipboard is empty put a new informational item, else create the clear item */
    if (!hasOne) {
        mi = gtk_menu_item_new_with_label (N_("< Clipboard Empty >"));
        gtk_widget_show (mi);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
	else {
        mi = gtk_separator_menu_item_new ();
        gtk_widget_show (mi);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_menu_item_new_with_label (N_("Clear Clipboard"));
        gtk_widget_show (mi);
        g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (clearClipboard), (gpointer)clipman);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	}

    clipman->menu = GTK_WIDGET(menu);
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static gboolean checkClip (t_clipman *clipman) {
    gchar *txt = NULL;

    XFCE_PANEL_LOCK();
    
    /* Check for text in X clipboard */
    txt = gtk_clipboard_wait_for_text (primaryClip);
    if (txt != NULL) {
        if (!isThere(clipman, txt)) {
                g_string_assign(clipman->content[clipman->iter], txt);
                if (clipman->iter < (clipman->content_size - 1))
                    clipman->iter++;
                else
                    clipman->iter = 0;
        }
        g_free(txt);
        txt = NULL;
    }

    /* Check for text in default clipboard */
    txt = gtk_clipboard_wait_for_text (defaultClip);
    if (txt != NULL) {
        if (!isThere(clipman, txt)) {
            g_string_assign(clipman->content[clipman->iter], txt);
            if (clipman->iter < (clipman->content_size - 1))
                clipman->iter++;
            else
                clipman->iter = 0;
        }
        g_free(txt);
        txt = NULL;
    }

    XFCE_PANEL_UNLOCK();

    return TRUE;
}

static void
clipman_content_alloc(t_clipman *clipman, gint new_size)
{
    gint i;

    if (clipman->content) { /* memory already allocated */
        for (i=0; i<clipman->content_size; i++) { /* remove old */
            if (clipman->content[i])
                g_string_free(clipman->content[i], TRUE);
        }
        clipman->content = g_renew(pGString, clipman->content, new_size);
    }
    else /* first time */
        clipman->content = g_new(pGString, new_size);

    clipman->content_size=new_size;
    for (i=0; i<clipman->content_size; i++) {
        clipman->content[i] = g_string_new("");
    }
}

static t_clipman *
clipman_new(void)
{
	t_clipman *clipman;

	clipman = g_new(t_clipman, 1);

	clipman->ebox = gtk_event_box_new();
	gtk_widget_show(clipman->ebox);

	clipman->button = gtk_button_new();
    clipman->tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip (GTK_TOOLTIPS(clipman->tooltip), clipman->button, 
            _("Clipboard Manager"), NULL);
    gtk_button_set_relief (GTK_BUTTON(clipman->button), GTK_RELIEF_NONE);
    gtk_widget_show(clipman->button);

	gtk_container_add(GTK_CONTAINER(clipman->ebox), clipman->button);

    clipman->img = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (clipman->img);
    gtk_container_add (GTK_CONTAINER (clipman->button), clipman->img);

    /* Element to be modified */
    clipman->iter = 0;
	clipman->timeId = 0;
    clipman->killing = FALSE;
    clipman->content = NULL;

    clipman_content_alloc(clipman, DEFHISTORY);

    defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);

    clipman->timeId = g_timeout_add_full(G_PRIORITY_LOW, CLIPMAN_TIMER_INTERVAL, (GSourceFunc)checkClip, clipman, (GDestroyNotify)resetTimer);
    g_signal_connect(clipman->button, "clicked", G_CALLBACK(clicked_cb), clipman);

	return clipman;
}

static void resetTimer (gpointer data)
{
    t_clipman *clipman = (t_clipman *)data;
    if (!(clipman->killing)) {
        if (clipman->timeId != 0)
            g_source_remove(clipman->timeId);
        clipman->timeId = g_timeout_add_full(G_PRIORITY_LOW, CLIPMAN_TIMER_INTERVAL, (GSourceFunc)checkClip, data, (GDestroyNotify)resetTimer);
    }
}

static gboolean
clipman_control_new(Control *ctrl)
{
	t_clipman *clipman;

	clipman = clipman_new();

	gtk_container_add(GTK_CONTAINER(ctrl->base), clipman->ebox);

	ctrl->data = (gpointer)clipman;
	ctrl->with_popup = FALSE;

	gtk_widget_set_size_request(ctrl->base, -1, -1);

	return(TRUE);
}

static void
clipman_free(Control *ctrl)
{
    t_clipman *clipman;
    GtkItemFactory  *ifactory;
    gint i;

	g_return_if_fail(ctrl != NULL);
	g_return_if_fail(ctrl->data != NULL);

	clipman = (t_clipman *)ctrl->data;

    clipman->killing = TRUE;
	if (clipman->timeId != 0)
	    g_source_remove(clipman->timeId);

    clearClipboard (NULL, clipman);

    for (i=0; i<clipman->content_size; i++) {
        if (clipman->content[i])
            g_string_free(clipman->content[i], TRUE);
    }

    g_free(clipman->content);
    g_free(clipman);
}


static void
clipman_read_config(Control *ctrl, xmlNodePtr parent)
{
	t_clipman *clipman=(t_clipman *)ctrl->data;
    xmlNodePtr node;
    char      *str;
	
    if (!parent)
        return;
    for (node = parent->children; node; node = node->next) {
        if (xmlStrEqual(node->name, PLUGIN_NAME))
            if (str = xmlGetProp(node, BUFFERS_XML_KEY)) {
                clipman_content_alloc(clipman,atoi(str));
                xmlFree(str);
                break; /* no need to check more nodes, we found ours */
            }
    }
}

static void
clipman_write_config(Control *ctrl, xmlNodePtr parent)
{
	t_clipman *clipman=(t_clipman *)ctrl->data;
    xmlNodePtr root;
    char       str[20];

    root = xmlNewTextChild (parent, NULL, PLUGIN_NAME, NULL);
    sprintf(str, "%d", clipman->content_size);
    xmlSetProp(root, BUFFERS_XML_KEY, str);
}

static void
clipman_attach_callback(Control *ctrl, const gchar *signal, GCallback cb,
		gpointer data)
{
	t_clipman *clipman;

	clipman = (t_clipman *)ctrl->data;
	g_signal_connect(clipman->ebox, signal, cb, data);
	g_signal_connect(clipman->button, signal, cb, data);
}

static void
clipman_set_size(Control *ctrl, int size)
{
    t_clipman *clipman = (t_clipman *)ctrl->data;
    switch (size) {
        case 0:
            gtk_image_set_from_stock (GTK_IMAGE(clipman->img), "gtk-paste", GTK_ICON_SIZE_MENU);
            break;
        case 1:
            gtk_image_set_from_stock (GTK_IMAGE(clipman->img), "gtk-paste", GTK_ICON_SIZE_BUTTON);
            break;
        case 2:
            gtk_image_set_from_stock (GTK_IMAGE(clipman->img), "gtk-paste", GTK_ICON_SIZE_DND);
            break;
        case 3:
            gtk_image_set_from_stock (GTK_IMAGE(clipman->img), "gtk-paste", GTK_ICON_SIZE_DIALOG);
            break;
        default:
            break;
    }
}

static void
clipman_process_options (GtkWidget *widget, t_options *options)
{
    t_clipman   *clipman=(t_clipman *)options->clip;
    gint         val;

    val=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(options->sb_clips)); 
    clipman_content_alloc(clipman,val);
}

static void
clipman_close_options (GtkWidget *widget, t_options *options)
{
    g_free(options); /* free memory used in options dialog */
}

/* options dialog */
static void
clipman_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    t_clipman   *clipman=(t_clipman *)ctrl->data;
    GtkWidget   *vbox1, *hbox1, *label1, *sbut1, *top;
    GtkSizeGroup *sg1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    GtkTooltips *tooltips=gtk_tooltips_new ();
    t_options   *opt_data;

    opt_data = g_new(t_options, 1);
    opt_data->clip = (t_clipman *)ctrl->data;

    top=gtk_widget_get_toplevel(done);
    
    vbox1 = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(vbox1);
                                                                                
    hbox1 = gtk_hbox_new(FALSE, 6);
    gtk_widget_show(hbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);

    label1 = gtk_label_new _("Number of clip buffers:");
    gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);
    gtk_size_group_add_widget(sg1, label1);
    gtk_widget_show(label1);
    gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 0);

    opt_data->sb_clips = gtk_spin_button_new_with_range(1, MAXHISTORY, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(opt_data->sb_clips), 
        clipman->content_size); 
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), opt_data->sb_clips, 
        _("How many clips should be stored and showed"), NULL);
    gtk_widget_show(opt_data->sb_clips);
    gtk_box_pack_start(GTK_BOX(hbox1), opt_data->sb_clips, FALSE, FALSE, 0);

    g_signal_connect(done, "clicked",
        G_CALLBACK(clipman_process_options), opt_data);
    g_signal_connect(top, "destroy",
        G_CALLBACK(clipman_close_options), opt_data);

    gtk_container_add(con, vbox1);
}

/* initialization */
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
	/* these are required */
	cc->name		= PLUGIN_NAME;
	cc->caption		= _("Clipboard Manager");

	cc->create_control	= (CreateControlFunc)clipman_control_new;

	cc->free		= clipman_free;
	cc->attach_callback	= clipman_attach_callback;

	/* options; don't define if you don't have any ;)  */
	cc->read_config		= clipman_read_config;
	cc->write_config	= clipman_write_config;
	cc->create_options	= clipman_create_options;

	/* Don't use this function at all if you want xfce to
	 * do the sizing.
	 * Just define the set_size function to NULL, or rather, don't 
	 * set it to something else.
	 */
	cc->set_size    = clipman_set_size;

	/* unused in the clipman:
	 * ->set_orientation
	 * ->set_theme
	 */
	 
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT
