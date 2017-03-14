#define ASIO_STANDALONE

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <string.h>
#include <csignal>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include <functional>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <cassert>
#include <vector>
#include <fstream>
#include <future>
#include <set>

#include <json.hpp>


#include <asio.hpp>

#include <mutex>
#include <ctime>

namespace Cappuccino {

	const int BUF_SIZE   = 4096;
	const int MAX_LISTEN = 128;

	class Request;
	class Response;
	using std::string;

	enum class Method{
		GET,
		HEAD,
		POST,
		OPTIONS,
		PUT,
		DELETE,
		TRACE,

		INVALID
	};

	Method method_of(const string& m){
		if(m == "GET"){
			return Method::GET;
		}else if(m == "HEAD"){
			return Method::HEAD;
		}else if(m == "POST"){
			return Method::POST;
		}else if(m == "OPTIONS"){
			return Method::OPTIONS;
		}else if(m == "PUT"){
			return Method::PUT;
		}else if(m == "DELETE"){
			return Method::DELETE;
		}else if(m == "TRACE"){
			return Method::DELETE;
		}
		return Method::INVALID;
	}

	namespace utils{
		const std::string current();
	};

	struct {
		time_t time;
   	struct tm *t_st;
    
		string address = "0.0.0.0";
		int port = 1204;
		int sockfd = 0;
		int sessionfd = 0;

		std::shared_ptr<std::string> view_root;
		std::shared_ptr<std::string> static_root;

		std::unordered_map<
			string,
			std::pair<
				std::function<Response(std::shared_ptr<Request>)>,
				std::set<Method>
			>
		> routes;

	} context;

	namespace Log {

		static int LogLevel = 0;
		static void debug(const string& msg){
			if(LogLevel >= 1){
				std::cout <<utils::current()<<"[debug] "<< msg << std::endl;
			}
		}

		static void info(const string& msg){
			if(LogLevel >= 2){
				std::cout <<utils::current()<<"[info] "<< msg << std::endl;
			}
		}

	};

	namespace signal_utils{

		void signal_handler(int signal){
			close(context.sessionfd);
			close(context.sockfd);
			exit(0);
		}

		void signal_handler_child(int SignalName){
			while(waitpid(-1,NULL,WNOHANG)>0){}
	        signal(SIGCHLD, signal_utils::signal_handler_child);
		}

		void init_signal(){
			if(signal(SIGINT, signal_utils::signal_handler) == SIG_ERR){
				exit(1);
			}
			if(signal(SIGCHLD, signal_utils::signal_handler_child) == SIG_ERR){
				exit(1);
			}
		}
	}

	namespace utils{

		const std::string current(){
			char timestr[256];
   			time(&context.time);
			strftime(timestr, 255, "%a, %d %b %Y %H:%M:%S %Z", localtime(&context.time));
			return timestr;
		}

		const string stripNl(const std::string& msg) {
			string res = "";
			for (const auto c : msg) {
				if (c != '\n' && c != '\r') {
					res += c;
				}
			}
			return res;
		}

		std::vector<std::unique_ptr<std::string>> splitRequest(const std::string& str) noexcept{
			std::vector<std::unique_ptr<std::string>> result;
	    std::string::size_type pos = str.find("\n", 0);
			if(pos != std::string::npos){
				result.push_back(std::make_unique<std::string>(
					str.substr( 0, pos)
				));
				auto sub_str_start_pos = pos;
		    std::string::size_type sec_pos = str.find("{");
				if( sec_pos != std::string::npos){
					result.push_back(
						std::make_unique<std::string>(
							 str.substr( sub_str_start_pos, sec_pos - sub_str_start_pos)
						)
					);
					result.push_back(std::make_unique<std::string>(str.substr( sec_pos, str.size())));
					return result;
				}
			}
			return result;
		}

