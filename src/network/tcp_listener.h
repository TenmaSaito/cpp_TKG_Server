//==================================================================================
//
// TCP接続受付クラスのヘッダーファイル [tcp_listener.h]
// Author : TENMA SAITO
//
//==================================================================================
#ifndef _TCP_LISTENER_H_
#define _TCP_LISTENER_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "app/main.h"
#include <memory>

//**********************************************************************************
// *** 定数宣言 ***
//**********************************************************************************
constexpr long TIMEOUT_INFINITY = -1;		// 無限待機

//**********************************************************************************
// *** 前方宣言 ***
//**********************************************************************************
class CTcpClient;		// 送受信クラス

//**********************************************************************************
// *** TCP接続受付クラス ***
//**********************************************************************************
class CTcpListener
{
public:
	CTcpListener() = default;
	~CTcpListener() { Uninit(); }
	CTcpListener(const CTcpListener &) = delete;				// ソケットの二重解放防止の為コピー禁止
	CTcpListener &operator=(const CTcpListener &) = delete;

	bool Init(const u_short nPortID, const int nMaxStay);
	void Uninit(void);
	std::unique_ptr<CTcpClient> Accept(void);
	std::unique_ptr<CTcpClient> Accept(const long lSeconds, const long lUsec);

private:
	SOCKET m_sockServer = INVALID_SOCKET;		// 接続受付用ソケット
};
#endif
