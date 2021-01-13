#define _CRT_NO_SECURE_WARNINGS

#include <stdio.h>
#include <string.h> //for strlen()
#include <stdlib.h> //for srand() and rand()
#include <time.h> //for time(NULL)

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define MAX_OBSTACLE_ON_SCREEN_COUNT 7 
#define DEFAULT_OBSTACLE_DIMENSION 30
#define DEFAULT_UNICORN_DIMENSION 40
#define UP_KEY_UNICORN_MOVE 20
#define MOVE_OBSTACLE_VELOCITY 100 //SINGLE OBJECT MOVEMENT OPERATION MOVES AND OBSTACLE BY 1 PIXEL
#define GRAVITY_CONSTANT 100
#define UNICORN_ACCELERATION 0.01 //PIXELS PER SECOND^2
#define RUSH_TIME 0.4 //HOW LONG THE RUSH LASTS
#define JUMP_HEIGHT 80 //HEIGHT OF UNICORN JUMP
#define JUMP_PRESSED_HEIGHT_MULTIPLIER 0.8 //x-delta x THE DESTINATION ALTITUDE IS MULTIPLIED BY THIS VALUE <1 because xy are inverted
#define LONG_JUMP_MIN_TIME 0.05 //MIN TIME PRESSING JUMP KEY TO 
#define OBJECTS_MIN_BETWEEN_DISTANCE 130 //minimum distance between each obstacle in pixels
#define LIFES_COUNT 3
#define HEART_SIZE 40 //icon size
#define PLATFORM_LENGTH 220
#define PLATFORM_HEIGHT 40
#define PLATFORMS_HMAX_DISTANCE 100 //max distance between platforms vertically
#define PLATFORM_WMIN_DISTANCE 70 //min distance between platforms horizontally
#define JUMP_LIMIT 10 //MAX HEIGHT TO JUMP

enum rushes_or_not {
	does_not_rush = 1,
	rushes = 2,
};

enum jump_stage {
	never_jumped,
	single_jump,
	double_jump,
};

enum steering_ways {
	normal,
	advanced,
};

enum where_am_i {
	in_game,
	score_screen,
	exit_stage,
};

typedef struct Unicorn_Positioning {
	double position_x;
	double position_y;
	double unicorn_speed;
}unicorn_position;

typedef struct Obstacles_Positioning {
	double position_x;
	double position_y;
}ObstaclesPositions;

typedef struct WholePositioning {
	unicorn_position unicorn_double_cordinates; //Double for more precise calculations
	SDL_Rect unicorn_cordinates; //Only for rendering
	SDL_Rect* obstacle_cordinates;
	ObstaclesPositions* obstacle_double_cordinates;
	SDL_Rect rectangle_background_image; //For background image locationing
	SDL_Rect rectangle_hearts[LIFES_COUNT];
	SDL_Rect* rectangle_platform_image;
	ObstaclesPositions* rectangle_double_platform_image;
}PositioningType;

typedef struct Images { //loaded bitmaps
	SDL_Surface* background;
	SDL_Surface* background_info;
	SDL_Surface* unicorn;
	SDL_Surface* obstacle;
	SDL_Surface* charset;
	SDL_Surface* screen;
	SDL_Surface* heart;
	SDL_Surface* platform;
}ImagesType;

typedef struct time_struct {
	int time_start;
	int time_end;
	double time_delta;
	double gameplay_time;
	double jump_press_time; //how long is the jump key pressed
	int jump_press_time_t1; //counter for ^
}TimeType;

typedef struct frame_struct {
	double fpsTimer;
	double fps;
	int frames;
}FrameType;

typedef struct unicorn_struct {
	int jump_stage; //0 1 2 0-did not, 1-once, 2-twice
	bool is_jumping; //false or true
	double jumping_destination_altitude; //altitude that unicorn reaches when jumping up
	bool did_rush; //check if rushed already, regeneration = touch ground
	int is_rushing; //does_not_rush if not, rushes if does
	double time_rush; // from define
}UnicornType;

typedef struct WholeGame {
	SDL_Event event;
	SDL_Texture* screen_texture;
	SDL_Window* window;
	SDL_Renderer* renderer;
	PositioningType objects_positions; //positions of all object on screen
	ImagesType images; //here are loaded bitmaps
	TimeType time_variables;
	FrameType frame_variables; //fps variables
	UnicornType unicorn_variables;
	int stage; //Depends if Fail Screen or in game
	int quit;
	int steering_way; //normal or advanced 1 point of "not required" requirements
	char printf_text[128]; //for printf time and fps using sprintf_s
	int lifes_count;
}GameValues;

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
                SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};

