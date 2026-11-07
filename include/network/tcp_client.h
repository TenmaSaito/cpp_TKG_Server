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
#include "core/main.h"

//**********************************************************************************
// *** マクロ定義 ***
//**********************************************************************************
#define CLIENT_TCP_SUCCESS(code)	((code) >= CLIENT_S_FUNCTION)		// 処理の成功判定マクロ
#define CLIENT_TCP_FAILED(code)		((code) < CLIENT_S_FUNCTION)		// 処理の失敗判定マクロ

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
	CTcpClient() = default;
	~CTcpClient() { CloseSocket(); }
	CTcpClient(const CTcpClient &) = delete;				// ソケットの二重解放防止の為コピー禁止
	CTcpClient &operator=(const CTcpClient &) = delete;

	void Init(const SOCKET sock, const struct sockaddr_in &client);
	int Init(const char *pIPAddress, const u_short nPortID);
	void Uninit(void) { CloseSocket(); }
	struct sockaddr_in GetClientAddr(void) const { return m_addr; }
	std::string GetClientIP(void) const;
	template<class T> int Send(const T *pData, const int nNum);
	template<class T> int SendWithSize(const T *pData, const int nNum);
	template<class T> int Recv(T *pOut, const int nNum);
	template<class T> int RecvAll(T *pOut, const int nNum);
	template<class T> int Recv(T *pOut, const int nNum, const long lSeconds, const long lUsec = 0);

private:
	void CloseSocket(void);
	bool WaitReadable(const long lSeconds, const long lUsec) const;

	SOCKET m_sock = INVALID_SOCKET;		// 通信用ソケット
	struct sockaddr_in m_addr = {};		// クライアントの情報
};

//==================================================================================
// --- データ送信処理 (エンディアン変換は外部で行う) ---
// 指定バイト数を全て送り切るまで送信を繰り返す
//==================================================================================
template<class T> int CTcpClient::Send(const T *pData, const int nNum)
{
	if (m_sock == INVALID_SOCKET) return CLIENT_E_INVALID_SOCKET;		// ソケットが無効
	if (pData == nullptr) return CLIENT_E_INVALID_POINTER;				// 読み取り元がNULL
	if (nNum <= 0) return CLIENT_E_INVALID_ARG;							// 配列数が0以下

	const int nByte = static_cast<int>(sizeof(T)) * nNum;			// 処理するバイト数
	const char *pByte = reinterpret_cast<const char *>(pData);		// 送信元のバイト列
	int nTotal = 0;													// 送信済みのバイト数

	while (nTotal < nByte)
	{ // 全て送信するまで繰り返す
		const int nSentByte = send(m_sock, &pByte[nTotal], nByte - nTotal, 0);
		if (nSentByte == SOCKET_ERROR) return CLIENT_E_CONNECTION;		// 送信失敗

		nTotal += nSentByte;
	}

	return nTotal;
}

//==================================================================================
// --- データ送信処理 (サイズ自動送信) ---
//==================================================================================
template<class T> int CTcpClient::SendWithSize(const T *pData, const int nNum)
{
	const int nByte = static_cast<int>(sizeof(T)) * nNum;		// 処理するバイト数

	// 送信するデータのバイト数を先に送信
	const int nResult = Send(&nByte, 1);
	if (CLIENT_TCP_FAILED(nResult)) return nResult;

	// データを送信
	return Send(pData, nNum);
}

//==================================================================================
// --- データ受信処理 (エンディアン変換は外部で行う) ---
//==================================================================================
template<class T> int CTcpClient::Recv(T *pOut, const int nNum)
{
	if (m_sock == INVALID_SOCKET) return CLIENT_E_INVALID_SOCKET;		// ソケットが無効
	if (pOut == nullptr) return CLIENT_E_INVALID_POINTER;				// 書き出し先がNULL
	if (nNum <= 0) return CLIENT_E_INVALID_ARG;							// 配列数が0以下

	const int nByte = static_cast<int>(sizeof(T)) * nNum;		// 処理するバイト数

	// データを受信する
	const int nRecvSize = recv(m_sock, reinterpret_cast<char *>(pOut), nByte, 0);
	if (nRecvSize <= 0)
	{ // 受信失敗時、ソケットを閉じる
		CloseSocket();
		return CLIENT_E_CONNECTION;
	}

	// 受信したサイズを返す
	return nRecvSize;
}

//==================================================================================
// --- データ受信処理 (指定サイズを全て受信) ---
//==================================================================================
template<class T> int CTcpClient::RecvAll(T *pOut, const int nNum)
{
	if (pOut == nullptr) return CLIENT_E_INVALID_POINTER;		// 書き出し先がNULL
	if (nNum <= 0) return CLIENT_E_INVALID_ARG;					// 配列数が0以下

	const int nByteSize = static_cast<int>(sizeof(T)) * nNum;	// 受け取るデータのバイト数
	char *pByte = reinterpret_cast<char *>(pOut);				// 受け取ったデータ
	int nTotal = 0;												// 合計で受け取ったバイト数

	while (nTotal < nByteSize)
	{ // 受け取ったバイト数が指定のバイト数未満の間受け取り続ける
		const int nRecvByte = Recv(&pByte[nTotal], nByteSize - nTotal);
		if (nRecvByte <= 0) return nRecvByte;		// 受信失敗

		nTotal += nRecvByte;
	}

	return nTotal;
}

//==================================================================================
// --- データ受信処理 (時間指定) ---
// lSeconds に RECV_INFINITY を指定すると無限待機
//==================================================================================
template<class T> int CTcpClient::Recv(T *pOut, const int nNum, const long lSeconds, const long lUsec)
{
	if (m_sock == INVALID_SOCKET) return CLIENT_E_INVALID_SOCKET;		// ソケットが無効

	// 受信可能になるまで待機
	if (WaitReadable(lSeconds, lUsec) == false) return CLIENT_E_RECV_TIMEOUT;

	return Recv(pOut, nNum);
}
#endif
