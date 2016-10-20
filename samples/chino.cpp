#include "../cappuccino.hpp"

#include <json.hpp>

using json = nlohmann::json;
using Response = Cappuccino::Response;
using Request = Cappuccino::Request;


int main(int argc, char *argv[]){

#ifdef TEST
	Cocoa::testOpenFile();
	Cocoa::testOpenInvalidFile();
#else

	Cappuccino::Cappuccino(argc, argv);	
	
	Cappuccino::templates("html");
	Cappuccino::publics("public");

	Cappuccino::route("/",[](std::shared_ptr<Request> request) -> Response{
		auto res =  Response(request);
		res.file("index.html");
		return res;
	});

	Cappuccino::route("/json",[](std::shared_ptr<Request> request) -> Response{
		json res_json = {
			{"message","Hello, World!"}
		};
		auto res = Response(request);
		res.json(res_json);
		return res;
	});

	Cappuccino::run();

#endif
	return 0;
}
