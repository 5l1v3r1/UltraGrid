#include <stdio.h> // FILE *

struct wav_metadata {
        unsigned int ch_count;
        unsigned int sample_rate;
        unsigned int bits_per_sample;

        unsigned int data_size;
        unsigned int data_offset; // from the beginning of file
};

#define WAV_HDR_PARSE_OK           0
#define WAV_HDR_PARSE_READ_ERROR   1
#define WAV_HDR_PARSE_WRONG_FORMAT 2
#define WAV_HDR_PARSE_NOT_PCM      3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function reads wav header
 *
 * If read successfully, it leaves read file position at the beginning of data.
 * Currently, only interleaved PCM is supported.
 *
 * @retval WAV_HDR_PARSE_OK if ok
 * @retval WAV_HDR_PARSE_READ_ERROR in case of file read error
 * @retval WAV_HDR_PARSE_WRONG_FORMAT if unsupported wav format
 * @retval WAV_HDR_PARSE_NOT_PCM non-PCM WAV detected
 */
int read_wav_header(FILE *wav_file, struct wav_metadata *metadata);

const char *get_wav_error(int errcode);

#ifdef __cplusplus
}
#endif

