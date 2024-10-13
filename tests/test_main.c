#include "test_hash_map.h"
#include "test_uuid.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    test_hash_map();
    test_uuid();
    return EXIT_SUCCESS;
}