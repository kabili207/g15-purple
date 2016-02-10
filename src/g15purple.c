/**
 * @mainpage
 * @author  Kabili < kabili207@gmail.com
 * @version 0.7
 *
 * @section DESCRIPTION
 *
 * A plugin which interfaces the Logitech GamePanel LCD with Pidgin.
 * These LCDs are currently found in the G15 Keyboards and G13 Gamepads.
 *
 * The pidgin-libnotify plugin was used as a base for the interface with libpurple.
 * pidgin-libnotify is available at http://gaim-libnotify.sourceforge.net/
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 */


#define PURPLE_PLUGINS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libg15.h>
#include <libg15render.h>
#include <g15daemon_client.h>

#include <pthread.h>
#include <plugin.h>
#include <notify.h>
#include <version.h>
#include <debug.h>
#include <util.h>
#include <privacy.h>
#include <glib.h>
#include <account.h>

#include "images.h"


#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif
#endif

#define PLUGIN_ID    "core-kabili207-g15purple"
#define PLUGIN_NAME  "G15Purple"
#define VERSION      "0.7"
#define SUMMARY      "Displays information on a G15 lcd"
#define DESCRIPTION  "G15 plugin for libpurple"
#define AUTHOR       "Andrew Nagle <kabili207@gmail.com>"
#define WEBSITE      "http://www.zyrenth.com"

#define MIN_WINDOWS 1
#define MAX_WINDOWS 3

#define SET_PROTO_ICON_IF(PROTO, ICON) ((strcmp(proto,PROTO) == 0) ? ICON ## _bits : NULL)

static GHashTable *buddy_hash;


g15canvas *canvas;
int screen;
int textSize = 1;
char *g15title = "";
char *g15message = "";
char *currTime ="";
unsigned char *proto_icon;

/**
 * Creates window to set pugin options.
 *
 * @param plugin The plugin which the option window is for.
 * @return Returns the frame which will display the option window.
 */
static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_name_and_label(
									"/plugins/core/g15purple/proto_icon",
									"Show protocol icons");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
									"/plugins/core/g15purple/text_size",
									"Smaller text");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
									"/plugins/core/g15purple/time_choice",
									"Time format");
	purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
	purple_plugin_pref_add_choice(ppref, "Disabled", GINT_TO_POINTER(0));
	purple_plugin_pref_add_choice(ppref, "14:34:56", GINT_TO_POINTER(1));
	purple_plugin_pref_add_choice(ppref, "14:34:56 PM", GINT_TO_POINTER(2));
	purple_plugin_pref_add_choice(ppref, "02:34:56", GINT_TO_POINTER(3));
	purple_plugin_pref_add_choice(ppref, "02:34:56 PM", GINT_TO_POINTER(4));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

/**
 * Sets the value of t to the current time. The format of the
 * current time is determined by user's settings.
 *
 * @param t The variable which will be modified.
 */
void get_time(char **t) {

	struct tm *ptr;
	time_t lt;
	static char str[200];

	lt = time(NULL);
	ptr = localtime(&lt);
	int choice = purple_prefs_get_int("/plugins/core/g15purple/time_choice");
	switch(choice) {
		case 0:
			break;
		case 1:
			strftime(str, 100, "%H:%M:%S", ptr);
			break;
		case 2:
			strftime(str, 100, "%H:%M:%S %p", ptr);
			break;
		case 3:
			strftime(str, 100, "%I:%M:%S", ptr);
			break;
		case 4:
			strftime(str, 100, "%I:%M:%S %p", ptr);
			break;
		default:
			break;
	}
	*t = str;
}

/**
 * Sets the value of proto_icon to reflect the protocol of the buddy.
 *
 * @param buddy Buddy who's protocol is to be determined
 */