		std::vector<std::unique_ptr<std::string>> split(const string& str, const string& delim) noexcept{
			std::vector<std::unique_ptr<std::string>> result;
		    std::string::size_type pos = 0;
		    while(pos != std::string::npos) {
		        auto p = str.find(delim, pos);
		        if(p == std::string::npos){
		            result.push_back(std::make_unique<std::string>(str.substr(pos)));
		            break;
		        }else{
		            result.push_back(std::make_unique<std::string>(str.substr(pos, p - pos)));
		        }
		        pos = p + delim.size();
		    }
		    return result;
		}
	};

	std::pair<string,string> openFile(const string& aFilename){
		std::ifstream ifs( aFilename, std::ios::in | std::ios::binary);
		if(ifs.fail()){
			throw std::runtime_error("No such file or directory \""+ aFilename +"\"\n");
		}
		ifs.seekg( 0, std::ios::end);
		auto pos = ifs.tellg();
		ifs.seekg( 0, std::ios::beg);

		std::vector<char> buf(pos);
		ifs.read(buf.data(), pos);
		string response(buf.cbegin(), buf.cend());

		if( response[0] == '\xFF' && response[1] == '\xD8'){
			return std::make_pair(response, "image/jpg");
		}else if( response[0] == '\x89' && response[1] == 'P' && response[2] == 'N' && response[3] == 'G'){
			return std::make_pair(response, "image/png");
		}else if(
			response[0] == 'G' && response[1] == 'I' && response[2] == 'F' && response[3] == '8'
			&&
			(response[4] == '7' || response[4] == '9')
		){
			return std::make_pair(response, "image/gif");
		}else{
			return std::make_pair(response, "text/html");
		}
	}

	void option(int argc, char *argv[]) noexcept{
		char result;
		while((result = getopt(argc,argv,"dvp:")) != -1){
			switch(result){
			case 'd':
				Log::LogLevel = 1;
				break;
			case 'p':
				context.port = atoi(optarg);
				break;
			case 'a':
				context.address = optarg;
				break;
			case 'v':
				Log::debug("version 0.1.0");
				exit(0);
			}
		}
	}

	class Request{
		std::unordered_map<string, string> headerset;
		std::unordered_map<string, string> paramset;
		bool correctRequest;
	 public:
		Request(
			std::unique_ptr<string> aMethod,
			std::unique_ptr<string> aPath,
			std::unique_ptr<string> aProtocol,
			std::unique_ptr<string> abody
		):
			method(move(aMethod)),
			protocol(move(aProtocol)),
			body(move(abody))
		{
			std::vector<std::unique_ptr<string>> path_param = utils::split( *aPath, "?");
			path = std::move(path_param.at(0));
			if( path_param.size() == 2){
				auto params = utils::split( *path_param[1],"&");
				for(auto&& param : params){
					auto param_key_val = utils::split( *param, "=");
					if(param_key_val.size() == 2) {
						paramset.insert(
							make_pair( *param_key_val[0], *param_key_val[1])
						);
					}
				}
			}
			correctRequest = validateHttpVersion(*protocol);
		}

		const std::shared_ptr<string> method;
		std::shared_ptr<string> path;
		const std::shared_ptr<string> protocol;
		const std::shared_ptr<string> body;

		//  HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
		bool validateHttpVersion(const string& v){
			if(v.size() < 5) return false;
			if(v[0] != 'H' || v[1] != 'T' ||
				v[2] != 'T' || v[3] != 'P') return false;
			if(v[4] != '/') return false;
			for(size_t i=5;i < v.size()-1;i++){
				if(!isdigit(v[i]) && v[i] != '.') return false;
			}
			return true;
		}

		void addHeader(const string& key,string&& value){
			headerset[key] = move(value);
		}

		void addParams(const string& key,string&& value){
			paramset[key] = move(value);
		}

		const string header(const string& key){
			if(headerset.find(key) == headerset.end())
				return "INVALID";
			return headerset[key];
		}

		const string params(const string& key){
			if(paramset.find(key) == paramset.end())
				return "INVALID";
			return paramset[key];
		}

		const nlohmann::json json(){
			try{
				return nlohmann::json::parse(*body);
			} catch(std::invalid_argument e){}
			return  nlohmann::json({});
		}

		bool isCorrect() {
			return correctRequest;
		}
	};

  class Response{

