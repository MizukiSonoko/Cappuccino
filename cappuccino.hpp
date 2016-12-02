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

#include <json.hpp>

#include <mutex>
#include <ctime>

#define BUF_SIZE 4096
#define MAX_LISTEN 128

namespace Cappuccino {

	class Request;
	class Response;

	struct {

		time_t time;
   	struct tm *t_st;

		int port = 1204;
		int sockfd = 0;
		int sessionfd = 0;
	    fd_set mask1fds, mask2fds;

		std::shared_ptr<std::string> view_root;
		std::shared_ptr<std::string> static_root;

		std::unordered_map<std::string,
			std::function<Response(std::shared_ptr<Request>)>
		> routes;

	} context;

	namespace Log{

		std::string current(){
			char timestr[256];
   			time(&context.time);
			strftime(timestr, 255, "%a, %d %b %Y %H:%M:%S %Z", localtime(&context.time));
			return timestr;
		}

		static int LogLevel = 0;
		static void debug(std::string msg){
			if(LogLevel >= 1){
				std::cout <<current()<<"[debug] "<< msg << std::endl;
			}
		}

		static void info(std::string msg){
			if(LogLevel >= 2){
				std::cout <<current()<<"[info] "<< msg << std::endl;
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

		// ア！同じ処理の関数が2つもあるぞ！！！
		const std::string current() {
			char timestr[256];
   			time(&context.time);
			strftime(timestr, 255, "%a, %d %b %Y %H:%M:%S %Z", localtime(&context.time));
			return timestr;
		}

		std::string stripNl(const std::string& msg){
			std::string res = "";
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

		std::vector<std::unique_ptr<std::string>> split(const std::string& str, std::string delim) noexcept{
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

	void init_socket(){
	    struct sockaddr_in server;
		if((context.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			exit(EXIT_FAILURE);
		}
		memset( &server, 0, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(context.port);

		char opt = 1;
		setsockopt(context.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(char));

		int temp = 1;
  		if(setsockopt(context.sockfd, SOL_SOCKET, SO_REUSEADDR,
	        &temp, sizeof(int))){
		}

		if (bind(context.sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
			exit(EXIT_FAILURE);
		}

		if(listen(context.sockfd,  MAX_LISTEN) < 0) {
			exit(EXIT_FAILURE);
		}

	    FD_ZERO(&context.mask1fds);
	    FD_SET(context.sockfd, &context.mask1fds);
	}

	using namespace std;

	pair<string,string> openFile(string aFilename){
		auto filename = aFilename;
		std::ifstream ifs( filename, std::ios::in | std::ios::binary);
		if(ifs.fail()){
			throw std::runtime_error("No such file or directory \""+ filename +"\"\n");
		}
		ifs.seekg( 0, std::ios::end);
		auto pos = ifs.tellg();
		ifs.seekg( 0, std::ios::beg);

		std::vector<char> buf(pos);
		ifs.read(buf.data(), pos);
		string response(buf.cbegin(), buf.cend());

		if( response[0] == '\xFF' && response[1] == '\xD8'){
			return make_pair(response, "image/jpg");
		}else if( response[0] == '\x89' && response[1] == 'P' && response[2] == 'N' && response[3] == 'G'){
			return make_pair(response, "image/png");
		}else if( response[0] == 'G' && response[1] == 'I' && response[2] == 'F' && response[3] == '8' && (response[4] == '7' || response[4] == '9') && response[2] == 'a'){
			return make_pair(response, "image/gif");
		}else{
			return make_pair(response, "text/html");
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

		void addHeader(const string& key,string value){
			headerset[key] = move(value);
		}

		void addParams(const string& key,string value){
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

		unordered_map<string, string> headerset;

		int status_;
		shared_ptr<string> message_;
		shared_ptr<string> path_;
		shared_ptr<string> protocol_;
		shared_ptr<string> method_;
		shared_ptr<string> body_;
      public:

		Response(weak_ptr<Request> req){
			auto r = req.lock();
			if(r){
				method_ = r->method;
				path_ = r->path;
				protocol_ = r->protocol;

				// 基本的にはOKでしょ
				status_  = 200;
				body_ = std::make_shared<string>("");
				message_ = std::make_shared<string>("OK");
			}else{
				throw std::runtime_error("Request expired!\n");
			}
		}

		Response(int st,string msg, string pro, string bod):
			status_(st),
			message_(std::make_shared<string>(msg)),
			protocol_(std::make_shared<string>(pro)),
			body_(std::make_shared<string>(bod))
		{}

		void message(string msg){
			message_ = std::make_shared<string>(std::move(msg));
		}

		void status(int st){
			status_ = st;
		}

		void header(const string& key,string val){
			if(headerset.find(key)!= headerset.end()){
				Log::debug(key+" is already setted.");
			}
			headerset[key] = val;
		}

		void json(const nlohmann::json& text){
			body_ = std::make_shared<string>(text.dump());
			headerset["Content-Type"] = "application/json";
		}

		void file(const string&  filename){
			auto file = openFile(*context.view_root + "/" + filename);
			body_ = std::make_shared<string>(file.first);
			headerset["Content-Type"] = move(file.second);
		}

  	operator string() const{
      		std::string res = utils::stripNl(*protocol_)
			 +" "+ std::to_string(status_)
			 +" "+ *message_ +"\n";
      		/*std::string res = "HTTP/1.1 OK 200\n";//utils::stripNl(*protocol_);
      		std::string res = utils::stripNl(*protocol_)
			 +" "+ utils::stripNl(std::to_string(status_))
			 +" "+ utils::stripNl(*message_) + "\n";
		*/
			for(auto it = headerset.begin(); it != headerset.end(); ++it) {
				res += it->first + ": " + it->second +"\n";
			}
			res += "Content-Length: " + std::to_string((*body_).size()+1)+"\n";
			res += "Date: " + utils::current()+"\n";
			res += "Server: Cappuccino";

			res += "\n\n";
			res += *body_ +"\n";
			return res;
      	}
    };

	string createResponse(char* req) noexcept{
		auto lines = utils::splitRequest(string(req));
		if(lines.empty()){
			Log::debug("REQUEST is empty ");
			return Response(400, "Bad Request", "HTTP/1.1", "NN");
		}
		auto tops = utils::split( *lines[0], " ");
		if(tops.size() < 3){
			Log::debug("REQUEST header is invalied! ["+*tops[0]+"] ");
			return Response(401, "Bad Request", "HTTP/1.1",  "NN");
		}
		Log::debug(*tops[0] +" "+ *tops[1] +" "+ *tops[2]);

		shared_ptr<Request> request;
		if (lines.size() == 3){
			request = shared_ptr<Request>(
				new Request(
					std::move(tops[0]),
					std::move(tops[1]),
					std::move(tops[2]),
					std::move(lines[2])
			));
		}else{
			request = shared_ptr<Request>(
				new Request(
					std::move(tops[0]),
					std::move(tops[1]),
					std::move(tops[2]),
					std::make_unique<std::string>("")
			));
		}
		if(!request->isCorrect()){
			return Response( 401,"Bad Request", *request->protocol, "AA");
		}

		if(context.routes.find(*request->path) != context.routes.end()){
			auto f = context.routes[*request->path];
			return f(move(request));
		}

		return Response( 404, "Not found", *request->protocol, "<h1>Not found!! </h1>");
	}

	string receiveProcess(int sessionfd){
		char buf[BUF_SIZE] = {};

		if (recv(sessionfd, buf, sizeof(buf), 0) < 0) {
			exit(EXIT_FAILURE);
		}
		do{
			if(strstr(buf, "\r\n")){
				break;
			}
			if (strlen(buf) >= sizeof(buf)) {
				memset(&buf, 0, sizeof(buf));
			}
		}while(read(sessionfd, buf+strlen(buf), sizeof(buf) - strlen(buf)) > 0);
		Log::debug("==========\n");
		Log::debug(std::string(buf));
		Log::debug("==========\n");
		return createResponse(buf);
	}

	void load(string directory, string filename) noexcept{
		if(filename == "." || filename == "..") return;
		if(filename != "")
			directory += "/" + move(filename);
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
			context.routes.insert( make_pair(
				"/" + directory,
				[directory,filename](std::shared_ptr<Request> request) -> Cappuccino::Response{
					auto file = openFile(directory);
					auto res = Response(200,"OK","HTTP/1.1",file.first);
					res.header("Content-Type", file.second);
					return res;
				}
			));
		}
	}

	void loadStaticFiles() noexcept{
		load(*context.static_root,"");
	}

	void route(string path,std::function<Response(std::shared_ptr<Request>)> F){
		context.routes.insert( make_pair( move(path), move(F) ));
	}

	void templates(string r){
		context.view_root =  make_shared<string>(move(r));
	}

	void publics(string s){
		context.static_root = make_shared<string>(move(s));
	}


	void run() {
		init_socket();
		signal_utils::init_signal();
		loadStaticFiles();

	    int cd[FD_SETSIZE];
		struct sockaddr_in client;
        int    fd;
        struct timeval tv;

	    for(int i = 0;i < FD_SETSIZE; i++){
	        cd[i] = 0;
	    }

	    while(1) {

	        tv.tv_sec = 0;
	        tv.tv_usec = 0;
			// mask2fds <- mask1fds
	        memcpy(&context.mask2fds, &context.mask1fds, sizeof(context.mask1fds));

			// mask2fdsを監視
	        int select_result = select(FD_SETSIZE, &context.mask2fds, (fd_set *)0, (fd_set *)0, &tv);
			//
	        if(select_result < 1) {
	            for(fd = 0; fd < FD_SETSIZE; fd++) {
	                if(cd[fd] == 1) {
	                    close(fd);
	                    FD_CLR(fd, &context.mask1fds);
	                    cd[fd] = 0;
	                }
	            }
	            continue;
	        }

			for(fd = 0; fd < FD_SETSIZE; fd++){
				// i番目のfdに読み込みデータがある
	            if(FD_ISSET(fd,&context.mask2fds)) {
	                if(fd == context.sockfd) {
	                	memset( &client, 0, sizeof(client));
						int len = sizeof(client);
						int clientfd = accept(context.sockfd,
							(struct sockaddr *)&client,(socklen_t *) &len);
						FD_SET(clientfd, &context.mask1fds);
	                }else {
	                    if(cd[fd] == 1) {
	                        close(fd);
	                        FD_CLR(fd, &context.mask1fds);
	                        cd[fd] = 0;
	                    } else {
					string response = receiveProcess(fd);
					std::cout <<"[debug] res:"<< response << std::endl;
					Log::debug("--------\n");
					Log::debug(response);
					Log::debug("--------\n");
					write(fd, response.c_str(), response.size());
		                        cd[fd] = 1;
	                    }
	                }
	            }
	        }
	    }
	}

	void Cappuccino(int argc, char *argv[]) {
		option(argc, argv);
		context.view_root = make_shared<string>("html");
		context.static_root = make_shared<string>("public");
	}
};

namespace Cocoa{
	using namespace Cappuccino;
	using namespace std;

	// Unit Test
	void testOpenFile(){
		auto res = openFile("html/index.html");
		auto lines = utils::split(res.first, "\n");
		assert(!lines.empty());
	}

	void testOpenInvalidFile(){
		try{
			auto res = openFile("html/index");
		}catch(std::runtime_error e){
			cout<< e.what() << endl;
		}
	}
};