void set_protocol (PurpleBuddy *buddy)
{
	char *proto = buddy->account->protocol_id;

		if (strcmp(proto,"prpl-aim") == 0) proto_icon = aim;
		else if (strcmp(proto,"prpl-bigbrownchunx-facebookim") == 0) proto_icon = facebook;
		else if (strcmp(proto,"prpl-bigbrownchunx-skype") == 0) proto_icon = skype;
		else if (strcmp(proto,"prpl-bonjour") == 0) proto_icon = bonjour;
		else if (strcmp(proto,"prpl-facebook") == 0) proto_icon = facebook;
		else if (strcmp(proto,"prpl-gg") == 0) proto_icon = gadu;
		else if (strcmp(proto,"prpl-hangouts") == 0) proto_icon = hangouts;
		else if (strcmp(proto,"prpl-icq") == 0) proto_icon = icq;
		else if (strcmp(proto,"prpl-irc") == 0) proto_icon = irc;
		else if (strcmp(proto,"prpl-jabber") == 0) proto_icon = jabber;
		else if (strcmp(proto,"prpl-meanwhile") == 0) proto_icon = meanwhile;
		else if (strcmp(proto,"prpl-msn") == 0) proto_icon = msn;
		else if (strcmp(proto,"prpl-myspace") == 0) proto_icon = myspace;
		else if (strcmp(proto,"prpl-mxit") == 0) proto_icon = mxit;
		else if (strcmp(proto,"prpl-napster") == 0) proto_icon = napster;
		else if (strcmp(proto,"prpl-novell") == 0) proto_icon = novell;
		else if (strcmp(proto,"prpl-qq") == 0) proto_icon = qq;
		else if (strcmp(proto,"prpl-seishun-steam") == 0) proto_icon = steam;
		else if (strcmp(proto,"prpl-silc") == 0) proto_icon = silc;
		else if (strcmp(proto,"prpl-simple") == 0) proto_icon = simple;
		else if (strcmp(proto,"prpl-sipe") == 0) proto_icon = sipe;
		else if (strcmp(proto,"prpl-steam-mobile") == 0) proto_icon = steam;
		else if (strcmp(proto,"prpl-webqq") == 0) proto_icon = qq;
		else if (strcmp(proto,"prpl-yahoo") == 0) proto_icon = yahoo;
		else if (strcmp(proto,"prpl-zephyr") == 0) proto_icon = zephyr;
		
		// Everything else
		else proto_icon = unknown;


}

/**
 * Draws the date stored in the global variables g15message and g15title.
 * Originally, this was to be one of many screens, however that functionality
 * could not be created due to a combination how purple plugins are created
 * (it's not thread-safe) and my novice skills as a C programmer.
 *
 * @param canvas The G15 canvas which the data will be rendered to.
 * @bug
 */
void draw_messageWindow(g15canvas *canvas)
{
	int maxLength;
	int maxNum;

	switch(textSize) {
		case G15_TEXT_SMALL:
			maxLength = 40;
			maxNum = 5;
			break;
		case G15_TEXT_MED:
		default:
			maxLength = 32;
			maxNum = 5;
			break;
		case G15_TEXT_LARGE:
			maxLength = 20;
			maxNum = 5;
			break;
			
	}
	int textLen;
	int titleLoc;
	int totalMaxLen = maxLength * maxNum;

	g15r_clearScreen(canvas, G15_COLOR_WHITE);
	char *title;
	char *text;
	char *text1,*text2,*text3,*text4, *text5;
	char buff[totalMaxLen];
	char buff1[maxLength + 1],buff2[maxLength + 1],buff3[maxLength + 1],buff4[maxLength + 1],buff5[maxLength + 1];
	title = g15title;
	text = g15message;
	textLen = strlen(text);
	textLen = textLen < totalMaxLen ? textLen : totalMaxLen;
	
	memset(buff, 0, sizeof(buff));
	memset(buff1, 0, sizeof(buff1));
	memset(buff2, 0, sizeof(buff2));
	memset(buff3, 0, sizeof(buff3));
	memset(buff4, 0, sizeof(buff4));

	strncpy(buff, text,textLen);
	text = buff;

	strncpy(buff1,text,maxLength);
	strncpy(buff2,text+maxLength,maxLength);
	strncpy(buff3,text+maxLength*2,maxLength);
	strncpy(buff4,text+maxLength*3,maxLength);
	strncpy(buff5,text+maxLength*4,maxLength);

	text1= buff1;
	text2= buff2;
	text3= buff3;
	text4= buff4;
	text5= buff5;


	if(purple_prefs_get_bool("/plugins/core/g15purple/proto_icon")) {
		g15rx_drawXBM(canvas, proto_icon, proto_width, proto_height, 1, 0);
		titleLoc = 12;
	} else {
		titleLoc = 1;
	}

	if (textSize == G15_TEXT_SMALL){
		g15r_renderString (canvas, (unsigned char*)title, 0, textSize, titleLoc, 1);
		g15r_renderString (canvas, (unsigned char*)currTime, 0, textSize, 159-(4*strlen(currTime)), 1);
	} else {
		g15r_renderString (canvas, (unsigned char*)title, 0, textSize, titleLoc, 0);
		g15r_renderString (canvas, (unsigned char*)currTime, 0, textSize, 159-(5*strlen(currTime)), 0);
	}

	g15r_pixelReverseFill (canvas, 0, 0, 159, 6, G15_COLOR_WHITE, G15_COLOR_BLACK);

	g15r_renderString (canvas, (unsigned char*)text1, 0, textSize, 0, 8);
	g15r_renderString (canvas, (unsigned char*)text2, 0, textSize, 0, 15);
	g15r_renderString (canvas, (unsigned char*)text3, 0, textSize, 0, 22);
	g15r_renderString (canvas, (unsigned char*)text4, 0, textSize, 0, 29);
	g15r_renderString (canvas, (unsigned char*)text5, 0, textSize, 0, 36);

}

