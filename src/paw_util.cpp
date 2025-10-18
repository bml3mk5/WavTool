/// @file paw_util.cpp
///
/// @brief 関数群
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2017.12.01
///
#include "paw_util.h"


namespace PARSEWAV
{

/// @brief コンストラクタ
///
Util::Util()
{
	InitConvSampleData();
}

/// @brief デストラクタ
///
Util::~Util()
{
}

/// @brief waveサンプリングデータのレートとビット変換用の変数初期化
///
void Util::InitConvSampleData()
{
	divide = 0;
	surplus = 0;
	remain_buf_len = 0;
	memset(remain_buf, 0, sizeof(remain_buf));
}

/// @brief waveサンプリングデータのレートとビットを変換してファイルに出力
///
/// @param[in] in_data      サンプリングデータ
/// @param[in] in_rate      bufのサンプルレート
/// @param[in] outrate      出力wavファイルのサンプルレート
/// @param[in] out_blk_size 出力wavファイルのサンプルバイト
/// @param[in,out] file     出力wavファイル
void Util::OutConvSampleData(WaveData &in_data, int in_rate
						, int outrate, int out_blk_size, OutputFile &file)
{
	CSampleBytes in_buf(in_data, in_data.GetStartPos(), in_data.Length());
	in_data.SetStartPos(in_data.GetWritePos());
	OutConvSampleData(in_buf.Get(), in_rate, 1, in_buf.Length(), outrate, out_blk_size, file);
}

/// @brief waveサンプリングデータのレートとビットを変換してファイルに出力
///
/// @param[in] in_buf       サンプリングデータ(8 or 16bit)
/// @param[in] in_rate      bufのサンプルレート
/// @param[in] in_blk_size  bufのサンプルバイト(1 or 2)
/// @param[in] in_len       buf長さ
/// @param[in] outrate      出力wavファイルのサンプルレート
/// @param[in] out_blk_size 出力wavファイルのサンプルバイト
/// @param[in,out] file     出力wavファイル
void Util::OutConvSampleData(const void *in_buf, int in_rate, int in_blk_size, size_t in_len
								, int outrate, int out_blk_size, OutputFile &file)
{
	size_t src_len = 0;
	size_t pos = 0;
	int new_data = 0;
	size_t new_len = 0;
	size_t i = 0;
	int16_t *dst_buf = new_buf;
	size_t dst_len = 0;

//	memset(src_buf, 0, sizeof(src_buf));
//	memset(new_buf, 0, sizeof(new_buf));
	for (i=0; i<(size_t)remain_buf_len; i++) {
		src_buf[i]=remain_buf[i];
	}
	src_len = remain_buf_len;

	if (in_blk_size == 1) {
		// 8ビット
		for(i=0; i<in_len; i++) {
			src_buf[src_len]=(int16_t)*((uint8_t *)in_buf + i);
			src_len++;
		}
	} else if (in_blk_size == 2) {
		// 16ビット
		for(i=0; i<in_len; i++) {
			src_buf[src_len]=(int16_t)*((int16_t *)in_buf + i);
			src_len++;
		}
	}

//	memset(remain_buf, 0, sizeof(remain_buf));
	remain_buf_len = 0;

	if (in_rate > outrate) {
		// rate 下げる場合
		pos = 0;
		while (pos < src_len) {
			divide = (in_rate + surplus) / outrate;
			if (pos + divide + 2 >= src_len) {
				break;
			}
			int n_surplus = (in_rate + surplus) % outrate;

			// 平均化
			new_data = 0;
			if (surplus >= 0) {
				new_data += (int)((double)src_buf[pos] * (double)((int)outrate - surplus) / (double)outrate);
				pos++;
			}
			for(i = 1; i < (size_t)divide; i++) {
				new_data += src_buf[pos];
				pos++;
			}
			if (n_surplus > 0) {
				new_data += (int)((double)src_buf[pos] * (double)n_surplus / (double)outrate);
			}
			new_data = (int)((double)new_data * (double)outrate / (double)in_rate);

			surplus = n_surplus;
			new_buf[new_len] = (int16_t)new_data;
			new_len++;
		}
		if (pos < src_len) {
			// 余ったデータは次回にまわす
			for(i=pos; i<src_len; i++) {
				remain_buf[i-pos]=src_buf[i];
			}
			remain_buf_len = (int)(src_len - pos);
		}
		dst_len = new_len; 
	} else if (in_rate < outrate) {
		// rate 上げる場合
		pos = 0;
		while (pos < src_len - 1) {
			divide = (outrate + surplus) / in_rate;
			surplus = (outrate + surplus) % in_rate;
			// サンプル間を線形で補完
			for(i=0; i<(size_t)divide; i++) {
				new_data = (int)src_buf[pos] + (int)i * ((int)src_buf[pos+1] - (int)src_buf[pos]) / divide;
				new_buf[new_len] = (int16_t)new_data;
				new_len++;
				// バッファがいっぱいなのでファイルに出す
				if (new_len >= OUTCONV_BUFFER_SIZE - 1) {
					out_sample_buffer(new_buf, new_len, in_blk_size, out_blk_size, file);
//					memset(new_buf, 0, sizeof(new_buf));
					new_len = 0;
				}
			}
			pos++;
		}
		// 最後のデータを次に回す
		remain_buf[0]=src_buf[src_len-1];
		remain_buf_len = 1;
		dst_len = new_len; 
	} else {
		// 同じレート
//		memcpy(new_buf, src_buf, src_len * sizeof(int16_t));
		dst_buf = src_buf;
		dst_len = src_len;
	}

	out_sample_buffer(dst_buf, dst_len, in_blk_size, out_blk_size, file);
}


/// @brief waveサンプリングデータのビット変換
///
/// @param[in] in_buf       入力データ
/// @param[in] in_len       in_buf長さ
/// @param[in] in_blk_size  in_bufのサンプルバイト(1 or 2)
/// @param[in] out_blk_size 出力ファイルのサンプルバイト
/// @param[out] file        出力ファイル
/// @return 出力サイズ(バイト)
size_t Util::out_sample_buffer(int16_t *in_buf, size_t in_len, int in_blk_size, int out_blk_size, OutputFile &file)
{
	int16_t new_data = 0;
	size_t i = 0;
	size_t out_len = 0;

	// 出力
	if (in_blk_size > out_blk_size) {
		// ビット 下げる場合
		for(i=0; i<in_len; i++) {
			new_data = (in_buf[i] / 256) + 128;

			file.Fputc(new_data);
			out_len++;
		}
	} else if (in_blk_size < out_blk_size) {
		// ビット 上げる場合
		for(i=0; i<in_len; i++) {
			new_data = (in_buf[i] - 128) * 256;

			file.Fwrite(&new_data, sizeof(int16_t), 1);
			out_len+=2;
		}
	} else {
		// 同じビット
		if (out_blk_size == 1) {
			for(i=0; i<in_len; i++) {
				new_data = in_buf[i];
				file.Fputc(new_data);
				out_len++;
			}
		} else {
			for(i=0; i<in_len; i++) {
				new_data = in_buf[i];
				file.Fwrite(&new_data, sizeof(int16_t), 1);
				out_len+=2;
			}
		}
	}
	return out_len;
}

/// @brief WAVEファイルのフォーマットをチェックする
///
/// @param[in]  file     入力ファイル
/// @param[out] format   waveフォーマット
/// @param[out] data_len サンプル数
/// @return pwOK / pwErrNotPCMFormat / pwErrSampleRate
PwErrCode Util::CheckWavFormat(InputFile &file, WaveFormat &format, size_t *data_len)
{
	return CheckWavFormat(file, format.GetHead(), format.GetFmtChank(), format.GetDataChank(), data_len);
}

/// @brief WAVEファイルのフォーマットをチェックする
///
/// @param[in]   file     入力ファイル
/// @param[out]  head     waveヘッダ
/// @param[out]  fmt      waveフォーマットタイプ
/// @param[out]  data     waveデータ
/// @param[out]  data_len サンプル数
/// @return pwOK / pwErrNotPCMFormat / pwErrSampleRate
PwErrCode Util::CheckWavFormat(InputFile &file, wav_header_t *head, wav_fmt_chank_t *fmt, wav_data_chank_t *data, size_t *data_len)
{
	char buf[10];
	long offset = 0;
	long fpos_data = 0;
	wav_unknown_chank_t unk;

	memset(head, 0, sizeof(wav_header_t));
	memset(fmt, 0, sizeof(wav_fmt_chank_t));
	memset(data, 0, sizeof(wav_data_chank_t));

	file.Fread(head, sizeof(wav_header_t), 1);
	if(memcmp(head->RIFF,"RIFF",4) != 0 || memcmp(head->WAVE,"WAVE",4) != 0) {
		// this is not wave format !!!
		return pwErrNotPCMFormat;
	}

	for (int i=0; i<10; i++) {
		file.Fread(buf, sizeof(char), 4);
		if (memcmp(buf, "fmt ", 4) == 0) {
			// fmt chank
			file.Fseek(-4, SEEK_CUR);
			file.Fread(fmt, sizeof(wav_fmt_chank_t), 1);
			if (fmt->format_id != 1) {
				// this is not pcm format !!!
				return pwErrNotPCMFormat;
			}

			// 11025 - 48000Hz
			if (fmt->sample_rate < 11025 || 48000 < fmt->sample_rate) {
				return pwErrSampleRate;
			}

			if (fpos_data != 0) break;

			offset = 8 + fmt->fmt_size - sizeof(wav_fmt_chank_t);

		} else if (memcmp(buf, "data", 4) == 0) {
			// data chank
			file.Fseek(-4, SEEK_CUR);
			file.Fread(data, sizeof(wav_data_chank_t), 1);
			fpos_data = file.Ftell();

			if (fmt->format_id != 0) break;

			offset = data->data_len;

		} else {
			// unknown chank
			file.Fseek(-4, SEEK_CUR);
			file.Fread(&unk, sizeof(wav_unknown_chank_t), 1);

			offset = unk.len;
		}
		file.Fseek(offset, SEEK_CUR);
	}

	if (fpos_data == 0 || fmt->format_id != 1) {
		// this is not pcm format !!!
		return pwErrNotPCMFormat;
	}
	// 実際のサンプルデータがはじまる位置の先頭
	file.Fseek(fpos_data, SEEK_SET);

	return pwErrNone;
}

}; /* namespace PARSEWAV */
