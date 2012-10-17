#include "Blobber.h"

/*=========================================================================
Implementation of the CMVision real time Color Machine Vision library
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
-------------------------------------------------------------------------
Revision History:
  1999-11-18:  Initial release version (JRB)
  2000-05-20:  Added Bugfixes from Peter,
               fixed bounding box bug (JRB)
  2000-06-04:  Some other minor fixes (JRB)
  2000-07-02:  Added average color and density merging (JRB)
  2000-07-20:  Added dual threshold capability (JRB)
=========================================================================*/

int Blobber::log2modp[] = {0, 1, 2,27, 3,24,28, 0, 4,17,25,31,29,12, 0,14, 5, 8,18, 0,26,23,32,16,30,11,13, 7, 0,22,15,10, 6,21, 9,20,19};

Blobber::Blobber() {
    clear();
    mapFilter = NULL;
}
Blobber::~Blobber() {
    close();
}

void Blobber::classifyFrame(Pixel* restrict img,unsigned* restrict map)
// Classifies an image passed in as img, saving bits in the entries
// of map representing which thresholds that pixel satisfies.
{
    int i,m,s;
    int m1,m2;
    Pixel p;

    unsigned* uclas = uClass; // Ahh, the joys of a compiler that
    unsigned* vclas = vClass; //   has to consider pointer aliasing
    unsigned* yclas = yClass;

    s = width * height;

    if(options & BLOBBER_DUAL_THRESHOLD) {
        for(i=0; i<s; i+=2) {
            p = img[i/2];
            m = uclas[p.u] & vclas[p.v];
            m1 = m & yclas[p.y1];
            m2 = m & yclas[p.y2];
            map[i + 0] = m1 | (m1 >> 16);
            map[i + 1] = m2 | (m2 >> 16);
        }
    } else {
        for(i=0; i<s; i+=2) {
            p = img[i/2];
            m = uclas[p.u] & vclas[p.v];
            map[i + 0] = m & yclas[p.y1];
            map[i + 1] = m & yclas[p.y2];
        }
    }

    if (mapFilter != NULL) {
        mapFilter->filterMap(map);
    }
}

int Blobber::encodeRuns(ColorRun* restrict out,unsigned* restrict map)
// Changes the flat array version of the threshold satisfaction map
// into a run length encoded version, which speeds up later processing
// since we only have to look at the points where values change.
{
    int x,y,j,l;
    unsigned m,save;
    unsigned* row;
    ColorRun r;

    // initialize terminator restore
    save = map[0];

    j = 0;
    for(y=0; y<height; y++) {
        row = &map[y * width];

        // restore previous terminator and store next
        // one in the first pixel on the next row
        row[0] = save;
        save = row[width];
        row[width] = BLOBBER_NONE;

        x = 0;
        while(x < width) {
            m = row[x];
            // m = m & (~m + 1); // get last bit
            l = x;
            while(row[x] == m) x++;
            // x += (row[x] == BLOBBER_NONE); //  && (last & m);

            r.color  = m;
            r.length = x - l;
            r.parent = j;
            out[j++] = r;
            if(j >= BLOBBER_MAX_RUNS) return(0);
        }
    }

    return(j);
}

