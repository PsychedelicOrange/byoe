#include "test_hash_map.h"
#include "test_uuid.h"
#include "test_rng.h"
#include "test_sdf_scene.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Tests
    test_hash_map();
    test_uuid();
    test_rng();
    test_sdf_scene();

    return EXIT_SUCCESS;
}