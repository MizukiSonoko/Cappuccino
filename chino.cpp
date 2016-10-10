#include "cappuccino.hpp"

#include <json.hpp>

using json = nlohmann::json;

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

	Cappuccino::route("/json",[](std::shared_ptr<Cappuccino::Request> request) -> Cappuccino::Response{
		json res = {
			{"status","ok"}
		};
		Cappuccino::Log::debug("+++ ["+*request->body+"]");
		if(*request->body != ""){
			Cappuccino::Log::debug(json::parse((*request->body))["A"]);
		}
		return *Cappuccino::Response(request).status(200)->message("OK")->json(res);
	});

	Cappuccino::run();
#endif
	return 0;
}
