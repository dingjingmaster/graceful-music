#include "comment.h"
#include "xmalloc.h"
#include "utils.h"
#include "uchar.h"

#include <string.h>
#include <strings.h>

static int is_various_artists(const char *a)
{
	return strcasecmp(a, "Various Artists") == 0 ||
	       strcasecmp(a, "Various")         == 0 ||
	       strcasecmp(a, "VA")              == 0 ||
	       strcasecmp(a, "V/A")             == 0;
}

int track_is_compilation(const KeyValue* comments)
{
	const char *c, *a, *aa;

	c = key_value_get_value (comments, "compilation");
	if (c && is_freeform_true(c))
		return 1;

	c = key_value_get_value (comments, "partofacompilation");
	if (c && is_freeform_true(c))
		return 1;

	aa = key_value_get_value (comments, "albumartist");
	if (aa && is_various_artists(aa))
		return 1;

	a = key_value_get_value (comments, "artist");
	if (a && is_various_artists(a))
		return 1;

	if (aa && a && !u_strcase_equal(aa, a))
		return 1;

	return 0;
}

int track_is_va_compilation(const KeyValue* comments)
{
	const char *c, *a, *aa;

	aa = key_value_get_value (comments, "albumartist");
	if (aa)
		return is_various_artists(aa);

	a = key_value_get_value (comments, "artist");
	if (a && is_various_artists(a))
		return 1;

	c = key_value_get_value (comments, "compilation");
	if (c && is_freeform_true(c))
		return 1;

	c = key_value_get_value (comments, "partofacompilation");
	if (c && is_freeform_true(c))
		return 1;

	return 0;
}

const char *comments_get_albumartist(const KeyValue* comments)
{
	const char *val = key_value_get_value (comments, "albumartist");

	if (!val || strcmp(val, "") == 0)
		val = key_value_get_value (comments, "artist");

	return val;
}

const char *comments_get_artistsort(const KeyValue* comments)
{
	const char *val;

	if (track_is_va_compilation(comments))
		return NULL;

	val = key_value_get_value (comments, "albumartistsort");
	if (!track_is_compilation(comments)) {
		if (!val || strcmp(val, "") == 0)
			val = key_value_get_value (comments, "artistsort");
	}

	if (!val || strcmp(val, "") == 0)
		return NULL;

	return val;
}

int comments_get_int(const KeyValue* comments, const char *key)
{
	const char *val;
	long int ival;

	val = key_value_get_value (comments, key);
	if (val == NULL)
		return -1;
	while (*val && !(*val >= '0' && *val <= '9'))
		val++;
	if (str_to_int(val, &ival) == -1)
		return -1;
	return ival;
}

int comments_get_signed_int(const KeyValue* comments, const char *key, long int *ival)
{
	const char *val;

	val = key_value_get_value (comments, key);
	if (val == NULL)
		return -1;
	while (*val && !(*val == '+' || *val == '-' || (*val >= '0' && *val <= '9')))
		val++;
	return str_to_int(val, ival);
}

double comments_get_double(const KeyValue* comments, const char *key)
{
	const char *val;
	char *end;
	double d;

	val = key_value_get_value (comments, key);
	if (!val || strcmp(val, "") == 0)
		goto error;

	d = strtod(val, &end);
	if (val == end)
		goto error;

	return d;

error:
	return strtod("NAN", NULL);
}

/* Return date as an integer in the form YYYYMMDD, for sorting purposes.
 * This function is not year 10000 compliant. */
int comments_get_date(const KeyValue* comments, const char *key)
{
	const char *val;
	char *endptr;
	int year, month, day;
	long int ival;

	val = key_value_get_value (comments, key);
	if (val == NULL)
		return -1;

	year = strtol(val, &endptr, 10);
	/* Looking for a four-digit number */
	if (year < 1000 || year > 9999)
		return -1;
	ival = year * 10000;

	if (*endptr == '-' || *endptr == ' ' || *endptr == '/') {
		month = strtol(endptr+1, &endptr, 10);
		if (month < 1 || month > 12)
			return ival;
		ival += month * 100;
	}

	if (*endptr == '-' || *endptr == ' ' || *endptr == '/') {
		day = strtol(endptr+1, &endptr, 10);
		if (day < 1 || day > 31)
			return ival;
		ival += day;
	}


	return ival;
}

static const char *interesting[] = {
	"artist", "album", "title", "tracknumber", "discnumber", "genre",
	"date", "compilation", "partofacompilation", "albumartist", "artistsort", "albumartistsort",
	"albumsort",
	"originaldate",
	"r128_track_gain",
	"r128_album_gain",
	"replaygain_track_gain",
	"replaygain_track_peak",
	"replaygain_album_gain",
	"replaygain_album_peak",
	"musicbrainz_trackid",
	"comment",
	"bpm",
	"arranger", "composer", "conductor", "lyricist", "performer",
	"remixer", "label", "publisher", "work", "opus", "partnumber", "part",
	"subtitle", "media",
	NULL
};

static struct {
	const char *old;
	const char *new;
} key_map[] = {
	{ "album_artist", "albumartist" },
	{ "album artist", "albumartist" },
	{ "disc", "discnumber" },
	{ "tempo", "bpm" },
	{ "track", "tracknumber" },
	{ "WM/Year", "date" },
	{ "WM/ArtistSortOrder", "artistsort" },
	{ "WM/AlbumArtistSortOrder", "albumartistsort" },
	{ "WM/AlbumSortOrder", "albumsort" },
	{ "WM/OriginalReleaseYear", "originaldate" },
	{ "WM/Media", "media" },
	{ "sourcemedia", "media" },
	{ "MusicBrainz Track Id", "musicbrainz_trackid" },
	{ "version", "subtitle" },
	/* ffmpeg id3 */
	{ "artist-sort", "artistsort" },
	{ "TSO2", "albumartistsort" },
	{ "album-sort", "albumsort" },
	/* ffmpeg mp4 */
	{ "sort_artist", "artistsort" },
	{ "sort_album_artist", "albumartistsort" },
	{ "sort_album", "albumsort" },
	{ NULL, NULL }
};

static const char *fix_key(const char *key)
{
	int i;

	for (i = 0; interesting[i]; i++) {
		if (!strcasecmp(key, interesting[i]))
			return interesting[i];
	}
	for (i = 0; key_map[i].old; i++) {
		if (!strcasecmp(key, key_map[i].old))
			return key_map[i].new;
	}
	return NULL;
}

int comments_add(GrowingKeyValues* c, const char *key, char *val)
{
	if (!strcasecmp(key, "songwriter")) {
		int r = comments_add_const(c, "lyricist", val);
		return comments_add(c, "composer", val) && r;
	}

	key = fix_key(key);
	if (!key) {
		free(val);
		return 0;
	}

	if (!strcmp(key, "tracknumber") || !strcmp(key, "discnumber")) {
		char *slash = strchr(val, '/');
		if (slash)
			*slash = 0;
	}

	/* don't add duplicates */
	if (key_value_get_val_growing(c, key)) {
		free(val);
		return 0;
	}

    key_value_add (c, key, val);
	return 1;
}

int comments_add_const(GrowingKeyValues* c, const char *key, const char *val)
{
	return comments_add(c, key, xstrdup(val));
}
