#include "cappuccino.hpp"

int main(int argc, char *argv[]) {
	Cappuccino::Cappuccino(argc, argv);	
	Cappuccino::root("html");
	Cappuccino::route("/",[](std::unique_ptr<Cappuccino::Request> request) -> Cappuccino::Response{
		return *Cappuccino::Response(200,"/","OK").file("index.html");
	});
	Cappuccino::run();
	return 0;
}
/*[directory,filename](std::unique_ptr<Request> request) -> Cappuccino::Response{
					return *Cappuccino::Response(200,"OK").file(openFile(directory +"/" + filename));
				}
				*/