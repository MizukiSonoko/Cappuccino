#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <functional>
#include <map>
#include <list>
#include <algorithm>
#include <iostream>

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49
#include <regex>
#endif

#include <vector>
#include <fstream>
#include <future>

#define BUF_SIZE 4096
#define MAX_LISTEN 128

namespace Logger{

	using string = std::string;

	static bool debug_ = false;

   	static void d(string msg){
		if(debug_)
	   		std::cout<< msg << "\n";
   	}

   	static void safe(string msg){
   		std::cerr << "\x1b[32m"<< msg << "\x1b[39m\n";
   	}

   	template<typename T>
   	static void i(T msg){
   		std::cout<< msg << "\n";
   	}
   	static void e(string msg){
   		std::cout<<"\033[1;31m"<< msg << "\033[0m\n";
   	} 
}


namespace Cappuccino{
	
	using string = std::string;

	static int port_ = 1204;
	static int sockfd_ = 0;
	static int sessionfd_ = 0;
    fd_set mask1fds, mask2fds;

	namespace security{
		void replaces_body(string* str){
			// ToDo add more case.
			std::vector<std::pair<string, string>> replaces(4);
			replaces.push_back( std::make_pair("<","&lt;"));
			replaces.push_back( std::make_pair(">","&gt;"));
			replaces.push_back( std::make_pair("&","&amp;"));
			replaces.push_back( std::make_pair("\"","&quot"));
			
			for(auto patterm : replaces){
				std::string::size_type pos( str->find(patterm.first) );
				while( pos != std::string::npos ){			
				    str->replace( pos, patterm.first.length(), patterm.second );
				    pos = str->find( patterm.first, pos + patterm.second.length() );
				}
				replaces.pop_back();
			}
		}		
	}

	namespace utils{

		std::vector<string> split(const string& str, const string& delim){
			std::vector<string> result;
		    string::size_type pos = 0;
		    while(pos != string::npos ){
		        string::size_type p = str.find(delim, pos);
		        if(p == string::npos){
		            result.push_back(str.substr(pos));
		            break;
		        }else{
		            result.push_back(str.substr(pos, p - pos));
		        }
		        pos = p + delim.size();
		    }
		    return result;
		}

		int hex2int(char hex){
			if(hex >= 'A' && hex <= 'F'){
		        return hex - 'A' + 10;
		    }else if(hex >= 'a' && hex <= 'f'){
		        return hex - 'a' + 10;
		    }else if(hex >= '0' && hex <= '9'){
		        return hex - '0';
		    }
		    return -1;
		}

		char hex2char(char high,char low){
		    char res = hex2int(high);
		    res = res << 4;
		    return res + hex2int(low);
		}

		string url_decode(const string& str){
		    string res = "";
		    char tmp;
		    auto len = str.size();
		    for(auto i = 0; i < len; i++){
		        if(str[i] == '+'){
		            res += ' ';
		        }else if(str[i] == '%' && (i+2) < len){
		            if((tmp = hex2char((char)str[i+1],(char)str[i+2])) != -1){
		                i += 2;
		                res += tmp;
		            }else{
		                res += '%';
		            }
		        }else{
		            res += str[i];
		        }
		    }
			security::replaces_body(&res);
		    return res;
		}


		static std::vector<char> fileInput(const string& filename){
			std::ifstream ifs( filename, std::ios::in | std::ios::binary);
			if(ifs.fail()){
				throw std::runtime_error("No such file or directory \""+ filename +"\"\n");
			}

			ifs.seekg( 0, std::ios::end);
			auto pos = ifs.tellg();
			ifs.seekg( 0, std::ios::beg);

			std::vector<char> buf(pos);
			ifs.read(buf.data(), pos);
			return buf;
		}

