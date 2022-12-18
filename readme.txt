==============================================================================
    L3/S1 BASIC

                                Copyright(C) Sasaji 2013 All Rights Reserved.
==============================================================================

ファイル構成

  src/
    readme.txt ............. このファイル
    Makefile.xxx ........... 各OSごとのmakeファイル
    data/ .................. 変換用データファイル
    docs/ .................. ドキュメント
    lang/ .................. ローカライゼーション
      ja/ .................. 日本語用
        l3s1basic.po ....... 翻訳ファイル
        l3s1basic.mo ....... コンパイル済み翻訳ファイル
    src/ ................... ソースファイル
      res/ ................. リソースファイル
    VisualC/ ............... VisualC用プロジェクトファイル
    Xcode/ ................. Xcode用プロジェクトファイル

  *.cpp ... BOM付きのUTF-8エンコードファイル
  *.h ..... BOM付きのUTF-8エンコードファイル


コンパイル方法

●VC++ (Windows)版

 1. 開発環境の構築

  (1) コンパイルに必要なライブラリをインストールします。
	・wxWidgets-3.0.2
		wxWidgets-3.0.2.zipをダウンロードして適当なフォルダに展開。
		build\mswにあるwx_vc??.slnをVC++で開く。
		Debug/Releaseでソリューションをビルドすると、lib\vc_lib\に
		ライブラリが生成される。

 2. コンパイル
  (1) *.vcprojを開きます。
  (2)［表示］→[プロパティマネージャー]を開き、Debug/Release配下にある
	l3s1basicをクリックしてプロパティページを開きます。
	ユーザーマクロにある値をwxWidgetsをインストールしたパスに設定して
	ください。


●MinGW + MSYS (Windows)版

 1. 開発環境の構築

  (1) MinGW+MSYS 20110530版 をインストール
	インストーラに従ってインストールします。
	C Compiler, C++ Compiler, MSYS Basic SYSTEM, MSYS Developer Toolkit
	をチェックしてインストール。
	* インターネットから必要なモジュールがダウンロードされる。

  (2) MinGW Shellを起動してコンパイルに必要なライブラリをインストールします。
	・wxWidgets-3.0.2
		ソースからインストールする場合
			mkdir build_release_static_unicode
			cd build_release_static_unicode
			../configure --with-msw --disable-debug --disable-shared --enable-unicode
			make

 2. コンパイル
  MinGW Shell上で行います。

    Makefile.winを編集

      MinGWのバージョンが異なる場合、GCCLIBDIRを修正。
      WXCONFIG_XXのパスを修正。

  make -f Makefile.win st_install でmakeを実行したディレクトリの上に
  ReleaseMディレクトリを作成し、そこに必要なファイルをコピーします。


●Linux版

 1. 開発環境の構築

    ディストリビューションに付属する開発用ライブラリをインストール。
	Debianの場合はSynapticパッケージマネージャでインストールします。
	必要なライブラリ:
	gcc, g++, make, libgtk+(version 2)
	libX11-dev, libext-dev, libasound2-dev, libfreetype-dev,
	libGL-mesa-dev などなど

    コンパイルに必要なライブラリをインストールします。
	・wxWidgets-3.0.2
		ソースからインストールする場合
			mkdir build_release_static_unicode
			cd build_release_static_unicode
			../configure --with-gtk --disable-debug --disable-shared --enable-unicode
			make

 2. コンパイル
  ターミナル(端末)上で行います。

    Makefile.linuxを編集

      WXCONFIG_XXのパスを修正。

  make -f Makefile.linux st_install でmakeを実行したディレクトリの上に
  Releaseディレクトリを作成し、そこに必要なファイルをコピーします。


●MacOSX版

 1. 開発環境の構築

    Xcode と Command Line Tools for Xcode を インストールします。

    コンパイルに必要なライブラリをインストールします。
	・wxWidgets-3.0.2
		ソースからインストールする場合
			mkdir build_release_static_unicode
			cd build_release_static_unicode
			../configure --with-osx_cocoa --disable-debug --disable-shared --enable-unicode
			make

 2. コンパイル
  ターミナル上で行います。(Xcodeは使用しません。)

    Makefile.macosxを編集

      WXCONFIG_XXのパスを修正。

  make -f Makefile.macosx st_install でmakeを実行したディレクトリの上に
  Releaseディレクトリを作成し、そこに必要なファイルをコピーします。


● 免責事項

・このソフトはフリーウェアです。ただし、著作権は放棄しておりません。
  実行モジュールについては作者Sasajiにあります。
  ソースコードについてはそれぞれの作者にあります。
・このソフトによって発生したいかなる損害についても著作権者は一切責任を負いません。
  このソフトを使用するにあたってはすべて自己責任で行ってください。
・雑誌やネットなどに転載される場合、不特定多数の方に再配布を行う場合でも
  承諾の必要はありませんが、転載の旨をご連絡いただけたら幸いです。

==============================================================================

連絡先：
  Sasaji (sasaji@s-sasaji.ddo.jp)
  http://s-sasaji.ddo.jp/bml3mk5/
  (Twitter: http://twitter.com/bml3mk5)

==============================================================================

