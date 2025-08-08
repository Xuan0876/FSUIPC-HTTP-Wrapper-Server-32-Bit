#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geohash.h"
#include "float.h"
#include <time.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define FILTER_TARGET 1000

struct hms { short h, m; unsigned short ms; };

static struct hms
hms_unpack(double a)
{
    double f  = fabs(fmod(a, 1.0));
    int    h  = a;
    int    m  = f*60;
    int    ms = (f*3600 - m*60) * 1000;
    return (struct hms){h, m, ms};
}

static double
hms_pack(struct hms v)
{
    int sign = v.h < 0 ? -1 : +1;
    return sign * (labs(v.h) + v.m/60.0 + (v.ms/1000.0)/3600.0);
}

static int
hms_parse(struct hms *lat, struct hms *lon, const char *s)
{
    unsigned lath, latm, lats;
    unsigned lonh, lonm, lons;
    char latf[4], lonf[4];
    int r, latsign, lonsign;
    char ns, ew;

    r = sscanf(s, "%u@%u'%d.%3[0-9]\" %c %u@%u'%u.%3[0-9]\" %c",
               &lath, &latm, &lats, latf, &ns,
               &lonh, &lonm, &lons, lonf, &ew);
    if (r != 10) {
        return 0;
    }
    if (lath>89 || latm>59 || lats>59 || strlen(latf) < 3) {
        return 0;
    }
    if (lonh>179 || lonm>59 || lons>59 || strlen(lonf) < 3) {
        return 0;
    }

    switch (ns) {
    case 'N': latsign = +1; break;
    case 'S': latsign = -1; break;
    default: return 0;
    }
    switch (ew) {
    case 'E': lonsign = +1; break;
    case 'W': lonsign = -1; break;
    default: return 0;
    }

    lat->h  = latsign*lath;
    lat->m  = latm;
    lat->ms = lats*1000 + atoi(latf);
    lon->h  = lonsign*lonh;
    lon->m  = lonm;
    lon->ms = lons*1000 + atoi(lonf);
    return 1;
}

static void
hms_print(char *buf, struct hms lat, struct hms lon)
{
    sprintf(buf, "%2d@%3d'%3d.%03d\" %c%5d@%3d'%3d.%03d\" %c", // 38 bytes
            (int)labs(lat.h), lat.m, lat.ms/1000, lat.ms%1000, "NS"[lat.h<0],
            (int)labs(lon.h), lon.m, lon.ms/1000, lon.ms%1000, "EW"[lon.h<0]);
}

static void
usage(FILE *f, const char *name)
{
    fprintf(f, "usage: %s <-d|-e> [latlon...]\n", name);
    fprintf(f, "examples:\n");
    fprintf(f, "    $ %s -d dppn59uz86jzd\n", name);
    fprintf(f, "    $ %s -e \"40@26'26.160\\\"N 79@59'45.239\\\"W\"\n", name);
}

static int
decode(int argc, char *argv[])
{
    for (int i = 2; i < argc; i++) {
        double latlon[2];
        size_t len = strlen(argv[i]);
        int r = geohash_decode(latlon, latlon+1, argv[i], len > 14 ? 14 : len);
        if (!r) {
            fprintf(stderr, "%s: invalid geohash, %s\n", argv[0], argv[i]);
            return 1;
        }

        char repr[64];
        struct hms lat = hms_unpack(latlon[0]);
        struct hms lon = hms_unpack(latlon[1]);
        hms_print(repr, lat, lon);
        printf("%-15.14s%s\n", argv[i], repr);
    }
    return 0;
}

static int
encode(int argc, char *argv[])
{
    for (int i = 2; i < argc; i++) {
        struct hms lat, lon;
        if (!hms_parse(&lat, &lon, argv[i])) {
            fprintf(stderr, "%s: invalid lat/lon, %s\n", argv[0], argv[i]);
            return 1;
        }

        char target[64];
        hms_print(target, lat, lon);

        char buf[32];
        double latlon[] = { hms_pack(lat), hms_pack(lon) };
        geohash_encode(buf, latlon[0], latlon[1]);

        // Find shortest geohash with the same printout
        int best = 14;
        for (int n = 13; n >= 0; n--) {
            char repr[64];
            geohash_decode(latlon, latlon+1, buf, n);
            lat = hms_unpack(latlon[0]);
            lon = hms_unpack(latlon[1]);
            hms_print(repr, lat, lon);
            if (!strcmp(target, repr)) {
                best = n;
            }
        }
        buf[best] = 0;

        printf("%-15.14s%s\n", buf, target);
    }
    return 0;
}
// Add this struct and function to the file

struct Airport {
    char geohash[64];
    char ident[16];
    char type[32];
    char name[128];
    double lat;
    double lon;
};

static struct Airport* airports = NULL;
static size_t airport_count = 0, airport_cap = 0;

