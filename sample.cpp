#include "cappuccino.h"

int main(int argc, char *argv[]) {
	Cappuccino::Cappuccino(argc, argv);
	Cappuccino::add_static_root("public");
	Cappuccino::add_view_root("html");

	Cappuccino::add_route("/", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.file("index.html")
					.build();
	});

	Cappuccino::add_route("/hello", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.text("Hello world!")
					.build();
	});

	Cappuccino::add_route("/json", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.header_param("Content-type","json")
					.file("sample.json")
					.build();
	});


#ifndef TEST
	Cappuccino::run();
#endif

	return 0;
}