/**
 * Draws the welcome screen.
 *
 * @param canvas The G15 canvas which the data will be rendered to.
 * @image html welcome.jpg
 */
void draw_welcomeScreen(g15canvas * canvas)
{
	char* ver;
	char* ver1 = "v";
	char* ver2 = VERSION;
	ver = (char *)calloc(strlen(ver1) + strlen(ver2) + 1, sizeof(char));
	
	g15r_clearScreen(canvas, G15_COLOR_WHITE);
	
	g15rx_drawXBM(canvas, welcome, g15_res_width, g15_res_height, 0, 0);

	strcat(ver, ver1);
	strcat(ver, ver2);


	g15r_renderString (canvas, (unsigned char*)ver, 0, G15_TEXT_MED, 159-(5*strlen(ver)), 34);

	free(ver);

	canvas->mode_xor = 0;

	g15_send(screen,(char *)canvas->buffer,G15_BUFFER_LEN);

}


/**
 * Draws the closing screen.
 *
 * @param canvas The G15 canvas which the data will be rendered to.
 * @image html goodbye.jpg
 */
void draw_goodbyeScreen(g15canvas * canvas)
{

	g15r_clearScreen(canvas, G15_COLOR_WHITE);
	g15rx_drawXBM(canvas, goodbye, g15_res_width, g15_res_height, 0, 0);

	canvas->mode_xor = 0;
	g15_send(screen,(char *)canvas->buffer,G15_BUFFER_LEN);


}

/*

 Purple stuff below

*/

/* you must g_free the returned string
 * num_chars is utf-8 characters */
static gchar *
truncate_escape_string (const gchar *str,
						int num_chars)
{
	gchar *escaped_str;

	if (g_utf8_strlen (str, num_chars*2+1) > num_chars) {
		gchar *truncated_str;
		gchar *str2;

		/* allocate number of bytes and not number of utf-8 chars */
		str2 = g_malloc ((num_chars-1) * 2 * sizeof(gchar));

		g_utf8_strncpy (str2, str, num_chars-2);
		truncated_str = g_strdup_printf ("%s..", str2);
		escaped_str = g_markup_escape_text (truncated_str, strlen (truncated_str));
		g_free (str2);
		g_free (truncated_str);
	} else {
		escaped_str = g_markup_escape_text (str, strlen (str));
	}

	return escaped_str;
}

static gchar *
best_name (PurpleBuddy *buddy)
{
	if (buddy->alias) {
		return buddy->alias;
	} else if (buddy->server_alias) {
		return buddy->server_alias;
	} else {
		return buddy->name;
	}
}

static void
notify_msg_sent (PurpleAccount *account,
				 const gchar *sender,
				 const gchar *message)
{
	PurpleBuddy *buddy = purple_find_buddy (account, sender);
	if (!buddy)
		return;
	set_protocol(buddy);

	char *title;

	if(purple_prefs_get_bool("/plugins/core/g15purple/text_size")) {

		textSize = G15_TEXT_SMALL;
		title = truncate_escape_string(best_name(buddy), 24);
	} else {
		textSize = G15_TEXT_MED;
		title = truncate_escape_string(best_name(buddy), 17);
	}

	char *body = purple_markup_strip_html (message);

	g15title = (char*)title;
	g15message = (char*)body;
	get_time(&currTime);


	purple_debug_info (PLUGIN_ID, "g15purple(), new: "
					 "title: '%s', body: '%s', buddy: '%s'\n",
					 title, body, best_name(buddy));

	draw_messageWindow(canvas);

	canvas->mode_xor = 0;

	g15_send(screen,(char *)canvas->buffer,G15_BUFFER_LEN);

	g_free (title);

}

