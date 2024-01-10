/// @file paw_file.cpp
///
/// @author Sasaji
/// @date   2019.08.01
///

#include "paw_file.h"
#include "utils.h"


namespace PARSEWAV 
{

static const _TCHAR *c_open_mode[] = {
	_T("r"),
	_T("rb"),
	_T("w"),
	_T("wb"),
	NULL
};

File::File()
{
	fio = NULL;
	type = FILETYPE_UNKNOWN;

	opened_file_count = 0;
}
File::~File()
{
}
bool File::Fopen(const wxString &file_name, enum_open_mode mode)
{
	Fclose();
	name = file_name;
	fio = UTILS::pw_fopen(file_name, c_open_mode[mode]);
	opened_file_count++;
	opened_file_count &= 0xfffffff;
	return (fio != NULL);
}
void File::Fclose()
{
	if (fio) fclose(fio);
	fio = NULL;
}
int File::Fgetc()
{
	if (!fio) return 0;
	return fgetc(fio);
}
size_t File::Fread(void *buf, size_t buf_siz, size_t cnt)
{
	if (!fio) return 0;
	return fread(buf, buf_siz, cnt, fio);
}
size_t File::Fwrite(const void *buf, size_t buf_siz, size_t cnt)
{
	if (!fio) return 0;
	return fwrite(buf, buf_siz, cnt, fio);
}
int File::Fprintf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int len = Vfprintf(format, ap);
	va_end(ap);
	return len;
}
int File::Vfprintf(const char *format, va_list ap)
{
	if (!fio) return 0;
	return vfprintf(fio, format, ap);
}
int File::Fseek(long offset, int origin)
{
	if (!fio) return 0;
	return fseek(fio, offset, origin);
}
int File::Fputc(int c)
{
	if (!fio) return 0;
	return fputc(c, fio);
}
int File::Fputs(const wxString &str)
{
	if (!fio) return 0;
	return _fputts(str.t_str(), fio);
}
int File::Fputts(const _TCHAR *str)
{
	if (!fio) return 0;
	return _fputts(str, fio);
}
long File::Ftell()
{
	if (!fio) return 0;
	return ftell(fio);
}
int File::GetSize()
{
	if (!fio) return 0;
	fseek(fio, 0, SEEK_END);
	long num = ftell(fio);
	fseek(fio, 0, SEEK_SET);
	return (int)num;
}
/// Fopenをコールした回数を返す
/// @return -1:クローズ時
int File::OpenedFileCount()
{
	return IsOpened() ? opened_file_count : -1;
}

//

SamplePosition::SamplePosition()
{
	ClearSample();
}
void SamplePosition::ClearSample()
{
	m_sample_pos = 0;
	m_sample_num = 0;
//	m_sample_usec = 0;
}
int SamplePosition::AddSamplePos(int offset)
{
	m_sample_pos += offset;
	return m_sample_pos;
}
int SamplePosition::IncreaseSamplePos()
{
	m_sample_pos++;
	return m_sample_pos;
}
int SamplePosition::DecreaseSamplePos()
{
	m_sample_pos--;
	return m_sample_pos;
}

/// 現在位置の時間を計算
uint32_t SamplePosition::CalcrateSampleUSec()
{
	return CalcrateSampleUSec(m_sample_pos, m_sample_rate);
}

/// 現在位置の時間を計算
uint32_t SamplePosition::CalcrateSampleUSec(int pos)
{
	return CalcrateSampleUSec(pos, m_sample_rate);
}

/// 現在位置の時間を計算
uint32_t SamplePosition::CalcrateSampleUSec(int pos, double rate)
{
	return (uint32_t)(1000000.0 * (double)pos / rate);
}

/// 時間から位置を計算
double SamplePosition::CalcrateSamplePos(uint32_t usec)
{
	return CalcrateSamplePos(usec, m_sample_rate);
}

/// 時間から位置を計算
double SamplePosition::CalcrateSamplePos(uint32_t usec, double rate)
{
	return (double)usec * rate / 1000000.0;
}

/// ストックしたサンプル数を戻す
void SamplePosition::RestoreSampleNum(int offset) 
{
	m_sample_num = m_sample_num_stocked + offset;
}

bool SamplePosition::IsFirstPos(int offset) const
{
	return (m_sample_pos < offset);
}
bool SamplePosition::IsEndPos(int offset) const
{
	return ((m_sample_pos + offset) >= m_sample_num);
}

void SamplePosition::SamplePos(int val)
{
	m_sample_pos = val;
}
void SamplePosition::SampleNum(int val)
{
	m_sample_num = val;
	m_sample_num_stocked = val;
}
#if 0
void SamplePosition::SampleUSec(uint32_t val)
{
	m_sample_usec = val;
}
#endif
void SamplePosition::SampleRate(double rate)
{
	m_sample_rate = rate;
}

//

InputFile::InputFile()
	: File(), SamplePosition()
{
}
/// ファイル先頭にセット
void InputFile::First()
{
	Fseek(0, SEEK_SET);
	SamplePos(0);
//	SampleUSec(0);
}

OutputFile::OutputFile()
	: File()
{
}

int OutputFile::WriteData(CSampleArray &data)
{
	int len = 0;
	for(int i=data.GetStartPos(); i<data.GetWritePos(); i++) {
		Fputc(data.At(i).Data());
		len++;
	}
	data.SetStartPos(data.GetWritePos());
	return len;
}

//

LogFile::LogFile()
	: File()
{
}
bool LogFile::Open(const wxString &file_name)
{
	return Fopen(file_name, WRITE_ASCII);
}
void LogFile::Close()
{
	Fclose();
}
void LogFile::Write(const wxString &buff, int crlf)
{
	UTILS::write_log(buff, crlf, fio, m_logbuf);
}

/// ログファイル実体
LogFile gLogFile;

}; /* namespace PARSEWAV */
