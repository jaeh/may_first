//ui.c
/******************************************************************************
* USER INTERFACE
*******************************************************************************
* This module encapsulates everything needed to work with the operating system
* (desktop), like creating a window, handling input, etc.
******************************************************************************/

#include <SDL/SDL.h>

#include "ui.h"
#include "main.h"
#include "gl_helpers.h"
#include "draw_frame.h"
#include "world.h"
#include "game.h"
#include "level_design.h"
#include "player.h"


// INITIALIZATION /////////////////////////////////////////////////////////////

void init_sdl( program_state_t* PS, game_state_t* GS )
{
	int w = INITIAL_WINDOW_WIDTH;
	int h = INITIAL_WINDOW_HEIGHT;

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_WM_SetCaption( WINDOW_CAPTION, NULL );

#if START_IN_FULLSCREEN
	const SDL_VideoInfo* info = SDL_GetVideoInfo();
	w = info->current_w;
	h = info->current_h;
#else
	w = INITIAL_WINDOW_WIDTH;
	h = INITIAL_WINDOW_HEIGHT;
#endif

	PS->window_width  = w;
	PS->window_height = h;

	PS->screen = SDL_SetVideoMode(
		w, h,
		SCREEN_BPP,
		PS->screen_flags
	);
	if (PS->screen == NULL) {
		error_quit( ERROR_SDL_IMG_LOAD_RETURNED_NULL );
	}

	PS->mouse.visible_until_us = get_time() + MOUSE_CURSOR_VISIBLE_US;
}

void init_sound( program_state_t* PS, game_state_t* GS )
{
#if (PLAY_SOUNDS || PLAY_MUSIC)
	sounds_t* snd = &(GS->sounds);
#endif
#if PLAY_SOUNDS
	//if (Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 ) {
	if (Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) == -1 ) {
		error_quit( "Mix_OpenAudio() returned NULL" );
	}

	snd->laser  = NULL;	//... do I really need these initializations?
	snd->hit    = NULL;
	snd->punch  = NULL;
	snd->blast  = NULL;
	snd->denied = NULL;
	snd->alarm  = NULL;
	snd->music  = NULL;

	snd->laser  = Mix_LoadWAV( LASER_WAV  );
	snd->hit    = Mix_LoadWAV( HIT_WAV    );
	snd->punch  = Mix_LoadWAV( PUNCH_WAV  );
	snd->blast  = Mix_LoadWAV( BLAST_WAV  );
	snd->denied = Mix_LoadWAV( DENIED_WAV );
	snd->alarm  = Mix_LoadWAV( ALARM_WAV  );
	snd->blub   = Mix_LoadWAV( BLUB_WAV   );

	if (!snd->laser || !snd->hit || !snd->punch
	|| !snd->blast || !snd->denied || !snd->alarm) {
		error_quit( "One of the sounds could not be loaded" );
	}
#endif

#if PLAY_MUSIC
	snd->music = Mix_LoadMUS( GAME_MUSIC );

	if (snd->music == NULL) {
		error_quit( "The background music could not be loaded" );
	}
#endif

	PS->volume_fx = MIX_MAX_VOLUME / 2;
	PS->volume_music = MIX_MAX_VOLUME / 2;

#if (PLAY_SOUNDS || PLAY_MUSIC)
	Mix_Volume( -1, PS->volume_fx );
	Mix_VolumeMusic( PS->volume_music );
#endif
}

void init_font( program_state_t* PS )
{
	PS->font_size = FONT_SIZE * PS->window_height / 1050;
	PS->line_height = PS->font_size + 2;

	TTF_Init();
	PS->font = TTF_OpenFont( FONT_FILENAME, PS->font_size );
	if (PS->font == NULL) {
		error_quit( ERROR_SDL_TTF_OPENFONT_RETURNED_NULL );
	}
}


/******************************************************************************
* SDL HELPERS
******************************************************************************/

