#ifndef _APP_H
#define _APP_H

#include <cstdlib>
#include <cstdio>

#define SAFE_DELETE(x){delete x; x=NULL;}
#define SAFE_DELETEA(x){delete[]x; x=NULL;}
inline bool debug_enabled() {
    static bool checked = false;
    static bool enabled = false;
    if (!checked) {
        enabled = (getenv("DCC_DEBUG") != nullptr);
        checked = true;
    }
    return enabled;
}
#define DBG(...) do { if (debug_enabled()) fprintf(stderr, __VA_ARGS__); } while(0)
#endif