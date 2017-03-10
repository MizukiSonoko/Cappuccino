# Cappuccino
### ~ご注文はCappuccinoですか？~

[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=master)](https://travis-ci.org/MizukiSonoko/Cappuccino)
[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=develop)](https://travis-ci.org/MizukiSonoko/Cappuccino)

Tiny HTTP server library.

# version
```
0.5.1
```

# Micro sample
```shell
$ git clone https://github.com/MizukiSonoko/Cappuccino.git
$ cd Cappuccino
$ git clone https://github.com/nlohmann/json lib/json
$ cd samples; make
$ ./chino
```

# Environments

### Language
- C++14

### Compiler

- clang version 3.4~
- gcc   version 5.0~


### Dependics
- [Json](https://github.com/nlohmann/json)

# Usage

#### 1. include "cappuccino.hpp"
#### 2. write code
```cpp
#include "../cappuccino.hpp"
#include <json.hpp>

using json = nlohmann::json;
using Response = Cappuccino::Response;
using Request = Cappuccino::Request;

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
```

### 3. compile
```shell
clang++ -std=c++14 -Wall app.cpp -o app
```
### 4. run it
```shell
$ ./chino
```

# Directory
```
├── html
│   └── index.html
├── public
│   └── css
│       └── sample.css
└── chino.cpp
```

# LICENCE
MIT
