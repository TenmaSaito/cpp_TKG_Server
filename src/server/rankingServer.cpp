//==================================================================================
//
// ランキングサーバークラスのソースファイル [rankingServer.cpp]
// Author : TENMA SAITO
//
//==================================================================================
//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "server/rankingServer.h"
#include "server/ranking.h"
#include "network/tcp_client.h"

//**********************************************************************************
// *** 定数宣言 ***
//**********************************************************************************
namespace
{
	constexpr int MAX_RECV_SIZE = 256;		// 1回の通信で受け取るデータの最大バイト数
}

//==================================================================================
// --- コンストラクタ ---
//==================================================================================
CRankingServer::CRankingServer() = default;

//==================================================================================
// --- デストラクタ ---
//==================================================================================
CRankingServer::~CRankingServer()
{
	Uninit();
}

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
HRESULT CRankingServer::Init(std::string_view path,
	const u_short nPortID,
	const int nMaxStay)
{
	m_pListener = std::make_unique<CTcpListener>();
	m_pRanking = std::make_unique<Ranking>();

	// サーバーオープン
	if (m_pListener->Init(nPortID, nMaxStay) == false)
	{ // エラーコードを表示
		std::cerr << "Listener初期化エラー！ エラーコード : " << WSAGetLastError() << std::endl;
		return E_FAIL;
	}

	// ランキングクラスの初期化
	if (FAILED(m_pRanking->Init(path)))
	{ // エラーを表示
		std::cerr << "ランキング初期化エラー！" << std::endl;
		return E_FAIL;
	}

	return S_OK;
}

//==================================================================================
// --- 終了処理 ---
//==================================================================================
void CRankingServer::Uninit(void)
{
	SafeUniqueUninit(m_pListener);		// 接続受付の終了処理
	SafeUniqueUninit(m_pRanking);		// ランキングの終了処理
}

//==================================================================================
// --- 更新処理 ---
//==================================================================================
void CRankingServer::Update(void)
{
	if (m_pListener == nullptr) return;		// 未初期化

	std::unique_ptr<CTcpClient> pClient = m_pListener->Accept(m_waitSeconds.lSec, m_waitSeconds.lUsec);
	if (pClient == nullptr) return;		// 接続無し

	// クライアントから接続リクエストが来た場合、スレッドを分けて処理を続行
	printf("クライアントから接続リクエストを受信！\n");
	printf("接続先IPアドレス : [%s]\n", pClient->GetClientIP().c_str());
	std::thread(&CRankingServer::ThreadProc, this, std::move(pClient)).detach();
}

//==================================================================================
// --- 待機時間設定処理 ---
//==================================================================================
void CRankingServer::SetWaitSeconds(const long lSec, const long lUsec)
{
	m_waitSeconds.lSec = lSec;
	m_waitSeconds.lUsec = lUsec;
}

//==================================================================================
// --- スレッド別処理 ---
// 受信形式 : [int データサイズ][char コマンド][コマンドごとのデータ...]
//==================================================================================
void CRankingServer::ThreadProc(std::unique_ptr<CTcpClient> pClient)
{
	if (pClient == nullptr) return;

	const std::string sIP = pClient->GetClientIP();		// 接続先IPアドレス
	int nRecvByteAll = 0;								// 受け取るバイトデータのサイズ
	char aRecvData[MAX_RECV_SIZE] = {};					// 受け取ったバイナリデータ

	if (CLIENT_TCP_FAILED(pClient->RecvAll(&nRecvByteAll, 1)))
	{ // サイズの受信失敗
		printf("クライアントから受け取るバイトサイズの取得に失敗しました...\n");
	}
	else if (nRecvByteAll <= 0 || nRecvByteAll > MAX_RECV_SIZE)
	{ // サイズが不正 (バッファオーバーラン防止)
		printf("不正なデータサイズを受信しました... [%d byte]\n", nRecvByteAll);
	}
	else if (CLIENT_TCP_FAILED(pClient->RecvAll(aRecvData, nRecvByteAll)))
	{ // 受信失敗
		printf("バイトデータの取得に失敗しました...\n");
	}
	else
	{ // 1バイト目のコマンドで分岐
		const COMMAND_TYPE command = EnumUtil::FromUnderlying<COMMAND_TYPE>(aRecvData[0]);		// 受け取ったコマンド
		switch (IsValid(command) ? command : COMMAND_TYPE::NONE)
		{
		case COMMAND_TYPE::SET_RANKING:		// ランキングデータ設定
			SetRanking(*pClient, &aRecvData[1], nRecvByteAll - 1);
			break;

		case COMMAND_TYPE::GET_RANKING:		// ランキングデータ取得
			GetRanking(*pClient);
			break;

		default:
			printf("不明なコマンドを受信しました... [%d]\n", aRecvData[0]);
			break;
		}
	}

	// クライアントとの接続終了
	printf("[%s] との接続を終了します！\n", sIP.c_str());
	pClient->Uninit();
}

//==================================================================================
// --- ランキング設定コマンドの処理 ---
// 受信データ : [int スコア (ネットワークバイトオーダー)]
// 送信データ : [char 順位 (ランキング外なら UNRANKING)]
//==================================================================================
void CRankingServer::SetRanking(CTcpClient &client, const char *pPayload, const int nSize)
{
	printf("ランキングデータを設定します！\n");

	if (nSize < static_cast<int>(sizeof(uint32_t)))
	{ // スコア分のデータが無い
		printf("スコアのデータが不足しています... [%d byte]\n", nSize);
		return;
	}

	// バイトデータをスコアに変換
	uint32_t uScore = 0;
	memcpy(&uScore, pPayload, sizeof(uScore));
	const int nScore = static_cast<int>(ntohl(uScore));

	// スコア追加後順位取得
	const int nRank = m_pRanking->Set(nScore);
	const char cRank = (nRank == Ranking::RANK_OUT) ? static_cast<char>(UNRANKING) : static_cast<char>(nRank);

	// 順位を送信 (1バイトの為エンディアン変換は不要)
	client.SendWithSize(&cRank, 1);
	printf("順位を送信しました！ [%d位]\n", cRank);
}

//==================================================================================
// --- ランキング取得コマンドの処理 ---
// 送信データ : [int スコア (ネットワークバイトオーダー)] × MAX_RANKING
//==================================================================================
void CRankingServer::GetRanking(CTcpClient &client)
{
	printf("現在のランキングデータを送信します！\n");

	// 全データをネットワークバイトオーダーに変換
	const auto aRawData = m_pRanking->Get();
	u_long aConvert[MAX_RANKING] = {};
	std::ranges::transform(aRawData, aConvert, [](int nScore) { return htonl(static_cast<u_long>(nScore)); });

	// ランキングデータを送信
	client.SendWithSize(aConvert, MAX_RANKING);
	printf("現在のランキングデータを送信しました！\n");
}