void Blobber::connectComponents(ColorRun* restrict map,int num)
// Connect components using four-connecteness so that the runs each
// identify the global parent of the connected blob they are a part
// of.  It does this by scanning adjacent rows and merging where similar
// colors overlap.  Used to be union by rank w/ path compression, but now
// is just uses path compression as the global parent index seems to be
// a simpler fast approximation of rank in practice.
// WARNING: This code is *extremely* complicated and twitchy.  It appears
//   to be a correct implementation, but minor changes can easily cause
//   big problems.  Read the papers on this library and have a good
//   understanding of tree-based union find before you touch it
{
    int x1,x2;
    int l1,l2;
    ColorRun r1,r2;
    int i,p,s,n;

    l1 = l2 = 0;
    x1 = x2 = 0;

    // Lower scan begins on second line, so skip over first
    while(x1 < width) {
        x1 += map[l1++].length;
    }
    x1 = 0;

    // Do rest in lock step
    r1 = map[l1];
    r2 = map[l2];
    s = l1;
    while(l1 < num) {
        if(r1.color==r2.color && r1.color) {
            if((x1>=x2 && x1<x2+r2.length) || (x2>=x1 && x2<x1+r1.length)) {
                if(s != l1) {
                    map[l1].parent = r1.parent = r2.parent;
                    s = l1;
                } else {
                    // find terminal roots of each path
                    n = r1.parent;
                    while(n != map[n].parent) n = map[n].parent;
                    p = r2.parent;
                    while(p != map[p].parent) p = map[p].parent;

                    // must use smaller of two to preserve DAGness!
                    if(n < p) {
                        map[p].parent = n;
                    } else {
                        map[n].parent = p;
                    }
                }
            }
        }

        // Move to next point where values may change
        if(x1+r1.length < x2+r2.length) {
            x1 += r1.length;
            r1 = map[++l1];
        } else {
            x2 += r2.length;
            r2 = map[++l2];
        }
    }

    // Now we need to compress all parent paths
    for(i=0; i<num; i++) {
        p = map[i].parent;
        if(p > i) {
            while(p != map[p].parent) p = map[p].parent;
            map[i].parent = p;
        } else {
            map[i].parent = map[p].parent;
        }
    }

    // Ouch, my brain hurts.
}

int Blobber::extractBlobs(Blob* restrict reg,ColorRun* restrict runMap,int num)
// Takes the list of runs and formats them into a blob table,
// gathering the various statistics we want along the way.
// num is the number of runs in the runMap array, and the number of
// unique blobs in reg[] (< BLOBBER_MAX_REGIONS) is returned.
// Implemented as a single pass over the array of runs.
{
    int x,y,i;
    int b,n,a;
    ColorRun r;
    FormatYUV black = {0,0,0};

    x = y = n = 0;
    for(i=0; i<num; i++) {
        r = runMap[i];

        if(r.color) {
            if(r.parent == i) {
                // Add new blob if this run is a root (i.e. self parented)
                runMap[i].parent = b = n;  // renumber to point to blob id
                reg[b].color = bottomBit(r.color) - 1;
                reg[b].area = r.length;
                reg[b].x1 = x;
                reg[b].y1 = y;
                reg[b].x2 = x + r.length;
                reg[b].y2 = y;
                reg[b].sumX = rangeSum(x,r.length);
                reg[b].sumY = y * r.length;
                reg[b].average = black;
                // reg[b].area_check = 0; // DEBUG ONLY
                n++;
                if(n >= BLOBBER_MAX_REGIONS) return(BLOBBER_MAX_REGIONS);
            } else {
                // Otherwise update blob stats incrementally
                b = runMap[r.parent].parent;
                runMap[i].parent = b; // update to point to blob id
                reg[b].area += r.length;
                reg[b].x2 = max(x + r.length,reg[b].x2);
                reg[b].x1 = min(x,reg[b].x1);
                reg[b].y2 = y; // last set by lowest run
                reg[b].sumX += rangeSum(x,r.length);
                reg[b].sumY += y * r.length;
            }
            /* DEBUG
            if(r.color == 1){
              printf("{%d,%d,%d} ",i,runMap[i].parent,b);
            }
            */
        }

        // step to next location
        x = (x + r.length) % width;
        y += (x == 0);
    }

    // printf("\n");

    // calculate centroids from stored temporaries
    for(i=0; i<n; i++) {
        a = reg[i].area;
        reg[i].centerX = (float)reg[i].sumX / a;
        reg[i].centerY = (float)reg[i].sumY / a;
    }

    return(n);
}

void Blobber::calculateAverageColors(Blob* restrict reg,int blobCount,
                                Pixel* restrict img,
                                ColorRun* restrict runMap,int runCount)
