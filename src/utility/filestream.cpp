//==================================================================================
//
// ファイル入出力クラスの作成！ [fliestream.cpp]
// Author : TENMA SAITO
//
//==================================================================================
//**********************************************************************************
// *** インクルードファイル ***
//**********************************************************************************
#include "utility/filestream.h"
#include <filesystem>

//==================================================================================
// --- コンストラクタ ---
//==================================================================================
CFileStream::CFileStream() noexcept
{ // メンバ変数をクリア
	m_size = 0;
	m_bBinary = false;
	m_bInStream = false;
}

//==================================================================================
// --- ファイルオープンコンストラクタ ---
//==================================================================================
CFileStream::CFileStream(std::string_view sFilename, const bool bBinary)
{ // メンバ変数をクリア
	m_size = 0;
	m_bBinary = false;
	m_bInStream = false;

	// ファイルオープン
	Open(sFilename, bBinary);
}

//==================================================================================
// --- ファイル作成コンストラクタ ---
//==================================================================================
CFileStream::CFileStream(std::string_view sFilename, const bool bBinary, const FLAG flag)
{ // メンバ変数をクリア
	m_size = 0;
	m_bBinary = false;
	m_bInStream = false;

	// ファイル作成
	Create(sFilename, bBinary, flag);
}

//==================================================================================
// --- ムーブコンストラクタ ---
//==================================================================================
CFileStream::CFileStream(CFileStream &&other) noexcept
{ // メンバ変数をムーブ
	this->m_bBinary = std::move(other.m_bBinary);
	this->m_bInStream = std::move(other.m_bInStream);
	this->m_filename = std::move(other.m_filename);
	this->m_size = std::move(other.m_size);
	this->m_Infile = std::move(other.m_Infile);
	this->m_Outfile = std::move(other.m_Outfile);
}

//==================================================================================
// --- デストラクタ ---
//==================================================================================
CFileStream::~CFileStream()
{ // ファイルクローズ
	Close();
}

//==================================================================================
// --- ファイルオープン処理 ---
//==================================================================================
bool CFileStream::Open(std::string_view sFilename, const bool bBinary)
{ // ファイルを閉じる
	Close();

	// ファイルを入力ストリームに設定
	m_bInStream = true;

	// バイナリフラグによってオープンの仕方を変える
	if (bBinary)
	{ // バイナリファイルのオープン
		m_Infile.open(sFilename, std::ios::binary);
		
		// バイナリフラグを立てる
		m_bBinary = true;
	}
	else
	{ // テキストファイルのオープン
		m_Infile.open(sFilename);

		// バイナリフラグを下す
		m_bBinary = false;
	}

	if (IsOpen() == false)
	{ // ファイルのオープンに失敗
		return false;
	}

	// ファイルのサイズを取得
	ScanSizeInStream();

	// ファイル名を保存
	m_filename = sFilename;

	// 成功
	return true;
}

//==================================================================================
// --- ファイル作成処理 ---
//==================================================================================
bool CFileStream::Create(std::string_view sFilename, const bool bBinary, const FLAG flag)
{
	std::ios::openmode mode = std::ios::out;	// openフラグ

	// ファイルを閉じる
	Close();

	// ファイルのサイズをリセット
	m_size = 0;

	// FLAGによってmodeにフラグを追加
	switch (flag)
	{
	case FLAG_OVERWRITE:		// 上書きの場合
		// 上書きフラグ追加
		mode |= std::ios::trunc;
		break;

	case FLAG_CONTINUE:			// 追記の場合
		if (std::filesystem::exists(sFilename))
		{ // ファイルが開けた場合(ファイルが存在する場合)
			// ファイルサイズを取得
			m_size = std::filesystem::file_size(sFilename);
		}

		// 追記フラグ追加
		mode |= std::ios::app;
		break;

	case FLAG_CANCEL:			// 取り消しの場合
		// 一度ファイルを入力ストリームで開いて、存在するか確認
		if (std::filesystem::exists(sFilename))
		{ // ファイルが開けた場合(ファイルが存在する場合)
			// 処理を中断する
			return false;
		}

		// 上書きフラグ追加
		mode |= std::ios::trunc;
		break;

	default:
		break;
	}

	// ファイルを出力ストリームに設定
	m_bInStream = false;

	// バイナリフラグによって作成の仕方を変える
	if (bBinary)
	{ // バイナリファイルの作成
		// バイナリフラグ追加
		mode |= std::ios::binary;

		// バイナリフラグを立てる
		m_bBinary = true;
	}
	else
	{ // テキストファイルの作成
		// バイナリフラグを下す
		m_bBinary = false;
	}

	// ファイル作成
	m_Outfile.open(sFilename, mode);

	if (IsOpen() == false)
	{ // ファイルの作成に失敗
		return false;
	}

	// ファイル名を保存
	m_filename = sFilename;

	// 成功
	return true;
}

