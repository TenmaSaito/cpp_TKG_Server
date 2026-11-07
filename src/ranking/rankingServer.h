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
#include <mutex>
#include "app/main.h"
#include "ranking/protocol_ranking.h"

//**********************************************************************************
// *** 前方宣言 ***
//**********************************************************************************
class CTcpClient;
class CTcpListener;
template<typename T, size_t maxNum, bool isGreater> class CRanking;

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
	void ThreadProc(std::unique_ptr<CTcpClient> pClient);
	void SetWaitSeconds(const long lSec, const long lUsec);

private:
	std::unique_ptr<CTcpListener> m_pListener;							// 接続受付インスタンスへのポインタ
	std::unique_ptr<CRanking<int, MAX_RANKING, false>> m_pRanking;		// 接続受付インスタンスへのポインタ
	std::mutex m_fileMutex;			// スレッド間のデータ設定の入れ替わり防止用mutex
	struct
	{
		long lSec = 0L;		// 秒
		long lUsec = 0L;	// ミリ秒?
	} m_waitSeconds;
};
#endif