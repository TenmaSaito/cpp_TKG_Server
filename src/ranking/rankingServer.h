//==================================================================================
//
// ランキングサーバークラスのヘッダーファイル [rankingServer.h]
// Author : TENMA SAITO
//
//==================================================================================
#ifndef _RANKING_SERVER_H_
#define _RANKING_SERVER_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include <memory>
#include <string_view>
#include "app/main.h"
#include "ranking/protocol_ranking.h"
#include "network/tcp_listener.h"

//**********************************************************************************
// *** 前方宣言 ***
//**********************************************************************************
class CTcpClient;
template<typename T, size_t MAX_NUM, bool IS_ASCENDING> class CRanking;

//**********************************************************************************
// *** ランキングサーバークラス ***
//**********************************************************************************
class CRankingServer
{
public:
	CRankingServer();
	~CRankingServer();

	HRESULT Init(std::string_view path,
		const u_short nPortID,
		const int nMaxStay);
	void Uninit(void);
	void Update(void);
	void SetWaitSeconds(const long lSec, const long lUsec);

private:
	using Ranking = CRanking<int, MAX_RANKING, false>;		// スコアが大きいほど上位のランキング

	void ThreadProc(std::unique_ptr<CTcpClient> pClient);
	void SetRanking(CTcpClient &client, const char *pPayload, const int nSize);
	void GetRanking(CTcpClient &client);

	std::unique_ptr<CTcpListener> m_pListener;		// 接続受付インスタンスへのポインタ
	std::unique_ptr<Ranking> m_pRanking;			// ランキングインスタンスへのポインタ
	struct
	{
		long lSec = TIMEOUT_INFINITY;	// 秒 (TIMEOUT_INFINITY なら無限待機)
		long lUsec = 0L;				// マイクロ秒
	} m_waitSeconds;					// 接続待ちのタイムアウト時間
};
#endif