//==================================================================================
// --- ファイルクローズ処理 ---
//==================================================================================
void CFileStream::Close(void)
{ // ファイルのチェック
	if (IsOpen() == true)
	{ // ファイルが開かれていれば
		// ストリームの種類で分岐
		if (m_bInStream == true)
		{ // 入力ストリームの場合、ifStreamのクローズ処理を呼ぶ
			CloseFileInStream();
		}
		else
		{ // 出力ストリームの場合、ofStreamのクローズ処理を呼ぶ
			CloseFileOutStream();
		}

		// ファイルサイズをリセット
		m_size = 0;

		// ファイル名をリセット
		m_filename.clear();
	}
}

//==================================================================================
// --- ファイルのオープン確認処理 ---
//==================================================================================
bool CFileStream::IsOpen(void) const 
{ // ストリームの種類で分岐
	if (m_bInStream == true)
	{ // 入力ストリームの場合、ifStreamのファイル確認処理を呼ぶ
		return IsOpenInStream();
	}
	else
	{ // 出力ストリームの場合、ofStreamのファイル確認処理を呼ぶ
		return IsOpenOutStream();
	}
}

//==================================================================================
// --- ファイル状態の確認処理 ---
//==================================================================================
bool CFileStream::Check(void) const
{ // ストリームの種類で分岐
	if (m_bInStream == true)
	{ // 入力ストリームの場合、ifStreamのファイル状態確認処理を呼ぶ
		return CheckFileInStream();
	}
	else
	{ // 出力ストリームの場合、ofStreamのファイル状態確認処理を呼ぶ
		return CheckFileOutStream();
	}
}

//==================================================================================
// --- EOFの確認処理 ---
//==================================================================================
bool CFileStream::IsEoF(void) const
{ // ファイルのオープン確認
	if (IsOpen() == false)
	{ // ファイルが開かれていなければ、EOF扱い
		return true;
	}

	// ファイルのEOFを返す
	return m_Infile.eof();
}

//==================================================================================
// --- 自作読み込み処理 ---
//==================================================================================
bool CFileStream::ReadByFunc(std::function<void(std::ifstream&)> readFunc)
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

	if (readFunc)
	{ // 呼び出し可能なら、引数の関数を呼び出し
		readFunc(m_Infile);
		return true;
	}
	else
	{ // 呼び出し不能なら、失敗
		return false;
	}
}

//==================================================================================
// --- 1行読み込み処理 ---
//==================================================================================
bool CFileStream::ReadString(std::string &string)
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

	// 文字列をクリア
	string.clear();

	// 1行読み取り
	std::getline(m_Infile, string);

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
// --- 文字列から文字列を検索する処理 ---
// 
// pOut		: 検索した文字列の最初のインデックス
// bSkip	: 検索した文字列の一番最後のインデックスを返すフラグ
//==================================================================================
bool CFileStream::FindString(std::string_view source,
	std::string_view s,
	size_t *pOut,
	const bool bSkip)
{ // 文字列検索
	size_t pos = source.find(s);		// 見つかったインデックス

	// nullじゃなければ、見つかったインデックスを返す
	if (pOut) *pOut = (bSkip) ? pos + s.length() : pos;

	// 文字列が見つかったかの判定を返す
	return pos != std::string::npos;
}

//==================================================================================
// --- 自作読み込み処理 ---
//==================================================================================
bool CFileStream::WriteByFunc(std::function<void(std::ofstream &)> writeFunc)
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

	if (writeFunc)
	{ // 呼び出し可能なら、引数の関数を呼び出し
		writeFunc(m_Outfile);
		return true;
	}
	else
	{ // 呼び出し不能なら、失敗
		return false;
	}
}

