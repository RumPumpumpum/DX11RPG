#include <memory>
#include <windows.h>

#include "AppBase.h"
#include "Game.h"

using namespace std;

int main(int argc, char* argv[]) 
{
	unique_ptr<AppBase> game;
	game = make_unique<Game>();


	if (!game->Initialize()) {
		cout << "Initialization failed." << endl;
		return -1;
	}

	return game->Run();
}