// calculates the average color for each blob.
// num is the number of runs in the runMap array, and the number of
// unique blobs in reg[] (< BLOBBER_MAX_REGIONS) is returned.
// Implemented as a single pass over the image, and a second pass over
// the blobs.
{
    int i,j,x,l;
    Pixel p;
    ColorRun r;
    int sumY,sum_u,sum_v;
    int b,xs;

    FormatYUV avg;
    int area;

    // clear out temporaries
    for(i=0; i<blobCount; i++) {
        reg[i].sumX = 0;
        reg[i].sumY = 0;
        reg[i].sumZ = 0;
    }

    x = 0;

    // printf("FRAME_START\n");

    // sum up color components for each blob, by traversing image and runs
    for(i=0; i<runCount; i++) {
        r = runMap[i];
        l = r.length;

        if(!r.color) {
            x += l;
        } else {
            xs = x;
            p = img[x / 2];

            if(x & 1) {
                sumY = p.y2;
                sum_u = p.u;
                sum_v = p.v;
                // area = 1;
                x++;
                l--;
            } else {
                sumY = sum_u = sum_v = 0;
                area = 0;
            }

            for(j=0; j<l/2; j++) {
                p = img[x / 2];
                sumY += p.y1 + p.y2;
                sum_u += 2 * p.u;
                sum_v += 2 * p.v;
                x+=2;
                // area += 2;
            }

            if(l & 1) {
                x++;
                p = img[x / 2];
                sumY += p.y1;
                sum_u += p.u;
                sum_v += p.v;
                // area++;
            }

            // add sums to blob
            b = r.parent;
            reg[b].sumX += sumY;
            reg[b].sumY += sum_u;
            reg[b].sumZ += sum_v;
            // reg[b].area_check += area;

            /*
            if((r.color & (1 << reg[b].color)) != (1 << reg[b].color)){
              printf("(%d,%d)",r.color,reg[b].color);
            }

            if(x != xs + r.length){
            	printf("Length mismatch %d:%d\n",x,xs + r.length);
            }
            */

            x = xs + r.length;
        }
    }

    // Divide sums by area to calculate average colors
    for(i=0; i<blobCount; i++) {
        area = reg[i].area;
        avg.y = reg[i].sumX / area;
        avg.u = reg[i].sumY / area;
        avg.v = reg[i].sumZ / area;

        /*
        if(reg[i].area != reg[i].area_check){
          printf("Area Mismatch: %d %d\n",reg[i].area,reg[i].area_check);
        }

        x = (yClass[avg.y] & uClass[avg.u] & vClass[avg.v]);
        j = reg[i].color;
        l = (1 << j);
        if((x & l) != l){
          printf("Error: c=%d a=%d (%d,%d) (%d,%d,%d)\n",
             reg[i].color,area,
             (int)reg[i].centerX,(int)reg[i].centerY,
                 avg.y,avg.u,avg.v);
        }
        */

        reg[i].average = avg;
    }
}

int Blobber::separateBlobs(Blob* restrict reg,int num)
// Splits the various blobs in the blob table a separate list
// for each color.  The lists are threaded through the table using
// the blob's 'next' field.  Returns the maximal area of the
// blobs, which we use below to speed up sorting.
{
    Blob* p;
    int i,l;
    int area,maxArea;

    // clear out the blob table
    for(i=0; i<BLOBBER_MAX_COLORS; i++) {
        blobCount[i] = 0;
        blobList[i] = NULL;
    }

    // step over the table, adding successive
    // blobs to the front of each list
    maxArea = 0;
    for(i=0; i<num; i++) {
        p = &reg[i];
        area = p->area;
        if(area >= BLOBBER_MIN_AREA) {
            if(area > maxArea) maxArea = area;
            l = p->color;
            blobCount[l]++;
            p->next = blobList[l];
            blobList[l] = p;
        }
    }

    return(maxArea);
}

