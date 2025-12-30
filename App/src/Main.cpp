
#include "Application.h"
#include "Utils/Log.h"

int main(int argc, char** argv) {
	Log::Init();

	Application app;

	try {
		app.Run();
	} catch (const std::exception& e) {
		LOG_CRITICAL("Exception: {0}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
