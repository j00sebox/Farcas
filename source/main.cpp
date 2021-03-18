
#include <nds.h>
#include <stdio.h>
#include <vector>

// include sprites
#include "ship.h" 
#include "barrel.h"
#include "laser.h"
#include "fracas.h"

// include bitmaps
#include "game_over.h"

/*****************************************************************************************************************************************************/

/* Constants */

/*****************************************************************************************************************************************************/

#define NUM_STARS 40
#define BARRELS_ON_SCREEN 3

#define LASER_SPEED 6

// radius' were chosen based off sprite size and playtesting
#define BARREL_RADIUS 12
#define SHIP_RADIUS 12
#define LASER_RADIUS 8

/*****************************************************************************************************************************************************/

/* Structs */

/*****************************************************************************************************************************************************/

typedef struct
{
	int x;
	int y;
	int speed;
	unsigned short color;

}Star;

typedef struct
{
	int x;
	int y;
	int speed;
	int sprite_id;
	bool destroyed;
}Barrel;

typedef struct
{
	int x;
	int y;
	int sprite_id;
}Laser;

/*****************************************************************************************************************************************************/

/* Global Variables */

/*****************************************************************************************************************************************************/

bool GameOver = false;
int score = 0;
Star stars[NUM_STARS];
Barrel barrels[BARRELS_ON_SCREEN];
std::vector<Laser> lasers;

/*****************************************************************************************************************************************************/

/* Starfield */

/*****************************************************************************************************************************************************/

void MoveStar(Star* star)
{
	star->x -= star->speed;

	if (star->x <= 0)
	{
		star->color = RGB15(31, 31, 31);
		star->x = 255;
		star->y = rand() % 192;
		star->speed = rand() % 4 + 1;
	}
}

void ClearScreen(int bg)
{
	int i;

	for (i = 0; i < 256 * 192; i++)
		bgGetGfxPtr(bg)[i] = RGB15(0, 0, 0);
}

void InitStars(void)
{
	int i;

	for (i = 0; i < NUM_STARS; i++)
	{
		stars[i].color = RGB15(31, 31, 31);
		stars[i].x = rand() % 256;
		stars[i].y = rand() % 192;
		stars[i].speed = rand() % 4 + 1;
	}
}

void DrawStar(Star* star, int bg)
{
	bgGetGfxPtr(bg)[star->x + star->y * SCREEN_WIDTH] = star->color | BIT(15);
}

void EraseStar(Star* star, int bg)
{
	bgGetGfxPtr(bg)[star->x + star->y * SCREEN_WIDTH] = RGB15(0, 0, 0) | BIT(15);
}

/*****************************************************************************************************************************************************/

/* Barrel Logic */

/*****************************************************************************************************************************************************/

void UpdateBarrels(u16* gfx) {
	for (int b = 0; b < BARRELS_ON_SCREEN; b++)
	{
		if (barrels[b].destroyed)
		{
			// determine if a barrel will appear
			int r = rand() % 100;
			if (r <= 30)
			{
				barrels[b].destroyed = false;
				barrels[b].speed = rand() % 4 + 1; // randomize barrel speed
				barrels[b].x = 255;
				barrels[b].y = rand() % (176 - 16 + 1) + 8; // randomize y-postion,
			}
		}
		else {
			oamSet(&oamMain, //main graphics engine context
				barrels[b].sprite_id,           //oam index (0 to 127)  
				barrels[b].x, barrels[b].y,   //x and y pixle location of the sprite
				0,                    //priority, lower renders last (on top)
				1,                                        //this is the palette index if multiple palettes or the alpha value if bmp sprite     
				SpriteSize_32x32,
				SpriteColorFormat_16Color,
				gfx,                  //pointer to the loaded graphics
				-1,                  //sprite rotation data  
				false,               //double the size when rotating?
				false,                  //hide the sprite?
				false, false, //vflip, hflip
				false   //apply mosaic
			);

			barrels[b].x -= barrels[b].speed;

			// barrels are destroyed when offscreen and score is increased
			if (barrels[b].x <= 0) { barrels[b].destroyed = true; score++; }
		}
	}
}

