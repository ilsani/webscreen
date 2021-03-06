
#ifndef __BROWSER_H__
#define __BROWSER_H__

typedef struct {

  GtkWidget* webview;
  char* out_dir;
  char* requested_uri;

} Browser;

extern void browser_navigate_to(Browser* browser, const char* target_url);
extern Browser* browser_create(const char* out_dir);
extern void browser_close(Browser* browser);

#endif


