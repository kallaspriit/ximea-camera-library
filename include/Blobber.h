/**
 * Blobber is a fast library for finding blobs of color in realtime video
 * images.
 *
 * It's a derivative of the great CMVision library from by James R. Bruce
 * from Carnegie Mellon University http://www.cs.cmu.edu/~jbruce/cmvision/.
 *
 * Adapted by Priit Kallas <kallaspriit@gmail.com>
 */

/*=========================================================================
API definition for the CMVision real time Color Machine Vision library
-------------------------------------------------------------------------
Copyright 1999, 2000         #### ### ### ## ## ## #### ##  ###  ##  ##
James R. Bruce              ##    ####### ## ## ## ##   ## ## ## ######
School of Computer Science  ##    ## # ## ## ## ##  ### ## ## ## ## ###
Carnegie Mellon University   #### ##   ##  ###  ## #### ##  ###  ##  ##
-------------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
=========================================================================*/

#ifndef BLOBBER_H
#define BLOBBER_H

// uncomment if your compiler supports the "restrict" keyword
#define restrict __restrict__
//#define restrict

#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

// color options
#define BLOBBER_COLOR_LEVELS  256
#define BLOBBER_MAX_COLORS     32

// sets tweaked optimal values for image size
#define BLOBBER_DEFAULT_WIDTH  640
#define BLOBBER_DEFAULT_HEIGHT 512

// values may need tweaked, although these seem to work usually
#define BLOBBER_MAX_RUNS     (BLOBBER_DEFAULT_WIDTH * BLOBBER_DEFAULT_HEIGHT) / 4
#define BLOBBER_MAX_REGIONS  BLOBBER_MAX_RUNS / 4
#define BLOBBER_MIN_AREA     1

#define BLOBBER_NONE ((unsigned)(-1))

// Options for level of processing, use enable()/disable() to change
#define BLOBBER_THRESHOLD      0x01
#define BLOBBER_COLOR_AVERAGES 0x02
#define BLOBBER_DUAL_THRESHOLD 0x04
#define BLOBBER_DENSITY_MERGE  0x08

#define BLOBBER_VALID_OPTIONS  0x0F

class Blobber {
    public:
        struct FormatYUV {
            unsigned char y, u, v;
        };

        struct FormatYUV422 {
            unsigned char y1, u, y2, v;
        };

        struct FormatUYUY {
            unsigned char u, y1, v, y2;
        };

        typedef struct FormatYUV422 Pixel;

        struct Rgb {
            Rgb() : red(0), green(0), blue(0) {}
            Rgb(char red, char green, char blue) : red(red), green(green), blue(blue) {}

            unsigned char red, green, blue;
        };

        struct Blob {
            int color;               // id of the color
            int area;                // occupied area in pixels
            int x1, y1, x2, y2;      // bounding box (x1,y1) - (x2,y2)
            float centerX, centerY;  // centroid
            FormatYUV average;       // average color (if BLOBBER_COLOR_AVERAGES enabled)
            int sumX, sumY, sumZ;    // temporaries for centroid and avg color
            Blob* next;              // next blob in list
        };

        struct ColorRun {
            unsigned color;  // which color(s) this run represents
            int length;      // the length of the run (in pixels)
            int parent;      // run's parent in the connected components tree
        };

        struct Color {
            Rgb color;              // example color (such as used in test output)
            char* name;             // color's meaninful name (e.g. ball, goal)
            double mergeThreshold;  // merge density threshold
            int expectedBlobs;      // expected number of blobs (used for merge)
            int yLow, yHigh;        // Y,U,V component thresholds
            int uLow, uHigh;
            int vLow, vHigh;
        };

        class MapFilter {
            public:
                virtual void filterMap(unsigned* map) = 0;
        };

        Blobber();
        ~Blobber();

        bool initialize(int width, int height);
        bool loadOptions(std::string filename);
        bool saveOptions(std::string filename);
        bool enable(unsigned opt);
        bool disable(unsigned opt);
        void close();

        void setMapFilter(MapFilter* mapFilter) {
            this->mapFilter = mapFilter;
        }

        MapFilter* getMapFilter() const {
            return mapFilter;
        }

        bool classify(Rgb* restrict out, Pixel* restrict image);