/*****************************************************************************************************************************************************/

/* Laser Logic */

/*****************************************************************************************************************************************************/

void UpdateLasers(u16* gfx)
{
	for (int l = 0; l < (int)lasers.size(); l++)
	{

		if (lasers[l].x >= 255)
		{
			// clear lasers if the go offscreen
			oamClearSprite(&oamMain, lasers[l].sprite_id);
			lasers.erase(lasers.begin() + l);
		}
		else {
			// lasers use same pallette as farcas 
			oamSet(&oamMain, //main graphics engine context
				lasers[l].sprite_id,           //oam index (0 to 127)  
				lasers[l].x, lasers[l].y,   //x and y pixle location of the sprite
				0,                    //priority, lower renders last (on top)
				2,                                        //this is the palette index if multiple palettes or the alpha value if bmp sprite     
				SpriteSize_8x8,
				SpriteColorFormat_16Color,
				gfx,                  //pointer to the loaded graphics
				-1,                  //sprite rotation data  
				false,               //double the size when rotating?
				false,                  //hide the sprite?
				false, false, //vflip, hflip
				false   //apply mosaic
			);
			 
			lasers[l].x += LASER_SPEED;
		}
		
	}
}

/*****************************************************************************************************************************************************/

/* Collision Logic */

/*****************************************************************************************************************************************************/

// Barrel-Ship Collision calculated as if they were two circles
void BarrelShipCollision(int ship_y)
{
	int result = 0;
	int r_square = (SHIP_RADIUS + BARREL_RADIUS)*(SHIP_RADIUS + BARREL_RADIUS);

	// based off the inequality (Ax - Bx)^2 + (Ay - By)^2 <= (Ar + Br)^2
	for (int i = 0; i < BARRELS_ON_SCREEN; i++)
	{
		if (!barrels[i].destroyed)
		{
			result = (5 - barrels[i].x)*(5 - barrels[i].x) +
				(ship_y - barrels[i].y)*(ship_y - barrels[i].y);

			if (result <= r_square)
				GameOver = true;
		}
	}
}

// collision is done the same way as the ship-barrel detection
void BarrelLaserCollision(void)
{
	int res = 0;
	int r_square = (LASER_RADIUS + BARREL_RADIUS)*(LASER_RADIUS + BARREL_RADIUS);


	for (int i = 0; i < (int)lasers.size(); i++)
	{
		for (int j = 0; j < BARRELS_ON_SCREEN; j++)
		{
			res = (barrels[j].x - lasers[i].x)*(barrels[j].x - lasers[i].x) +
				(barrels[j].y - lasers[i].y)*(barrels[j].y - lasers[i].y);

			if (res <= r_square)
			{
				oamClearSprite(&oamMain, lasers[i].sprite_id);
				lasers.erase(lasers.begin() + i);
				barrels[j].destroyed = true;
				score++;
			}
		}
	}
}

void DisplayBoss(u16* gfx)
{
	oamSet(&oamMain, //main graphics engine context
		127,           //oam index (0 to 127)  
		150, 70,   //x and y pixle location of the sprite
		0,                    //priority, lower renders last (on top)
		2,                                        //this is the palette index if multiple palettes or the alpha value if bmp sprite     
		SpriteSize_64x64,
		SpriteColorFormat_16Color,
		gfx,                  //pointer to the loaded graphics
		-1,                  //sprite rotation data  
		false,               //double the size when rotating?
		false,                  //hide the sprite?
		false, false, //vflip, hflip
		false   //apply mosaic
	);
}

/*****************************************************************************************************************************************************/

/* Main */

/*****************************************************************************************************************************************************/

