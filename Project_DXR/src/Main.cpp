#include "pch.h"
#include "Game.h"

using namespace std;

int main(int argc, char *argv[])
{
	// Check for memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	int status = game.startGameLoop();
	
	return status;
};
