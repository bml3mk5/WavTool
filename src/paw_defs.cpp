/// @file paw_defs.cpp
///
/// @author Sasaji
/// @date   2019.08.01
///

#include "paw_defs.h"
#include "paw_file.h"


namespace PARSEWAV 
{

/// ボーレート位置変換
const int c_baud_min_to_s1[] = {
	3,	//  300baud
	0,	//  600baud
	1,	// 1200baud
	2	// 2400baud
};
/// ボーレート位置変換
const int c_baud_s1_to_min[] = {
	1,	//  600
	2,	// 1200
	3,	// 2400
	0	//  300
};
/// ボーレート
const int c_baud_rate[] = {
	 600,
	1200,
	2400,
	 300
};

//

ChkWave::ChkWave()
{
	Clear();
}

ChkWave::~ChkWave()
{
}

void ChkWave::Clear()
{
	num = 0;
	analyze_num = 0;
	for(int i=0; i<2; i++) {
		for(int j=0; j<5; j++) {
			sample_num[i][j] = 0;
//			sample_odd[i][j] = 0;
		}
		for(int j=0; j<4; j++) {
			baud_num[i][j] = 0;
		}
		for(int j=0; j<2; j++) {
			rev_num[i][j] = 0;
		}
		us0avg[i] = 0.0;
		us1avg[i] = 0.0;
		amp_max[i] = 0;
		amp_min[i] = 0;
		ser_err[i] = 0;
	}
}

}; /* namespace PARSEWAV */
