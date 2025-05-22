#include "include/game_resources.h"
Mix_Music* backgroundMusic = nullptr;

Mix_Chunk* soundEffects[3] = {nullptr};

const char* playerSkinNames[SKIN_COUNT][2] = {
    {"assets/images/players/default/player.png", "assets/images/players/default/player_on_target.png"},
    {"assets/images/players/alt1/player.png", "assets/images/players/alt1/player_on_target.png"},
    {"assets/images/players/alt2/player.png", "assets/images/players/alt2/player_on_target.png"},
    {"assets/images/players/alt3/player.png", "assets/images/players/alt3/player_on_target.png"},
    {"assets/images/players/alt4/player.png", "assets/images/players/alt4/player_on_target.png"},
    {"assets/images/players/alt5/player.png", "assets/images/players/alt5/player_on_target.png"},
    {"assets/images/players/alt6/player.png", "assets/images/players/alt6/player_on_target.png"},
    {"assets/images/players/alt7/player.png", "assets/images/players/alt7/player_on_target.png"}
};