void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void InitiateObstacles(GameValues* game_values) {
	SDL_Rect* obstacle_array = (SDL_Rect*)malloc(MAX_OBSTACLE_ON_SCREEN_COUNT * sizeof(SDL_Rect));
	ObstaclesPositions* obstacle_double_array = (ObstaclesPositions*)malloc(MAX_OBSTACLE_ON_SCREEN_COUNT * sizeof(ObstaclesPositions));
	SDL_Rect* platforms_array = (SDL_Rect*)malloc(MAX_OBSTACLE_ON_SCREEN_COUNT * sizeof(SDL_Rect));
	ObstaclesPositions* platforms_double_array = (ObstaclesPositions*)malloc(MAX_OBSTACLE_ON_SCREEN_COUNT * sizeof(ObstaclesPositions)); //double means more precise calculations
	if (obstacle_array == NULL || obstacle_double_array == NULL || platforms_array == NULL || platforms_double_array == NULL) {
		game_values->quit = 1;
		game_values->stage = exit_stage;
		return;
	}
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		obstacle_double_array[i].position_x = -1.5 * DEFAULT_OBSTACLE_DIMENSION;
		obstacle_double_array[i].position_y = double(SCREEN_HEIGHT)-double(DEFAULT_OBSTACLE_DIMENSION);
		obstacle_array[i].h = DEFAULT_OBSTACLE_DIMENSION;
		obstacle_array[i].w = DEFAULT_OBSTACLE_DIMENSION;
		platforms_double_array[i].position_x = -PLATFORM_LENGTH;
		platforms_double_array[i].position_y = double(SCREEN_HEIGHT) - double(DEFAULT_OBSTACLE_DIMENSION);
		platforms_array[i].x = -PLATFORM_LENGTH;
		platforms_array[i].y = SCREEN_HEIGHT - DEFAULT_OBSTACLE_DIMENSION;
	}
	game_values->objects_positions.obstacle_cordinates = obstacle_array;
	game_values->objects_positions.obstacle_double_cordinates = obstacle_double_array;
	game_values->objects_positions.rectangle_double_platform_image = platforms_double_array;
	game_values->objects_positions.rectangle_platform_image = platforms_array;
}

void EasyMoveObstacles(GameValues* game_values) { //For Normal Steering using arrows
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		game_values->objects_positions.obstacle_double_cordinates[i].position_x -= MOVE_OBSTACLE_VELOCITY * 0.05;
		game_values->objects_positions.rectangle_double_platform_image[i].position_x -= MOVE_OBSTACLE_VELOCITY * 0.05;
	}
}

void MoveObstacles(GameValues* game_values) {
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		game_values->objects_positions.obstacle_double_cordinates[i].position_x -= MOVE_OBSTACLE_VELOCITY * game_values->time_variables.time_delta * game_values->unicorn_variables.is_rushing * game_values->objects_positions.unicorn_double_cordinates.unicorn_speed;
		game_values->objects_positions.rectangle_double_platform_image[i].position_x -= MOVE_OBSTACLE_VELOCITY * game_values->time_variables.time_delta * game_values->unicorn_variables.is_rushing * game_values->objects_positions.unicorn_double_cordinates.unicorn_speed;
	}
}

void CreateObstacle(GameValues* game_values) {
	ObstaclesPositions* obstacle_array = game_values->objects_positions.obstacle_double_cordinates;
	int should_create = rand() % 8500; //ELEMENTS OF RANDOMNESS
	if (should_create % 31 == 0) {
		int which_one = rand() % MAX_OBSTACLE_ON_SCREEN_COUNT;
		if (obstacle_array[which_one].position_x < -DEFAULT_OBSTACLE_DIMENSION) {
			for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
				if (SCREEN_WIDTH - obstacle_array[i].position_x < OBJECTS_MIN_BETWEEN_DISTANCE) {
					return;
				}
			}
			obstacle_array[which_one].position_x = SCREEN_WIDTH;
			obstacle_array[which_one].position_y = double(SCREEN_HEIGHT) - double(DEFAULT_OBSTACLE_DIMENSION);
			for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
				if (game_values->objects_positions.rectangle_double_platform_image[i].position_x + PLATFORM_LENGTH - DEFAULT_OBSTACLE_DIMENSION > SCREEN_WIDTH) {
					obstacle_array[which_one].position_y -= (SCREEN_HEIGHT - game_values->objects_positions.rectangle_double_platform_image[i].position_y);
					return;
				}
			}
		}
	}
}

double ClosestPlatformHeight(ObstaclesPositions* platforms_array, int which) { //returns closest platform in front of one. If there is no such first platform is returned
	double closest_height = platforms_array[0].position_y;
	double closest_distance = platforms_array[which].position_x - platforms_array[0].position_x;
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		if (platforms_array[which].position_x - platforms_array[i].position_x < closest_distance && platforms_array[which].position_x - platforms_array[i].position_x > 0 && i != which) {
			closest_distance = platforms_array[which].position_x - platforms_array[i].position_x;
			closest_height = platforms_array[i].position_y;
		}
	}
	return closest_height;
}

