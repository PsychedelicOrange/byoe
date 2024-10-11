#include "player.h"
#include "camera_controller.h"

int game_main(void)
{
    REGISTER_GAME_OBJECT(playerUUID, Player, PlayerData, Player_Start, Player_Update);
    REGISTER_GAME_OBJECT(cameraUUID, Camera, CamData, Camera_Start, Camera_Update);


    return EXIT_SUCCESS;
}