		static std::pair<string,string> file2str(const string& filename){	
			try{
		        auto result = std::async( std::launch::async, fileInput, filename);
		        auto buf = result.get();

		        string response(buf.begin(), buf.end());

#if !defined(__APPLE__) && defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ < 49
				if( response[0] == '\xFF' && response[1] == '\xD8'){
					return make_pair(response, "image/jpg");
				}else if( response[0] == '\x89' && response[1] == 'P' && response[2] == 'N' && response[3] == 'G'){
					return make_pair(response, "image/png");
				}else if( response[0] == 'G' && response[1] == 'I' && response[2] == 'F' && response[3] == '8' && (response[4] == '7' || response[4] == '9') && response[2] == 'a'){
					return make_pair(response, "image/gif");
				}else{
					replace_all(&response);
					return std::make_pair(response, "text/html");
				}
#else
				std::smatch m;
				std::regex rejpg( R"(^\xFF\xD8)");
				std::regex repng( R"(^\x89PNG)");
				std::regex regif( R"(^GIF8[79]a)");

				if(std::regex_search( response, m, rejpg )){
					return make_pair(response, "image/jpg");
			    }else if(std::regex_search( response, m, repng )){
					return make_pair(response, "image/png");
			    }else if(std::regex_search( response, m, regif )){
					return make_pair(response, "image/gif");
				}else{
					return std::make_pair(response, "text/html");
				}
#endif

		    } catch ( std::exception & exception ){
				Logger::e(exception.what());
				return std::make_pair("", "text/html");	
            }            
		}

	}

	namespace signal_utils{
		static void signal_handler(int SignalName){
			Logger::i("\nserver terminated!\n");		
			close(sessionfd_);
			close(sockfd_);
			exit(0);
			return;
		}

		static void signal_handler_child(int SignalName){
			while(waitpid(-1,NULL,WNOHANG)>0){}
	        signal(SIGCHLD, Cappuccino::signal_utils::signal_handler_child);
		}

		static void init_signal(){
			if (signal(SIGINT, Cappuccino::signal_utils::signal_handler) == SIG_ERR) {
				Logger::e("signal setting error");
				exit(1);
			}
			if (signal(SIGCHLD, Cappuccino::signal_utils::signal_handler_child) == SIG_ERR) {
				Logger::e("signal setting error");
				exit(1);
			}
		}
	}

	class Request{
  	  public:
		enum Method{
			GET,
			POST,
			PUT,
			DELETE,
			HEAD,
			OPTION
		};

	  private:
		std::map<string,string> headers_;
		std::map<string,string> get_params_;
		std::map<string,string> post_params_;

		string url_;
		string protocol_;
		Method method_;		

		void set_method(const string& m){
			if(m == "GET"){
				method_ = GET;
			}else if(m == "POST"){
				method_ = POST;
			}else if(m == "PUT"){
				method_ = PUT;
			}else if(m == "DELETE"){
				method_ = DELETE;
			}else if(m == "HEAD"){
				method_ = HEAD;
			}else if(m == "OPTION"){
				method_ = OPTION;
			}
		}

		void set_url(const string& u){
			url_ = u;
		}

		void set_protocol(const string& p){
			protocol_ = p;
		}

	  public:
	  	string protocol() const{
	  		return protocol_;
	  	}
		string url() const{
			return url_;
		}
		Method method() const{
			return method_;
		}
		std::map<string,string> headers() const{
			return headers_;
		}		