void CreatePlatforms(ObstaclesPositions* platforms_array) {
	int should_create = rand() % 8500;
	if (should_create % 31 == 0) {
		int which_one = rand() % MAX_OBSTACLE_ON_SCREEN_COUNT; //again, elements of randomness
		if (platforms_array[which_one].position_x < -PLATFORM_LENGTH) {
			for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
				if (SCREEN_WIDTH - platforms_array[i].position_x < double(PLATFORM_WMIN_DISTANCE) + double(PLATFORM_LENGTH)) {
					return;
				}
			}
			platforms_array[which_one].position_x = SCREEN_WIDTH;
			double previous_platform_height = ClosestPlatformHeight(platforms_array, which_one); //here V the height of the obstacle is calculated so that the unicorn will be able to jump onto it
			int y = rand() % SCREEN_HEIGHT;
			while (y > previous_platform_height + PLATFORMS_HMAX_DISTANCE || y<previous_platform_height - PLATFORMS_HMAX_DISTANCE 
				|| y > SCREEN_HEIGHT - 2 * DEFAULT_OBSTACLE_DIMENSION - PLATFORM_HEIGHT || y < JUMP_LIMIT+50) {
				y = rand() % SCREEN_HEIGHT;
			}
			platforms_array[which_one].position_y = y;
		}
	}
}

bool EasyRoof(GameValues* game_values) { //Check if the unicorn hit the roof. If so returns true, otherwise returns false. Only for Normal Steering (using arrows).
	ObstaclesPositions* platforms_array = game_values->objects_positions.rectangle_double_platform_image;
	double x1 = game_values->objects_positions.unicorn_double_cordinates.position_x;
	double y1 = game_values->objects_positions.unicorn_double_cordinates.position_y;
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		if (platforms_array[i].position_x <= x1 + double(DEFAULT_UNICORN_DIMENSION) && platforms_array[i].position_x + PLATFORM_LENGTH >= x1 //CHECK IF BETWEEN HEIGHTS
			&& abs(int(y1 - game_values->objects_positions.rectangle_double_platform_image[i].position_y - PLATFORM_HEIGHT)) < UP_KEY_UNICORN_MOVE/2) {
			return true;
		}
	}
	return false;
}

void HitTheRoof(GameValues* game_values) {//Checks if the unicorn hit the roof. If so it cancels jumping. Only for Advanced Steering (using z).
	ObstaclesPositions* platforms_array = game_values->objects_positions.rectangle_double_platform_image;
	double x1 = game_values->objects_positions.unicorn_double_cordinates.position_x;
	double y1 = game_values->objects_positions.unicorn_double_cordinates.position_y;
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		if (platforms_array[i].position_x <= x1 + double(DEFAULT_UNICORN_DIMENSION) && platforms_array[i].position_x + PLATFORM_LENGTH >= x1 //CHECK IF BETWEEN HEIGHTS
			&& abs(int(y1 - game_values->objects_positions.rectangle_double_platform_image[i].position_y - PLATFORM_HEIGHT)) < 1) {
			game_values->unicorn_variables.is_jumping = 0;
			game_values->unicorn_variables.time_rush = 0;
		}
	}
}

bool CheckCollision(GameValues* game_values) { //Checks collision with obstacles. If so it returns true.
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {//WITH OBSTACLE
		if (game_values->objects_positions.obstacle_double_cordinates[i].position_x <= DEFAULT_UNICORN_DIMENSION && game_values->objects_positions.obstacle_double_cordinates[i].position_x+DEFAULT_OBSTACLE_DIMENSION > 0){
			//CHECK COLLISION ON GROUND
			if ((SCREEN_HEIGHT - game_values->objects_positions.unicorn_double_cordinates.position_y - DEFAULT_UNICORN_DIMENSION) < DEFAULT_OBSTACLE_DIMENSION
				&& (SCREEN_HEIGHT - game_values->objects_positions.obstacle_double_cordinates[i].position_y == DEFAULT_OBSTACLE_DIMENSION)
				&& game_values->objects_positions.unicorn_double_cordinates.position_y + DEFAULT_UNICORN_DIMENSION >= double(SCREEN_HEIGHT) - double(DEFAULT_OBSTACLE_DIMENSION)) {
				return true;
			}
			for (int j = 0; j < MAX_OBSTACLE_ON_SCREEN_COUNT; j++) {
				if (game_values->objects_positions.rectangle_double_platform_image[j].position_x <= DEFAULT_UNICORN_DIMENSION 
					&& game_values->objects_positions.rectangle_double_platform_image[j].position_x + PLATFORM_LENGTH > 0){
					if (abs(int(game_values->objects_positions.rectangle_double_platform_image[j].position_y - game_values->objects_positions.unicorn_double_cordinates.position_y - DEFAULT_UNICORN_DIMENSION)) <= DEFAULT_OBSTACLE_DIMENSION
						&& (game_values->objects_positions.rectangle_double_platform_image[j].position_y - game_values->objects_positions.unicorn_double_cordinates.position_y) > 0
						&& (game_values->objects_positions.obstacle_double_cordinates[i].position_y < (double(SCREEN_HEIGHT) - double(DEFAULT_OBSTACLE_DIMENSION)))) {
						return true; //COLLISION ON PLATFORM
					}
				}
			}
		}//WITH PLATFORM
		if (abs(int(game_values->objects_positions.rectangle_double_platform_image[i].position_x - game_values->objects_positions.unicorn_double_cordinates.position_x - DEFAULT_UNICORN_DIMENSION))<1 //Check IF X is correct.
			&& game_values->objects_positions.rectangle_double_platform_image[i].position_y < game_values->objects_positions.unicorn_double_cordinates.position_y + DEFAULT_UNICORN_DIMENSION //CHECK IF BETWEEN HEIGHTS
			&& game_values->objects_positions.rectangle_double_platform_image[i].position_y + PLATFORM_HEIGHT > game_values->objects_positions.unicorn_double_cordinates.position_y) { //Note the collision can only happen on one side of the platform
			return true; //^the platform is lower than the unicorn
		}
	}
	return false;
}

