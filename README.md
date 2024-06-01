# KpAsciiMml_e: ASCII music.com MML Player [Extended Edition]

本ソフトウェアは、 PC-9801 シリーズ上の ASCII ツクールシリーズを中心に使用されていた ASCII music.com の MML ファイルを再生するための KbMedia Player プラグインです。
asmichi 氏の [ASCII music.com MML Player (KbAsciiMml)](https://github.com/asmichi/kb-ascii-mml) を元に、再現性を高めています。

EE 版の強化点として、LOGIN SOFCOM No.8 に収録されていた楽曲集 MML700 のうち、ASCII music.com でも 文法エラーとなる 2 曲を除いて、すべて再生できるようになりました。
また、EE 版でも D パートの再生には対応していませんが、D パート未使用曲については概ね違和感なく再生できるようになりました。

便宜上 KpAsciiMml_e を名乗っていますが、生成ファイルなどはオリジナル版から変わらず KbAsciiMml のままです。
混同されないようご注意ください。
なお、Kb ではなく Kp としたのは、KbMedia Player 公式プラグインと誤解されないようにするためです。

オリジナル版の README は [README_org.md](README_org.md) を参照してください。

## ビルド

次の手順でビルドできます。

- 環境変数 `BOOST_ROOT` を Boost のルートディレクトリにセットする
- 環境変数 `KPISDK_ROOT` を KbMedia Player プラグイン SDK のルートディレクトリにセットする
- "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" を実行する
  - 環境によって異なりますので、適切なパスを指定してください。
- build.cmd を実行する

ビルドに使用した環境は次の通りです。

- Visual Studio Community 2022 Version 17.10.1 (「C++ ネイティブ開発」ワークロードあり)
- Boost 1.85.0
- KbMedia Player プラグイン SDK 2024/04/30版

なお、動作確認には KbMedia Player 3.13 を使用しています。

## 更新履歴

[history.md](history.md) を参照してください。

## ライセンス・謝辞

FM音源のエミュレーションには、cisc 氏の fmgen ライブラリを使用しており、このプラグインで実現できた高い再現性のほとんどはこのライブラリによります。
この部分 ([external/fmgen](external/fmgen) ディレクトリ配下) の著作権は cisc 氏に帰属します。
詳細は [external/fmgen/readme.txt](external/fmgen/readme.txt) を参照してください。

> ```
> ------------------------------------------------------------------------------
>         FM Sound Generator with OPN/OPM interface
>         Copyright (C) by cisc 1998, 2003.
> ------------------------------------------------------------------------------
> ```
project M88 http://www.retropc.net/cisc/m88/

fmgen **以外**の著作権は nullre-orz ならびにオリジナル版の制作者である asmichi 氏に帰属し、[MIT ライセンス](LICENSE) でライセンスされます。
本ソフトウェアの大部分は asmichi 氏の功績によるものです。

本ソフトウェア全体が MIT ライセンスでライセンスされているわけでは **ない** ことに改めて注意してください。
