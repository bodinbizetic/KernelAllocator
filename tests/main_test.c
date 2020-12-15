#include "helper.h"

bool suite_buddy();
bool suite_buddy_2();
bool suite_slab();
bool suite_cache();

int main(int argc, char const *argv[])
{
    int cnt = 4;
    cnt -= suite_buddy();
    cnt -= suite_buddy_2();
    cnt -= suite_slab();
    cnt -= suite_cache();
    printf("\n\nSuites failed: %d\n", cnt);
    return 0;
}