bool SDLSetup(GameValues* game_values) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return false;
	}
	if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &game_values->window, &game_values->renderer) != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return false;
	};
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(game_values->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(game_values->renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(game_values->window, "Projekt 2 Podstawy Programowania Wojciech Panfil");
	game_values->images.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	game_values->screen_texture = SDL_CreateTexture(game_values->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_ShowCursor(SDL_DISABLE);
	return true;
}

bool LoadImages(GameValues* game_values) {
	game_values->images.charset = SDL_LoadBMP("Grafika/cs8x8.bmp");
	game_values->images.background = SDL_LoadBMP("Grafika/background2.bmp");
	game_values->images.background_info = SDL_LoadBMP("Grafika/background_information.bmp");
	game_values->images.unicorn = SDL_LoadBMP("Grafika/jednorozec.bmp");
	game_values->images.obstacle = SDL_LoadBMP("Grafika/small_stone.bmp");
	game_values->images.heart = SDL_LoadBMP("Grafika/heart.bmp");
	game_values->images.platform = SDL_LoadBMP("Grafika/platform_220.bmp");
	
	//CHECK IF LOADING SUCCEEDED
	if (game_values->images.charset == NULL || game_values->images.background == NULL || 
		game_values->images.background_info == NULL || game_values->images.unicorn == NULL || game_values->images.obstacle == NULL || game_values->images.heart == NULL) {
		return false;
	}
	SDL_SetColorKey(game_values->images.heart, true, 0xffffff);
	SDL_SetColorKey(game_values->images.unicorn, true, 0xffffff);
	SDL_SetColorKey(game_values->images.charset, true, 0x000000);
	SDL_SetColorKey(game_values->images.platform, true, 0xffffff);
	return true;
}

void FreeMemoryAfterSDL(GameValues* game_values) {
	SDL_FreeSurface(game_values->images.charset);
	SDL_FreeSurface(game_values->images.screen);
	SDL_FreeSurface(game_values->images.background);
	SDL_FreeSurface(game_values->images.background_info);
	SDL_FreeSurface(game_values->images.obstacle);
	SDL_FreeSurface(game_values->images.unicorn);
	SDL_FreeSurface(game_values->images.heart);
	SDL_FreeSurface(game_values->images.platform);
	SDL_DestroyTexture(game_values->screen_texture);
	SDL_DestroyWindow(game_values->window);
	SDL_DestroyRenderer(game_values->renderer);
	free(game_values->objects_positions.obstacle_cordinates);
	free(game_values->objects_positions.obstacle_double_cordinates);
	free(game_values->objects_positions.rectangle_platform_image);
	free(game_values->objects_positions.rectangle_double_platform_image);
	SDL_Quit();
}

bool InitializeTheRest(GameValues* game_values, int lifes_count, double time) { //Sets up all the variables to let the game begin!
	game_values->objects_positions.rectangle_background_image.x = 0;
	game_values->objects_positions.rectangle_background_image.y = 0;
	game_values->objects_positions.unicorn_cordinates.h = DEFAULT_UNICORN_DIMENSION;
	game_values->objects_positions.unicorn_cordinates.w = DEFAULT_UNICORN_DIMENSION;
	game_values->objects_positions.unicorn_cordinates.x = 0;
	game_values->objects_positions.unicorn_cordinates.y = SCREEN_HEIGHT - DEFAULT_UNICORN_DIMENSION;
	game_values->objects_positions.unicorn_double_cordinates.position_x = 0;
	game_values->objects_positions.unicorn_double_cordinates.position_y = game_values->objects_positions.unicorn_cordinates.y;
	game_values->unicorn_variables.is_jumping = false;
	game_values->time_variables.time_start = SDL_GetTicks();
	game_values->frame_variables.frames = 0;
	game_values->frame_variables.fpsTimer = 0;
	game_values->frame_variables.fps = 0;
	game_values->time_variables.gameplay_time = time;
	game_values->quit = false;
	game_values->stage = in_game;
	game_values->objects_positions.unicorn_double_cordinates.unicorn_speed = 1.0;
	game_values->steering_way = normal;
	game_values->unicorn_variables.jump_stage = never_jumped;
	game_values->unicorn_variables.did_rush = false;
	game_values->unicorn_variables.is_rushing = does_not_rush; //ONE IF NOT, TWO IF RUSHES (TWICE FASTER)
	game_values->unicorn_variables.time_rush = 0;
	game_values->time_variables.jump_press_time = 0;
	game_values->lifes_count = lifes_count;
	game_values->time_variables.jump_press_time_t1 = 0;
	for (int i = 0; i < LIFES_COUNT; i++) {
		game_values->objects_positions.rectangle_hearts[i].y = int(double(-SCREEN_HEIGHT) / 2.22);
		game_values->objects_positions.rectangle_hearts[i].x = i * HEART_SIZE;
	}
	return true;
}

bool EasyIsUnicornOnPlatform(GameValues* game_values) {// Checks if the unicorn is on platform. Only for Normal Steering (using arrows).
	double x1 = game_values->objects_positions.unicorn_double_cordinates.position_x;
	double y1 = game_values->objects_positions.unicorn_double_cordinates.position_y;
	ObstaclesPositions* platforms_array = game_values->objects_positions.rectangle_double_platform_image;
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		if (platforms_array[i].position_x < x1 + double(DEFAULT_UNICORN_DIMENSION) && platforms_array[i].position_x + PLATFORM_LENGTH > x1 
			&& platforms_array[i].position_y - y1 - DEFAULT_UNICORN_DIMENSION <= UP_KEY_UNICORN_MOVE
			&& y1<platforms_array[i].position_y) {
			while (int(platforms_array[i].position_y - game_values->objects_positions.unicorn_double_cordinates.position_y - DEFAULT_UNICORN_DIMENSION) > 0) {
				game_values->objects_positions.unicorn_double_cordinates.position_y++;
			}
			return true;
		}
	}
	return false;
}

bool IsUnicornOnPlatform(double x1, double y1, ObstaclesPositions* platforms_array) { // Checks if the unicorn is on platform. Only for Advanced Steering (using z). To use with isUnicornOnGround
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		if (platforms_array[i].position_x < x1 + double(DEFAULT_UNICORN_DIMENSION) 
			&& platforms_array[i].position_x + PLATFORM_LENGTH > x1 
			&& platforms_array[i].position_y-y1-DEFAULT_UNICORN_DIMENSION<=1 && y1<platforms_array[i].position_y) {
			return true;
		}
	}
	return false;
}

