/// @file paw_file.h
///
/// @brief ファイル入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2019.08.01
///
#ifndef _PARSEWAV_FILE_H_
#define _PARSEWAV_FILE_H_

#include "common.h"
#include <stdio.h>
#include "paw_defs.h"
#include <wx/string.h>
#if defined(_WIN32)
#include <tchar.h>
#else
#include "tchar.h"
#endif
#include "paw_datas.h"


namespace PARSEWAV 
{

/// ファイルラッパ
class File
{
protected:
	FILE *fio;
	enum_file_type type;

	wxString name;

	int opened_file_count;

public:
	File();
	~File();

	enum enum_open_mode {
		READ_ASCII = 0,
		READ_BINARY,
		WRITE_ASCII,
		WRITE_BINARY,
	};

	bool Fopen(const wxString &file_name, enum_open_mode mode);
	void Fclose();

	int Fgetc();
	size_t Fread(void *buf, size_t buf_siz, size_t cnt);
	size_t Fwrite(const void *buf, size_t buf_siz, size_t cnt);
	int Fprintf(const char *format, ...);
	int Vfprintf(const char *format, va_list ap);

	int Fseek(long offset, int origin);

	int Fputc(int c);
	int Fputs(const wxString &str);
	int Fputts(const _TCHAR *str);

	long Ftell();
	int GetSize();

	bool IsOpened() { return (fio != NULL); }
	int  OpenedFileCount();

	FILE *Fio() { return fio; }
	enum_file_type GetType() { return type; }
	const wxString &GetName() const { return name; }
	void SetType(enum_file_type val) { type = val; }
	void SetName(const wxString &str) { name = str; }
};

/// サンプル位置保持用
class SamplePosition
{
protected:
	int      m_sample_pos;
	int      m_sample_num;
//	uint32_t m_sample_usec;

	int      m_sample_num_stocked;

	double   m_sample_rate;	///< サンプルレート

public:
	SamplePosition();
	void ClearSample();

	int      AddSamplePos(int offset);
	int      IncreaseSamplePos();
	int      DecreaseSamplePos();

	uint32_t CalcrateSampleUSec();
	uint32_t CalcrateSampleUSec(int pos);
	static uint32_t CalcrateSampleUSec(int pos, double rate);
	double CalcrateSamplePos(uint32_t usec);
	static double CalcrateSamplePos(uint32_t usec, double rate);

	void RestoreSampleNum(int offset = 0);

	bool     IsFirstPos(int offset = 0) const;
	bool     IsEndPos(int offset = 0) const;

	int      SamplePos() const { return m_sample_pos; }
	int      SampleNum() const { return m_sample_num; }
//	uint32_t SampleUSec() const { return m_sample_usec; }
	double   SampleRate() const { return m_sample_rate; }
	void SamplePos(int val);
	void SampleNum(int val);
//	void SampleUSec(uint32_t val);
	void SampleRate(double rate);
};

/// 入力ファイルクラス
class InputFile : public File, public SamplePosition
{
public:
	InputFile();

	void First();
};

/// 出力ファイルクラス
class OutputFile : public File
{
public:
	OutputFile();

	int WriteData(CSampleArray &data);
};

/// ログファイルクラス
class LogFile : public File
{
private:
	wxString *m_logbuf;

public:
	LogFile();

	bool Open(const wxString &file_name);
	void Close();

	void Write(const wxString &buff, int crlf);

	void SetLogBuf(wxString *logbuf) { m_logbuf = logbuf; }

};

/// ログファイル
extern LogFile gLogFile;

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_FILE_H_ */
