//==================================================================================
// 
// TCP送受信クラスのヘッダーファイル [tcp_client.h]
// Author : TENMA SAITO
// 
//==================================================================================
#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "app/main.h"

//**********************************************************************************
// *** マクロ定義 ***
//**********************************************************************************
#define CLIENT_TCP_SUCCESS(code)	(code >= CLIENT_S_FUNCTION)		// 処理の成功判定マクロ
#define CLIENT_TCP_FAILED(code)		(code < CLIENT_S_FUNCTION)		// 処理の失敗判定マクロ

//**********************************************************************************
// *** 定数宣言 ***
//**********************************************************************************
constexpr int CLIENT_S_FUNCTION = 1;			// 処理成功
constexpr int CLIENT_E_INVALID_SOCKET = -1;		// 無効なソケットだった場合
constexpr int CLIENT_E_INVALID_POINTER = -2;	// 引数が無効なポインタだった場合
constexpr int CLIENT_E_INVALID_ARG = -3;		// 引数が無効な値だった場合
constexpr int CLIENT_E_ALLOCATE = -4;			// メモリ確保に失敗した場合
constexpr int CLIENT_E_CONNECTION = -5;			// 接続に失敗した場合
constexpr int CLIENT_E_RECV_TIMEOUT = -6;		// 受信中にタイムアウト時間が経過した場合
constexpr int RECV_INFINITY = -1;				// 無限待機

//**********************************************************************************
// *** クライアントのTCP送受信クラス ***
//**********************************************************************************
class CTcpClient
{
public:
	CTcpClient();
	~CTcpClient();

	void Init(const SOCKET sock, const struct sockaddr_in &client);
	int Init(const char *pIPAddress, const u_short nPortID);
	void Uninit(void);
	struct sockaddr_in GetClientAddr(void) const { return m_addr; }
	template<class T> int Send(const T *pData, const int num);
	template<class T> int SendWithSize(const T *pData, const int num);
	template<class T> int Recv(T *pOut, const int num);
	template<class T> int RecvAll(T *pOut, const int num);
	template<class T> int Recv(T *pOut, const int num, const long nSeconds, const long nMill = 0);

private:
	void CloseSocket(void);

	SOCKET m_sock = INVALID_SOCKET;		// 通信用ソケット
	struct sockaddr_in m_addr = {};		// クライアントの情報
};

//==================================================================================
// --- データ送信処理 (エンディアン変換は外部で行う) ---
//==================================================================================
template<class T> int CTcpClient::Send(const T *pData, const int num)
{
	if (m_sock == INVALID_SOCKET)
	{ // ソケットが無効なら失敗
		return CLIENT_E_INVALID_SOCKET;
	}
	else if (pData == NULL)
	{ // データの読み取り元がNULLなら失敗
		return CLIENT_E_INVALID_POINTER;
	}
	else if (num <= 0)
	{ // 配列が0以下の場合失敗
		return CLIENT_E_INVALID_ARG;
	}

	int nByte = sizeof(T) * num;		// 処理するバイト数
	int nSentByte = 0;					// 送信されたデータサイズ

	// unsigned char*に変換し、データを送信
	nSentByte = send(m_sock, reinterpret_cast<const char *>(pData), nByte, 0);

	// 送信成功
	return nSentByte;
}

//==================================================================================
// --- データ送信処理 (サイズ自動送信) ---
//==================================================================================
template<class T> int CTcpClient::SendWithSize(const T *pData, const int num)
{
	int nByte = sizeof(T) * num;		// 処理するバイト数

	// 送信するデータのバイト数を先に送信
	Send(&nByte, 1);

	// データを送信
	return Send(pData, num);
}

//==================================================================================
// --- データ受信処理 (エンディアン変換は外部で行う) ---
//==================================================================================
template<class T> int CTcpClient::Recv(T *pOut, const int num)
{
	if (m_sock == INVALID_SOCKET)
	{ // ソケットが無効なら失敗
		return CLIENT_E_INVALID_SOCKET;
	}
	else if (pOut == NULL)
	{ // データの書き出し先がNULLなら失敗
		return CLIENT_E_INVALID_POINTER;
	}
	else if (num <= 0)
	{ // 配列が0以下の場合失敗
		return CLIENT_E_INVALID_ARG;
	}

	int nByte = sizeof(T) * num;		// 処理するバイト数

	// データを受信する
	int nRecvSize = recv(m_sock, reinterpret_cast<char *>(pOut), nByte, 0);
	if (nRecvSize <= 0)
	{ // 受信失敗時
		// ソケットを閉じる
		CloseSocket();

		// 失敗
		return CLIENT_E_CONNECTION;
	}

	// サイズを返す
	return nRecvSize;
}

//==================================================================================
// --- データ受信処理 (エンディアン変換は外部で行う) ---
//==================================================================================
template<class T> int CTcpClient::RecvAll(T *pOut, const int num)
{
	int nTotal = 0;			// 合計で受け取ったバイト数
	int nByteSize = sizeof(T) * num;	// 受け取るデータのバイト数
	char *pByte = reinterpret_cast<char *>(pOut);	// 受け取ったデータ

	while (nTotal < nByteSize)
	{ // 受け取ったバイト数が指定のバイト数未満の間受け取り続ける
		int nRecvByte = Recv(&pByte[nTotal], nByteSize - nTotal);
		if (nRecvByte <= 0) return nRecvByte;		// 受信失敗

		nTotal += nRecvByte;		// 受け取ったバイト数を加算
	}

	return nTotal;
}

//==================================================================================
// --- データ受信処理 (時間指定) ---
//==================================================================================
template<class T> int CTcpClient::Recv(T *pOut, const int num, const long nSeconds, const long nMill)
{
	if (m_sock == INVALID_SOCKET)
	{ // ソケットが無効なら失敗
		return CLIENT_E_INVALID_SOCKET;
	}
	else if (pOut == NULL)
	{ // データの書き出し先がNULLなら失敗
		return CLIENT_E_INVALID_POINTER;
	}
	else if (num <= 0)
	{ // 配列が0以下の場合失敗
		return CLIENT_E_INVALID_ARG;
	}

	// タイムアウト設定関連変数
	timeval timeout;				// タイムアウト用変数
	fd_set fd_Wait;					// ソケット登録用変数
	timeval *pTimeout = nullptr;	// 条件分岐用ポインタ
	timeout.tv_sec = nSeconds;		// タイムアウト時間を設定
	timeout.tv_usec = nMill;		// ミリ秒

	// ソケットとfd_set変数を紐づけ
	FD_ZERO(&fd_Wait);
	FD_SET(m_sock, &fd_Wait);

	// 無限待機の場合、selectの第5引数をnullに指定する
	pTimeout = (nSeconds == RECV_INFINITY) ? nullptr : &timeout;

	// 現在ソケットから情報を受け取れるのかを確認
	if (select(m_sock + 1, &fd_Wait, NULL, NULL, pTimeout) <= 0)
	{
		return CLIENT_E_RECV_TIMEOUT;
	}

	int nByte = sizeof(T) * num;		// 処理するバイト数

	// データを受信する
	int nRecvSize = recv(m_sock, reinterpret_cast<char *>(pOut), nByte, 0);
	if (nRecvSize <= 0)
	{ // 受信失敗時
		// ソケットを閉じる
		CloseSocket();

		// 失敗
		return CLIENT_E_CONNECTION;
	}

	// サイズを返す
	return nRecvSize;
}
#endif