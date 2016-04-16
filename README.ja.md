# ご注文はCappuccinoですか？
[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=master)](https://travis-ci.org/MizukiSonoko/Cappuccino)

# version
```
0.0.1
```

C++で書かれたWebFrameworkシングルヘッダライブラリです。

[Demo web page](http://cappuccino.mizuki.io/)

# サンプル

動かすだけならクローンしてmakeすると動きます。
```shell
$ git clone https://github.com/MizukiSonoko/Cappuccino.git
$ cd Cappuccino
$ make
$ ./sample
```

# Environments

- Ubuntu 14
- Mac  15.0.0 Darwin Kernel Version 15.0.0
- clang version 3.4


Windowsでは動かないです。すみません

# 使い方

#### 1. include "cappuccino.h"
	基本的にincludeするだけでいいです。
	お好みでMakefileを書いてください。

#### 2. write code

```cpp
// 初期化、mainの引数をそのまま入れてください。オプションの処理をします。
Cappuccino::Cappuccino(argc, argv);
// DocumentRootを設定します。おなし名前のDirectoryを同じ階層においてください。
Cappuccino::document_root("html");
// 第一引数にRoot,第二引数に処理を書いてください。
Cappuccino::add_route("/", [&](Cappuccino::Request* req) -> Cappuccino::Response{
	auto response = Cappuccino::Response(req->protocol(), Cappuccino::Response::FILE);
  // 送るファイル名を書いてください。DocumentRoot内から探します。
	response.set_filename("index.html");
	return response;
});

// サーバーを起動します。
Cappuccino::run();
```

### 3. コンパイル
C++0xのオプションをつけてください。
```shell
clang++ -std=c++0x -Wall app.cpp -o app
```
### 4. runnig
できたものを実行したら多分動いています。
```shell
$ ./app
```

# Directory
階層設定は以下のようになっています。
```
├── html
│   └── index.html
├── public
│   └── css
│       └── sample.css
└── sample.cpp
```

# Option

- デバックモード
```
-d
```
いろいろなログが表示されます。

- ポート指定
```
-p portnumber
```
ポート番号を指定します


# LICENCE
MITライセンスを採用しています。

