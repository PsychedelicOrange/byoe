#include "player.h"

int game_main_register_gameobjects()
{
    REGISTER_GAME_OBJECT(playerUUID, Player, PlayerData, Player_Start, Player_Update);

    return EXIT_SUCCESS;
}