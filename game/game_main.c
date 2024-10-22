#include "player.h"
#include "camera_controller.h"

#include <logging/log.h>
#include <rng/rng.h>

int game_main(void)
{
    REGISTER_GAME_OBJECT("Camera", 0, Camera_Start, Camera_Update);
    REGISTER_GAME_OBJECT("Player", PlayerData, Player_Start, Player_Update);

    return EXIT_SUCCESS;
}
