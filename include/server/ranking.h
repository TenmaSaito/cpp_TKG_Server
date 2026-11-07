//==================================================================================
//
// ランキングクラスのヘッダーファイル [ranking.h]
// Author : TENMA SAITO
//
//==================================================================================
#ifndef _RANKING_H_
#define _RANKING_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "core/main.h"
#include "util/filestream.h"

//**********************************************************************************
// *** ランキングクラス (スレッドセーフ) ***
// T			: スコアの型
// MAX_NUM		: ランキングに残す数
// IS_ASCENDING	: true なら小さい値ほど上位、false なら大きい値ほど上位
//**********************************************************************************
template<typename T, size_t MAX_NUM, bool IS_ASCENDING> class CRanking
{
public:
	static constexpr int RANK_OUT = -1;		// ランキング外だった場合の順位

	CRanking() = default;
	~CRanking() = default;

	HRESULT Init(std::string_view path);
	void Uninit(void) {}
	int Set(const T &score);
	std::array<T, MAX_NUM> Get(void) const;

private:
	bool Save(void) const;

	std::array<T, MAX_NUM> m_aScore = {};		// ランキングのスコア
	std::string m_sRankingPath;					// ランキングデータファイルのパス
	mutable std::mutex m_mutex;					// スレッド間のデータ競合防止用mutex
};

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
template<typename T, size_t MAX_NUM, bool IS_ASCENDING>
HRESULT CRanking<T, MAX_NUM, IS_ASCENDING>::Init(std::string_view path)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_sRankingPath = path;		// ファイル名保存

	// バイナリファイルから現在のランキングのスコアを取得
	CFileStream file;
	if (file.Open(path, true))
	{ // オープン成功時, ランキングデータ読み込み
		file.Read(m_aScore.data(), sizeof(T) * m_aScore.size());
		return S_OK;
	}

	// ファイルが存在しない場合、初期値を書き出す
	return Save() ? S_OK : E_FAIL;
}

//==================================================================================
// --- スコア追加処理 (戻り値は順位、ランキング外なら RANK_OUT) ---
//==================================================================================
template<typename T, size_t MAX_NUM, bool IS_ASCENDING>
int CRanking<T, MAX_NUM, IS_ASCENDING>::Set(const T &score)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 現在のランキングに引数のスコアを加えて並べ替え
	std::array<T, MAX_NUM + 1> aRanking = {};
	std::ranges::copy(m_aScore, aRanking.begin());
	aRanking[MAX_NUM] = score;

	if constexpr (IS_ASCENDING)
	{ // 昇順 (小さい値ほど上位)
		std::ranges::sort(aRanking);
	}
	else
	{ // 降順 (大きい値ほど上位)
		std::ranges::sort(aRanking, std::greater{});
	}

	// 上位のみ残してファイルへ書き出し
	std::copy(aRanking.begin(), aRanking.begin() + MAX_NUM, m_aScore.begin());
	Save();

	// 順位を取得 (同点の場合は上位扱い)
	const auto it = std::ranges::find(aRanking, score);
	const int nRank = static_cast<int>(std::distance(aRanking.begin(), it)) + 1;
	return (nRank > static_cast<int>(MAX_NUM)) ? RANK_OUT : nRank;
}

//==================================================================================
// --- ランキング取得処理 (コピーを返す) ---
//==================================================================================
template<typename T, size_t MAX_NUM, bool IS_ASCENDING>
std::array<T, MAX_NUM> CRanking<T, MAX_NUM, IS_ASCENDING>::Get(void) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_aScore;
}

//==================================================================================
// --- ファイル書き出し処理 (呼び出し側でロックしておくこと) ---
//==================================================================================
template<typename T, size_t MAX_NUM, bool IS_ASCENDING>
bool CRanking<T, MAX_NUM, IS_ASCENDING>::Save(void) const
{
	CFileStream file;
	if (file.Create(m_sRankingPath, true, CFileStream::FLAG::OVERWRITE) == false) return false;

	return file.Write(m_aScore.data(), sizeof(T) * m_aScore.size());
}
#endif
