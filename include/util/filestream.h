//==================================================================================
//
// ファイル入出力クラス！ [filestream.h]
// Author : TENMA SAITO
// DATE : 2026/04/28
//
//==================================================================================
#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_

//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include <iostream>
#include "util/enumUtil.h"
#include <fstream>
#include <string>
#include <string_view>
#include <format>
#include <functional>

#pragma push_macro("CreateFile")
#undef CreateFile

//**********************************************************************************
// *** ファイル入出力クラス ***
//**********************************************************************************
class CFileStream
{
public:
	// 書き出し時、ファイルが存在した場合のフラグの種類
	enum class FLAG
	{
		OVERWRITE = 0,		// ファイルを初期化し上書き
		CONTINUE,			// ファイルの続きから記入
		CANCEL,			// 処理を取り消し
		MAX
	};

	// 有効値かどうかの判定
	static constexpr bool IsValid(const FLAG value) noexcept
	{
		return EnumUtil::InRange(value, FLAG::OVERWRITE, FLAG::MAX);
	}

	CFileStream() noexcept = default;
	CFileStream(std::string_view sFilename, const bool bBinary);
	CFileStream(std::string_view sFilename, const bool bBinary, const FLAG flag);
	CFileStream(const CFileStream &file) = delete;
	CFileStream(CFileStream &&other) noexcept = default;
	~CFileStream();

	bool Open(std::string_view sFilename, const bool bBinary);
	bool Create(std::string_view sFilename, const bool bBinary, const FLAG flag);
	void Close(void);
	bool IsOpen(void) const;
	bool Check(void) const;
	bool IsInStream(void) const { return m_bInStream; }
	uintmax_t GetSize(void) const { return m_size; }
	std::string_view GetPath(void) const { return m_filename; }
	std::ifstream *GetInStream(void) { return &m_Infile; }
	std::ofstream *GetOutStream(void) { return &m_Outfile; }

	template<class T> bool Read(T &data);
	template<class T> bool Read(T *pData, const int nArray);
	template<class T> bool Read(T *pData, const size_t size);
	bool ReadByFunc(std::function<void(std::ifstream&)> readFunc);
	bool ReadString(std::string &string);
	static bool FindString(const std::string_view source, 
		const std::string_view s,
		size_t *pOut = nullptr,
		const bool bSkip = false);
	static int ToInt(const char *pStr, char **ppEnd = nullptr) { return std::strtol(pStr, ppEnd, 0); }
	static float ToFloat(const char *pStr, char **ppEnd = nullptr) { return std::strtof(pStr, ppEnd); }
	bool IsEoF(void) const;

	template<class T> bool Write(const T &data);
	template<class T> bool Write(const T *pData, const int nArray);
	template<class T> bool Write(const T *pData, const size_t size);
	bool WriteByFunc(std::function<void(std::ofstream&)> writeFunc);
	bool WriteString(const std::string_view &string);
	template<class... Args> static std::string ToString(std::_Fmt_string<Args...> fmt, Args&&... args);

	CFileStream &operator=(const CFileStream &file) = delete;
	CFileStream &operator=(CFileStream &&other) noexcept = default;
	template<class T> CFileStream &operator >> (T &data);
	template<class T, size_t num> CFileStream &operator >> (T (&data)[num]);
	template<class T> CFileStream &operator << (const T &data);
	template<class T, size_t num> CFileStream &operator << (const T (&data)[num]);
	operator bool(void) const;

private:
	void CloseFileInStream(void);
	void CloseFileOutStream(void);
	bool IsOpenInStream(void) const;
	bool IsOpenOutStream(void) const;
	bool CheckFileInStream(void) const;
	bool CheckFileOutStream(void) const;
	void ScanSizeInStream(void);

	std::ifstream m_Infile;		// ファイル入力ストリーム
	std::ofstream m_Outfile;	// ファイル出力ストリーム
	std::string m_filename;		// 開いているファイル名
	uintmax_t m_size = 0U;		// ファイルのサイズ
	bool m_bBinary = false;		// バイナリかどうか
	bool m_bInStream = false;	// ファイル入力かどうか
};

