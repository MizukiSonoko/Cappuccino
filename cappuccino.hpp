#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49
#include <regex>
#endif

#include <vector>
#include <fstream>
#include <future>

#define BUF_SIZE 4096
#define MAX_LISTEN 128

namespace Cappuccino{
	
	namespace Log{

		static int LogLevel = 0;
		static void debug(std::string msg){
			if(LogLevel >= 1){
				std::cout << msg << std::endl;
			}
		}

		static void info(std::string msg){
			if(LogLevel >= 2){
				std::cout << msg << std::endl;
			}
		}
	};

	class Request;
	class Response;

	struct {
		int port = 1204;
		int sockfd = 0;
		int sessionfd = 0;
	    fd_set mask1fds, mask2fds;

		std::shared_ptr<std::string> view_root;
		std::shared_ptr<std::string> static_root;

		std::map<std::string,
			std::function<Response(std::unique_ptr<Request>)>
		> routes;
	} context;

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
		std::vector<std::string> split(const std::string& str, std::string delim) noexcept{
			std::vector<std::string> result;
		    std::string::size_type pos = 0;
		    while(pos != std::string::npos ){
		        std::string::size_type p = str.find(delim, pos);
		        if(p == std::string::npos){
		            result.push_back(str.substr(pos));
		            break;
		        }else{
		            result.push_back(str.substr(pos, p - pos));
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

	void option(int argc, char *argv[]) noexcept{
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				Log::LogLevel = 1;	
				break;
			case 'p':
				context.port = atoi(optarg);
				break;
			}
		}
	}

	class Request{
		map<string, string> headerset;
		map<string, string> paramset;
	  public:
		Request(string method, string url,string protocol):
		method(move(method)),
		url(move(url)),
		protocol(move(protocol))
		{}

		const string method;
		const string url;
		const string protocol;

		void addHeader(string key,string value){
			headerset[key] = move(value);
		}

		void addParams(string key,string value){
			paramset[key] = move(value);
		}

		string header(string key){
			if(headerset.find(key) == headerset.end())
				return "INVALID";
			return headerset[key];
		}

		string params(string key){
			if(paramset.find(key) == paramset.end())
				return "INVALID";
			return paramset[key];
		}
	};

    class Response{
		int status;
		string message;
      public:
		Response(int st,string msg){
			status = st;
			message = msg;
		}
	

		Response(string msg){
			message = msg;
		}

      	operator string(){
      		return to_string(status) + " / " + message;
      	}
    };



	string createResponse(char* req) noexcept{
		auto lines = utils::split(string(req), "\n");
		if(lines.empty())
			return Response(400, "Bad Request");
 
		auto tops = utils::split(lines[0], " ");
		if(tops.size() < 3)
			return Response(401, "Bad Request");

		auto request = unique_ptr<Request>(new Request(tops[0],tops[1],tops[2]));
 		
		if(context.routes.find(tops[1]) != context.routes.end()){
			return context.routes[tops[1]](move(request));
		}
		return Response( 404, "Not found");
	}

	string receiveProcess(int sessionfd){
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

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
		return createResponse(buf);	
	}
	
	string openFile(string aFilename){
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
		return string(buf.begin(), buf.end());;
	}

	time_t client_info[FD_SETSIZE];

	void load(string directory, string filename) noexcept{
		if(filename == "." || filename == "..") return;
		if(filename!="")
			directory += "/" + filename;
		DIR* dir = opendir(directory.c_str());
		if(dir != NULL){
			struct dirent* dent;
	        dent = readdir(dir);
		    while(dent!=NULL){
		        dent = readdir(dir);
		        if(dent!=NULL)
			        load(directory, string(dent->d_name));
		    }
		    closedir(dir);
		}else{
//			auto static_file = Cappuccino::FileLoader(directory);
//			static_file.preload();
			context.routes.insert( make_pair(
				directory +"/" + filename, 
				[directory,filename](std::unique_ptr<Request> request) -> Cappuccino::Response{
					return Cappuccino::Response(openFile(directory +"/" + filename));
				}
			));			
		}
	}

	void load_static_files() noexcept{
		load(*context.static_root,"");
	}

	void run(){

		context.view_root = make_shared<string>("");		
		context.static_root = make_shared<string>("public");

		init_socket();
		signal_utils::init_signal();
		load_static_files();


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

	        memcpy(&context.mask2fds, &context.mask1fds, sizeof(context.mask1fds));

	        int select_result = select(FD_SETSIZE, &context.mask2fds, (fd_set *)0, (fd_set *)0, &tv);
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
	            if(FD_ISSET(fd,&context.mask2fds)) {
	                if(fd == context.sockfd) {
	                	memset( &client, 0, sizeof(client));
						int len = sizeof(client);
	                    int clientfd = accept(context.sockfd, (struct sockaddr *)&client,(socklen_t *) &len);
                        FD_SET(clientfd, &context.mask1fds);
	                }else {
	                    if(cd[fd] == 1) {
	                        close(fd);
	                        FD_CLR(fd, &context.mask1fds);
	                        cd[fd] = 0;
	                    } else {
	                    	string response = receiveProcess(fd);
							write(fd, response.c_str(), receiveProcess(fd).size());
	                        cd[fd] = 1;
	                    }
	                }
	            }
	        }
	    }	    
	}

	void Cappuccino(int argc, char *argv[]) {
		option(argc, argv);
	}
};

namespace Cocoa{
	class App{};
};

