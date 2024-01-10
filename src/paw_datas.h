/// @file paw_datas.h
///

#ifndef _PARSEWAV_DATAS_H_
#define _PARSEWAV_DATAS_H_

#include "common.h"
#include <vector>


namespace PARSEWAV
{

#ifndef DATA_ARRAY_SIZE
#define DATA_ARRAY_SIZE	131072
#endif

#define USE_SAMPLEDATA_SPOS
//#define USE_SAMPLEARRAY_POINTER

/// サンプルデータ保持用クラス
class CSampleData
{
protected:
#ifdef USE_SAMPLEDATA_SPOS
	int      m_spos;	///< サンプル位置
#endif
	union {
		struct {
			uint8_t m_data;			///< サンプルデータ
			uint8_t m_baud: 4;		///< ボーレート
			uint8_t m_err:  4;		///< エラーフラグ
			uint8_t m_c_phase: 4;	///< C解析フェーズ
			uint8_t m_c_frip: 4;	///< C解析フリップ
			uint8_t m_sn_sta: 4;	///< snスタート境界
			uint8_t m_user: 4;		///< その他データ
		};
		uint32_t m_data_all;
	};
public:
	CSampleData();
	CSampleData(const CSampleData &src);
	CSampleData(uint8_t data, int spos, int8_t baud, uint8_t err, uint8_t c_phase, uint8_t c_frip, uint8_t sn_sta, uint8_t user);
	CSampleData &operator=(const CSampleData &src);
	void Clear();
	uint8_t  Data() const;
	int      SPos() const;
	uint32_t DataAll() const;
	int8_t   Baud() const;
	uint8_t  Err() const;
	uint8_t  CPhase() const;
	uint8_t  CFrip() const;
	uint8_t  SnSta() const;
	uint8_t  User() const;
	void Set(const CSampleData &src);
	void Data(uint8_t val);
	void SPos(int val);
	void DataAll(uint32_t val);
	void Baud(int8_t val);
	void Err(uint8_t val);
	void CPhase(uint8_t val);
	void CFrip(uint8_t val);
	void SnSta(uint8_t val);
	void User(uint8_t val);
};

/// サンプルデータ配列クラス
class CSampleArray
{
protected:
	int     m_size;			///< 配列サイズ(要素数)

#ifdef USE_SAMPLEARRAY_POINTER
	CSampleData *m_datas;
#else
	CSampleData  m_datas[DATA_ARRAY_SIZE];
#endif
	CSampleData  m_dummy;

	double  m_rate;

	int     m_w_pos;		///< 書き込んだ位置
	int     m_r_pos;		///< 読み込んだ位置
	int     m_total_w_pos;	///< 書き込んだ位置の合計
	int     m_total_r_pos;	///< 読み込んだ位置の合計
	int     m_start_pos;	///< 書き込み開始位置（ファイル出力時に使用）
	bool    m_last_data;	///< 最後のデータ

	CSampleArray(const CSampleArray &src);

public:
	CSampleArray(int init_size = DATA_ARRAY_SIZE);
	~CSampleArray();

	void Init();
	void Clear();
	const CSampleData &At(int pos = 0) const;
	const CSampleData &GetRead(int offset = 0) const;
	const CSampleData &GetWrite(int offset = 0) const;

	int Length() const;
	int FreeSize() const;

	CSampleData *GetPtr(int pos = 0);
	CSampleData *GetStartPtr(int offset = 0);
	CSampleData *GetReadPtr(int offset = 0);
	CSampleData *GetWritePtr(int offset = 0);

	void Add(const CSampleData &val);
	void Add(uint8_t data, int spos, int8_t baud = -1, uint8_t err = 0, uint8_t c_phase = 0, uint8_t c_frip = 0, uint8_t sn_sta = 0, uint8_t user = 0);
	int AddString(const uint8_t *str, int len, int spos, int8_t baud = -1, uint8_t err = 0, uint8_t c_phase = 0, uint8_t c_frip = 0, uint8_t sn_sta = 0, uint8_t user = 0);
	int Repeat(uint8_t data, int len, int spos, int8_t baud = -1, uint8_t err = 0, uint8_t c_phase = 0, uint8_t c_frip = 0, uint8_t sn_sta = 0, uint8_t user = 0);
	bool IsFull(int offset = 0) const;
	bool IsTail(int offset = 0) const;
	int RemainLength();

	bool IsLastData() const;
	void LastData(bool val);

	double GetRate() const;
	int GetSize() const;
	int GetStartPos() const;
	int GetReadPos() const;
	int GetWritePos() const;
	int GetTotalReadPos() const;
//	int GetTotalWritePos() const;
	//
	void SetRate(double val);
	void SetStartPos(int pos);
	//
	int AddReadPos(int num);
	int AddWritePos(int num);
	int IncreaseReadPos();
	int IncreaseWritePos();
	//
	void SkipReadPos();
	void SkipToTail();
	void Shift();
	void Shift(int offset);
	void Revert();

	int Compare(int offset, const CSampleArray &dst, int dst_offset, int len);
	int Compare(int offset, const uint8_t *dat, int len);
	int CompareRead(int offset, const uint8_t *dat, int len);
	int Find(int offset, const uint8_t *ptn, int len);
	int FindRead(int offset, const uint8_t *ptn, int len);
	bool SameAsRead(int offset, int len, const CSampleData &dat);

	int FindSPos(int offset, int spos);
	int FindRevSPos(int offset, int spos);

};

/// サンプルデータ配列を文字列にする
class CSampleString
{
private:
	char *m_str;
	int   m_len;

	CSampleString();
	CSampleString(const CSampleString &src);

public:
	CSampleString(const CSampleArray &data, int offset, int len);
	~CSampleString();
	const char *Get() const;
	int Length() const;
};

/// サンプルデータ配列をバイト列にする
class CSampleBytes
{
private:
	uint8_t *m_str;
	int      m_len;

	CSampleBytes();
	CSampleBytes(const CSampleBytes &src);

public:
	CSampleBytes(const CSampleArray &data, int offset, int len);
	~CSampleBytes();
	const uint8_t *Get() const;
	int Length() const;
};

/// サンプルデータリスト
class CSampleList : public std::vector<CSampleData>
{
private:
	CSampleList(const CSampleList &src);

public:
	CSampleList();
	CSampleList(size_t size);
};

//

/// 波形解析用バッファ
class WaveData : public CSampleArray
{
private:
	WaveData(const WaveData &src);

public:
	WaveData(int init_size = DATA_ARRAY_SIZE);
};

//

/// 搬送波バッファ
class CarrierData : public CSampleArray
{
private:
	CarrierData(const CarrierData &src);

public:
	CarrierData(int init_size = DATA_ARRAY_SIZE);
};

//

/// シリアルデータバッファ
class SerialData : public CSampleArray
{
private:
	SerialData(const SerialData &src);

public:
	SerialData(int init_size = DATA_ARRAY_SIZE);
};

//

/// バイナリデータバッファ
class BinaryData : public CSampleArray
{
private:
	BinaryData(const BinaryData &src);

public:
	BinaryData(int init_size = DATA_ARRAY_SIZE);
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_DATAS_H_ */