bool IsUnicornOnGround(double x1, double y1, ObstaclesPositions* platforms_array) { //Checks if the unicorn stands on any ground. Literally any.
	if (y1 == double(SCREEN_HEIGHT) - double(DEFAULT_UNICORN_DIMENSION))
		return true;
	if (IsUnicornOnPlatform(x1, y1, platforms_array) == true)
		return true;
	return false;
}

bool CanIJump(double current_y) { //No explanation required :)
	return current_y >= (double(JUMP_LIMIT)+double(JUMP_HEIGHT));
}

void NormalSteering(SDL_Event event, GameValues* game_values) { //Handles the keyboard input from the player. Easy lvl. Only for debugging and exploring the map
	switch (game_values->event.type) {
	case SDL_KEYDOWN:
		if (game_values->event.key.keysym.sym == SDLK_ESCAPE) {
			game_values->quit = true;
			game_values->stage = exit_stage;
		}
		else if (game_values->event.key.keysym.sym == SDLK_UP) {
			if (CanIJump(game_values->objects_positions.unicorn_double_cordinates.position_y - JUMP_HEIGHT) && EasyRoof(game_values) != true) {
				game_values->objects_positions.unicorn_double_cordinates.position_y -= UP_KEY_UNICORN_MOVE;
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_RIGHT) {
			if (CheckCollision(game_values) != true) {
				EasyMoveObstacles(game_values);
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_DOWN) {
			if (IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image)
				|| CheckCollision(game_values) == true || EasyIsUnicornOnPlatform(game_values) == true) {
				break;
			}
			if (game_values->objects_positions.unicorn_double_cordinates.position_y + UP_KEY_UNICORN_MOVE > double(SCREEN_HEIGHT) - double(DEFAULT_UNICORN_DIMENSION)) {
				game_values->objects_positions.unicorn_double_cordinates.position_y = double(SCREEN_HEIGHT) - double(DEFAULT_UNICORN_DIMENSION);
			}
			else {
				game_values->objects_positions.unicorn_double_cordinates.position_y += UP_KEY_UNICORN_MOVE;
				while (CheckCollision(game_values) == true) {
					game_values->objects_positions.unicorn_double_cordinates.position_y -= 1;
				}
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_d) {
			game_values->steering_way = advanced;
			break;
		}
		else if (game_values->event.key.keysym.sym == SDLK_n) {
			InitiateObstacles(game_values);
			InitializeTheRest(game_values, LIFES_COUNT, 0);
		}
		else if (game_values->event.key.keysym.sym == SDLK_SPACE) {
			if (CanIJump(game_values->objects_positions.unicorn_double_cordinates.position_y - 2 * double(JUMP_HEIGHT))&& IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image)) {
				game_values->objects_positions.unicorn_double_cordinates.position_y -= 2 * double(JUMP_HEIGHT);
			}
		}
		break;
	case SDL_KEYUP:
		game_values->objects_positions.unicorn_double_cordinates.unicorn_speed = 1.0;
		break;
	case SDL_QUIT:
		game_values->quit = true;
		game_values->stage = exit_stage;
		break;
	};
}

void AdvancedSteering(SDL_Event event, GameValues* game_values) {
	switch (game_values->event.type) {
	case SDL_KEYDOWN:
		if (game_values->event.key.keysym.sym == SDLK_ESCAPE) {
			game_values->quit = true;
			game_values->stage = exit_stage;
		}
		else if (game_values->event.key.keysym.sym == SDLK_z) {
			if (game_values->time_variables.jump_press_time_t1 == 0) {
				game_values->time_variables.jump_press_time_t1 = SDL_GetTicks();
			}
			game_values->time_variables.jump_press_time += (SDL_GetTicks() - double(game_values->time_variables.jump_press_time_t1))*0.001;
			game_values->time_variables.jump_press_time_t1 = SDL_GetTicks();
			if (game_values->unicorn_variables.jump_stage < double_jump && !game_values->event.key.repeat && game_values->unicorn_variables.is_rushing != rushes) {
				game_values->unicorn_variables.jump_stage++;
				game_values->unicorn_variables.is_jumping = true;
				game_values->unicorn_variables.jumping_destination_altitude = game_values->objects_positions.unicorn_double_cordinates.position_y - JUMP_HEIGHT;
				if (game_values->unicorn_variables.jumping_destination_altitude < JUMP_LIMIT) {
					game_values->unicorn_variables.jumping_destination_altitude = JUMP_LIMIT;
				}
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_x) {
			if (game_values->unicorn_variables.did_rush == false) {
				game_values->unicorn_variables.did_rush = true;
				game_values->unicorn_variables.is_rushing = rushes;
			}
			if (IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image)) {
				game_values->unicorn_variables.did_rush = false; //Checks whether the rush renewed. Applies only when the unicorn is on the ground.
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_p) {
			game_values->steering_way = normal;
			break;
		}
		else if (game_values->event.key.keysym.sym == SDLK_n) {
			InitiateObstacles(game_values);
			InitializeTheRest(game_values, LIFES_COUNT, 0);
		}
		break;
	case SDL_KEYUP:
		if (game_values->time_variables.jump_press_time >= LONG_JUMP_MIN_TIME) {
			game_values->unicorn_variables.jumping_destination_altitude *= JUMP_PRESSED_HEIGHT_MULTIPLIER;
		}
		game_values->time_variables.jump_press_time = 0;
		game_values->time_variables.jump_press_time_t1 = 0;
		break;
	case SDL_QUIT:
		game_values->quit = true;
		game_values->stage = exit_stage;
		break;
	};
}

void CountTheTime(GameValues* game_values) {
	game_values->time_variables.time_end = SDL_GetTicks();
	game_values->time_variables.time_delta = (double(game_values->time_variables.time_end) - double(game_values->time_variables.time_start)) * 0.001;
	game_values->time_variables.time_start = game_values->time_variables.time_end;
	game_values->time_variables.gameplay_time += game_values->time_variables.time_delta;
}

void RushOrNot(GameValues* game_values) {
	if (game_values->unicorn_variables.is_rushing == rushes) {
		game_values->unicorn_variables.time_rush += game_values->time_variables.time_delta;
		if (game_values->unicorn_variables.time_rush > RUSH_TIME) {
			game_values->unicorn_variables.is_rushing = does_not_rush;
			game_values->unicorn_variables.time_rush = 0;
			if (game_values->unicorn_variables.jump_stage > never_jumped) {
				game_values->unicorn_variables.jump_stage--;
			}
		}
	}
}

void PrintWhatsNeeded(GameValues* game_values) { //Puts what's required on the screen!
	SDL_BlitSurface(game_values->images.background, NULL, game_values->images.screen, &(game_values->objects_positions.rectangle_background_image));
	DrawSurface(game_values->images.screen, game_values->images.background_info, int(SCREEN_WIDTH / 2), int(-SCREEN_HEIGHT / 2.22));
	sprintf_s(game_values->printf_text, "Czas gry = %.1lf s %.0lf fps", game_values->time_variables.gameplay_time, game_values->frame_variables.fps);
	DrawString(game_values->images.screen, game_values->images.screen->w / 2 - strlen(game_values->printf_text) * 8 / 2, 10, game_values->printf_text, game_values->images.charset);
	SDL_BlitScaled(game_values->images.unicorn, NULL, game_values->images.screen, &(game_values->objects_positions.unicorn_cordinates));
	for (int i = 0; i < game_values->lifes_count; i++) {
		SDL_BlitSurface(game_values->images.heart, NULL, game_values->images.screen, &(game_values->objects_positions.rectangle_hearts[i]));
	}
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		SDL_BlitSurface(game_values->images.obstacle, NULL, game_values->images.screen, &(game_values->objects_positions.obstacle_cordinates[i]));
		SDL_BlitSurface(game_values->images.platform, NULL, game_values->images.screen, &(game_values->objects_positions.rectangle_platform_image[i]));
	}
}

void DealWithCollision(GameValues* game_values) {// Handles collision detection and further actions related.
	if (CheckCollision(game_values)) {
		game_values->lifes_count--;
		SDL_Delay(700);
		if (game_values->lifes_count == false) {
			game_values->stage = score_screen;
		}
		else {
			InitiateObstacles(game_values);
			InitializeTheRest(game_values, game_values->lifes_count, game_values->time_variables.gameplay_time);
		}
	}
	HitTheRoof(game_values);
}

void PerformGravity(GameValues* game_values) {// Handles the gravitation implementation. Checks if the unicorn is in air and is neither rushing nor jumping up.
	if (IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image) == false
		&& game_values->unicorn_variables.is_rushing == does_not_rush && game_values->unicorn_variables.is_jumping == false) {
		game_values->objects_positions.unicorn_double_cordinates.position_y += game_values->time_variables.time_delta * GRAVITY_CONSTANT;
		if (game_values->objects_positions.unicorn_double_cordinates.position_y > double(SCREEN_HEIGHT) - double(DEFAULT_UNICORN_DIMENSION)) //Not allowing the unicorn to get under the screen.
			game_values->objects_positions.unicorn_double_cordinates.position_y = double(SCREEN_HEIGHT) - double(DEFAULT_UNICORN_DIMENSION);
		if (IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image) == true) {
			game_values->unicorn_variables.jump_stage = never_jumped; //If landed on ground we can renew hero powers.
			game_values->unicorn_variables.did_rush = false;
		}
	}
}

