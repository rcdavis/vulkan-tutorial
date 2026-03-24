
#include "Utils/Log.h"
#include "Application.h"

int main(int argc, char** argv) {
	Log::Init();

	Application app;
	app.Run();

	return 0;
}