			std::unordered_map<string, string> headerset;

			int status_;
			std::shared_ptr<string> message_;
			std::shared_ptr<string> path_;
			std::shared_ptr<string> protocol_;
			std::shared_ptr<string> method_;
			std::shared_ptr<string> body_;
    public:

			Response(std::weak_ptr<Request> req){
				auto r = req.lock();
				if(r){
					method_ = r->method;
					path_ = r->path;
					protocol_ = r->protocol;

					status_  = 200;
					body_ = std::make_shared<string>("");
					message_ = std::make_shared<string>("OK");
				}else{
					throw std::runtime_error("Request expired!\n");
				}
			}

		Response(
			int status,
			const string& msg,
			const string& protocol,
			const string& method,
			const string& body,
			const string& path):
			status_(status),
			message_(std::make_shared<string>(msg)),
			path_(std::make_shared<string>(path)),
			protocol_(std::make_shared<string>(protocol)),
			method_(std::make_shared<string>(method)),
			body_(std::make_shared<string>(body))
		{}

		void message(string msg){
			message_ = std::make_shared<string>(std::move(msg));
		}

		void status(int st){
			status_ = st;
		}

		void header(const string& key,const string& val){
			if(headerset.find(key)!= headerset.end()){
				Log::debug(key+" is already setted.");
			}
			headerset[key] = val;
		}

		void json(const nlohmann::json& text){
			body_ = std::make_shared<string>(text.dump());
			headerset["Content-Type"] = "application/json";
		}

		void file(const string& filename){
			auto file = openFile(*context.view_root + "/" + filename);
			body_ = std::make_shared<string>(file.first);
			headerset["Content-Type"] = move(file.second);
		}

  	operator string() const{
  		std::string res = utils::stripNl(*protocol_)
			 +" "+ std::to_string(status_)
			 +" "+ *message_ +"\n";

			for(const auto& it: headerset) {
				res += it.first + ": " + it.second +"\n";
			}
			res += "Content-Length: " + std::to_string((*body_).size()+1)+"\n";
			res += "Date: " + utils::current()+"\n";
			res += "Server: Cappuccino";

			res += "\n\n";
			if(*method_ != "HEAD"){
				res += *body_ +"\n";
			}
			return res;
  	}
  };

	string createResponse(string req) noexcept{
		auto lines = utils::splitRequest(req);
		if(lines.empty()){
			Log::debug("REQUEST is empty ");
			return Response(400, "Bad Request", "HTTP/1.1", "", "<h1>Bad Request</h1>", "");
		}
		auto tops = utils::split( *lines[0], " ");
		if(tops.size() < 3){
			Log::debug("REQUEST header is invalied! ["+*tops[0]+"] ");
			return Response(401, "Bad Request", "HTTP/1.1", "", "<h1>Bad Request</h1>", "");
		}
		Log::debug(*tops[0] +" "+ *tops[1] +" "+ *tops[2]);

		std::shared_ptr<Request> request;
		if (lines.size() == 3){
			request = std::shared_ptr<Request>(
				new Request(
					std::move(tops[0]),
					std::move(tops[1]),
					std::move(tops[2]),
					std::move(lines[2])
			));
		}else{
			request = std::shared_ptr<Request>(
				new Request(
					std::move(tops[0]),
					std::move(tops[1]),
					std::move(tops[2]),
					std::make_unique<std::string>("")
			));
		}
		if(!request->isCorrect()){
			return Response( 401,"Bad Request", *request->protocol, *request->method, "<h1>Bad Request</h1>", *request->path);
		}

		if(context.routes.find(*request->path) != context.routes.end()){
			auto f = context.routes[*request->path].first;
			if(
				context.routes[*request->path].second.find(method_of(*request->method)) !=
				context.routes[*request->path].second.end()
				||
				method_of(*request->method) == Method::HEAD
			){
				return f(move(request));
			}else{
				return Response( 405, "Method Not Allowed", *request->protocol, *request->method, "<h1>Method Not Allowed!</h1>", *request->path);
			}
		}
		return Response( 404, "Not found", *request->protocol, *request->method, "<h1>Not found!! </h1>", *request->path);
	}

