//level_design.h
/******************************************************************************
* LEVEL DESIGN
******************************************************************************/
#ifndef LEVEL_DESIGN_H_PARSED
#define LEVEL_DESIGN_H_PARSED


#include "main.h"
#include "game.h"


// OPTIONS ////////////////////////////////////////////////////////////////////

#define WARP_AROUND_SPAWNS_ENEMY	FALSE


// PROTOTYPES /////////////////////////////////////////////////////////////////

void populate_level( game_state_t* GS );


// Old routines

void generate_black_hole( game_state_t* GS );
void blackhole_attracts_ship( game_state_t* GS );

void advance_to_next_level( game_state_t* GS );
void prepare_first_level( game_state_t* GS );

void player_warped_around( game_state_t* GS );


#endif
//EOF