// These are the tweaking values for the radix sort given below
// Feel free to change them, though these values seemed to work well
// in testing.  Don't worry about extra passes to get all 32 bits of
// the area; the implementation only does as many passes as needed to
// touch the most significant set bit (MSB of biggest blob's area)
#define BLOBBER_RBITS 6
#define BLOBBER_RADIX (1 << BLOBBER_RBITS)
#define BLOBBER_RMASK (BLOBBER_RADIX-1)

Blobber::Blob* Blobber::sortBlobListByArea(Blob* restrict list,int passes)
// Sorts a list of blobs by their area field.
// Uses a linked list based radix sort to process the list.
{
    Blob* tbl[BLOBBER_RADIX],*p,*pn;
    int slot,shift;
    int i,j;

    // Handle trivial cases
    if(!list || !list->next) return(list);

    // Initialize table
    for(j=0; j<BLOBBER_RADIX; j++) tbl[j] = NULL;

    for(i=0; i<passes; i++) {
        // split list into buckets
        shift = BLOBBER_RBITS * i;
        p = list;
        while(p) {
            pn = p->next;
            slot = ((p->area) >> shift) & BLOBBER_RMASK;
            p->next = tbl[slot];
            tbl[slot] = p;
            p = pn;
        }

        // integrate back into partially ordered list
        list = NULL;
        for(j=0; j<BLOBBER_RADIX; j++) {
            p = tbl[j];
            tbl[j] = NULL;  // clear out table for next pass
            while(p) {
                pn = p->next;
                p->next = list;
                list = p;
                p = pn;
            }
        }
    }

    return(list);
}

void Blobber::sortBlobs(int maxArea)
// Sorts entire blob table by area, using the above
// function to sort each threaded blob list.
{
    int i,p;

    // do minimal number of passes sufficient to touch all set bits
    p = topBit((maxArea + BLOBBER_RBITS-1) / BLOBBER_RBITS);

    // sort each list
    for(i=0; i<BLOBBER_MAX_COLORS; i++) {
        blobList[i] = sortBlobListByArea(blobList[i],p);
    }
}

int Blobber::mergeBlobs(Blob* p,int num,double densityThreshold)
// Looks through blobs and merges pairs of the same color that would
// have a high density after combining them (where density is the area
// in pixels of the blob divided by the bounding box area).  This
// implementation sucks, and I promise real spatial data structures in
// the future so n^2 ugliness like this is not necessary.
{
    Blob* q,*s;
    int l,r,t,b;
    int a;
    int merged;

    merged = 0;

    while(p && merged<num) {
        q = p->next;
        s = p;

        while(q) {
            // find union box and get its total area
            l = min(p->x1,q->x1);
            r = max(p->x2,q->x2);
            t = min(p->y1,q->y1);
            b = max(p->y2,q->y2);
            a = (r-l) * (b-t);

            // if density of merged blob is still above threshold
            if((double)(p->area + q->area) / a > densityThreshold) {
                // merge them to create a new blob
                a = p->area + q->area;
                p->x1 = l;
                p->x2 = r;
                p->y1 = t;
                p->y2 = b;
                p->centerX = ((p->centerX * p->area) + (q->centerX * q->area)) / a;
                p->centerY = ((p->centerY * p->area) + (q->centerY * q->area)) / a;
                p->area = a;

                // remove q from list (old smaller blob)
                q = q->next;
                s->next = q;
                merged++;
            } else {
                s = q;
                q = q->next;
            }
        }
        p = p->next;
    }

    return(merged);
}

int Blobber::mergeBlobs()
// Apply merge operation to all blobs using the above function.
{
    int i,m;
    int num;

    num = 0;

    for(i=0; i<BLOBBER_MAX_COLORS; i++) {
        m = mergeBlobs(blobList[i],colors[i].expectedBlobs,colors[i].mergeThreshold);
        blobCount[i] -= m;
        num += m;
    }

    return(num);
}

//==== Interface/Public Functions ==================================//

#define ZERO(x) memset(x,0,sizeof(x))

