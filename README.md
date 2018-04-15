# webscreen
Simple tool to screenshot a website.

# Build
```
gcc -g -o webscreen main.c browser.c str_utils.c $(pkg-config --cflags --libs webkit2gtk-4.0)
```
or use `build.sh`.

# Usage
```
./webscreen -o $(pwd)/screens -u http://google.com
```
or
```
./webscreen -o $(pwd)/screens -u http://google.com:8080
```
or
```
./webscreen -o $(pwd)/screens -u file:///dev/shm/test.html
```

`-i` flag is not implemented, yet. To process multiple URLs you could use:
```
for url in $(cat targets.txt); do ./webscreen -o $(pwd)/screens -u ${url}; done
```

if you haven't X you could use:
```
xvfb-run -a ./webscreen -o $(pwd)/screens -u http://google.com
```

# TODO
- [ ] Multithreading

# Libraries
* https://webkitgtk.org
