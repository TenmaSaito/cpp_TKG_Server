//==================================================================================
//
// TCP送受信クラスのソースファイル [tcp_client.cpp]
// Author : TENMA SAITO
//
//==================================================================================
//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "network/tcp_client.h"

//==================================================================================
// --- 初期化処理 (接続済みソケットを割り当て) ---
//==================================================================================
void CTcpClient::Init(const SOCKET sock, const struct sockaddr_in &client)
{
	// 既に持っているソケットは閉じる
	CloseSocket();

	m_sock = sock;		// ソケットを保存
	m_addr = client;	// クライアントの情報を保存
}

//==================================================================================
// --- 初期化処理 (IPアドレスとポート番号指定) ---
//==================================================================================
int CTcpClient::Init(const char *pIPAddress, const u_short nPortID)
{
	if (pIPAddress == nullptr) return CLIENT_E_INVALID_POINTER;		// IPアドレスがNULL

	// 既に持っているソケットは閉じる
	CloseSocket();

	// 接続先を設定
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPortID);
	if (inet_pton(AF_INET, pIPAddress, &addr.sin_addr) != 1)
	{ // IPアドレスの変換に失敗
		return CLIENT_E_INVALID_ARG;
	}

	// 通信用のソケットを作成
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == INVALID_SOCKET) return CLIENT_E_INVALID_SOCKET;	// ソケット作成失敗

	// サーバーに接続する！
	if (connect(m_sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0)
	{ // 接続失敗時、ソケットを閉じる
		CloseSocket();
		return CLIENT_E_CONNECTION;
	}

	// 接続先の情報を保存
	m_addr = addr;
	return CLIENT_S_FUNCTION;
}

//==================================================================================
// --- 接続先IPアドレスの文字列取得処理 (スレッドセーフ) ---
//==================================================================================
std::string CTcpClient::GetClientIP(void) const
{
	char aIP[INET_ADDRSTRLEN] = {};		// IPアドレスの文字列
	if (inet_ntop(AF_INET, &m_addr.sin_addr, aIP, sizeof(aIP)) == nullptr)
	{ // 変換失敗
		return "unknown";
	}

	return aIP;
}

//==================================================================================
// --- ソケット切断処理 ---
//==================================================================================
void CTcpClient::CloseSocket(void)
{
	if (m_sock == INVALID_SOCKET) return;		// 既に無効

	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
	m_addr = {};
}

//==================================================================================
// --- 受信可能になるまで待機する処理 ---
//==================================================================================
bool CTcpClient::WaitReadable(const long lSeconds, const long lUsec) const
{
	fd_set fdWait;		// ソケット登録用変数
	FD_ZERO(&fdWait);
	FD_SET(m_sock, &fdWait);

	timeval timeout = { lSeconds, lUsec };		// タイムアウト時間

	// 無限待機の場合、selectの第5引数をnullに指定する
	timeval *pTimeout = (lSeconds == RECV_INFINITY) ? nullptr : &timeout;

	// Windowsではselectの第1引数は無視される
	return select(0, &fdWait, nullptr, nullptr, pTimeout) > 0;
}
