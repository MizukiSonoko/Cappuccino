#include "cappuccino.h"

int main(int argc, char *argv[]) {
	using namespace Cappuccino;
	Cappuccino::Cappuccino(argc, argv);
	
	Cappuccino::add_route("/",[](Request* request) -> Response{
		auto response = Cappuccino::ResponseBuilder()
					.status(200,"OK")
					.file("html/index.html")
					.build();
		return response;
	});

	run();
	return 0;
}