void PerformJumping(GameValues* game_values) { //Handles jumping. Just jumping. Only jumping.
	if (game_values->unicorn_variables.is_jumping == true && game_values->objects_positions.unicorn_double_cordinates.position_y - game_values->time_variables.time_delta * GRAVITY_CONSTANT > 0 
		&& game_values->unicorn_variables.is_rushing != rushes) {
		game_values->objects_positions.unicorn_double_cordinates.position_y -= game_values->time_variables.time_delta * GRAVITY_CONSTANT;
		if (game_values->objects_positions.unicorn_double_cordinates.position_y < game_values->unicorn_variables.jumping_destination_altitude) {
			game_values->unicorn_variables.is_jumping = false;
		}
	}
}

void CountTheFrameRate(GameValues* game_values) { //Handles the fps counting system. It's not anyhow modified - just as it was in the template.
	game_values->frame_variables.fpsTimer += game_values->time_variables.time_delta;
	if (game_values->frame_variables.fpsTimer > 0.5) {
		game_values->frame_variables.fps = double(game_values->frame_variables.frames) * 2;
		game_values->frame_variables.frames = 0;
		game_values->frame_variables.fpsTimer -= 0.5;
	};
}

void CalculateWhatsNeeded(GameValues* game_values) { //Do calculations at the end of while iteration (mainly change the valuables of SDL_Rect from Double_Rect
	game_values->frame_variables.frames++;
	for (int i = 0; i < MAX_OBSTACLE_ON_SCREEN_COUNT; i++) {
		game_values->objects_positions.obstacle_cordinates[i].x = int(game_values->objects_positions.obstacle_double_cordinates[i].position_x);
		game_values->objects_positions.obstacle_cordinates[i].y = int(game_values->objects_positions.obstacle_double_cordinates[i].position_y);
		game_values->objects_positions.rectangle_platform_image[i].x = int(game_values->objects_positions.rectangle_double_platform_image[i].position_x);
		game_values->objects_positions.rectangle_platform_image[i].y = int(game_values->objects_positions.rectangle_double_platform_image[i].position_y);
	}
	game_values->objects_positions.unicorn_cordinates.y = int(game_values->objects_positions.unicorn_double_cordinates.position_y); //I perform operation on double to be more precise
}

