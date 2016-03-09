#include "cappuccino.hpp"

int main(int argc, char *argv[]){
/*
	Cocoa::testOpenFile();
	Cocoa::testOpenInvalidFile();
*/
	Cappuccino::Cappuccino(argc, argv);	
	Cappuccino::root("html");
	Cappuccino::route("/",[](std::shared_ptr<Cappuccino::Request> request) -> Cappuccino::Response{
		return *Cappuccino::Response(request).status(200)->message("OK")->file("index.html");
	});
	Cappuccino::run();
	return 0;
}
