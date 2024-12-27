/// @file paw_datas.cpp
///

#include "paw_datas.h"
#include <string.h>


namespace PARSEWAV 
{

//

CSampleData::CSampleData()
{
	Clear();
}
CSampleData::CSampleData(const CSampleData &src)
{
	Set(src);
}
CSampleData::CSampleData(uint8_t data, int spos, int8_t baud, uint8_t err, uint8_t c_phase, uint8_t c_frip, uint8_t sn_sta, uint8_t user)
{
	Data(data);
	SPos(spos);
	Baud(baud);
	Err(err);
	CPhase(c_phase);
	CFrip(c_frip);
	SnSta(sn_sta);
	User(user);
}
CSampleData &CSampleData::operator=(const CSampleData &src)
{
	Set(src);
	return *this;
}
void CSampleData::Clear()
{
	m_spos = 0;
	m_data_all = 0;
	Baud(-1);
}
uint8_t CSampleData::Data() const
{
	return m_data;
}
int CSampleData::SPos() const
{
	return m_spos;
}
uint32_t CSampleData::DataAll() const
{
	return m_data_all;
}
int8_t CSampleData::Baud() const
{
	return static_cast<int8_t>(m_baud > 7 ? m_baud | 0xf8 : m_baud);
}
uint8_t CSampleData::Err() const
{
	return m_err;
}
uint8_t CSampleData::CPhase() const
{
	return m_c_phase;
}
uint8_t CSampleData::CFrip() const
{
	return m_c_frip;
}
uint8_t CSampleData::SnSta() const
{
	return m_sn_sta;
}
uint8_t CSampleData::User() const
{
	return m_user;
}
void CSampleData::Set(const CSampleData &src)
{
	SPos(src.SPos());
	DataAll(src.DataAll());
}
void CSampleData::Data(uint8_t val)
{
	m_data = val;
}
void CSampleData::SPos(int val)
{
	m_spos = val;
}
void CSampleData::DataAll(uint32_t val)
{
	m_data_all = val;
}
/// @brief ボーレート(4ビット)
void CSampleData::Baud(int8_t val)
{
	m_baud = (val < 0 ? val | 0x8 : val & 0x7);
}
/// @brief エラー情報(4ビット)
void CSampleData::Err(uint8_t val)
{
	m_err = val;
}
/// @brief Cデータフェーズ(4ビット)
void CSampleData::CPhase(uint8_t val)
{
	m_c_phase = val;
}
/// @brief Cデータフリップ(4ビット)
void CSampleData::CFrip(uint8_t val)
{
	m_c_frip = val;
}
/// @brief Sデータスタート位置(4ビット)
void CSampleData::SnSta(uint8_t val)
{
	m_sn_sta = val;
}
/// @brief ユーザ依存(4ビット)
void CSampleData::User(uint8_t val)
{
	m_user = val;
}

//

CSampleArray::CSampleArray(int init_size)
{
#ifdef USE_SAMPLEARRAY_POINTER
	m_datas = NULL;
	m_size = 0;
	Init();

	if (init_size > 0) {
		m_size = init_size;
		m_datas = new CSampleData[init_size];
	}
#else
	Init();
	m_size = DATA_ARRAY_SIZE;
#endif
}

CSampleArray::CSampleArray(const CSampleArray &src)
{
#ifdef USE_SAMPLEARRAY_POINTER
	m_datas = NULL;
#endif
	m_size = 0;
	Init();
}

CSampleArray::~CSampleArray()
{
#ifdef USE_SAMPLEARRAY_POINTER
	delete [] m_datas;
#endif
}

/// @brief 初期化
void CSampleArray::Init()
{
	Clear();
	m_rate = 0.0;
	m_total_w_pos = 0;
	m_total_r_pos = 0;
	m_last_data = false;
}

/// @brief クリア
void CSampleArray::Clear()
{
//	for(int i=0; i<m_size; i++) {
//		m_datas[i].Clear();
//	}
	m_w_pos = 0;
	m_r_pos = 0;
	m_start_pos = 0;
}

/// @brief 指定位置のデータ
/// @param[in] pos 位置
const CSampleData &CSampleArray::At(int pos) const
{
#ifdef USE_SAMPLEARRAY_POINTER
	if (!m_datas) return;
#endif
	if (0 <= pos && pos < m_size) {
		return m_datas[pos];
	} else {
		return m_dummy;
	}
}

/// @brief リード位置のデータ
/// @param[in] offset オフセット
const CSampleData &CSampleArray::GetRead(int offset) const
{
	return At(m_r_pos + offset);
}

/// @brief ライト位置のデータ
/// @param[in] offset オフセット
const CSampleData &CSampleArray::GetWrite(int offset) const
{
	return At(m_w_pos + offset);
}

/// @brief スタート位置からの長さ
int CSampleArray::Length() const
{
	return m_w_pos - m_start_pos;
}

/// @brief 書き込み可能な残りフリーサイズ
int CSampleArray::FreeSize() const
{
	return m_size - m_w_pos;
}

/// @brief 指定位置のデータポインタを返す
CSampleData *CSampleArray::GetPtr(int pos)
{
#ifdef USE_SAMPLEARRAY_POINTER
	if (!m_datas) return;
#endif
	if (0 <= pos && pos < m_size) {
		return &m_datas[pos];
	} else {
		return NULL;
	}
}

/// @brief スタート位置のデータポインタを返す
/// @param[in] offset オフセット
CSampleData *CSampleArray::GetStartPtr(int offset)
{
	return GetPtr(m_start_pos + offset);
}

/// @brief リード位置のデータポインタを返す
/// @param[in] offset オフセット
CSampleData *CSampleArray::GetReadPtr(int offset)
{
	return GetPtr(m_r_pos + offset);
}

/// @brief ライト位置のデータポインタを返す
/// @param[in] offset オフセット
CSampleData *CSampleArray::GetWritePtr(int offset)
{
	return GetPtr(m_w_pos + offset);
}

/// @brief 追加してライト位置を＋１
/// @param[in] val 追加するデータ
void CSampleArray::Add(const CSampleData &val)
{
#ifdef USE_SAMPLEARRAY_POINTER
	if (!m_datas) return;
#endif
	if (m_w_pos < m_size) {
		m_datas[m_w_pos] = val;
		m_w_pos++;
		m_total_w_pos++;
	}
}

/// @brief 追加してライト位置を＋１
/// @param[in] data    追加するデータ
/// @param[in] spos    サンプリング位置
/// @param[in] baud    ボーレート(4ビット)
/// @param[in] err     エラー情報(4ビット)
/// @param[in] c_phase Cフェーズ(4ビット)
/// @param[in] c_frip  Cフリップ(4ビット)
/// @param[in] sn_sta  SNスタート(4ビット)
/// @param[in] user    ユーザ(4ビット)
void CSampleArray::Add(uint8_t data, int spos, int8_t baud, uint8_t err, uint8_t c_phase, uint8_t c_frip, uint8_t sn_sta, uint8_t user)
{
	Add(CSampleData(data, spos, baud, err, c_phase ,c_frip, sn_sta, user));
}

/// @brief 文字列を追加
/// @param[in] str     追加するデータ列
/// @param[in] len     データ列長さ
/// @param[in] spos    サンプリング位置
/// @param[in] baud    ボーレート(4ビット)
/// @param[in] err     エラー情報(4ビット)
/// @param[in] c_phase Cフェーズ(4ビット)
/// @param[in] c_frip  Cフリップ(4ビット)
/// @param[in] sn_sta  SNスタート(4ビット)
/// @param[in] user    ユーザ(4ビット)
/// @return 追加した文字数
int CSampleArray::AddString(const uint8_t *str, int len, int spos, int8_t baud, uint8_t err, uint8_t c_phase, uint8_t c_frip, uint8_t sn_sta, uint8_t user)
{
	int n = 0;
	while(*str != 0 && n < len && m_w_pos < m_size) {
		Add(*str, spos, baud, err, c_phase ,c_frip, sn_sta, user);
		str++;
		n++;
	}
	return n;
}

/// @brief 繰り返しセット
/// @param[in] data    追加するデータ
/// @param[in] len     追加を繰り返す回数
/// @param[in] spos    サンプリング位置
/// @param[in] baud    ボーレート(4ビット)
/// @param[in] err     エラー情報(4ビット)
/// @param[in] c_phase Cフェーズ(4ビット)
/// @param[in] c_frip  Cフリップ(4ビット)
/// @param[in] sn_sta  SNスタート(4ビット)
/// @param[in] user    ユーザ(4ビット)
/// @return セットした文字数
int CSampleArray::Repeat(uint8_t data, int len, int spos, int8_t baud, uint8_t err, uint8_t c_phase, uint8_t c_frip, uint8_t sn_sta, uint8_t user)
{
	int n = 0;
	while(n < len && m_w_pos < m_size) {
		Add(data, spos, baud, err, c_phase ,c_frip, sn_sta, user);
		n++;
	}
	return n;
}

/// @brief バッファが書き込めないか
/// @param[in] offset オフセット
bool CSampleArray::IsFull(int offset) const
{
	return (m_w_pos + offset >= m_size);
}

/// @brief 末尾まで読み込んでいるか
/// @param[in] offset オフセット
bool CSampleArray::IsTail(int offset) const
{
	return (m_r_pos + offset >= m_w_pos);
}

/// @brief 残りのデータサイズ
int CSampleArray::RemainLength()
{
	return m_w_pos - m_r_pos;
}

/// @brief 最後のデータか
bool CSampleArray::IsLastData() const
{
	return m_last_data;
}

/// @brief 最後のデータかをセット
void CSampleArray::LastData(bool val)
{
	m_last_data = val;
}

/// @brief サンプリングレート
double CSampleArray::GetRate() const
{
	return m_rate;
}

/// @brief バッファサイズ(個数)
int CSampleArray::GetSize() const
{
	return m_size;
}

/// @brief スタート位置
int CSampleArray::GetStartPos() const
{
	return m_start_pos;
}

/// @brief リード位置
int CSampleArray::GetReadPos() const
{
	return m_r_pos;
}

/// @brief ライト位置
int CSampleArray::GetWritePos() const
{
	return m_w_pos;
}

/// @brief トータルリード位置
int CSampleArray::GetTotalReadPos() const
{
	return m_total_r_pos;
}
#if 0
/// @brief トータルライト位置
int CSampleArray::GetTotalWritePos() const
{
	return m_total_w_pos;
}
#endif

/// @brief サンプリングレートをセット
void CSampleArray::SetRate(double val)
{
	m_rate = val;
}

/// @brief スタート位置をセット
void CSampleArray::SetStartPos(int pos)
{
	m_start_pos = pos;
}

/// @brief リード位置をセット
int CSampleArray::AddReadPos(int num)
{
	m_r_pos += num;
	m_total_r_pos += num;
	return m_r_pos;
}

/// @brief ライト位置をセット
int CSampleArray::AddWritePos(int num)
{
	m_w_pos += num;
	m_total_w_pos += num;
	return m_w_pos;
}

/// @brief リード位置を＋１
int CSampleArray::IncreaseReadPos()
{
	m_r_pos++;
	m_total_r_pos++;
	return m_r_pos;
}

/// @brief ライト位置を＋１
int CSampleArray::IncreaseWritePos()
{
	m_w_pos++;
	m_total_w_pos++;
	return m_w_pos;
}

/// @brief リード位置をライト位置まで進める
void CSampleArray::SkipReadPos()
{
	m_total_r_pos += m_w_pos - m_r_pos;
	m_r_pos = m_w_pos;
}

/// @brief リード位置を末尾まで進める
void CSampleArray::SkipToTail()
{
	m_total_r_pos += m_w_pos;
}

/// @brief 未処理のデータをバッファの先頭にシフトする。
void CSampleArray::Shift()
{
	Shift(m_r_pos);
}

/// @brief バッファの先頭にシフトする。
void CSampleArray::Shift(int offset)
{
	if (offset <= 0) return;
	for(int i=0; i<(m_w_pos - offset); i++) {
		m_datas[i] = m_datas[i + offset];
	}
	m_w_pos -= offset;
	if (m_w_pos < 0) m_w_pos = 0;
//	for(int i=m_w_pos; i<m_size; i++) {
//		m_datas[i].Clear();
//	}
	m_r_pos -= offset;
	if (m_r_pos < 0) m_r_pos = 0;

	m_start_pos -= offset;
	if (m_start_pos < 0) m_start_pos = 0;
}

/// @brief バッファの内容を無効にしてトータル位置を戻す
void CSampleArray::Revert()
{
	m_total_w_pos -= m_w_pos;
	if (m_total_w_pos < 0) m_total_w_pos = 0;
	m_total_r_pos -= m_r_pos;
	if (m_total_r_pos < 0) m_total_r_pos = 0;

	m_w_pos = 0;
	m_r_pos = 0;
}

/// @brief 比較
/// @param[in] offset     比較を開始するオフセット
/// @param[in] dst        対象
/// @param[in] dst_offset 対象側の比較を開始するオフセット
/// @param[in] len        比較するバイト数
/// @return -1: 引数の方が小さい 0:同じ 1:引数の方が大きい
int CSampleArray::Compare(int offset, const CSampleArray &dst, int dst_offset, int len)
{
	int cmp = 0;
	if (offset + len >= m_w_pos) {
		cmp = 1;
	} else if (dst_offset + len >= dst.m_w_pos) {
		cmp = -1;
	} else {
		for(int n = 0; n < len; n++) {
			cmp = ((int)dst.m_datas[dst_offset + n].Data() - (int)m_datas[offset + n].Data());
			if (cmp != 0) break;
		}
	}
	return cmp > 0 ? 1 : cmp < 0 ? -1 : 0;
}

/// @brief 比較
/// @param[in] offset     比較を開始するオフセット
/// @param[in] ptn        対象
/// @param[in] len        比較するバイト数
/// @return -1: 引数の方が小さい 0:同じ 1:引数の方が大きい
int CSampleArray::Compare(int offset, const uint8_t *ptn, int len)
{
	int cmp = 0;
	if (offset + len > m_w_pos) {
		cmp = 1;
	} else {
		for(int n = 0; n < len; n++) {
			cmp = ((int)ptn[n] - (int)m_datas[offset + n].Data());
			if (cmp != 0) break;
		}
	}
	return cmp > 0 ? 1 : cmp < 0 ? -1 : 0;
}
/// @brief リード位置から比較
/// @param[in] offset     比較を開始するオフセット
/// @param[in] ptn        対象
/// @param[in] len        比較するバイト数
/// @return -1: 引数の方が小さい 0:同じ 1:引数の方が大きい
int CSampleArray::CompareRead(int offset, const uint8_t *ptn, int len)
{
	return Compare(m_r_pos + offset, ptn, len);
}

/// @brief パターンに一致するデータ位置をさがす
/// @param[in] offset     比較を開始するオフセット
/// @param[in] ptn        対象
/// @param[in] len        比較するバイト数
/// @return 見つかった場合 (offset)を基準とした位置 <0:なし
int CSampleArray::Find(int offset, const uint8_t *ptn, int len)
{
	int idx = -1;
	int tail = m_w_pos - len;
	for(int pos = offset; pos <= tail; pos++) {
		if (Compare(pos, ptn, len) == 0) {
			idx = pos - offset;
			break;
		}
	}
	return idx;
}
/// @brief リード位置からパターンに一致するデータ位置をさがす
/// @param[in] offset     比較を開始するリード位置からのオフセット
/// @param[in] ptn        対象
/// @param[in] len        比較するバイト数
/// @return 見つかった場合 (ReadPos + offset)を基準とした位置 <0:なし
int CSampleArray::FindRead(int offset, const uint8_t *ptn, int len)
{
	return Find(m_r_pos + offset, ptn, len);
}

/// @brief 値が指定した範囲で同じデータか
bool CSampleArray::SameAsRead(int offset, int len, const CSampleData &dat)
{
	bool same = true;
	for(int i=offset; i<(offset+len); i++) {
		if (m_datas[m_r_pos + i].Data() != dat.Data()) {
			same = false;
			break;
		}
	}
	return same;
}

/// @brief spos位置をさがす
int CSampleArray::FindSPos(int offset, int spos)
{
	int pos = -1;
	for(int i=offset; i<m_w_pos; i++) {
		if (spos <= At(i).SPos()) {
			pos = i;
			break;
		}
	}
	return pos;
}

/// @brief 末尾からspos位置をさがす
int CSampleArray::FindRevSPos(int offset, int spos)
{
	int pos = -1;
	for(int i=(m_w_pos-1); i>=offset; i--) {
		if (spos > At(i).SPos()) {
			pos = i + 1;
			if (pos >= m_w_pos) {
				pos = m_w_pos - 1;
			}
			break;
		}
	}
	if (pos < 0 && m_w_pos > offset + 1) {
		if (spos == At(offset).SPos()) {
			pos = offset;
		}
	}
	return pos;
}

//

CSampleString::CSampleString()
{
}
CSampleString::CSampleString(const CSampleString &src)
{
}
CSampleString::CSampleString(const CSampleArray &data, int offset, int len)
{
	m_str = new char[len + 1];
	memset(m_str, 0, len + 1);
	m_len = len;
	for(int i=0; i<len; i++) {
		m_str[i] = (char)data.At(i + offset).Data();
	}
}
CSampleString::~CSampleString()
{
	delete [] m_str;
}
const char *CSampleString::Get() const
{
	return m_str;
}
int CSampleString::Length() const
{
	return m_len;
}

//

CSampleBytes::CSampleBytes()
{
}
CSampleBytes::CSampleBytes(const CSampleBytes &src)
{
}
CSampleBytes::CSampleBytes(const CSampleArray &data, int offset, int len)
{
	m_str = new uint8_t[len + 1];
	memset(m_str, 0, len + 1);
	m_len = len;
	for(int i=0; i<len; i++) {
		m_str[i] = (uint8_t)data.At(i + offset).Data();
	}
}
CSampleBytes::~CSampleBytes()
{
	delete [] m_str;
}
const uint8_t *CSampleBytes::Get() const
{
	return m_str;
}
int CSampleBytes::Length() const
{
	return m_len;
}

//

CSampleList::CSampleList(const CSampleList &src)
	: std::vector<CSampleData>(src)
{
}
CSampleList::CSampleList()
	: std::vector<CSampleData>()
{
}
CSampleList::CSampleList(size_t size)
	: std::vector<CSampleData>(size)
{
}


//

WaveData::WaveData(const WaveData &src)
	: CSampleArray(src)
{
}

WaveData::WaveData(int init_size)
	: CSampleArray(init_size)
{
}

//

CarrierData::CarrierData(const CarrierData &src)
	: CSampleArray(src)
{
}

CarrierData::CarrierData(int init_size)
	: CSampleArray(init_size)
{
}

//

SerialData::SerialData(const SerialData &src)
	: CSampleArray(src)
{
}

SerialData::SerialData(int init_size)
	: CSampleArray(init_size)
{
}

//

BinaryData::BinaryData(const BinaryData &src)
	: CSampleArray(src)
{
}

BinaryData::BinaryData(int init_size)
	: CSampleArray(init_size)
{
}

}; /* namespace PARSEWAV */