void play_sound( Mix_Chunk* sound )
{
#if PLAY_SOUNDS
	if (Mix_PlayChannel( -1, sound, 0 ) == -1) {
		//...error_quit( "play_sound: Mix_PlayChannel() returned -1" );
	}
#endif
}

void play_music( Mix_Music* music )
{
#if PLAY_MUSIC
	if (Mix_PlayingMusic() == 0) {
		if (Mix_PlayMusic(music, -1) == -1) {
			error_quit( "Mix_PlayMusic() returned -1" );
		}
	}
#endif
}

void toggle_music()
{
	if (Mix_PausedMusic() == 1) {
		Mix_ResumeMusic();
	}
	else {
		Mix_PauseMusic();
	}
}

void volume_up( program_state_t* PS, int channel )
{
	if (channel & VOLUME_FX) {
		PS->volume_fx += VOLUME_STEPS;
		if (PS->volume_fx > MIX_MAX_VOLUME) {
			PS->volume_fx = MIX_MAX_VOLUME;
		}
		Mix_Volume( -1, PS->volume_fx );
	}
	if (channel & VOLUME_MUSIC) {
		PS->volume_music += VOLUME_STEPS;
		if (PS->volume_music > MIX_MAX_VOLUME) {
			PS->volume_music = MIX_MAX_VOLUME;
		}
		Mix_VolumeMusic( PS->volume_music );
	}
}

void volume_down( program_state_t* PS, int channel )
{
	if (channel & VOLUME_FX) {
		PS->volume_fx -= VOLUME_STEPS;
		if (PS->volume_fx < 0) {
			PS->volume_fx = 0;
		}
		Mix_Volume( -1, PS->volume_fx );
	}

	if (channel & VOLUME_MUSIC) {
		PS->volume_music -= VOLUME_STEPS;
		if (PS->volume_music < 0) {
			PS->volume_music = 0;
		}
		Mix_VolumeMusic( PS->volume_music );
	}
}

void hide_cursor( void )
{
#ifdef DISABLED_CODE
	int x, y;

	SDL_GetMouseState( &x, &y );
	SDL_WarpMouse( x, y );	//...what for is this needed anyways?
#endif
	SDL_ShowCursor( SDL_DISABLE );
}

void show_cursor( void )
{
#ifdef DISABLED_CODE
	int x, y;

	SDL_GetMouseState( &x, &y );
	SDL_WarpMouse( x, y );	//...what for is this needed anyways?
#endif
	SDL_ShowCursor( SDL_ENABLE );

}


/******************************************************************************
* HANDLE UI EVENTS
******************************************************************************/

void do_QUIT( program_state_t* PS ) {
	PS->run_mode = RM_EXIT;
}

void do_RESIZE( program_state_t* PS )
{
	int w = PS->window_width;
	int h = PS->window_height;

	printf(
		"Registering new window size: %dx%d\n",
		PS->window_width,
		PS->window_height
	);


#if START_IN_FULLSCREEN
	int screen_flags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_FULLSCREEN;
#else
	int screen_flags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE;
#endif

	PS->screen = SDL_SetVideoMode(
		w, h,
		SCREEN_BPP,
		screen_flags
	);
	if (PS->screen == NULL) {
		error_quit( "Resize: SDL_SetVideoMode() returned NULL" );
	}
}

void toggle_full_screen( program_state_t* PS )
{
#ifdef FULLSCREEN_WORKS_WITH_OPENGL
	uint32_t old_flags = PS->screen_flags;	// Save the current flags..
						// ..in case toggling fails
	int w, h;

	if (PS->screen_flags & SDL_FULLSCREEN) {
		w = INITIAL_WINDOW_WIDTH;
		h = INITIAL_WINDOW_HEIGHT;
		PS->screen_flags &=~ SDL_FULLSCREEN;
	}
	else {
		const SDL_VideoInfo* info = SDL_GetVideoInfo();
		w = info->current_w;
		h = info->current_h;
		PS->screen_flags |= SDL_FULLSCREEN;
	}

	screen = SDL_SetVideoMode( w, h, SCREEN_BPP, PS->screen_flags );

	if (screen == NULL) {
		// If toggle FullScreen failed, then switch back
		screen = SDL_SetVideoMode(0, 0, 0, old_flags);
	}

	if (screen == NULL) {
		// If you can't switch back for some reason, then epic fail
		error_quit(
			"SDL_SetVideoMode() returned NULL"
			" when toggling fullscreen."
		);
	}
#endif
}

