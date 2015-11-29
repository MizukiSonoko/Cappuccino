#include "cappuccino.h"

int main(int argc, char *argv[]) {
	Cappuccino::Cappuccino(argc, argv);
	
	Cappuccino::document_root("html");
	Cappuccino::static_directory("public");

	Cappuccino::add_route("/", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("index.html");
		return response;
	});

	Cappuccino::add_route("/gochiusa/<character_name>", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("chino.html");

		response.add_replace_value("@name", "mizuki");
		response.add_replace_value("@waiter", req->url_params()["character_name"]);

		return response;
	});

	Cappuccino::add_route("/hello", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::TEXT);
		response.set_text("Hello world");
		return response;
	});

	Cappuccino::add_route("/cocoa", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		if(req->method() == Cappuccino::Request::POST){
			auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
			response.set_filename("cocoa.html");
			response.add_replace_value("@order", req->get_param("order"));
			return response;
		}else{
			auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
			response.set_filename("cocoa.html");
			response.add_replace_value("@order", "うさぎ");
			return response;		
		}
	});

	Cappuccino::add_route("/chino", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("chino.html");

		response.add_replace_value("@name", "mizuki");
		response.add_replace_value("@waiter", "香風智乃");
		return response;
	});

	Cappuccino::run();

	return 0;
}