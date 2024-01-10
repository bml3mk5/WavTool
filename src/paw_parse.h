/// @file paw_parse.h
///
/// @author Sasaji
/// @date   2019.08.01
///


#ifndef _PARSEWAV_PARSE_H_
#define _PARSEWAV_PARSE_H_

#include "common.h"
#include "paw_defs.h"
#include "paw_param.h"
#include "paw_file.h"


namespace PARSEWAV 
{

/// 一定間隔で位置を覚えておく
class MileStone
{
private:
	int m_spos;
	union {
		struct {
			uint8_t m_baud: 4;
			uint8_t m_unused: 4;
			uint8_t m_c_phase: 4;
			uint8_t m_c_frip: 4;
			uint8_t m_sn_sta;
			int8_t m_s_data_pos;
		};
		uint32_t m_data_all;
	};
public:
	MileStone();
	MileStone(int spos_);
	void Clear();
	int SPos() const { return m_spos; }
	int8_t Baud() const;
	uint8_t CPhase() const { return m_c_phase; }
	uint8_t CFrip() const { return m_c_frip; }
	uint8_t SnSta() const { return m_sn_sta; }
	int8_t SDataPos() const { return m_s_data_pos; }
	void SPos(int val) { m_spos = val; }
	void Baud(int8_t val);
	void CPhase(uint8_t val) { m_c_phase = val; }
	void CFrip(uint8_t val) { m_c_frip = val; }
	void SnSta(uint8_t val) { m_sn_sta = val; }
	void SDataPos(int8_t val) { m_s_data_pos = val; }
};

/// 一定間隔で位置を覚えておく
class MileStoneList : public std::vector<MileStone>
{
public:
	int m_boundary;
	int m_next_spos;
	int m_prev_spos;

	MileStone m_dummy;
public:
	MileStoneList();
	void Clear(int boundary_);
	bool MarkIfNeed(int spos_);
	bool ModifyMarkIfNeed(int spos_, int8_t baud_ = -1, uint8_t c_phase_ = 0, uint8_t c_frip_ = 0, uint8_t sn_sta_ = 0, int8_t s_data_pos_ = -1);
	const MileStone &GetCurrent() const;
	int GetCurrentSPos() const;
	void Unshift(int cnt);
//	const MileStone &Unshift();
	void UnshiftBySPos(int spos);
	void SetBoundary(int boundary_) { m_boundary = boundary_; }
	void SetNextSPos(int next_spos_) { m_next_spos = next_spos_; }
	void SetPrevSPos(int prev_spos_) { m_prev_spos = prev_spos_; }
	int GetNextSPos() const { return m_next_spos; }
	int GetPrevSPos() const { return m_prev_spos; }
};

/// データ解析用基底クラス
class ParserBase
{
protected:
	/// 処理モード
	enum_process_mode process_mode;
	/// ダイアログパラメータ
	Parameter *param;
	TempParameter *tmp_param;
	/// 入力ファイル
	InputFile *infile;
	/// 一定間隔で位置を覚えておく
	MileStoneList *mile_stone;

public:
	ParserBase();
	void Init(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_);

	void SetParameter(Parameter &param_);
	void SetInputFile(InputFile &infile_);

};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARSE_H_ */