void test( program_state_t* PS, game_state_t* GS )	//...
{
	int i, j;
	formation_t* f = &(GS->formations[0]);

	for( i = 0 ; i < MAX_FORMATION_RANKS ; i++ ) {

		printf("Slot %d:", i);

		for( j = 0 ; j < NR_FILLFROM_RANKS ; j++ ) {
			printf( " %d", f->ranks[6].fillfrom_index[i] );
		}
		printf("\n");
	}


	enemy_t* e = GS->formations[0].ranks[0].occupied_by;

	e->ai_mode = FREE;

	if (sgn(e->velocity.x) == sgn(e->formation->velocity.x)) {
		e->velocity.x *= (-1);
	}
}


void handle_keydown( program_state_t* PS, game_state_t* GS, int ksym )
{
	switch (ksym) {

	case SDLK_a:						// fall through
	case SDLK_LEFT:		start_move( PS, GS, LEFT );		break;
	case SDLK_d:						// fall through
	case SDLK_RIGHT:	start_move( PS, GS, RIGHT );		break;
	case SDLK_w:						// fall through
	case SDLK_UP:		start_move( PS, GS, FORWARD );		break;
	case SDLK_s:						// fall through
	case SDLK_DOWN:		start_move( PS, GS, BACK );		break;
#if DEBUG
	case SDLK_k:		remove_all_objects( GS );		break;
#endif
	case SDLK_m:		toggle_music();				break;
	case SDLK_r:		reset_game( PS, GS );			break;
	case SDLK_t:		test( PS, GS );				break;
	case SDLK_PLUS:		volume_up  ( PS, VOLUME_ALL );		break;
	case SDLK_MINUS:	volume_down( PS, VOLUME_ALL );		break;
	case SDLK_KP_PLUS:	volume_up  ( PS, VOLUME_FX );		break;
	case SDLK_KP_MINUS:	volume_down( PS, VOLUME_FX );		break;
	case SDLK_KP_MULTIPLY:	volume_up  ( PS, VOLUME_MUSIC );	break;
	case SDLK_KP_DIVIDE:	volume_down( PS, VOLUME_MUSIC );	break;
	case SDLK_COMMA:					// fall through
	case SDLK_RCTRL:					// fall through
	case SDLK_LCTRL:
		if (PS->run_mode == RM_RUNNING) {
			start_fire( PS, GS, WEAPON_LASER_1, FM_SINGLE );
		}
		break;
	case SDLK_PERIOD:					// fall through
	case SDLK_RSHIFT:					// fall through
	case SDLK_LSHIFT:					// fall through
	case SDLK_LSUPER:
		if (PS->run_mode == RM_RUNNING) {
			start_fire( PS, GS, WEAPON_LASER_2, FM_SINGLE );
		}
		break;
	case SDLK_RALT:						// fall through
	case SDLK_LALT:
		if (PS->run_mode == RM_RUNNING) {
			start_round_shot( PS, GS, FM_SINGLE );
		}
		break;
	case SDLK_RETURN:
	//case SDLK_SPACE:
		if (PS->run_mode & (RM_INTRO | RM_MAIN_MENU | RM_AFTER_LIFE)) {
			reset_game( PS, GS );
		}
		else {
			toggle_pause( PS );
		}
		break;

	case SDLK_ESCAPE:
		switch (PS->run_mode) {
			case RM_INTRO:
			case RM_PAUSE:
			case RM_MAIN_MENU:	do_QUIT( PS );
		}
								// fall through
	case SDLK_p:						// fall through
	case SDLK_PAUSE:	toggle_pause( PS );			break;
	case SDLK_F3:		PS->debug = !PS->debug;			break;
	case SDLK_F11:		toggle_full_screen( PS );		break;
	case SDLK_F12:		error_quit("User abort [F12]");		break;

	}
}