void Blobber::clear() {
    ZERO(yClass);
    ZERO(uClass);
    ZERO(vClass);

    ZERO(blobList);
    ZERO(blobCount);

    ZERO(colors);

    map = NULL;
}

bool Blobber::initialize(int width, int height) {
    this->width = width;
    this->height = height;

    if (map) {
        delete map;
    }

    // need 1 extra element to store terminator value in encodeRuns()
    map = new unsigned[width * height + 1];

    options = BLOBBER_THRESHOLD;

    for(int i=0; i<BLOBBER_COLOR_LEVELS; i++) {
        yClass[i] = uClass[i] = vClass[i] = 0;
    }

    for(int i=0; i<BLOBBER_MAX_COLORS; i++) {
        if(colors[i].name) {
            delete(colors[i].name);
            colors[i].name = NULL;
        }
    }

    colorCount = 0;

    return map != NULL;
}

#define BLOBBER_STATE_SCAN   0
#define BLOBBER_STATE_COLORS 1
#define BLOBBER_STATE_THRESH 2
#define BLOBBER_MAX_BUF 256

bool Blobber::loadOptions(std::string filename)
// Loads in options file specifying color names and representative
// rgb triplets.  Also loads in color class threshold values.
{
    char buf[BLOBBER_MAX_BUF],str[BLOBBER_MAX_BUF];
    FILE* in;
    int state,i,n;

    int r,g,b;
    int exp_num;
    double merge;
    Color* c;

    int y1,y2,u1,u2,v1,v2;
    unsigned k;

    // Open options file
    in = fopen(filename.c_str(),"rt");
    if(!in) return(false);

    // Clear out previously set options
    for(i=0; i<BLOBBER_COLOR_LEVELS; i++) {
        yClass[i] = uClass[i] = vClass[i] = 0;
    }
    for(i=0; i<BLOBBER_MAX_COLORS; i++) {
        if(colors[i].name) {
            delete(colors[i].name);
            colors[i].name = NULL;
        }
    }

    colorCount = 0;

    // Loop ever lines, processing via a simple parser
    state = 0;
    while(fgets(buf,BLOBBER_MAX_BUF,in)) {
        switch(state) {
        case BLOBBER_STATE_SCAN:
            n = sscanf(buf,"[%s",str);
            if(n == 1) {
                if(!strncasecmp(str,"colors]",BLOBBER_MAX_BUF)) {
                    state = BLOBBER_STATE_COLORS;
                    i = 0;
                } else if(!strncasecmp(str,"thresholds]",BLOBBER_MAX_BUF)) {
                    state = BLOBBER_STATE_THRESH;
                    i = 0;
                } else {
                    printf("Blobber: Ignoring unknown option header '%s'.\n",str);
                }
            }
            break;
        case BLOBBER_STATE_COLORS:
            n = sscanf(buf,"(%d,%d,%d) %lf %d %s",&r,&g,&b,&merge,&exp_num,str);
            if(n == 6) {
                // printf("(%d,%d,%d) %lf %d '%s'\n",
                //        r,g,b,merge,exp_num,str); fflush(stdout);
                if(i < BLOBBER_MAX_COLORS) {
                    c = &colors[i];
                    c->color.red   = r;
                    c->color.green = g;
                    c->color.blue  = b;
                    c->name  = strdup(str);
                    c->mergeThreshold = merge;
                    c->expectedBlobs = exp_num;
                    i++;
                    colorCount++;
                } else {
                    printf("Blobber: Too many colors, ignoring '%s'.\n",str);
                }
            } else if(n == 0) {
                state = BLOBBER_STATE_SCAN;
            }
            break;
        case BLOBBER_STATE_THRESH:
            n = sscanf(buf,"(%d:%d,%d:%d,%d:%d)",&y1,&y2,&u1,&u2,&v1,&v2);
            if(n == 6) {
                // printf("(%d:%d,%d:%d,%d:%d)\n",y1,y2,u1,u2,v1,v2);
                if(i < BLOBBER_MAX_COLORS) {
                    c = &colors[i];
                    c->yLow = y1;
                    c->yHigh = y2;
                    c->uLow = u1;
                    c->uHigh = u2;
                    c->vLow = v1;
                    c->vHigh = v2;

                    k = (1 << i);
                    setBits(yClass,BLOBBER_COLOR_LEVELS,y1,y2,k);
                    setBits(uClass,BLOBBER_COLOR_LEVELS,u1,u2,k);
                    setBits(vClass,BLOBBER_COLOR_LEVELS,v1,v2,k);
                    i++;
                } else {
                    printf("Blobber: Too many thresholds.\n");
                }
            } else if(n == 0) {
                state = BLOBBER_STATE_SCAN;
            }
            break;
        }
    }

    /*
    for(i=0; i<BLOBBER_COLOR_LEVELS; i++){
      printf("%08X %08X %08X\n",yClass[i],uClass[i],vClass[i]);
    }
    */

    fclose(in);

    return(true);
}