static void
notify_new_message_cb (PurpleAccount *account,
					   const gchar *sender,
					   const gchar *message,
					   int flags,
					   gpointer data)
{

	notify_msg_sent(account, sender, message);
}

static void
notify_chat_nick (PurpleAccount *account,
				  const gchar *sender,
				  const gchar *message,
				  PurpleConversation *conv,
				  gpointer data)
{
	gchar *nick;

	nick = (gchar *)purple_conv_chat_get_nick (PURPLE_CONV_CHAT(conv));
	if (nick && !strcmp (sender, nick))
		return;

	if (!g_strstr_len (message, strlen(message), nick))
		return;

	notify_msg_sent(account, sender, message);
}


static void
init_plugin(PurplePlugin *plugin)
{

	canvas = (g15canvas *) malloc (sizeof (g15canvas));
	g15title = "        G15 Purple Plugin Loaded        ";
	g15message = "When a message is received, it will be  shown here. --Kabili";

	purple_prefs_add_none("/plugins/core/g15purple");
	purple_prefs_add_bool("/plugins/core/g15purple/proto_icon", TRUE);
	purple_prefs_add_bool("/plugins/core/g15purple/text_size", FALSE);
	purple_prefs_add_int("/plugins/core/g15purple/time_choice", 1);
}

static gboolean
plugin_load (PurplePlugin *plugin)
{
	void *conv_handle;

	conv_handle = purple_conversations_get_handle ();

	screen = new_g15_screen(G15_G15RBUF);
	if(screen < 0) {
		purple_debug_info (PLUGIN_ID, "g15purple(), load: Failed to initialize screen. Is the g15daemon running?\n");
		purple_notify_error(plugin, "Err not", "Err notee",
			"Failed to initialize screen. Is the g15daemon running?");
		return FALSE;
	}
	purple_debug_info (PLUGIN_ID, "g15purple(), load: Initializing canvas?\n");
		
	g15r_initCanvas(canvas);
	draw_welcomeScreen(canvas);
		
	buddy_hash = g_hash_table_new (NULL, NULL);

	purple_signal_connect (conv_handle, "received-im-msg", plugin,
						PURPLE_CALLBACK(notify_new_message_cb), NULL);

	purple_signal_connect (conv_handle, "received-chat-msg", plugin,
						PURPLE_CALLBACK(notify_chat_nick), NULL);



	return TRUE;
}

/**
 * Actions to be performed when plugin is unloaded
 *
 * @bug Attempting to free() canvas causes libpurple to crash. Valgrind reports this
 * as an attempt to free() canvas twice. Unsure if this means libpurple or g15daemon
 * does this automatically.
 *
 */
static gboolean
plugin_unload (PurplePlugin *plugin)
{
	void *conv_handle;

	if(screen < 0) {
		return TRUE;
	}
	
	draw_goodbyeScreen(canvas);

	conv_handle = purple_conversations_get_handle ();

	purple_signal_disconnect (conv_handle, "received-im-msg", plugin,
							PURPLE_CALLBACK(notify_new_message_cb));

	purple_signal_disconnect (conv_handle, "received-chat-msg", plugin,
							PURPLE_CALLBACK(notify_chat_nick));


	//free(canvas);
	g15_close_screen(screen);
	
	g_hash_table_destroy (buddy_hash);

	return TRUE;
}


static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,   /* page_num (Reserved) */
	NULL, /* frame (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

/**
 * Plugin information
 *
 *
 *
 */
static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,        /* magic number */
	PURPLE_MAJOR_VERSION,       /* purple major */
	PURPLE_MINOR_VERSION,       /* purple minor */
	PURPLE_PLUGIN_STANDARD,     /* plugin type */
	NULL,                       /* UI requirement */
	0,                          /* flags */
	NULL,                       /* dependencies */
	PURPLE_PRIORITY_DEFAULT,    /* priority */

	PLUGIN_ID,                  /* id */
	PLUGIN_NAME,                /* name */
	VERSION,                    /* version */
	SUMMARY,                    /* summary */
	DESCRIPTION,                /* description */
	AUTHOR,                     /* author */
	WEBSITE,                    /* homepage */

	plugin_load,                /* load */
	plugin_unload,              /* unload */
	NULL,                       /* destroy */

	NULL,                       /* ui info */
	NULL,                       /* extra info */
	&prefs_info,                /* prefs info */
	NULL,                       /* actions */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL                        /* reserved */
};

PURPLE_INIT_PLUGIN(g15purple, init_plugin, info)
