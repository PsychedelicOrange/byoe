#include "player.h"
#include "camera_controller.h"

#include <logging/log.h>

int game_main(void)
{
    REGISTER_GAME_OBJECT("Player", PlayerData, Player_Start, Player_Update);
    REGISTER_GAME_OBJECT("Camera", 0, Camera_Start, Camera_Update);

    for (size_t i = 0; i < 10; i++)
    {
        uuid_t goUUID;
        char playerName[250];
        sprintf(playerName, "Player-%zd", i);
        goUUID = REGISTER_GAME_OBJECT(playerName, PlayerData, Player_Start, Player_Update);
        uuid_print(&goUUID);
    }
    
    return EXIT_SUCCESS;
}