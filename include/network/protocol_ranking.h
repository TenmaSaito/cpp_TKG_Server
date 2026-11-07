//==================================================================================
// 
// 通信規則ヘッダファイル [protocol_ranking.h]
// Author : TENMA SAITO
// 
//==================================================================================
#ifndef _PROTOCOL_RANKING_H_
#define _PROTOCOL_RANKING_H_
#include "util/enumUtil.h"

//**********************************************************************************
// *** 定数宣言 ***
//**********************************************************************************
constexpr int MAX_RANKING = 5;		// ランキングの数
constexpr int UNRANKING = -1;		// ランキングの乗らなかった場合に帰ってくる値

//**********************************************************************************
// *** 通信コマンドの種類 ***
//**********************************************************************************
enum class COMMAND_TYPE
{
	NONE = 0,		// 無し
	SET_RANKING,	// ランキング設定
	GET_RANKING,	// ランキング取得
	MAX
};

// 有効値かどうかの判定
constexpr bool IsValid(const COMMAND_TYPE value) noexcept
{
	return EnumUtil::InRange(value, COMMAND_TYPE::SET_RANKING, COMMAND_TYPE::MAX);
}
#endif
