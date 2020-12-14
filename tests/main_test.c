#include "helper.h"

bool suite_buddy();
bool suite_buddy_2();
bool suite_slab();

int main(int argc, char const *argv[])
{
    suite_buddy();
    suite_buddy_2();
    suite_slab();
    return 0;
}