	void load(const string& aDirectory, const string& filename) noexcept{
		string directory = aDirectory;
		if(filename == "." || filename == "..") return;
		if(filename != "")
			directory += "/" + filename;
		DIR* dir = opendir(directory.c_str());
		if(dir != NULL){
			struct dirent* dent;
	        dent = readdir(dir);
		    while(dent!=NULL){
                load(directory, string(dent->d_name));
				dent = readdir(dir);
	        }
			if(dir!=NULL){
		    	closedir(dir);
			}
		}else{
			context.routes.insert( std::make_pair(
				"/" + directory,
				std::make_pair<
					std::function<Response(std::shared_ptr<Request>)>,
					std::set<Method>
				>( [directory,filename](std::shared_ptr<Request> request) -> Cappuccino::Response{
						auto file = openFile(directory);
						auto res = Response(200,"OK","HTTP/1.1","GET", file.first, "/" + directory);
						res.header("Content-Type", file.second);
						return res;
					}, { Method::GET }
				)
			));
		}
	}

	void loadStaticFiles() noexcept{
		load(*context.static_root,"");
	}

	template<Method m>
	void route(const string& path,std::function<Response(std::shared_ptr<Request>)> F){
		if( context.routes.find(path) != context.routes.end()){
			auto functions = context.routes[path];
			functions.second.insert(m);
		}else{
			context.routes.insert( std::make_pair( move(path), std::make_pair<
				std::function<Response(std::shared_ptr<Request>)>,
				std::set<Method>
			>( move(F), {m})));
		}
	}

	void templates(const string& r){
		context.view_root =  std::make_shared<string>(move(r));
	}

	void publics(const string& s){
		context.static_root = std::make_shared<string>(move(s));
	}

	class AsioServer {
	  asio::io_service io_service;
	  asio::ip::tcp::acceptor acceptor;
	  asio::ip::tcp::socket socket;
		asio::signal_set signals;
	public:
		AsioServer():
			io_service(),
	    acceptor(io_service),
	    socket(io_service),
			signals(io_service)
		{
			signals.add(SIGINT);
 		  signals.add(SIGTERM);

			do_await_stop();

		 	asio::ip::tcp::resolver resolver(io_service);
			asio::ip::tcp::endpoint endpoint = *resolver.resolve({context.address, std::to_string(context.port)});
			acceptor.open(endpoint.protocol());
			acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			acceptor.bind(endpoint);
 			acceptor.listen();
			do_accept();
		}

		void do_accept(){
		  acceptor.async_accept(socket,
	      [this](asio::error_code ec){
        if (!acceptor.is_open()){
          return;
        }
				std::array<char, 8192> buf;
        if (!ec){
					socket.async_read_some(asio::buffer(buf),
    				[this, &buf](asio::error_code ec, std::size_t len){
	        	if(!ec){
              std::string request = std::string{buf.data(), buf.data() + len};
							Log::debug("buffer:  " + request);
							Log::debug("bytes_transferred: "+ std::to_string(len));
							asio::async_write(
								socket,
								asio::buffer(createResponse(request)),
	      				[this](asio::error_code ec, std::size_t){
					        if (!ec){
					          asio::error_code ignored_ec;
					          socket.shutdown(asio::ip::tcp::socket::shutdown_both,ignored_ec);
					        }
					        if (ec != asio::error::operation_aborted){
										socket.close();
					        }
					      });
	  				}else if (ec != asio::error::operation_aborted){
								socket.close();
		        }
    			});
      	}
      	do_accept();
  		});
		}

		void do_await_stop(){
			signals.async_wait(
      	[this](asio::error_code ec, int signo){
					acceptor.close();
					socket.close();
    	});
		}

		void run(){
  		io_service.run();
		}

	};

	void run() {
		loadStaticFiles();
		AsioServer s;
		s.run();
	}

	void Cappuccino(int argc, char *argv[]) {
		option(argc, argv);
		context.view_root = std::make_shared<string>("html");
		context.static_root = std::make_shared<string>("public");
	}
};
