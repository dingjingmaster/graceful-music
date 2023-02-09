#ifndef CMUS_SF_H
#define CMUS_SF_H

/*
 *  0     1 big_endian 0-1
 *  1     1 is_signed  0-1
 *  2     1 unused     0
 *  3-5   3 bits >> 3  0-7 (* 8 = 0-56)
 *  6-23 18 rate       0-262143
 * 24-31  8 channels   0-255
 */
typedef unsigned int sample_format_t;

#define SF_BIGENDIAN_MASK	0x00000001
#define SF_SIGNED_MASK		0x00000002
#define SF_BITS_MASK		0x00000038
#define SF_RATE_MASK		0x00ffffc0
#define SF_CHANNELS_MASK	0xff000000

#define SF_BIGENDIAN_SHIFT	0
#define SF_SIGNED_SHIFT		1
#define SF_BITS_SHIFT		0
#define SF_RATE_SHIFT		6
#define SF_CHANNELS_SHIFT	24

#define sf_get_bigendian(sf)	(((sf) & SF_BIGENDIAN_MASK) >> SF_BIGENDIAN_SHIFT)
#define sf_get_signed(sf)	(((sf) & SF_SIGNED_MASK   ) >> SF_SIGNED_SHIFT)
#define sf_get_bits(sf)		(((sf) & SF_BITS_MASK     ) >> SF_BITS_SHIFT)
#define sf_get_rate(sf)		(((sf) & SF_RATE_MASK     ) >> SF_RATE_SHIFT)
#define sf_get_channels(sf)	(((sf) & SF_CHANNELS_MASK ) >> SF_CHANNELS_SHIFT)

#define sf_signed(val)		(((val) << SF_SIGNED_SHIFT   ) & SF_SIGNED_MASK)
#define sf_bits(val)		(((val) << SF_BITS_SHIFT     ) & SF_BITS_MASK)
#define sf_rate(val)		(((val) << SF_RATE_SHIFT     ) & SF_RATE_MASK)
#define sf_channels(val)	(((val) << SF_CHANNELS_SHIFT ) & SF_CHANNELS_MASK)
#define sf_bigendian(val)	(((val) << SF_BIGENDIAN_SHIFT) & SF_BIGENDIAN_MASK)
#ifdef WORDS_BIGENDIAN
#	define sf_host_endian()	sf_bigendian(1)
#else
#	define sf_host_endian()	sf_bigendian(0)
#endif

#define sf_get_sample_size(sf)	(sf_get_bits((sf)) >> 3)
#define sf_get_frame_size(sf)	(sf_get_sample_size((sf)) * sf_get_channels((sf)))
#define sf_get_second_size(sf)	(sf_get_rate((sf)) * sf_get_frame_size((sf)))

#endif
