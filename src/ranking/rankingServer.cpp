//==================================================================================
// 
// ランキングサーバークラスのソースファイル [rankingServer.cpp]
// Author : TENMA SAITO
// 
//==================================================================================
//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "ranking/rankingServer.h"
#include "ranking/ranking.h"
#include "network/tcp_listener.h"
#include "network/tcp_client.h"
#include <thread>

//==================================================================================
// --- コンストラクタ ---
//==================================================================================
CRankingServer::CRankingServer()
{
}

//==================================================================================
// --- デストラクタ ---
//==================================================================================
CRankingServer::~CRankingServer()
{
}

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
HRESULT CRankingServer::Init(std::string_view path,
	const u_short nPortID,
	const int nMaxStay)
{
	m_pListener = std::make_unique<CTcpListener>();
	m_pRanking = std::make_unique<CRanking<int, MAX_RANKING, false>>();
	if (m_pListener == nullptr || m_pRanking == nullptr) return E_FAIL;		// インスタンス生成失敗

	// サーバーオープン
	if (m_pListener->Init(nPortID, nMaxStay) == false)
	{ // エラーコード取得
		std::cerr << "Listener初期化エラー！ エラーコード : " << WSAGetLastError() << std::endl;
		return E_FAIL;		// サーバーの初期化に失敗
	}

	// ランキングクラスの初期化
	if (FAILED(m_pRanking->Init(path)))
	{ // エラーを表示
		std::cerr << "ランキング初期化エラー！" << std::endl;
		return E_FAIL;		// ランキングクラスの初期化に失敗
	}

	return S_OK;
}

//==================================================================================
// --- 終了処理 ---
//==================================================================================
void CRankingServer::Uninit(void)
{ // 各インスタンスの終了処理
	if (m_pRanking != nullptr)
	{ // ランキングの終了処理
		m_pRanking->Uninit();
		m_pRanking.reset();
	}

	if (m_pListener != nullptr)
	{ // サーバーの終了処理
		m_pListener->Uninit();
		m_pListener.reset();
	}
}

//==================================================================================
// --- 更新処理 ---
//==================================================================================
void CRankingServer::Update(void)
{
	std::unique_ptr<CTcpClient> pClient;
	pClient.reset(m_pListener->Accept());
	if (pClient == nullptr) return;		// 受信無し

	// クライアントから接続リクエストが来た場合、スレッドを分けて処理を続行
	printf("クライアントから接続リクエストを受信！\n");
	printf("接続先IPアドレス : [%s]\n", inet_ntoa(pClient->GetClientAddr().sin_addr));
	std::thread th(&CRankingServer::ThreadProc, this, std::move(pClient));		// 新規スレッド作成

	// スレッドを切り離し
	th.detach();
}

//==================================================================================
// --- スレッド別処理 ---
//==================================================================================
void CRankingServer::ThreadProc(std::unique_ptr<CTcpClient> pClient)
{
	if (pClient == nullptr) return;

	int nRecvByteAll = 0;				// 受け取るバイトデータのサイズ
	char aRecvData[MAX_PATH] = {};		// 受け取ったバイナリデータ
	COMMAND_TYPE command;				// 受け取ったコマンドの種類

	if (CLIENT_TCP_FAILED(pClient->RecvAll(&nRecvByteAll, 1)))
	{ // クライアントとの接続終了
		printf("クライアントから受け取るバイトサイズの取得に失敗しました...\n");
		printf("[%s] との接続を終了します！\n", inet_ntoa(pClient->GetClientAddr().sin_addr));
		pClient->Uninit();
		return;
	}

	if (CLIENT_TCP_FAILED(pClient->RecvAll(aRecvData, nRecvByteAll)))
	{ // 受信失敗
		printf("バイトデータの取得に失敗しました...\n");
		printf("[%s] との接続を終了します！\n", inet_ntoa(pClient->GetClientAddr().sin_addr));
		pClient->Uninit();
		return;
	}

	// 1バイト目をコマンドへ変換
	command = static_cast<COMMAND_TYPE>(aRecvData[0]);
	if (command == COMMAND_TYPE_SET_RANKING)
	{ // ランキングデータ設定
		printf("ランキングデータを設定します！\n");

		u_long ulScore = 0;			// 受信したスコア
		int nScore = 0;		// 変換後のスコア
		char cRank = 1;		// 順位

		// 残りのバイトデータをスコアに変換
		memcpy(&ulScore, &aRecvData[1], sizeof(int));
		nScore = ntohl(ulScore);

		{ // セクション開始
			std::lock_guard<std::mutex> lock(m_fileMutex);

			// スコア追加後順位取得
			cRank = m_pRanking->Set(nScore);
			if (cRank == -1) cRank = UNRANKING;		// ランキング外ならば、値を指定
		}

		// 順位を送信 (1バイトの為エンディアン変換は不要)
		printf("順位を送信しました！ [%d位]\n", cRank);
		pClient->SendWithSize(&cRank, 1);
	}
	else if (command == COMMAND_TYPE_GET_RANKING)
	{ // ランキングデータ取得
		printf("現在のランキングデータを送信します！");

		auto aRawData = m_pRanking->Get();	// ランキングデータ
		int aConvert[MAX_RANKING] = {};		// 変換後のランキングデータ
		int nCntRanking = 0;				// 現在の順位

		// 全データをエンディアン変換
		for (auto &rRankingData : aConvert)
		{ // 変換
			rRankingData = htonl(aRawData[nCntRanking]);
			nCntRanking++;
		}

		// ランキングデータを送信
		printf("現在のランキングデータを送信しました！\n");
		pClient->SendWithSize(aConvert, std::span(aConvert).size());
	}

	// クライアントとの接続終了
	printf("[%s] との接続を終了します！\n", inet_ntoa(pClient->GetClientAddr().sin_addr));
	pClient->Uninit();
}

//==================================================================================
// --- 待機時間設定処理 ---
//==================================================================================
void CRankingServer::SetWaitSeconds(const long lSec, const long lUsec)
{
	m_waitSeconds.lSec = lSec;
	m_waitSeconds.lUsec = lUsec;
}