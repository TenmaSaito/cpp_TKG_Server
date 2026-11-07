//==================================================================================
//
// TCP接続受付クラスのソースファイル [tcp_listener.cpp]
// Author : TENMA SAITO
//
//==================================================================================
//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "network/tcp_listener.h"
#include "network/tcp_client.h"

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
bool CTcpListener::Init(const u_short nPortID, const int nMaxStay)
{
	// 既に持っているソケットは閉じる
	Uninit();

	// ソケット作成
	m_sockServer = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockServer == INVALID_SOCKET) return false;		// ソケット作成失敗

	// 接続を受け付けるアドレスとポート番号を指定
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPortID);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	// ソケットにポートを割り当て、接続受付の準備をする (第二引数に最大何人待機できるか)
	if (bind(m_sockServer, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR
		|| listen(m_sockServer, nMaxStay) == SOCKET_ERROR)
	{ // 失敗した場合はソケットを閉じる (エラーコードは呼び出し側でWSAGetLastErrorから取得)
		const int nError = WSAGetLastError();
		Uninit();
		WSASetLastError(nError);
		return false;
	}

	return true;
}

//==================================================================================
// --- 終了処理 ---
//==================================================================================
void CTcpListener::Uninit(void)
{
	if (m_sockServer == INVALID_SOCKET) return;		// 既に無効

	closesocket(m_sockServer);
	m_sockServer = INVALID_SOCKET;
}

//==================================================================================
// --- 接続待ち受け処理 (接続されるまで待機) ---
//==================================================================================
std::unique_ptr<CTcpClient> CTcpListener::Accept(void)
{
	return Accept(TIMEOUT_INFINITY, 0);
}

//==================================================================================
// --- 接続待ち受け処理 (タイムアウトあり) ---
// lSeconds に TIMEOUT_INFINITY を指定すると無限待機
//==================================================================================
std::unique_ptr<CTcpClient> CTcpListener::Accept(const long lSeconds, const long lUsec)
{
	if (m_sockServer == INVALID_SOCKET) return nullptr;		// 接続受付の準備ができていない

	// ソケットとfd_set変数を紐づけ
	fd_set fdWait;
	FD_ZERO(&fdWait);
	FD_SET(m_sockServer, &fdWait);

	timeval timeout = { lSeconds, lUsec };		// タイムアウト時間

	// 無限待機の場合、selectの第5引数をnullに指定する
	timeval *pTimeout = (lSeconds == TIMEOUT_INFINITY) ? nullptr : &timeout;

	// 指定時間まで接続待機 (Windowsではselectの第1引数は無視される)
	if (select(0, &fdWait, nullptr, nullptr, pTimeout) <= 0) return nullptr;		// タイムアウト

	// 接続を受け付ける
	struct sockaddr_in clientAddr = {};
	int nLength = sizeof(clientAddr);
	const SOCKET sock = accept(m_sockServer, reinterpret_cast<struct sockaddr *>(&clientAddr), &nLength);
	if (sock == INVALID_SOCKET) return nullptr;		// 接続失敗

	// 送受信クラスを生成し、ソケットとクライアントの情報を割り当て
	auto pClient = std::make_unique<CTcpClient>();
	pClient->Init(sock, clientAddr);
	return pClient;
}
