/// @file paw_parse.cpp
///
/// @author Sasaji
/// @date   2019.08.01
///


#include "paw_parse.h"
#include <stdio.h>


namespace PARSEWAV
{

//

MileStone::MileStone()
{
	Clear();
}
MileStone::MileStone(int spos_)
{
	Clear();
	SPos(spos_);
}
void MileStone::Clear()
{
	m_spos = 0;
	m_data_all = 0;
	Baud(-1);
	SDataPos(-1);
}
int8_t MileStone::Baud() const
{
	return static_cast<int8_t>(m_baud > 7 ? m_baud | 0xf8 : m_baud);
}
void MileStone::Baud(int8_t val)
{
	m_baud = (val < 0 ? val | 0x8 : val & 0x7);
}

//

MileStoneList::MileStoneList()
	: std::vector<MileStone>()
{
	Clear(0);
}

void MileStoneList::Clear(int boundary_)
{
	clear();
	SetBoundary(boundary_);
	SetNextSPos(boundary_);
	SetPrevSPos(boundary_);
}
/// 境界に達していたらマークを追加する
bool MileStoneList::MarkIfNeed(int spos_)
{
	if (spos_ >= m_next_spos) {
		push_back(MileStone(spos_));
		m_prev_spos = m_next_spos;
		m_next_spos += m_boundary;
		return true;
	}
	return false;
}
/// 境界内のマークを変更する
bool MileStoneList::ModifyMarkIfNeed(int spos_, int8_t baud_, uint8_t c_phase_, uint8_t c_frip_, uint8_t sn_sta_, int8_t s_data_pos_)
{
	if (size() > 0 && spos_ > (m_prev_spos - (m_boundary / 8)) && spos_ <= m_prev_spos) {
		MileStone *ms = &at(size()-1);
		ms->SPos(spos_);
		ms->Baud(baud_);
		ms->CPhase(c_phase_);
		ms->CFrip(c_frip_);
		ms->SnSta(sn_sta_);
		ms->SDataPos(s_data_pos_);
		return true;
	}
	return false;
}
const MileStone &MileStoneList::GetCurrent() const
{
	if (size() > 0) {
		return at(size()-1);
	} else {
		return m_dummy;
	}
}
int MileStoneList::GetCurrentSPos() const
{
	return GetCurrent().SPos();
}
void MileStoneList::Unshift(int cnt)
{
	int nsize = (int)size() - cnt;
	for(int n=0; n < cnt; n++) {
		pop_back();
		m_next_spos = m_prev_spos;
		m_prev_spos -= m_boundary;
		nsize--;
		if (nsize <= 0) break;
	}
}
#if 0
const MileStone &MileStoneList::Unshift()
{
	Unshift(2);
	return at(size()-1);
}
#endif
void MileStoneList::UnshiftBySPos(int spos)
{
	int n = (int)size() - 1;
	for(; n >= 0; n--) {
		if (at(n).SPos() <= spos) {
			break;
		}
		pop_back();
		m_next_spos = m_prev_spos;
		m_prev_spos -= m_boundary;
	}
}

//

ParserBase::ParserBase()
{
	process_mode = PROCESS_IDLE;
	param = NULL;
	tmp_param = NULL;
	infile = NULL;
	mile_stone = NULL;
}

void ParserBase::Init(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	process_mode = process_mode_;
	tmp_param = &tmp_param_;
	mile_stone = &mile_stone_;
}

void ParserBase::SetParameter(Parameter &param_)
{
	param = &param_;
}

void ParserBase::SetInputFile(InputFile &infile_)
{
	infile = &infile_;
}

}; /* namespace PARSEWAV */
