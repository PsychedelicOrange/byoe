#include "player.h"
#include "camera_controller.h"

int game_main(void)
{
    REGISTER_GAME_OBJECT(playerUUID, "Player", PlayerData, Player_Start, Player_Update);
    REGISTER_GAME_OBJECT(cameraUUID, "Camera", CamData, Camera_Start, Camera_Update);

    for (size_t i = 0; i < MAX_OBJECTS - 2; i++)
    {
        uuid_t goUUID;
        char playerName[250];
        sprintf(playerName, "Player-%zd", i);
        REGISTER_GAME_OBJECT(goUUID, playerName, PlayerData, Player_Start, Player_Update);

        uuid_print(&goUUID);
    }

    return EXIT_SUCCESS;
}