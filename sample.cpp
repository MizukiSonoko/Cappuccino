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
		response.add_replace_value("@name", "mizuki");
		response.add_replace_value("@waiter", req->url_params()["character_name"]);

		response.set_filename("chino.html");

		return response;
	});

	Cappuccino::add_route("/hello", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::TEXT);
		response.set_text("Hello world");
		return response;
	});


	Cappuccino::add_route("/json", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
		response.set_filename("sample.json");
		response.add_header_value("Content-type", "json");
		return response;
	});

	Cappuccino::add_route("/cocoa", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		if(req->method() == Cappuccino::Request::POST){
			auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);

			response.add_replace_value("@order", req->get_param("order"));

			response.set_filename("cocoa.html");
			return response;
		}else{
			auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
			response.add_replace_value("@order", "うさぎ");

			response.set_filename("cocoa.html");
			return response;		
		}
	});

	Cappuccino::add_route("/chino", [&](Cappuccino::Request* req) -> Cappuccino::Response{
		auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);

		response.add_replace_value("@name", "mizuki");
		response.add_replace_value("@waiter", "香風智乃");

		response.set_filename("chino.html");

		return response;
	});


#ifdef TEST
	
	Cappuccino::add_spec("/ exist",[&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/", new Cappuccino::FakeRequest( "GET", "/"));
		if(res.find("Cappuccino", 0) != std::string::npos)
			return true;
		return false;
	});
	Cappuccino::add_spec("/hello plain text", [&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/hello", new Cappuccino::FakeRequest( "GET", "/hello"));
		if(res.find("Hello world", 0) != std::string::npos)
			return true;
		return false;
	});
	Cappuccino::add_spec("/hoge not exist",[&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/hoge", new Cappuccino::FakeRequest( "GET", "/hoge"));
		if(res.find("Page not found", 0) != std::string::npos)
			return true;
		return false;
	});
	Cappuccino::add_spec("/chino running replace",[&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/chino", new Cappuccino::FakeRequest( "GET", "/chino"));
		if(res.find("香風智乃", 0) != std::string::npos)
			return true;
		return false;
	});
	Cappuccino::add_spec("/gochiusa/chiya replace with url",[&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/gochiusa/chiya", new Cappuccino::FakeRequest( "GET", "/gochiusa/chiya"));
		if(res.find("chiya", 0) != std::string::npos)
			return true;
		return false;
	});
	Cappuccino::add_spec("/public/css/sample replace with url",[&](Cappuccino::Application* app) -> bool{
		std::string res = app->access("/gochiusa/chiya", new Cappuccino::FakeRequest( "GET", "/gochiusa/chiya"));
		if(res.find("chiya", 0) != std::string::npos)
			return true;
		return false;
	});

	Cappuccino::testRun();

#else
	Cappuccino::run();
#endif

	return 0;
}