void handle_keyup( program_state_t* PS, game_state_t* GS, int ksym )
{
	switch (ksym) {
		case SDLK_a:					// fall through
		case SDLK_LEFT:		stop_move( PS, GS, LEFT );	break;
		case SDLK_d:					// fall through
		case SDLK_RIGHT:	stop_move( PS, GS, RIGHT );	break;
		case SDLK_w:					// fall through
		case SDLK_UP:		stop_move( PS, GS, FORWARD );	break;
		case SDLK_s:					// fall through
		case SDLK_DOWN:		stop_move( PS, GS, BACK );	break;
	}
}

void handle_mouse( program_state_t* PS )
{
	mouse_t* m = &(PS->mouse);
	microtime_t T = get_time();

	SDL_GetMouseState( &(m->x), &(m->y) );

	if ((abs(m->x - m->previous_x) > 3)
	&& (abs(m->y - m->previous_y) > 3))
	{
		show_cursor();
		m->visible_until_us = T + MOUSE_CURSOR_VISIBLE_US;
	}
	else {
		if (T > m->visible_until_us) {
			hide_cursor();
		}
	}

	m->previous_x = m->x;
	m->previous_y = m->y;
}

void process_event_queue( program_state_t* PS, game_state_t* GS )
{
	SDL_Event event;

	uint8_t *keystates = SDL_GetKeyState( NULL );

	handle_mouse( PS );	// Update internal mouse coordinates

	// Auto-Fire
	if (PS->run_mode == RM_RUNNING) {

		if (	keystates[SDLK_LCTRL]
		||	keystates[SDLK_RCTRL]
		||	keystates[SDLK_COMMA]
		) {
			continue_fire( PS, GS, WEAPON_LASER_1 );
		}

		if (	keystates[SDLK_LSHIFT]
		||	keystates[SDLK_LSUPER]
		||	keystates[SDLK_RSHIFT]
		||	keystates[SDLK_RALT]
		) {
			continue_fire( PS, GS, WEAPON_LASER_2 );
		}

		if (	keystates[SDLK_LALT]
		||	keystates[SDLK_RALT]
		) {
			continue_fire( PS, GS, WEAPON_ROUNDSHOT );
		}
	}

	// Process event queue
	while (SDL_PollEvent(&event)) {

		switch (event.type) {
			case SDL_VIDEORESIZE:
				PS->window_width  = event.resize.w;
				PS->window_height = event.resize.h;
				do_RESIZE( PS );
				break;

			case SDL_ACTIVEEVENT:
				if ((event.active.gain != 1)
				&& (PS->run_mode == RM_RUNNING)
				) {
					// Pause game, when the window..
					// ..looses input focus
					PS->run_mode = RM_PAUSE;
				}
				break;

			case SDL_KEYDOWN:
				handle_keydown( PS, GS, event.key.keysym.sym );
				break;

			case SDL_KEYUP:
				handle_keyup( PS, GS, event.key.keysym.sym );
				break;

#ifdef DISABLED_CODE
			case SDL_MOUSEMOTION:
				PS->mouse.position_x = event.motion.x;
				PS->mouse.position_y = event.motion.y;
				break;
#endif

			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					PS->mouse.button.left = TRUE;
					break;
				case SDL_BUTTON_RIGHT:
					PS->mouse.button.right = TRUE;
					break;
				case SDL_BUTTON_WHEELUP:
					break;
				case SDL_BUTTON_WHEELDOWN:
					break;
				}
				break;

			case SDL_MOUSEBUTTONUP:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					PS->mouse.button.left = FALSE;
					break;
				case SDL_BUTTON_RIGHT:
					PS->mouse.button.right = FALSE;
					break;
				}
				break;

			case SDL_QUIT:
				do_QUIT( PS );
				break;
		}
	}
}


//EOF