void UpdateTheScene(GameValues* game_values) {
	SDL_UpdateTexture(game_values->screen_texture, NULL, game_values->images.screen->pixels, game_values->images.screen->pitch);
	SDL_RenderCopy(game_values->renderer, game_values->screen_texture, NULL, NULL);
	SDL_RenderPresent(game_values->renderer);
}

void DoIfAdvancedSteering(GameValues* game_values) {//Only done if the steering is set to advanced (using z).
	if (game_values->steering_way == advanced) {
		game_values->objects_positions.unicorn_double_cordinates.unicorn_speed += UNICORN_ACCELERATION * game_values->time_variables.time_delta;
		MoveObstacles(game_values);
		DealWithCollision(game_values);
		RushOrNot(game_values);
		PerformGravity(game_values);
		PerformJumping(game_values);
	}
}

void DoTheGame(GameValues* game_values) {
	CountTheTime(game_values);
	CreateObstacle(game_values);
	CreatePlatforms(game_values->objects_positions.rectangle_double_platform_image);
	PrintWhatsNeeded(game_values);
	DoIfAdvancedSteering(game_values);
	CountTheFrameRate(game_values);
	UpdateTheScene(game_values);
	CalculateWhatsNeeded(game_values);
	while (SDL_PollEvent(&(game_values->event))) {
		if (game_values->steering_way == normal) {
			NormalSteering(game_values->event, game_values);
		}
		else if (game_values->steering_way == advanced) {
			AdvancedSteering(game_values->event, game_values);
		}
	};
}

