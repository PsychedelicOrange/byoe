#include "player.h"
#include "camera_controller.h"

int game_main(void)
{
    REGISTER_GAME_OBJECT(playerUUID, Player, PlayerData, Player_Start, Player_Update);
    REGISTER_GAME_OBJECT(cameraUUID, Camera, CamData, Camera_Start, Camera_Update);

        uuid_t uuid = {0, 0, 0, 0};


    uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);

        uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);

        uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);


        uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);

        uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);

    return EXIT_SUCCESS;
}