        void addColor(
            std::string name,
            int red, int green, int blue,
            int yLow, int yHigh,
            int uLow, int uHigh,
            int vLow, int vHigh,
            double mergeThreshold = 0.5d,
            int expectedBlobs = 5
        );

        bool getThreshold(
            int color,
            int& yLow, int& yHigh,
            int& uLow, int& uHigh,
            int& vLow, int& vHigh
        );

        bool setThreshold(
            int color,
            int yLow, int yHigh,
            int uLow, int uHigh,
            int vLow, int vHigh
        );

        unsigned* getMap() const {
            return map;
        }

        Color* getColor(int color) {
            return &colors[color];
        }

        int getColorId(std::string name) {
            for (int i = 0; i < colorCount; i++) {
                if (strcmp(colors[i].name, name.c_str()) == 0) {
                    return i;
                }
            }

            return -1;
        }

        Color* getColor(std::string name) {
            for (int i = 0; i < colorCount; i++) {
                if (strcmp(colors[i].name, name.c_str()) == 0) {
                    return &colors[i];
                }
            }

            return NULL;
        }

        void setColor(int color, Color& info) {
            colors[color] = info;
        }

        inline int getColorCount() {
            return colorCount;
        }

        bool processFrame(Pixel* image);
        bool processFrame(unsigned* map);

        int getBlobCount(int colorId);
        Blob* getBlobs(int colorId);

        int getBlobCount(std::string colorName) {
            return getBlobCount(getColorId(colorName));
        }

        Blob* getBlobs(std::string colorName) {
            return getBlobs(getColorId(colorName));
        }

    private:
        unsigned yClass[BLOBBER_COLOR_LEVELS];
        unsigned uClass[BLOBBER_COLOR_LEVELS];
        unsigned vClass[BLOBBER_COLOR_LEVELS];

        Blob blobTable[BLOBBER_MAX_REGIONS];
        Blob* blobList[BLOBBER_MAX_COLORS];
        int blobCount[BLOBBER_MAX_COLORS];

        ColorRun runMap[BLOBBER_MAX_RUNS];

        Color colors[BLOBBER_MAX_COLORS];
        int colorCount;
        int width, height;
        unsigned* map;

        MapFilter* mapFilter;

        unsigned options;

        void classifyFrame(Pixel* restrict img, unsigned* restrict map);
        int encodeRuns(ColorRun* restrict out, unsigned* restrict map);
        void connectComponents(ColorRun* restrict map, int num);
        int extractBlobs(Blob* restrict reg, ColorRun* restrict runMap, int num);

        void calculateAverageColors(
            Blob* restrict reg,
            int blobCount,
            Pixel* restrict img,
            ColorRun* restrict runMap,
            int runCount
        );

        int separateBlobs(Blob* restrict reg,int num);
        Blob* sortBlobListByArea(Blob* restrict list, int passes);
        void sortBlobs(int maxArea);

        // density based merging support
        int mergeBlobs(Blob* p, int num, double densityThreshold);
        int mergeBlobs();

        void filterAbove(unsigned* map, int* edges);

        void clear();

        static int log2modp[];

        // sum of integers over range [x,x+w)
        static inline int rangeSum(int x,int w) {
            return (w * (2 * x + w - 1) / 2);
        }

        // returns maximal value of two parameters
        template <class num>
        static inline num max(num a, num b) {
            return ((a > b)? a : b);
        }

        // returns minimal value of two parameters
        template <class num>
        static inline num min(num a,num b) {
            return((a < b)? a : b);
        }

        static int bottomBit(int n) {
            return(log2modp[(n & -n) % 37]);
        }

        // returns index of most significant set bit
        template <class num>
        static inline num topBit(num n) {
            int i = 1;
            if(!n) return(0);
            while(n>>i) i++;
            return(i);
        }

        // sets bits in k in array arr[l..r]
        template <class num>
        static void setBits(num* arr,int len,int l,int r,num k) {
            int i;

            l = max(l,0);
            r = min(r+1,len);

            for(i=l; i<r; i++) arr[i] |= k;
        }

        template <class num>
        static void clearBits(num* arr,int len,int l,int r,num k) {
            int i;

            l = max(l,0);
            r = min(r+1,len);

            k = ~k;
            for(i=l; i<r; i++) arr[i] &= k;
        }
};

#endif // BLOBBER_H