//==================================================================================
// --- ファイルからの情報取得処理 ---
//==================================================================================
template<class T> bool CFileStream::Read(T &data)
{
	// EOFの確認
	if (IsEoF() == true)
	{ // EOFの場合、処理失敗
		return false;
	}

	// 入力ストリームかどうか確認
	if (m_bInStream == false)
	{ // 出力ストリームだった場合、失敗
		return false;
	}

	if (m_bBinary == true)
	{ // バイナリファイルの場合
		// バイナリデータを読み込み
		m_Infile.read((char *)&data, sizeof(T));
	}
	else
	{ // テキストファイルの場合
		// ストリーム演算子で読み込み
		m_Infile >> data;
	}

	// ファイル読み込みのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 読み込み時に問題が発生したため、falseを返す
		return false;
	}

	// 成功
	return true;
}

//==================================================================================
// --- ファイルからの情報取得処理 (配列) ---
//==================================================================================
template<class T> bool CFileStream::Read(T *pData, const int nArray)
{
	// EOFの確認
	if (IsEoF() == true)
	{ // EOFの場合、処理失敗
		return false;
	}

	// 入力ストリームかどうか確認
	if (m_bInStream == false)
	{ // 出力ストリームだった場合、失敗
		return false;
	}

	// nullチェック
	if (pData == nullptr)
	{ // nullならアサーション
		assert(pData != nullptr);
		return false;
	}

	if (m_bBinary == true)
	{ // バイナリファイルの場合
		// バイナリデータを配列サイズ分読み込み
		m_Infile.read(reinterpret_cast<char*>(pData), sizeof(T) * nArray);
	}
	else
	{ // テキストファイルの場合
		// 配列分データ読み込み
		for (int nCntRead = 0; nCntRead < nArray; nCntRead++)
		{ // スペース区切りで読み込み
			// ストリーム演算子で読み込み
			m_Infile >> pData[nCntRead];

			// EOFだったら終了
			if (IsEoF() == true) break;
		}
	}

	// ファイル読み込みのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 読み込み時に問題が発生したため、falseを返す
		return false;
	}

	// 読み込み成功
	return true;
}

//==================================================================================
// --- ファイルからの情報取得処理 (バイナリ限定カスタムサイズ) ---
//==================================================================================
template<class T> bool CFileStream::Read(T *pData, const size_t size)
{
	// ファイルが開かれているか確認
	if (IsOpen() == false)
	{ // 開かれていない場合、失敗
		return false;
	}

	// 入力ストリームかどうか確認
	if (m_bInStream == false)
	{ // 出力ストリームだった場合、失敗
		return false;
	}

	// バイナリファイルか確認
	if (m_bBinary == false)
	{ // バイナリファイルではない場合、失敗
		return false;
	}

	// nullチェック
	if (pData == nullptr)
	{ // nullならアサーション
		assert(pData != nullptr);
		return false;
	}

	// バイナリデータを配列サイズ分読み込み
	m_Infile.read(reinterpret_cast<char*>(pData), size);

	// ファイル読み込みのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 読み込み時に問題が発生したため、falseを返す
		return false;
	}

	// 読み込み成功
	return true;
}

//==================================================================================
// --- ファイルへの情報出力処理 ---
//==================================================================================
template<class T> bool CFileStream::Write(const T &data)
{
	// ファイルが開かれているか確認
	if (IsOpen() == false)
	{ // 開かれていない場合、失敗
		return false;
	}

	// 出力ストリームかどうか確認
	if (m_bInStream == true)
	{ // 入力ストリームだった場合、失敗
		return false;
	}

	if (m_bBinary == true)
	{ // バイナリファイルの場合
		// バイナリデータとして書き出し
		m_Outfile.write(reinterpret_cast<char*>(const_cast<T*>(&data)), sizeof(T));
	}
	else
	{ // テキストファイルの場合
		// ストリーム演算子で出力
		m_Outfile << data;
	}

	// データの書き出し
	m_Outfile.flush();

	// ファイル書き出しのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 書き込み時に問題が発生したため、falseを返す
		return false;
	}

	// 書き出したサイズ分加算
	m_size += sizeof(T);

	// 書き出し成功
	return true;
}

