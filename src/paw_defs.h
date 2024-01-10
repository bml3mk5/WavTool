/// @file paw_defs.h
///
/// @author Sasaji
/// @date   2019.08.01
///


#ifndef _PARSEWAV_DEFS_H_
#define _PARSEWAV_DEFS_H_

#include "common.h"


namespace PARSEWAV 
{

//#define TERMINATOR_SIZE		4
//#define DATA_BUFFER_SIZE	(65536 + TERMINATOR_SIZE)

#define PARSEWAV_USE_REPORT	1

//#define PARSEWAV_FILL_BUFFER	1

enum enum_process_mode {
	PROCESS_IDLE = 0,
	PROCESS_ANALYZING,
	PROCESS_DECODING,
	PROCESS_ENCODING,
	PROCESS_VIEWING,
};

enum enum_baud_rate_idx {
	IDX_PTN_600 =	0,
	IDX_PTN_1200 =	1,
	IDX_PTN_2400 =	2,
	IDX_PTN_300 =	3,
};

/// ファイル種類
enum enum_file_type {
	FILETYPE_UNKNOWN = -1,
	FILETYPE_WAV = 0,
	FILETYPE_L3C,
	FILETYPE_L3B,
	FILETYPE_T9X,
	FILETYPE_L3,
	FILETYPE_REAL,
	FILETYPE_PLAIN,
	FILETYPE_NO_FILE
};

/// パターン保存
struct st_pattern {
	const uint8_t *ptn;
	int            len;
};

/// 位置保存
typedef struct pos_st {
	int    start_pos;
	int    end_pos;
} pos_t;

/// 波形解析用
class ChkWave
{
public:
	int num;
	int sample_num[2][5];	///< 0:cos 1:sin / 0:long 1:short 2:middle 3:too long 4:too short
//	int sample_odd[2][5];
	int analyze_num;
	int baud_num[2][4];
	int rev_num[2][2];
	double us0avg[2];
	double us1avg[2];
	int amp_max[2];
	int amp_min[2];
	int ser_err[2];	///< serial error count
public:
	ChkWave();
	~ChkWave();
	void Clear();
};

/// マシン語アドレス格納用
typedef struct maddress_st {
	long  start_addr;
	long  exec_addr;
	long  data_size;
	long  file_size;
	bool  include_header;
	bool  valid;
} maddress_t;

/// ボーレート位置変換
extern const int c_baud_min_to_s1[];
/// ボーレート位置変換
extern const int c_baud_s1_to_min[];
/// ボーレート
extern const int c_baud_rate[];

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_DEFS_H_ */