bool Blobber::saveOptions(std::string filename) {
    Color* c;
    FILE* out;
    int i;

    out = fopen(filename.c_str(),"wt");
    if(!out) return(false);

    fprintf(out,"[Colors]\n");
    i = 0;
    while(colors[i].name) {
        c = &colors[i];
        fprintf(out,"(%3d,%3d,%3d) %6.4lf %d %s\n",
                c->color.red,c->color.green,c->color.blue,
                c->mergeThreshold,c->expectedBlobs,c->name);
        i++;
    }

    fprintf(out,"\n[Thresholds]\n");
    i = 0;
    while(colors[i].name) {
        c = &colors[i];
        fprintf(out,"(%3d:%3d,%3d:%3d,%3d:%3d)\n",
                c->yLow,c->yHigh,
                c->uLow,c->uHigh,
                c->vLow,c->vHigh);
        i++;
    }

    fclose(out);

    return(true);
}

bool Blobber::enable(unsigned opt) {
    unsigned valid;

    valid = opt & BLOBBER_VALID_OPTIONS;
    options |= valid;

    return(opt == valid);
}

bool Blobber::disable(unsigned opt) {
    unsigned valid;

    valid = opt & BLOBBER_VALID_OPTIONS;
    options &= ~valid;

    return(opt == valid);
}

void Blobber::close() {
    if(map) delete(map);
    map = NULL;
}


//==== Vision Testing Functions ====================================//

bool Blobber::classify(Rgb* restrict out,Pixel* restrict image) {
    int i,s;
    Rgb black(0,0,0);

    if(!image || !out) return(false);

    classifyFrame(image,map);

    s = width * height;

    i = 0;
    while(i < s) {
        if (map[i] == 0) {
            out[i] = black;
        } else {
            out[i] = colors[bottomBit(map[i])-1].color;
        }

        i++;
        /*
        while(i<s && !map[i]){
          out[i] = black;
          i++;
        }
        while(i<s && map[i]){
          out[i] = colors[bottomBit(map[i])-1].color;
          i++;
        }
        */
    }

    return(true);
}

void Blobber::addColor(
    std::string name,
    int red, int green, int blue,
    int yLow, int yHigh,
    int uLow, int uHigh,
    int vLow, int vHigh,
    double mergeThreshold,
    int expectedBlobs
) {
    unsigned k = (1 << colorCount);

    clearBits(yClass, BLOBBER_COLOR_LEVELS, yLow, yHigh, k);
    clearBits(uClass, BLOBBER_COLOR_LEVELS, uLow, uHigh, k);
    clearBits(vClass, BLOBBER_COLOR_LEVELS, vLow, vHigh, k);

    colors[colorCount].color.red = red;
    colors[colorCount].color.green = green;
    colors[colorCount].color.blue = blue;
    colors[colorCount].name = strdup(name.c_str());
    colors[colorCount].mergeThreshold = mergeThreshold;
    colors[colorCount].expectedBlobs = expectedBlobs;
    colors[colorCount].yLow = yLow;
    colors[colorCount].yHigh = yHigh;
    colors[colorCount].uLow = uLow;
    colors[colorCount].uHigh = uHigh;
    colors[colorCount].vLow = vLow;
    colors[colorCount].vHigh = vHigh;

    setBits(yClass, BLOBBER_COLOR_LEVELS, yLow, yHigh, k);
    setBits(uClass, BLOBBER_COLOR_LEVELS, uLow, uHigh, k);
    setBits(vClass, BLOBBER_COLOR_LEVELS, vLow, vHigh, k);

    colorCount++;
}

