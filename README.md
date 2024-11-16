# KpAsciiMml_e: ASCII music.com MML Player [Extended Edition]

本ソフトウェアは、PC-9801シリーズ上のASCIIツクールシリーズを中心に使用されていたASCII music.comのMMLファイルを再生するためのKbMedia Playerプラグインです。
asmichi氏の [ASCII music.com MML Player (KbAsciiMml)](https://github.com/asmichi/kb-ascii-mml) を元に、再現性を高めています。

便宜上 KpAsciiMml_e を名乗っていますが、生成ファイルなどはオリジナル版から変わらず KbAsciiMml のままです。
混同されないようご注意ください。
なお、Kb ではなく Kp としたのは、KbMedia Player公式プラグインと誤解されないようにするためです。

オリジナル版のREADMEは [README_org.md](./README_org.md) を参照してください。

## 特長

LOGIN SOFCOM No.8に収録されていた楽曲集 MML700 のうち、ASCII music.comでも文法エラーとなる2曲を除いて、すべて再生できるようになりました。

Dパートの再生に対応しました。MMLファイルと同じディレクトリにSOUND.datファイルがあればそれを参照します。読み込みに失敗した場合は、組み込みの効果音としてRPGツクール Dante98 II相当のデータを使用します。


## ビルド

次の手順でビルドできます。

- 環境変数 `BOOST_ROOT` をBoostのルートディレクトリにセットする
- 環境変数 `KPISDK_ROOT` をKbMedia PlayerプラグインSDKのルートディレクトリにセットする
- "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" を実行する
  - 環境によって異なりますので、適切なパスを指定してください。
- build.cmd を実行する

ビルドに使用した環境は次の通りです。

- Visual Studio Community 2022 Version 17.12.0 (「C++ ネイティブ開発」ワークロードあり)
- Boost 1.86.0 (ヘッダーファイルのみ)
- KbMedia PlayerプラグインSDK 2024/07/12版

なお、動作確認には KbMedia Player 3.15 を使用しています。

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

なお、fmgenには下記の改変を行っています。

- ~~PSG音源のノイズ周波数の調整(周波数を2倍にする)設定を追加~~  
- 警告・エラー除去
  - 不足していたincludeを追加
  - Windows API呼び出しをUnicode版からマルチバイト文字版に変更 (※実際には使用されていない)
  - uint8に負数を設定していたのをuint8にキャストして設定するよう変更
  - intとunsigned intで比較していた箇所をunsigned int同士の比較に変更

<br>

fmgen**以外**の著作権はnullre-orzならびにオリジナル版の制作者であるasmichi氏に帰属し、[MIT ライセンス](LICENSE) でライセンスされます。
本ソフトウェアの大部分はasmichi氏の功績によるものです。

本ソフトウェア全体がMITライセンスでライセンスされているわけでは **ない** ことに改めて注意してください。
