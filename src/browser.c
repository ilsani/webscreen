#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit2.h>
// #include <pthread.h>

#include "browser.h"
#include "str_utils.h"

#define USER_AGENT "Mozilla/5.0"

static int todo = 0;

// Private methods
// ///////////////////////////////////////////////////////////

static char* get_screenshot_filename(const char* target_uri);
static void done(Browser* browser);
static void take_webview_snapshot(WebKitWebView* webview, GAsyncResult* result, Browser* browser);
static void on_webview_load_changed(WebKitWebView* webview, WebKitLoadEvent status, Browser* browser);

// ///////////////////////////////////////////////////////////

// Public methods
// ///////////////////////////////////////////////////////////

void browser_navigate_to(Browser* browser, const char* target_url) {

  ++todo;

  browser->requested_uri = g_strdup(target_url);

  webkit_web_view_load_uri(WEBKIT_WEB_VIEW(browser->webview), target_url);
}

void browser_close(Browser* browser) {

  if (!browser)
    return;

  if (browser->out_dir)
    free(browser->out_dir);

  if (browser->requested_uri)
    free(browser->requested_uri);

  free(browser);

}

Browser* browser_create(const char* out_dir) {

  Browser* browser = malloc(sizeof(Browser));
  browser->out_dir = g_strdup(out_dir);

  GtkWidget* window = gtk_offscreen_window_new();
  gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);

  browser->webview = webkit_web_view_new();
  gtk_container_add(GTK_CONTAINER(window), browser->webview);

  WebKitSettings* settings = webkit_settings_new_with_settings(
      "user-agent", USER_AGENT,
      "enable-javascript", TRUE,
      "enable-java", FALSE,
      "enable-plugins", FALSE,
      "enable-private-browsing", TRUE,
      "enable-offline-web-application-cache", FALSE,
      "enable-page-cache", FALSE,
      NULL);
  
  webkit_web_view_set_settings(WEBKIT_WEB_VIEW(browser->webview), settings);
  
  g_signal_connect(browser->webview, "load-changed", G_CALLBACK(on_webview_load_changed), (Browser *)browser);

  // Ignore SSL errors
  webkit_web_context_set_tls_errors_policy(webkit_web_view_get_context(WEBKIT_WEB_VIEW(browser->webview)),
					   WEBKIT_TLS_ERRORS_POLICY_IGNORE);

  gtk_widget_show_all(window);
  
  return browser;
  
}


static char* get_screenshot_filename(const char* target_uri) {

  char* s1 = replace(target_uri, '/', "_");
  char* s2 = replace(s1, ':', "_");

  size_t len = strlen(s1) + strlen(s2) + 4 + 1;
  char* r = malloc(len);
  snprintf(r, len, "%s.png", s2);

  free(s1);
  free(s2);

  return r;
}


static void done(Browser* browser) {

  --todo;
  
  if (!todo) {
    browser_close(browser);
    gtk_main_quit();
  }

}

static void take_webview_snapshot(WebKitWebView* webview, GAsyncResult* result, Browser* browser) {

  GError* error = NULL;
  cairo_surface_t* surface = webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(webview), result, &error);

  if (error) {
    fprintf(stderr, "An error happened generating the snapshot: %s\n",error->message);
  }
  else {

    char* filename = get_screenshot_filename(browser->requested_uri);

    gchar out_file[4096];
    g_snprintf(out_file, sizeof(out_file), "%s/%s", browser->out_dir, filename);

    cairo_surface_write_to_png(surface, out_file);

    free(filename);

  }

  done(browser);

}

static void on_webview_load_changed(WebKitWebView* webview, WebKitLoadEvent status, Browser* browser) {

  if (status != WEBKIT_LOAD_FINISHED)
    return;

  // browser->current_uri = (char *)webkit_web_view_get_uri(webview);

  webkit_web_view_get_snapshot(webview,
			       WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
			       WEBKIT_SNAPSHOT_OPTIONS_NONE, // WEBKIT_SNAPSHOT_OPTIONS_TRANSPARENT_BACKGROUND ?
			       NULL,
			       (GAsyncReadyCallback)take_webview_snapshot,
			       browser);

}

// ///////////////////////////////////////////////////////////