void EndTurnDisplay(GameValues* game_values) {
	SDL_BlitSurface(game_values->images.background, NULL, game_values->images.screen, &(game_values->objects_positions.rectangle_background_image));
	sprintf_s(game_values->printf_text, "Przetrwales %.1lf sekund!. Wcisnij esc aby wyjsc lub n, aby zaczac nowa rozgrywke", game_values->time_variables.gameplay_time);
	DrawString(game_values->images.screen, game_values->images.screen->w / 2 - strlen(game_values->printf_text) * 8 / 2, 10, game_values->printf_text, game_values->images.charset);
	SDL_UpdateTexture(game_values->screen_texture, NULL, game_values->images.screen->pixels, game_values->images.screen->pitch);
	SDL_RenderCopy(game_values->renderer, game_values->screen_texture, NULL, NULL);
	SDL_RenderPresent(game_values->renderer);
	while (SDL_PollEvent(&(game_values->event))) {
		switch (game_values->event.type) {
		case SDL_KEYDOWN:
			if (game_values->event.key.keysym.sym == SDLK_ESCAPE) {
				game_values->quit = true;
				game_values->stage = exit_stage;
				break;
			}
			else if (game_values->event.key.keysym.sym == SDLK_n){
				InitiateObstacles(game_values);
				InitializeTheRest(game_values, LIFES_COUNT, 0);
				break;
			}
			break;
		case SDL_KEYUP:
			break;
		case SDL_QUIT:
			game_values->quit = true;
			game_values->stage = exit_stage;
			break;
		};
	};
}

int main(int argc, char **argv) {
	srand(time(NULL));
	GameValues game_values;
	if (SDLSetup(&game_values) == false){
		printf("Error has occurred. Please try again later.");
		return 1;
	}
	InitiateObstacles(&game_values);
	if (LoadImages(&game_values) == false) {
		printf("Error has occurred. Please try again later.");
		FreeMemoryAfterSDL(&game_values);
		return 1;
	}
	if (InitializeTheRest(&game_values, LIFES_COUNT, 0) == false) {
		printf("Error has occurred. Please try again later.");
		FreeMemoryAfterSDL(&game_values);
		return 1;
	}
	
	while (game_values.quit == false) {
		while (game_values.stage == in_game) {
			DoTheGame(&game_values);
		}

		while (game_values.stage == score_screen) {
			EndTurnDisplay(&game_values);
		}
	};
	FreeMemoryAfterSDL(&game_values);
	return 0;
	};



/* VERSION WITH GRAVITY IN NORMAL STEERING
* 
* 
* void DoTheGame(GameValues* game_values) {
	CountTheTime(game_values);
	RushOrNot(game_values);
	CreateObstacle(game_values);
	CreatePlatforms(game_values->objects_positions.rectangle_double_platform_image);
	MoveObstacles(game_values);
	PrintWhatsNeeded(game_values);
	DoIfAdvancedSteering(game_values);
	DealWithCollision(game_values);
	PerformGravity(game_values);
	PerformJumping(game_values);
	CountTheFrameRate(game_values);
	UpdateTheScene(game_values);
	CalculateWhatsNeeded(game_values);
	while (SDL_PollEvent(&(game_values->event))) {
		if (game_values->steering_way == normal) {
			NormalSteering(game_values->event, game_values);
		}
		else if (game_values->steering_way == advanced) {
			AdvancedSteering(game_values->event, game_values);
		}
	};
}
void NormalSteering(SDL_Event event, GameValues* game_values) {
	switch (game_values->event.type) {
	case SDL_KEYDOWN:
		if (game_values->event.key.keysym.sym == SDLK_ESCAPE) {
			game_values->quit = true;
			game_values->stage = exit_stage;
		}
		else if (game_values->event.key.keysym.sym == SDLK_UP) {
			if (CanIJump(game_values->objects_positions.unicorn_double_cordinates.position_y - JUMP_HEIGHT)
				&& IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image)) {
				game_values->objects_positions.unicorn_double_cordinates.position_y -= JUMP_HEIGHT;
			}
		}
		else if (game_values->event.key.keysym.sym == SDLK_RIGHT) {
			game_values->objects_positions.unicorn_double_cordinates.unicorn_speed = 2.0;
		}
		else if (game_values->event.key.keysym.sym == SDLK_d) {
			game_values->steering_way = advanced;
			break;
		}
		else if (game_values->event.key.keysym.sym == SDLK_n) {
			InitiateObstacles(game_values);
			InitializeTheRest(game_values, LIFES_COUNT, 0);
		}
		else if (game_values->event.key.keysym.sym == SDLK_SPACE) {
			if (CanIJump(game_values->objects_positions.unicorn_double_cordinates.position_y - 2 * double(JUMP_HEIGHT))&& IsUnicornOnGround(game_values->objects_positions.unicorn_double_cordinates.position_x, game_values->objects_positions.unicorn_double_cordinates.position_y, game_values->objects_positions.rectangle_double_platform_image)) {
				game_values->objects_positions.unicorn_double_cordinates.position_y -= 2 * double(JUMP_HEIGHT);
			}
		}
		break;
	case SDL_KEYUP:
		game_values->objects_positions.unicorn_double_cordinates.unicorn_speed = 1.0;
		break;
	case SDL_QUIT:
		game_values->quit = true;
		game_values->stage = exit_stage;
		break;
	};
}
*/