//==================================================================================
// --- ファイルへの情報出力処理 (配列) ---
//==================================================================================
template<class T> bool CFileStream::Write(const T *pData, const int nArray)
{
	// ファイルが開かれているか確認
	if (IsOpen() == false)
	{ // 開かれていない場合、失敗
		return false;
	}

	// 出力ストリームかどうか確認
	if (m_bInStream == true)
	{ // 入力ストリームだった場合、失敗
		return false;
	}

	// nullチェック
	if (pData == nullptr)
	{ // nullならアサーション
		assert(pData != nullptr);
		return false;
	}

	if (m_bBinary == true)
	{ // バイナリファイルの場合
		// バイナリデータとして書き出し
		m_Outfile.write(reinterpret_cast<char*>(const_cast<T*>(pData)), sizeof(T) * nArray);
	}
	else
	{ // テキストファイルの場合
		// 配列分データ書き出し
		for (int nCntRead = 0; nCntRead < nArray; nCntRead++)
		{
			// ストリーム演算子で出力
			m_Outfile << pData[nCntRead];

			// データの書き出し
			m_Outfile.flush();
		}
	}

	// ファイル書き出しのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 書き込み時に問題が発生したため、falseを返す
		return false;
	}

	// 書き出したサイズ分加算
	m_size += sizeof(T) * nArray;

	// 書き出し成功
	return true;
}

//==================================================================================
// --- ファイルへの情報出力処理 (バイナリ限定カスタムサイズ) ---
//==================================================================================
template<class T> bool CFileStream::Write(const T *pData, const size_t size)
{
	// ファイルが開かれているか確認
	if (IsOpen() != true)
	{ // 開かれていない場合、失敗
		return false;
	}

	// 出力ストリームかどうか確認
	if (m_bInStream == true)
	{ // 入力ストリームだった場合、失敗
		return false;
	}

	// バイナリモードか確認
	if (m_bBinary != true)
	{ // バイナリファイルではない場合、失敗
		return false;
	}

	// nullチェック
	if (pData == nullptr)
	{ // nullならアサーション
		assert(pData != nullptr);
		return false;
	}

	// バイナリデータとして書き出し
	m_Outfile.write(reinterpret_cast<char*>(const_cast<T*>(pData)), size);

	// データの書き出し
	m_Outfile.flush();

	// ファイル書き出しのチェック
	bool bCheck = Check();
	if (bCheck == false)
	{ // 書き込み時に問題が発生したため、falseを返す
		return false;
	}

	// 書き出したサイズ分加算
	m_size += size;

	// 書き出し成功
	return true;
}

//==================================================================================
// --- 文字列化のヘルパー関数 ---
//==================================================================================
template<class... Args> std::string CFileStream::ToString(std::_Fmt_string<Args...> fmt, Args&&... args)
{
	return std::format(fmt, std::forward<Args>(args)...);
}

//==================================================================================
// --- 演算子のオーバーロード (挿入) ---
//==================================================================================
template<class T> CFileStream &CFileStream::operator<<(const T &data)
{
	// 書き出し処理を行う
	Write(data);

	// 連続挿入用に自身を返す
	return *this;
}

//==================================================================================
// --- 演算子のオーバーロード (配列挿入) ---
//==================================================================================
template<class T, size_t num> CFileStream& CFileStream::operator<<(const T (&data)[num])
{
	// 書き出し処理を行う
	Write(data, num);

	// 連続挿入用に自身を返す
	return *this;
}

//==================================================================================
// --- 演算子のオーバーロード (抽出) ---
//==================================================================================
template<class T> CFileStream &CFileStream::operator>>(T &data)
{
	// 読み込み処理を行う
	Read(data);

	// 連続抽出用に自身を返す
	return *this;
}

//==================================================================================
// --- 演算子のオーバーロード (配列抽出) ---
//==================================================================================
template<class T, size_t num> CFileStream& CFileStream::operator>>(T (&data)[num])
{
	// 読み込み処理を行う
	Read(data, num);

	// 連続抽出用に自身を返す
	return *this;
}

#pragma pop_macro("CreateFile")
#endif // !_FILESTREAM_H_
