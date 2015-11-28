#include "cappuccino.h"

int main(int argc, char *argv[]) {
	Cappuccino::Cappuccino(argc, argv);
	
	Cappuccino::document_root("html");

	Cappuccino::add_route("/", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("index.html");
		return response;
	});


	Cappuccino::add_route("/chino", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("chino.html");
		return response;
	});

	Cappuccino::run();

}