//==================================================================================
//
// プリコンパイル済みヘッダーファイル [pch.h]
// Author : TENMA SAITO
//
//==================================================================================
#ifndef _PCH_H_
#define _PCH_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#define _WINSOCK_DEPRECATED_NO_WARNINGS		// WinSock2の警告対処マクロ
#include <WinSock2.h>						// 通信処理に必要
#include <WS2tcpip.h>						// inet_pton / inet_ntop に必要
#include <Windows.h>						// Windowsシステムに必要
#include <assert.h>							// アサーションに必要
#include <stdio.h>							// コンソール出力に必要
#include <stdint.h>							// 固定長整数型
#include <array>							// 固定長配列
#include <vector>							// 可変長配列
#include <memory>							// スマートポインタ
#include <algorithm>						// アルゴリズム
#include <functional>						// 関数オブジェクト
#include <string>							// 文字列
#include <string_view>						// 文字列参照
#include <span>								// 配列参照
#include <iostream>							// 標準入出力
#include <fstream>							// ファイル入出力
#include <filesystem>						// ファイルシステム
#include <format>							// 文字列フォーマット
#include <mutex>							// 排他制御
#include <thread>							// マルチスレッド

//**********************************************************************************
// *** ライブラリのリンク ***
//**********************************************************************************
#pragma comment(lib, "ws2_32.lib")			// winsockへのリンカー設定
#pragma comment(lib, "winmm.lib")			// システムの時刻取得・サーバー接続に必要
#endif
