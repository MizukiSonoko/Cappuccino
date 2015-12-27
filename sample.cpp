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

	Cappuccino::add_route("/rize", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.text("Hello world!")
					.build();
	});

	Cappuccino::add_route("/sharo",[&](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.header_param("Content-type","json")
					.replace("name",req->get("name"))
					.replace("value",req->get("value"))
					.file("sample.json")
					.build();
	});


	Cappuccino::add_route("/cocoa",[&](Cappuccino::Request* req) -> Cappuccino::Response{
		if(req->method() == Cappuccino::Request::POST){
			return Cappuccino::ResponseBuilder(req)
						.status(200,"OK")
						.replace("@order",req->post("order"))
						.file("cocoa.html")
						.build();
		}else{
			return Cappuccino::ResponseBuilder(req)
						.status(200,"OK")
						.replace("@order","コーヒー")
						.file("cocoa.html")
						.build();			
		}
	});

#ifndef TEST
	Cappuccino::run();
#endif

	return 0;
}