#include "../cappuccino.hpp"

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

	Cappuccino::run();

	return 0;
}
