//
// Created by dingjing on 1/31/23.
//

#ifndef GRACEFUL_MUSIC_GLOBAL_H
#define GRACEFUL_MUSIC_GLOBAL_H

#define CHUNK_SIZE                      (12 * 840 * 6)
extern unsigned int                     gBufferNRChunks;

extern char* gLogPath = NULL;
extern char* gMusicDataDir = NULL;
extern char* gMusicConfigDir = NULL;
extern char* gMusicConfigPath = NULL;
extern char* gMusicSocketPath = NULL;

/**
 *  0           1 big_endian    0-1
 *  1           1 is_signed     0-1
 *  2           1 unused        0
 *  3-5         3 bits >> 3     0-7 (* 8 = 0-56)
 *  6-23       18 rate          0-262143
 * 24-31        8 channels      0-255
 */
typedef unsigned int                    SampleFormat;

#define SF_BIGENDIAN_MASK               0X00000001
#define SF_SIGNED_MASK                  0X00000002
#define SF_BITS_MASK                    0X00000038
#define SF_RATE_MASK                    0X00FFFFC0
#define SF_CHANNELS_MASK                0XFF000000

#define SF_BIGENDIAN_SHIFT              0
#define SF_SIGNED_SHIFT                 1
#define SF_BITS_SHIFT                   0
#define SF_RATE_SHIFT                   6
#define SF_CHANNELS_SHIFT               24

#define SF_GET_BIGENDIAN(sf)            (((sf) & SF_BIGENDIAN_MASK) >> SF_BIGENDIAN_SHIFT)
#define SF_GET_SIGNED(sf)               (((sf) & SF_SIGNED_MASK) >> SF_SIGNED_SHIFT)
#define SF_GET_BITS(sf)                 (((sf) & SF_BITS_MASK) >> SF_BITS_SHIFT)
#define SF_GET_RATE(sf)                 (((sf) & SF_RATE_MASK) >> SF_RATE_SHIFT)
#define SF_GET_CHANNELS(sf)             (((sf) & SF_CHANNELS_MASK) >> SF_CHANNELS_SHIFT)

#define SF_SIGNED(val)                  (((val) << SF_SIGNED_SHIFT) & SF_SIGNED_MASK)
#define SF_BITS(val)                    (((val) << SF_BITS_SHIFT) & SF_BITS_MASK)
#define SF_RATE(val)                    (((val) << SF_RATE_SHIFT) & SF_RATE_MASK)
#define SF_CHANNELS(val)                (((val) << SF_CHANNELS_SHIFT) & SF_CHANNELS_MASK)
#define SF_BIGENDIAN(val)               (((val) << SF_BIGENDIAN_SHIFT) & SF_BIGENDIAN_MASK)

//#define SF_HOST_ENDIAN()                (SF_BIGENDIAN(1))
#define SF_HOST_ENDIAN()                (SF_BIGENDIAN(0))

#define SF_GET_SAMPLE_SIZE(sf)          (SF_GET_BITS((sf)) >> 3)
#define SF_GET_FRAME_SIZE(sf)           (SF_GET_SAMPLE_SIZE((sf)) * SF_GET_CHANNELS((sf)))
#define SF_GET_SECOND_SIZE(sf)          (SF_GET_RATE((sf)) * SF_GET_FRAME_SIZE((sf)))

#endif //GRACEFUL_MUSIC_GLOBAL_H
