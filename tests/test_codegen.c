#include <assert.h>
#include "codegen.h"

int main() {
    int r = codegen("tests/test_files/dummy.co");
    assert(r == 0);
    return 0;
}