// Reads simplifiedAirports.csv and loads all airports into memory.
// Returns 1 on success, 0 on failure.
static int load_airports_csv(const char* filename) {
    FILE* fin = fopen(filename, "r");
    if (!fin) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return 0;
    }

    char line[512];
    // Skip header
    if (!fgets(line, sizeof(line), fin)) {
        fclose(fin);
        return 0;
    }

    while (fgets(line, sizeof(line), fin)) {
        struct Airport ap;
        char* p = strchr(line, '\n');
        if (p) *p = 0;

        // Parse: geohash,ident,type,name,latitude,longitude
        // Name may contain spaces but not commas
        int n = sscanf(line, "%64[^,],%15[^,],%31[^,],%127[^,],%lf,%lf",
            ap.geohash, ap.ident, ap.type, ap.name, &ap.lat, &ap.lon);
        if (n != 6) continue;

        if (airport_count == airport_cap) {
            airport_cap = airport_cap ? airport_cap * 2 : 128;
            airports = (struct Airport*)realloc(airports, airport_cap * sizeof(struct Airport));
        }
        airports[airport_count++] = ap;
    }
    fclose(fin);
    return 1;
}
static double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;
    double a = sin(dlat/2)*sin(dlat/2) + sin(dlon/2)*sin(dlon/2)*cos(lat1)*cos(lat2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return 6371.0 * c; // Earth radius in km
}
static int initialise_hash() {
    clock_t t_start = clock();

    if (!load_airports_csv("simplifiedAirports.csv")) {
        return 1;
    }
    clock_t t_loaded = clock();

    //init end

    //input start
	printf("Loaded %zu airports.\n", airport_count);
    /*
	for (size_t i = 0; i < airport_count && i < 5; ++i) {
		printf("%-10s | %-8s | %-20s | %10.6f | %10.6f\n",
			airports[i].geohash,
			airports[i].ident,
			airports[i].name,
			airports[i].lat,
			airports[i].lon);
	}
    */
	printf("Time (ms): %.2f\n", 1000.0 * (t_loaded - t_start) / CLOCKS_PER_SEC);
    return 0;
}
static int finalise_hash() {
	free(airports);
}
static int query_location(double input_lat, double input_lon, char *airport_ICAO, char *airport_name, double *dist) {
    if (airports == NULL) {
		printf("Airports not loaded. Call initialise() first.\n");
    }
    clock_t t_input = clock();
	
	//input end

	//encode start
	char input_geohash[64];
	geohash_encode(input_geohash, input_lat, input_lon);


	//search start
	int prefix_len = 5;
	size_t* candidates = malloc(airport_count * sizeof(size_t));
	size_t candidate_count = 0;

	// Expand prefix until at least FILTER_TARGET candidates or prefix_len == 0
	while (candidate_count < FILTER_TARGET && prefix_len > 0) {
		candidate_count = 0;
		for (size_t i = 0; i < airport_count; ++i) {
			if (strncmp(input_geohash, airports[i].geohash, prefix_len) == 0) {
				candidates[candidate_count++] = i;
			}
		}
		if (candidate_count < FILTER_TARGET) {
			--prefix_len;
		}
	}
	clock_t t_filter = clock();

	struct {
		size_t idx;
		double dist;
	} nearest[FILTER_TARGET] = {0};
	for (int i = 0; i < FILTER_TARGET; ++i) nearest[i].dist = DBL_MAX;

	for (size_t i = 0; i < candidate_count; ++i) {
		size_t idx = candidates[i];
		double d = haversine(input_lat, input_lon, airports[idx].lat, airports[idx].lon);

		for (int j = 0; j < FILTER_TARGET; ++j) {
			if (d < nearest[j].dist) {
				for (int k = FILTER_TARGET-1; k > j; --k) nearest[k] = nearest[k-1];
				nearest[j].idx = idx;
				nearest[j].dist = d;
				break;
			}
		}
	}
	clock_t t_sort = clock();

	//output result
    printf("--------NEARBY ARPT LIST--------\n");
	printf("5 nearest airports:\n");
	for (int i = 0; i < 5 && nearest[i].dist < DBL_MAX; ++i) {
		size_t idx = nearest[i].idx;
		if (i == 0) {
			strncpy(airport_ICAO, airports[idx].ident, 63);
			strncpy(airport_name, airports[idx].name, 255);
			*dist = nearest[i].dist;
		}
		printf("%-10s | %-8s | %-20s | %10.6f | %10.6f | %8.2f km\n",
			airports[idx].geohash,
			airports[idx].ident,
			airports[idx].name,
			airports[idx].lat,
			airports[idx].lon,
			nearest[i].dist);
	}
    printf("------NEARBY ARPT LIST END------\n");
	//printf("100 nearest airports:\n");
	//for (int i = 0; i < 100 && nearest[i].dist < DBL_MAX; ++i) {
	//    size_t idx = nearest[i].idx;
	//    printf("%10.6f %10.6f\n",
	//        airports[idx].lat,
	//        airports[idx].lon);
	//}

	//stats
	printf("Timing (ms):\n");
	printf("  Filter:        %.2f\n", 1000.0 * (t_filter - t_input) / CLOCKS_PER_SEC);
	printf("  Distance sort: %.2f\n", 1000.0 * (t_sort - t_filter) / CLOCKS_PER_SEC);
	free(candidates);
    return 0;
}
