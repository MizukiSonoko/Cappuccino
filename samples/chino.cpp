#include "../cappuccino.hpp"

#include <json.hpp>

using json = nlohmann::json;
using Cappuccino::Response;
using Cappuccino::Request;
using Cappuccino::Method;

int main(int argc, char *argv[]){
	Cappuccino::Cappuccino(argc, argv);

	Cappuccino::templates("html");
	Cappuccino::publics("public");

	Cappuccino::route<Method::GET>("/",[](std::shared_ptr<Request> request) -> Response{
		auto res =  Response(request);
		res.file("index.html");
		return res;
	});

	Cappuccino::route<Method::GET>("/json",[](std::shared_ptr<Request> request) -> Response{
		json res_json = {
			{"message","Hello, World!"}
		};
		auto res = Response(request);
		res.json(res_json);
		return res;
	});

	Cappuccino::run();

	return 0;
}