		string post(const string& key) const{
			auto pos(post_params_.find(key));
			if( pos != post_params_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		string get(const string& key) const{
			auto pos(get_params_.find(key));
			if( pos != get_params_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		string header(const string& key) const{
			auto pos(headers_.find(key));
			if( pos != headers_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		Request() : url_("/"), protocol_("HTTP/1.1"), method_(GET){}

		static std::unique_ptr<Request> factory(string request){
			auto res = std::unique_ptr<Request>(new Request());

			auto lines = utils::split(request,"\n");			
			auto line_size = lines.size();
			if(line_size == 1) return res;

			auto request_head = utils::split(lines[0]," ");		
			res->set_method(request_head[0]);
			// get url paramaters.
			auto url_params = utils::split(request_head[1],"?");
			res->set_url(url_params[0]);

			res->set_protocol(request_head[2]);

			for(int i = 1;i < line_size; i++){
				auto key_val = utils::split(lines[i],": ");
				if(key_val.size() == 2){
					res->headers_.insert( std::map< string, string>::value_type( key_val[0], key_val[1]));
				}else{
					// POST paramaters
					auto param_key_val = utils::split(lines[i],"=");
					if(param_key_val.size() == 2){
						res->params_.insert( std::map< string, string>::value_type( param_key_val[0], utils::url_decode(param_key_val[1])));
					}
				}
			}
			if(url_params.size() == 2){
				// GET paramaters
				auto params = utils::split(url_params[1],"&");
				for(auto param : params){
					auto param_key_val = utils::split( param,"=");
					if(param_key_val.size() == 2){
						res->params_.insert( std::map< string, string>::value_type( param_key_val[0], utils::url_decode(param_key_val[1])));
					}
				}
			}
			return res;
		}
	};

	// This class is only wapper for response string. 
	class Response{
	  public:
	  protected:
	  	string response_;
	  public:
		explicit Response(const string& res):
			response_(res){}

		operator string() const{
			return response_;
		}
	};

	
	class ResponseBuilder{

		class Headers{
			int status_code_;
			string message_;
			string version_;
			std::map<string, string> params_;
		  public:
		  	void set_status_code(int code){
				status_code_ = code;	  		
		  	}
			void set_message(string msg){
				message_ = msg;
			}
			void set_version(string ver){
				version_ = ver;
			}
			void add_param(string key, string val){
				params_.insert( make_pair(key,val));
			}

			operator string() const{			
				string str = "HTTP/" + version_ + " " + std::to_string(status_code_) + " "+ message_ + "\n";
				for(auto value = params_.begin(); value != params_.end(); value++){
					str += value->first + ":" + value->second + "\n";
				}
				str += "\n";
				return "";
			}
		};

		Headers* headers_;
		string body_;
		string filename_;
		std::map<string, string>* replaces_;

	  public:
		ResponseBuilder& status(int code, string msg){
		    headers_->set_status_code(code);
		    headers_->set_message(msg);
		    return *this;
		}

		ResponseBuilder& http_version(string val){
		    headers_->set_version(val);
		    return *this;
		}

		ResponseBuilder& param(string key,string val){
		    headers_->add_param( key, val);
		    return *this;			
		}

		static std::vector<char> fileInput(string aFilename){
			//[WIP]
			string filename = aFilename;
			std::ifstream ifs( filename, std::ios::in | std::ios::binary);
			if(ifs.fail()){
				auto static_file_path = aFilename.substr(1, aFilename.size()-1);
			    ifs.open(static_file_path, std::ios::in | std::ios::binary);
				if(ifs.fail()){
					throw std::runtime_error("No such file or directory \""+ filename +"\" and "+static_file_path+"\"\n");
				}
			}
			ifs.seekg( 0, std::ios::end);
			auto pos = ifs.tellg();
			ifs.seekg( 0, std::ios::beg);

			std::vector<char> buf(pos);
			ifs.read(buf.data(), pos);
			return buf;
		}

		std::pair<string,string> file2str() const{	
			try{
		        auto result = std::async( std::launch::async, fileInput, filename_);
		        auto buf = result.get();

		        string response(buf.begin(), buf.end());
#if !defined(__APPLE__) && defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ < 49
				if( response[0] == '\xFF' && response[1] == '\xD8'){
					return make_pair(response, "image/jpg");
				}else if( response[0] == '\x89' && response[1] == 'P' && response[2] == 'N' && response[3] == 'G'){
					return make_pair(response, "image/png");
				}else if( response[0] == 'G' && response[1] == 'I' && response[2] == 'F' && response[3] == '8' && (response[4] == '7' || response[4] == '9') && response[2] == 'a'){
					return make_pair(response, "image/gif");
				}else{
					return std::make_pair(response, "text/html");
				}
#else
				std::smatch m;
				std::regex rejpg( R"(^\xFF\xD8)");
				std::regex repng( R"(^\x89PNG)");
				std::regex regif( R"(^GIF8[79]a)");

				if(std::regex_search( response, m, rejpg )){
					return make_pair(response, "image/jpg");
			    }else if(std::regex_search( response, m, repng )){
					return make_pair(response, "image/png");
			    }else if(std::regex_search( response, m, regif )){
					return make_pair(response, "image/gif");
				}else{
					return std::make_pair(response, "text/html");
				}
#endif

		    } catch ( std::exception & exception ){
				Logger::e(exception.what());
				return std::make_pair("", "text/html");	
            }            
		}

		ResponseBuilder& text(string& txt){
			body_ = txt;
			return *this;
		}

		ResponseBuilder& file(string& filename){
			filename_ = filename;
			return *this;
		}

		ResponseBuilder& replace(string key, string val){
			replaces_->insert( make_pair(key, val) );
		    return *this;
		}

		Response build(){
			auto body_pair = file2str();
			headers_->add_param( "Content-type", body_pair.second);
			return Response(string(*headers_) + body_pair.first);
		}
	};

// [WIP]

	static string view_root_ = "";
	static string static_root_ = "public";
	std::map<string, std::function<Response(Request*)>> routes_;
	std::map<string, std::function<Response(Request*)>> static_routes_;

	static void add_route(const string& route,const std::function<Response(Request*)>& function){
		routes_.insert( std::map<string,std::function<Response(Request*)>>::value_type(route, function));
	}

	static void add_static_root(const string& path){
		static_root_ = path;
	}

	static void add_view_root(const string& path){
		view_root_ = path;
	}

	static void set_argument_value(int argc, char *argv[]){
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				Logger::debug_ = true;
				break;
			case 'p':
				port_ = atoi(optarg);
				break;
			}
		}
		if(Logger::debug_){
			Logger::i("mode: debug");
		}else{			
			Logger::i("mode: product");
		}
		Logger::d("port:" + std::to_string(port_));
	}

	static void init_socket(){
	    struct sockaddr_in server;
		if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			Logger::e("socket create error");
			exit(EXIT_FAILURE);
		}
		memset( &server, 0, sizeof(server));
		server.sin_family = AF_INET;	
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(port_);

		char opt = 1;
		setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

		int temp = 1;
  		if(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
	            &temp, sizeof(temp))){
		    Logger::e("setsockopt() failed");
		}
		
		if (bind(sockfd_, (struct sockaddr *) &server, sizeof(server)) < 0) {
			Logger::e("socket bind error");
			exit(EXIT_FAILURE);
		}

		if(listen(sockfd_,  MAX_LISTEN) < 0) {
			Logger::e("listen error");
			exit(EXIT_FAILURE);
		}

	    FD_ZERO(&mask1fds);
	    FD_SET(sockfd_, &mask1fds);
	}

	namespace Regex{
		std::vector<string> findParent(string text){
			std::vector<string> res;
			string tmp = "";
			bool isIn = false;
			for(int i = 0; i< text.size(); ++i){
				if(text[i] == '<'){
					isIn = true;
					continue;
				}
				if(text[i] == '>'){
					isIn = false;
					res.push_back("<"+tmp+">");
					tmp = "";
				}

				if(isIn){
					tmp += text[i];
				}
			}
			return res;
		}
	}

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49	
	std::regex re( R"(<\w+>)");
    std::smatch m;
#endif    

	static Response create_response(char* req){
		std::unique_ptr<Request> request = Request().factory(string(req));
		for(auto route : routes_){
			if(route == request.url()){
				return route[request.url()](request);
			}
		}
		// WIP
		return Response();
	}

	static string receive_process(int sessionfd){
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

		if (recv(sessionfd, buf, sizeof(buf), 0) < 0) {
			Logger::e("receive error!");
			exit(EXIT_FAILURE);
		}
		bool isbody = false;
		do{
			if (!isbody && strstr(buf, "\r\n")) {
				isbody = true;
			}
			if(strstr(buf, "\r\n")){
				break;
			}
			if (strlen(buf) >= sizeof(buf)) {
				memset(&buf, 0, sizeof(buf));
			}
		} while (read(sessionfd, buf+strlen(buf), sizeof(buf) - strlen(buf)) > 0);
		return create_response(buf);
	}
	
	time_t client_info[FD_SETSIZE];

	static void run(){
		init_socket();
		signal_utils::init_signal();
		Logger::i("Running on http://localhost:" + std::to_string(port_) + "/");

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

	        memcpy(&mask2fds, &mask1fds, sizeof(mask1fds));

	        int select_result = select(FD_SETSIZE, &mask2fds, (fd_set *)0, (fd_set *)0, &tv);
	        if(select_result < 1) {
	            for(fd = 0; fd < FD_SETSIZE; fd++) {
	                if(cd[fd] == 1) {
	                    close(fd);
	                    FD_CLR(fd, &mask1fds);
	                    cd[fd] = 0;
	                }
	            }
	            continue;
	        }
	        for(fd = 0; fd < FD_SETSIZE; fd++) {
	            if(FD_ISSET(fd,&mask2fds)) {
	                if(fd == sockfd_) {
	                	memset( &client, 0, sizeof(client));
						int len = sizeof(client);
	                    int clientfd = accept(sockfd_, 
	                        (struct sockaddr *)&client,(socklen_t *) &len);
	                        FD_SET(clientfd, &mask1fds);
	                }else {
	                    if(cd[fd] == 1) {
	                        close(fd);
	                        FD_CLR(fd, &mask1fds);
	                        cd[fd] = 0;
	                    } else {
							string response = receive_process(fd);
							write(fd, response.c_str(), response.size());
	                        cd[fd] = 1;
	                    }
	                }
	            }
	        }
	    }	    
	}

	static void Cappuccino(int argc, char *argv[]) {
		port_ = 1204;
		set_argument_value(argc, argv);
	}

};
