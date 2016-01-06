#include "cappuccino.h"

int main(int argc, char *argv[]) {
	Cappuccino::Cappuccino(argc, argv);
	Cappuccino::add_static_root("public");
	Cappuccino::add_view_root("html");

	Cappuccino::add_route("/", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.file(Cappuccino::FileLoader("index.html"))
					.build();
	});

	Cappuccino::add_route("/rize", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.text("Hello world!")
					.build();
	});

	Cappuccino::add_other_route( 404, [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(404,"Not Found")
					.file(Cappuccino::FileLoader("404.html"))
					.build();
	});

	Cappuccino::add_route("/sharo",[&](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.header_param("Content-type","json")
					.file( Cappuccino::FileLoader("sample.json")
							.replace("name",req->get("name"))
							.replace("value",req->get("value"))
					)
					.build();
	});

	Cappuccino::add_route("/cocoa",[&](Cappuccino::Request* req) -> Cappuccino::Response{
		if(req->method() == Cappuccino::Request::Method::POST){
			return Cappuccino::ResponseBuilder(req)
						.status(200,"OK")
						.file(Cappuccino::FileLoader("cocoa.html")
							.replace("@order",req->post("order"))
						)
						.build();
		}else{
			return Cappuccino::ResponseBuilder(req)
						.status(200,"OK")
						.file(Cappuccino::FileLoader("cocoa.html")
							.replace("@order","コーヒー")
						)
						.build();			
		}
	});

#ifndef TEST
	Cappuccino::run();
#endif

	return 0;
}
