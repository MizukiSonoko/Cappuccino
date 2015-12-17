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

	static string document_root_ = "";
	static string static_directory_ = "public";

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

	void prevent_xss(string* str){
		// ToDo add more case.
		std::list<std::pair<string, string>> replaces;
		replaces.push_back( std::make_pair("<","&lt;"));
		replaces.push_back( std::make_pair(">","&gt;"));
		replaces.push_back( std::make_pair("&","&amp;"));
		replaces.push_back( std::make_pair("\"","&quot"));
		
		while(!replaces.empty()){
			std::string::size_type pos( str->find(replaces.front().first) );
			while( pos != std::string::npos ){			
			    str->replace( pos, replaces.front().first.length(), replaces.front().second );
			    pos = str->find( replaces.front().first, pos + replaces.front().second.length() );
			}
			replaces.pop_back();
		}
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
		prevent_xss(&res);
	    return res;
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
		std::map<string,string> params_;
		std::map<string,string> url_params_;

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
		std::map<string,string> params() const{
			return params_;
		}
		std::map<string,string> url_params() const{
			return url_params_;
		}

		string get_url_param(const string& key) const{
			auto pos(url_params_.find(key));
			if( pos != headers_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		string get_header_param(const string& key) const{
			auto pos(headers_.find(key));
			if( pos != headers_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		string get_param(const string& key) const{
			auto pos(params_.find(key));
			if( pos != headers_.end() ){
				return pos->second;
			}
			return "Invalid param name";
		}

		void add_url_param(const string& key,const string& val){			
			url_params_.insert( std::map< string, string>::value_type( key, val));
		}

		Request(const string& method,const string& url,const string& protocol,const string& request){
			set_method(method);
			url_ = url;
			protocol_ = protocol;

			auto lines = split(request,"\n");

			string name;

			for(auto p : lines){
				auto header_name_val = split(p,": ");
				if(header_name_val.size() == 2){
					name = header_name_val.front();
					header_name_val.pop_back();
					headers_.insert( std::map< string, string>::value_type( name, header_name_val.front()));
				}else{
					auto param_name_val = split(p,"=");
					if(param_name_val.size() == 2){
						name = param_name_val.front();
						param_name_val.pop_back();
						params_.insert( std::map< string, string>::value_type( name, url_decode(param_name_val.front())));
					}
				}
			}
		}
	};

	class Response{
	  public:
		enum Response_Type{
			TEXT,
			FILE
		};

	  protected:
		string response_;
		string filename_;
		bool is_ready_;

		string protocol_;
		Response_Type response_type_;
		int status_;

		std::map<string,string> replace_values_;
		std::map<string,string> headers_;

	  public:

	  	void replace_all(string* response) const{
			for(auto value = replace_values_.begin(); value != replace_values_.end(); value++){
			    std::string::size_type pos(response->find(value->first));
			    while( pos != std::string::npos ){
			        response->replace( pos, value->first.length(), value->second );
			        pos = response->find( value->first, pos + value->second.length() );
			    }
			}
	  	}

		explicit Response(const string& protocol,const Response_Type& response_type):			
			protocol_(protocol),
			response_type_(response_type),
			status_(200)
			{
				add_header_value("Content-type", "text/html");
				add_replace_value("@public", "/" + static_directory_);
			}

		void add_replace_value(string key, string val){
			replace_values_.insert( std::map<string, string>::value_type( key, val));
		}

		void set_status(int status){
			status_ = status;
		}		

		int status() const{
			return status_;
		}

		string status_str() const{
			switch(status_){
				case 200: return "OK";
				case 404: return "NotFound";

				/* ToDo: Add more status*/
				default: return "OK";
			}
		}
		void add_header_value(const string& key, const string& val){
			headers_.insert( std::map<string, string>::value_type( key, val));
		}

		void set_filename(const std::string& filename){
			filename_ = filename;	
			std::pair<string, string> response = file2str();
			add_header_value("Content-type", response.second);
			response_ = response.first;
		}

		void set_text(const std::string& text){
			response_ = text;
		}

		string header() const{
			string str = protocol_ + " " + std::to_string(status_) + " "+ status_str() + "\n";
			for(auto value = headers_.begin(); value != headers_.end(); value++){
				str += value->first + ":" + value->second + "\n";
			}
			str += "\n";
			return str;
		}

		static std::vector<char> fileInput(string aFilename){
			string filename = document_root_ + "/" + aFilename;
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
					replace_all(&response);
					return std::make_pair(response, "text/html");
				}
#endif

		    } catch ( std::exception & exception ){
				Logger::e(exception.what());
				return std::make_pair("", "text/html");	
            }            
		}
		operator string() const{
			return header() + response_;
		}
	};

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
			string str = "HTTP/" + version_ + " " + std::to_string(status_code_) + " "+ message_+ "\n";
			for(auto value = params_.begin(); value != params_.end(); value++){
				str += value->first + ":" + value->second + "\n";
			}
			str += "\n";
			return "";
		}
	};

	class ResponseBuilder{
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
			string filename = document_root_ + "/" + aFilename;
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

		string build(){
			auto body_pair = file2str();
			headers_->add_param( "Content-type", body_pair.second);
			if(body_pair.second == "text/html"){

			}
			return string(*headers_) + body_pair.first;
		}
	};

	class NotFound : public Response{
	  public:
	  	NotFound(string protocol):
	  		Response(protocol, Response_Type::TEXT)
  		{
  			status_ = 404;
  			response_ = "<html>\n<head>\n<title>Not found</title>\n</head>\n<body>\n<h1>Page not found! </h1>\n</body>\n</html>";
  		}
	  	string content_type(){
	  		return "html/text";
	  	}	
	};


	std::map<string, std::function<Response(Request*)>> routes_;

	static int port_ = 1204;
	static bool debug_ = false;
	static int sockfd_ = 0;
	static int sessionfd_ = 0;
    fd_set mask1fds, mask2fds;

	static void add_route(const string& route,const std::function<Response(Request*)>& function){
		routes_.insert( std::map<string,std::function<Response(Request*)>>::value_type(route, function));
	}

	static void document_root(const string& path){
		document_root_ = path;
	}

	static void static_directory(const string& directory_name){
		static_directory_ = directory_name;
		add_route(
			"/" + static_directory_ + "/<dirname>/<filename>", 
			[&](Cappuccino::Request* req) -> Cappuccino::Response{
				auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
				response.set_filename(req->url());
				return response;
			}
		);
	}

	static void load_argument_value(int argc, char *argv[]){
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				debug_ = true;
				break;
			case 'p':
				port_ = atoi(optarg);
				break;
			}
		}
		Logger::debug_ = debug_;
		if(debug_){
			Logger::d("mode: debug");
		}else{			
			Logger::d("mode: product");
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

		/* Debug !!! */
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

	static void signal_handler(int SignalName){
		Logger::i("server terminated");		
		close(sessionfd_);
		close(sockfd_);
		exit(0);
		return;
	}

	static void signal_handler_chld(int SignalName){
		while(waitpid(-1,NULL,WNOHANG)>0){}
        signal(SIGCHLD, Cappuccino::signal_handler_chld);
	}

	static void init_signal(){
		if (signal(SIGINT, Cappuccino::signal_handler) == SIG_ERR) {
			Logger::e("signal setting error");
			exit(1);
		}
		if (signal(SIGCHLD, Cappuccino::signal_handler_chld) == SIG_ERR) {
			Logger::e("signal setting error");
			exit(1);
		}
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

    static std::pair<string, std::map<string,string>> get_url_params(string u){
    	std::map<string,string> res;
    	auto url = split(u, "?");
    	if(url.size()==1) return make_pair( url[0], res);
    	auto params = split(url[1], "&");
    	for(auto param : params){
    		auto kv = split(param, "=");
    		if(kv.size() == 2)
	    		res.insert(make_pair( kv[0], kv[1] ));
    	}
    	return make_pair( url[0], res);
    }

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49	
	std::regex re( R"(<\w+>)");
    std::smatch m;
#endif    
	// Todo more short	
	static Response create_response(char* method, char* url, char* protocol,char* req){
		Request* request = new Request( string(method), string(url), string(protocol), string(req));
		
	    std::vector<string> reg;

	    bool correct = true;
		for(auto url = routes_.begin(), end = routes_.end(); url != end; ++url){
			correct = true;
		    if(url->first.find("<", 0) != string::npos){

#if !defined(__APPLE__) && defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ < 49

				reg = Regex::findParent(url->first);

#else
				auto iter = url->first.cbegin();
			    while ( std::regex_search( iter, url->first.cend(), m, re )){
			        reg.push_back(m.str());
			        iter = m[0].second;
			    }
#endif


			    if(reg.size() != 0){
			    	auto val = split(url->first, "/");
			    	auto inp = split(request->url(), "/");

			    	if(val.size() != inp.size()) continue;

			    	for(auto v : val){
			    		if(find(reg.begin(), reg.end(), v) != reg.end() && v.size() > 2){
			    			request->add_url_param( v.substr(1, v.size() - 2), url_decode(inp.front()));   
			    		}else if(v != inp.front()){  			
	    					correct = false;
	    				}
			    		inp.pop_back();
			    	}
			    }
			    if(correct){
					Response result = url->second(request);
					delete request;
					return result;			    	
			    }
			}else{
				if(url->first == request->url()){
					Response result = routes_[request->url()](request);
					delete request;
					return result;
				}
			}
		}
		Response response = NotFound(request->protocol());
		return response;
	}

	static string receive_process(int sessionfd){
		Logger::d("receive_process");
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

		if (recv(sessionfd, buf, sizeof(buf), 0) < 0) {
			Logger::e("receive error!");
			exit(EXIT_FAILURE);
		}
		sscanf(buf, "%s %s %s", method, url, protocol);
		bool isbody = false;
		do {
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
		Logger::d(url);
		return create_response( method, url, protocol, buf);
	}
	
	time_t client_info[FD_SETSIZE];

	static void run(){
		Logger::i(" * Running on http://localhost:" + std::to_string(port_) + "/");

	    int cd[FD_SETSIZE];
		struct sockaddr_in client;

	    for(int i = 0;i < FD_SETSIZE; i++){
	        cd[i] = 0;
	    }

	    while(1) {

	        int    fd;
	        struct timeval tv;
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
		debug_ = false;

		load_argument_value(argc, argv);
		init_socket();
		init_signal();
	}


	class FakeRequest : public Request{
		public:
			FakeRequest(string method,string url,string protocol = "HTTP/1.1"):
				Request( method, url, protocol, method + " " + url + " " + protocol){}
	};

	class Application{
		public:
			string access(string route, FakeRequest* req){
				if(routes_.find(route) != routes_.end()){
					return routes_[route](req);
				}else{
				    std::vector<string> reg;
				    bool correct = true;
					for(auto url = routes_.begin(), end = routes_.end(); url != end; ++url){
						correct = true;
					    if(url->first.find("<", 0) != string::npos){

#if !defined(__APPLE__) && defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ < 49

							reg = Regex::findParent(url->first);

#else
							auto iter = url->first.cbegin();
						    while ( std::regex_search( iter, url->first.cend(), m, re )){
						        reg.push_back(m.str());
						        iter = m[0].second;
						    }
#endif

						    if(reg.size() != 0){
						    	auto val = split(url->first, "/");
						    	auto inp = split(req->url(), "/");

						    	if(val.size() != inp.size()) continue;

						    	for(auto v : val){
						    		if(find(reg.begin(), reg.end(), v) != reg.end() && v.size() > 2){
						    			req->add_url_param( v.substr(1, v.size() - 2), url_decode(inp.front()));   
						    		}else if(v != inp.front()){  			
				    					correct = false;
				    				}
						    		inp.pop_back();
						    	}
						    }
						    if(correct){
								Response result = url->second(req);
								delete req;
								return result;			    	
						    }
						}else{
							if(url->first == req->url()){
								Response result = routes_[req->url()](req);
								delete req;
								return result;
							}
						}
					}
					return NotFound(req->protocol());
				}
			}
	};


	static void realTimeRun(){

	}


	std::map<string, std::function<bool(Application*)>> tests_;
	static void add_spec(const string name, const std::function<bool(Application*)>& spec){
		tests_.insert( std::map<string,std::function<bool(Application*)>>::value_type( name, spec));
	}
	static void testRun(){

		Application* app = new Application();
		bool allOk = true;
		for(auto spec = tests_.begin(), end = tests_.end(); spec != end; ++spec){
			if(spec->second(app)){
				Logger::safe(" -> Passed");		
			}else{
				Logger::e(" -> Error");				
				allOk = false;
			}
		}
		Logger::i("-------------");
		if(allOk){
			Logger::safe("All test passed!");		
		}else{
			Logger::e("It has Error");
		}
		delete app;
	}

};
