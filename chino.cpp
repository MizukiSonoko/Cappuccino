#include "cappuccino.hpp"

int main(int argc, char *argv[]){

#ifdef TEST
	Cocoa::testOpenFile();
	Cocoa::testOpenInvalidFile();
#else
	Cappuccino::Cappuccino(argc, argv);	
	Cappuccino::root("html");
	Cappuccino::route("/",[](std::shared_ptr<Cappuccino::Request> request) -> Cappuccino::Response{
		return *Cappuccino::Response(request).status(200)->message("OK")->file("index.html");
	});

	Cappuccino::route("/cocoa",[](std::shared_ptr<Cappuccino::Request> request) -> Cappuccino::Response{
		return *Cappuccino::Response(request).status(200)->message("OK")->file("cocoa.html");
	});

	Cappuccino::run();
#endif
	return 0;
}
