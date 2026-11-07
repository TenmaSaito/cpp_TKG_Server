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
#include <memory>
#include <array>
#include <span>
#include <string_view>
#include <algorithm>
#include "app/main.h"
#include "utility/filestream.h"

//**********************************************************************************
// *** ランキングクラス ***
//**********************************************************************************
template<typename T, size_t maxNum, bool isGreater> class CRanking
{
public:
	CRanking();
	~CRanking();

	HRESULT Init(std::string_view path);
	void Uninit(void) {}
	int Set(const T &score);
	std::span<const T> Get(void) const { return m_aScore; }

private:
	std::array<T, maxNum> m_aScore = {};			// ランキングのスコア
	std::string m_sRankingPath;		// ランキングデータファイルのパス
};

//==================================================================================
// --- コンストラクタ ---
//==================================================================================
template<typename T, size_t maxNum, bool isGreater> CRanking<T, maxNum, isGreater>::CRanking()
{
}

//==================================================================================
// --- デストラクタ ---
//==================================================================================
template<typename T, size_t maxNum, bool isGreater> CRanking<T, maxNum, isGreater>::~CRanking()
{
}

//==================================================================================
// --- 初期化処理 ---
//==================================================================================
template<typename T, size_t maxNum, bool isGreater> 
HRESULT CRanking<T, maxNum, isGreater>::Init(std::string_view path)
{ // バイナリファイルから現在のランキングのスコアを取得
	std::unique_ptr<CFileStream> pFile(new CFileStream);		// ファイルストリームへのポインタ
	if (pFile == nullptr) return E_FAIL;		// ファイルストリーム生成失敗

	if (pFile->Open(path, true))
	{ // オープン成功時, ランキングデータ読み込み
		pFile->Read(&m_aScore[0], sizeof(T) * maxNum);

		// ファイルを閉じる
		pFile->Close();
	}
	else if (pFile->Create(path, true, CFileStream::FLAG_OVERWRITE))
	{ // ファイルが存在せず、新規作成に成功した場合、初期値を書き出し
		pFile->Write(&m_aScore[0], sizeof(T) * m_aScore.size());

		// ファイルを閉じる
		pFile->Close();
	}
	else
	{ // 失敗
		return E_FAIL;
	}

	m_sRankingPath = path;		// ファイル名保存
	return S_OK;
}

//==================================================================================
// --- スコア追加関数 ---
//==================================================================================
template<typename T, size_t maxNum, bool isGreater> int CRanking<T, maxNum, isGreater>::Set(const T &score)
{
	std::array<T, maxNum + 1> aRanking = {};		// ランキング計算用変数

	// 現在のランキングの値をコピー
	std::ranges::copy(m_aScore, aRanking.begin());

	// 引数のスコアを追加
	aRanking[maxNum] = score;

	// ランキングをソート
	if constexpr (isGreater == true)
	{ // 降順
		std::ranges::sort(aRanking);
	}
	else
	{ // 昇順
		std::ranges::sort(aRanking, std::greater{});
	}

	// ソート後のランキングの値をコピー
	std::copy(aRanking.begin(), aRanking.end() - 1, m_aScore.begin());

	// ファイルへ書き出し
	std::unique_ptr<CFileStream> pFile(new CFileStream);		// ファイルストリームへのポインタ
	if (pFile != nullptr)
	{ // 生成成功時
		if (pFile->Create(m_sRankingPath, true, CFileStream::FLAG_OVERWRITE))
		{ // ファイル作成成功時、ランキングデータを新規で書き出し
			pFile->Write(&m_aScore[0], sizeof(T) * m_aScore.size());

			// ファイルを閉じる
			pFile->Close();
		}
	}

	// 順位を取得
	int nRank = 1;
	for (const auto &rRanking : aRanking)
	{ // ランキングの合計数分回す
		// 同じスコアだった場合ストップ
		if (rRanking == score) break;

		// 順位を増加
		nRank++;
	}
	
	// 範囲外の場合、-1
	if (nRank > maxNum) nRank = -1;
	return nRank;
}
#endif