//==================================================================================
// --- 1行書き出し処理 ---
//==================================================================================
bool CFileStream::WriteString(const std::string_view &string)
{
	// ファイルが開かれているか確認
	if (IsOpen() == false)
	{ // 開かれていない場合、失敗
		return false;
	}

	// 出力ストリームかどうか確認
	if (IsInStream() == true)
	{ // 入力ストリームだった場合、失敗
		return false;
	}

	if (m_bBinary == true)
	{ // バイナリファイルの場合
		// バイナリデータとして書き出し
		m_Outfile.write(string.data(), string.size());
	}
	else
	{ // テキストファイルの場合
		// 一行書き出し
		m_Outfile << string;
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
	m_size += string.size();

	return true;
}

//==================================================================================
// --- ファイルクローズ処理 (ifStream) ---
//==================================================================================
void CFileStream::CloseFileInStream(void)
{ // ファイルをクローズ
	m_Infile.close();
}

//==================================================================================
// --- ファイルクローズ処理 (ofStream) ---
//==================================================================================
void CFileStream::CloseFileOutStream(void)
{ // ファイルをクローズ
	m_Outfile.close();
}

//==================================================================================
// --- ファイルの確認処理 (ifStream) ---
//==================================================================================
bool CFileStream::IsOpenInStream(void) const
{ // ifStreamのファイル確認
	return m_Infile.is_open();
}

//==================================================================================
// --- ファイルの確認処理 (ofStream) ---
//==================================================================================
bool CFileStream::IsOpenOutStream(void) const
{ // ofStreamのファイル確認
	return m_Outfile.is_open();
}

//==================================================================================
// --- ファイル状態の確認処理 (ifStream) ---
//==================================================================================
bool CFileStream::CheckFileInStream(void) const
{
	if (IsOpenInStream() == false)
	{ // ファイルが開かれていなければ、失敗
		return false;
	}

	// ファイル読み込み状況のチェック
	if (m_Infile.bad() || m_Infile.fail())
	{ // ファイル読み込み時にエラーが発生した場合
		return false;
	}
	else
	{ // 問題ないなら成功
		return true;
	}
}

//==================================================================================
// --- ファイル状態の確認処理 (ofStream) ---
//==================================================================================
bool CFileStream::CheckFileOutStream(void) const
{
	if (IsOpenOutStream() == false)
	{ // ファイルが開かれていなければ、失敗
		return false;
	}

	// ファイル書き込み状況のチェック
	if (m_Outfile.bad() || m_Outfile.fail())
	{ // ファイル書き込み時にエラーが発生した場合
		return false;
	}
	else
	{ // 問題ないなら成功
		return true;
	}
}

//==================================================================================
// --- 読み込み時ファイルサイズ取得処理 ---
//==================================================================================
void CFileStream::ScanSizeInStream(void)
{ // ファイルのオープン確認
	if (IsOpen() == false)
	{ // ファイルが開かれていなければ、サイズをリセット
		m_size = 0;
	}

	// ファイルの現在位置を取得
	auto filePos = m_Infile.tellg();

	// ファイルの位置を終端までずらす
	m_Infile.seekg(0, std::ios::end);

	// ファイルのサイズを求める
	m_size = static_cast<size_t>(m_Infile.tellg());

	// ファイルの位置を戻す
	m_Infile.seekg(filePos);
}

//==================================================================================
// --- 演算子のオーバーロード (ムーブ) ---
//==================================================================================
CFileStream &CFileStream::operator=(CFileStream &&other) noexcept
{
	if (this == &other) return *this;

	// メンバ変数をムーブ
	this->m_bBinary = std::move(other.m_bBinary);
	this->m_bInStream = std::move(other.m_bInStream);
	this->m_filename = std::move(other.m_filename);
	this->m_size = std::move(other.m_size);
	this->m_Infile = std::move(other.m_Infile);
	this->m_Outfile = std::move(other.m_Outfile);

	return *this;
}

//==================================================================================
// --- 演算子のオーバーロード (bool値への変換) ---
//==================================================================================
CFileStream::operator bool(void) const
{
	return IsOpen() && IsEoF();
}