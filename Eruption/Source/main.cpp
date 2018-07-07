#include "Engine.h"

#include <thread>

bool* signal;

void run_server_thread() {
	Engine engine;
	engine.run_server(signal);
}

int main(int argc, char* argv[]) {
	Engine engine;
	engine.run();	
	
	//// ----- Separate thread ----- 
	//signal = new bool();
	//*signal = false;
	//std::thread t = std::thread(run_server_thread);
	//engine.run_client();
	//*signal = true;
	//t.join();


	// ----- Separate process ----- 
	//if (argc > 1 && strcmp(argv[1], "-s") == 0) {
	//	std::cout << "SERVER" << std::endl;
	//	engine.run_server();
	//} else {
	//	std::cout << "SPAWNING SERVER" << std::endl;
	//	system("start c:\\Windows\\System32\\cmd.exe /k \"Eruption.exe -s\"");
	//	std::cout << "CLIENT" << std::endl;
	//	engine.run_client();
	//}

	return 0;
}