int main(void)
{
	int i, ship_y = 0;

	irqInit();
	irqEnable(IRQ_VBLANK);

	// video mode 5 with background 3 active for extended backgrounds
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG3_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_EXT_PALETTE);

	// This just sets VRAM_C to be used on the sub screen with MODE_0_2D
	consoleDemoInit();
	
	// allocoate video bank A for the background rendering 
	// and video bank B for sprites
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_SPRITE);

	// initialize the background
	// the starfield needs to use RGB15 color while the game over screen needs to use it's pallette
	// so the solution I came up with is using two background layers
	int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	int bg2 = bgInit(2, BgType_Bmp8, BgSize_B8_256x256, 0, 0);

	oamInit(&oamMain, SpriteMapping_1D_32, false);

	// create gfx for each sprite
	u16* gfx_ship = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
	u16* gfx_barrel = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
	u16* gfx_fracas = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
	u16* gfx_laser = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_16Color);
	 
	// clear data cache
	DC_FlushAll();

	// copy over sprite graphics info
	dmaCopy(shipTiles, gfx_ship, shipTilesLen);
	dmaCopy(barrelTiles, gfx_barrel, barrelTilesLen);
	dmaCopy(laserTiles, gfx_laser, laserTilesLen);
	dmaCopy(fracasTiles, gfx_fracas, fracasTilesLen);

	// copy over color scheme for each sprite
	dmaCopy(shipPal, SPRITE_PALETTE, shipPalLen);
	dmaCopy(barrelPal, &SPRITE_PALETTE[16], barrelPalLen);
	dmaCopy(fracasPal, &SPRITE_PALETTE[32], fracasPalLen);
	
	// set all barrels to initially be destroyed
	for (i = 0;  i < BARRELS_ON_SCREEN; i++)
	{
		barrels[i].sprite_id = (i + 1);
		barrels[i].destroyed = true;
	}

	ClearScreen(bg);
	InitStars();

	// loop until player dies
	while (!GameOver) {

		swiWaitForVBlank();

		// generate stars
		for (i = 0; i < NUM_STARS; i++)
		{
			EraseStar(&stars[i], bg);
			MoveStar(&stars[i]);
			DrawStar(&stars[i], bg);
		}

		// look for keys being pressed
		scanKeys();

		int held = keysHeld(); 
		int down = keysDown(); 

		// vertical movement with boundary checking
		if ((held & KEY_UP) && ship_y >= 0)
			ship_y -= 5;
		else if ((held & KEY_DOWN) && ship_y <= 160)
			ship_y += 5;
		
		// look for 'A' key press before generating laser beam
		if (down & KEY_A)
		{
			Laser beam;
			// offset so lasers look like they are coming from right in front of the ship
			beam.x = 8;
			beam.y = ship_y + 12; 
			// number of barrels is used as the id offset
			beam.sprite_id = (lasers.size() + BARRELS_ON_SCREEN + 1);
			lasers.push_back(beam);
		}

		oamSet(&oamMain, //main graphics engine context
			0,           //oam index (0 to 127)  
			5, ship_y,   //x and y pixle location of the sprite
			0,                    //priority, lower renders last (on top)
			0,                                        //this is the palette index if multiple palettes or the alpha value if bmp sprite     
			SpriteSize_32x32,
			SpriteColorFormat_16Color,
			gfx_ship,                  //pointer to the loaded graphics
			-1,                  //sprite rotation data  
			false,               //double the size when rotating?
			false,                  //hide the sprite?
			false, false, //vflip, hflip
			false   //apply mosaic
		);

		// detect if a barrel has hit the player
		BarrelShipCollision(ship_y);

		// detect if a laser beam has hit a barrel
		BarrelLaserCollision();

		// update position and state of lasers
		UpdateLasers(gfx_laser);

		// update position and state of barrels
		UpdateBarrels(gfx_barrel);

		// check if score is big enough to display fracas
		if (score >= 25)
			DisplayBoss(gfx_fracas);

		// update oam with new sprite positions
		oamUpdate(&oamMain);

		// print score to sub screen
		iprintf("\x1b[10;0HScore = %d", score);

	}

	// clear all sprites and update oam
	oamClear(&oamMain, 0, 128);
	oamUpdate(&oamMain);

	// copy game over screen graphics data
	dmaCopy(game_overPal, BG_PALETTE, game_overPalLen);
	dmaCopy(game_overBitmap, bgGetGfxPtr(bg2), game_overBitmapLen);

	return 0;
}