bool Blobber::getThreshold(int color,
                           int& yLow,int& yHigh,
                           int& uLow,int& uHigh,
                           int& vLow,int& vHigh) {
    Color* c;

    if(color<0 || color>=BLOBBER_MAX_COLORS) return(false);

    c = &colors[color];
    yLow = c->yLow;
    yHigh = c->yHigh;
    uLow = c->uLow;
    uHigh = c->uHigh;
    vLow = c->vLow;
    vHigh = c->vHigh;

    return(true);
}

bool Blobber::setThreshold(
    int color,
    int yLow,int yHigh,
    int uLow,int uHigh,
    int vLow,int vHigh
) {
    Color* c;
    unsigned k;

    if(color<0 || color>=BLOBBER_MAX_COLORS) return(false);

    c = &colors[color];
    k = 1 << color;

    clearBits(yClass,BLOBBER_COLOR_LEVELS,c->yLow,c->yHigh,k);
    clearBits(uClass,BLOBBER_COLOR_LEVELS,c->uLow,c->uHigh,k);
    clearBits(vClass,BLOBBER_COLOR_LEVELS,c->vLow,c->vHigh,k);

    c->yLow = yLow;
    c->yHigh = yHigh;
    c->uLow = uLow;
    c->uHigh = uHigh;
    c->vLow = vLow;
    c->vHigh = vHigh;

    setBits(yClass,BLOBBER_COLOR_LEVELS,yLow,yHigh,k);
    setBits(uClass,BLOBBER_COLOR_LEVELS,uLow,uHigh,k);
    setBits(vClass,BLOBBER_COLOR_LEVELS,vLow,vHigh,k);

    return(true);
}

//==== Main Vision Functions =======================================//

bool Blobber::processFrame(Pixel* image) {
    int runs;
    int blobs;
    int maxArea;

    if(!image) return(false);

    if(options & BLOBBER_THRESHOLD) {

        classifyFrame(image,map);

        runs = encodeRuns(runMap,map);
        connectComponents(runMap,runs);

        blobs = extractBlobs(blobTable,runMap,runs);

        if(options & BLOBBER_COLOR_AVERAGES) {
            calculateAverageColors(blobTable,blobs,image,runMap,runs);
        }

        maxArea = separateBlobs(blobTable,blobs);
        sortBlobs(maxArea);

        if(options & BLOBBER_DENSITY_MERGE) {
            mergeBlobs();
        }
    }

    return(true);
}

bool Blobber::processFrame(unsigned* map) {
    int runs;
    int blobs;
    int maxArea;

    if(!map) return(false);

    runs = encodeRuns(runMap,map);
    connectComponents(runMap,runs);

    blobs = extractBlobs(blobTable,runMap,runs);

    // if(options & BLOBBER_COLOR_AVERAGES){
    //   calculateAverageColors(blobTable,blobs,image,runMap,runs);
    // }

    maxArea = separateBlobs(blobTable,blobs);
    sortBlobs(maxArea);

    if(options & BLOBBER_DENSITY_MERGE) {
        mergeBlobs();
    }

    return(true);
}

int Blobber::getBlobCount(int colorId) {
    if(colorId<0 || colorId>=BLOBBER_MAX_COLORS) return(BLOBBER_NONE);
    return(blobCount[colorId]);
}

Blobber::Blob* Blobber::getBlobs(int colorId) {
    if(colorId<0 || colorId>=BLOBBER_MAX_COLORS) return(NULL);
    return(blobList[colorId]);
}
