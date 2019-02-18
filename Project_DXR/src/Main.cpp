#include "pch.h"
#include "Game.h"

using namespace std;

//// TOTAL_TRIS pretty much decides how many drawcalls in a brute force approach.
//constexpr int TOTAL_TRIS = 100;
//// this has to do with how the triangles are spread in the screen, not important.
//constexpr int TOTAL_PLACES = 2 * TOTAL_TRIS;
//float xt[TOTAL_PLACES], yt[TOTAL_PLACES];
//
//// lissajous points
//typedef union { 
//	struct { float x, y, z, w; };
//	struct { float r, g, b, a; };
//} float4;
//
//typedef union { 
//	struct { float x, y; };
//	struct { float u, v; };
//} float2;


int main(int argc, char *argv[])
{
	// Check for memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	int status = game.startGameLoop();

	return status;
};
