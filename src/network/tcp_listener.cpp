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
// --- コンストラクタ ---
//==================================================================================
CTcpListener::CTcpListener()
{
}

//==================================================================================
// --- デストラクタ ---
//==================================================================================
CTcpListener::~CTcpListener()
{
}

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
bool CTcpListener::Init(u_short nPortID, int nMaxStay)
{
	// ソケット作成
	m_sockServer = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockServer == INVALID_SOCKET)
	{ // ソケット作成失敗
		return false;
	}

	// 接続受付の準備
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;

	// 接続を受け付けるポート番号を指定
	addr.sin_port = htons(nPortID);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	// ソケットにポートを割り当てる
	bind(m_sockServer, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

	// 接続受付の準備をする (第二引数に最大何人待機できるか)
	listen(m_sockServer, nMaxStay);
	return true;
}

//==================================================================================
// --- 終了処理 ---
//==================================================================================
void CTcpListener::Uninit(void)
{
	if (m_sockServer != INVALID_SOCKET)
	{ // ソケットが有効なら
		// ソケットを閉じて無効化
		closesocket(m_sockServer);
		m_sockServer = INVALID_SOCKET;
	}
}

//==================================================================================
// --- 接続待ち受け処理 ---
//==================================================================================
CTcpClient *CTcpListener::Accept(void)
{
	if (m_sockServer == INVALID_SOCKET)
	{ // 接続待ち受け失敗
		return nullptr;
	}

	// 接続待機
	struct sockaddr_in clientAddr;
	int nLength = sizeof(clientAddr);	
	SOCKET sock = INVALID_SOCKET;		// 接続用ソケット
	CTcpClient *pClient = nullptr;		// 送受信クラスへのポインタ

	// クライアントから接続されるまで待機
	sock = accept(m_sockServer, reinterpret_cast<struct sockaddr*>(&clientAddr), &nLength);

	// 接続成功時
	if (sock != INVALID_SOCKET)
	{
		// 送受信クラスを生成
		pClient = new CTcpClient;
		if (pClient == nullptr)
		{ // 生成失敗
			return nullptr;
		}

		// ソケットとクライアントの情報を割り当て
		pClient->Init(sock, clientAddr);
	}

	// 生成されていれば、送受信クラスへのポインタを返す
	return pClient;
}

//==================================================================================
// --- 接続待ち受け処理 (タイムアウトあり) ---
//==================================================================================
CTcpClient* CTcpListener::Accept(const long lSeconds,
	const long lUsec)
{
	if (m_sockServer == INVALID_SOCKET)
	{ // 接続待ち受け失敗
		return nullptr;
	}

	// タイムアウト設定関連変数
	timeval timeout;				// タイムアウト用変数
	fd_set fd_Wait;					// ソケット登録用変数
	timeval *pTimeout = nullptr;	// 条件分岐用ポインタ
	timeout.tv_sec = lSeconds;		// タイムアウト時間を設定
	timeout.tv_usec = lUsec;		// ミリ秒指定

	// ソケットとfd_set変数を紐づけ
	FD_ZERO(&fd_Wait);
	FD_SET(m_sockServer, &fd_Wait);

	// 無限待機の場合、selectの第5引数をnullに指定する
	pTimeout = (lSeconds == TIMEOUT_INFINITY) ? nullptr : &timeout;

	// 指定時間まで接続待機
	if (select(m_sockServer + 1, &fd_Wait, NULL, NULL, pTimeout) <= 0)
	{ // 指定時間経過後、接続しなかった場合タイムアウトの為失敗
		return nullptr;
	}

	// 接続待機
	struct sockaddr_in clientAddr;
	int nLength = sizeof(clientAddr);
	SOCKET sock = INVALID_SOCKET;		// 接続用ソケット
	CTcpClient *pClient = nullptr;		// 送受信クラスへのポインタ

	// クライアントから接続されるまで待機
	sock = accept(m_sockServer, reinterpret_cast<struct sockaddr*>(&clientAddr), &nLength);

	// 接続成功時
	if (sock != INVALID_SOCKET)
	{
		// 送受信クラスを生成
		pClient = new CTcpClient;
		if (pClient == nullptr)
		{ // 生成失敗
			return nullptr;
		}

		// ソケットとクライアントの情報を割り当て
		pClient->Init(sock, clientAddr);
	}

	// 生成されていれば、送受信クラスへのポインタを返す
	return pClient;
}