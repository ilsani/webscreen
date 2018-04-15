# webscreen
Simple tool to screenshot a website.

# Build
```
gcc -g -o webscreen main.c browser.c str_utils.c $(pkg-config --cflags --libs webkit2gtk-4.0)
```
or use `build.sh`.

# TODO
- [ ] Multithreading
