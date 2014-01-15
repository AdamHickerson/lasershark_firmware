/*
        OpenLase - a realtime laser graphics toolkit

Copyright (C) 2009-2011 Hector Martin "marcan" <hector@marcansoft.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 or version 3.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//#include "libol.h"
#include "ilda.h"

#include <stdlib.h>
#include <string.h>

#if BYTE_ORDER == LITTLE_ENDIAN
static inline uint16_t swapshort(uint16_t v) {
	return (v >> 8) | (v << 8);
}
# define MAGIC 0x41444C49
#else
static inline uint16_t swapshort(uint16_t v) {
	return v;
}
# define MAGIC 0x494C4441
#endif

#include <stdint.h>

struct ilda_hdr {
	uint32_t magic;
	uint8_t pad1[3];
	uint8_t format;
	char name[8];
	char company[8];
	uint16_t count;
	uint16_t frameno;
	uint16_t framecount;
	uint8_t scanner;
	uint8_t pad2;
} __attribute__((packed));


struct icoord3d {
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t state;
	uint8_t color;
} __attribute__((packed));

struct icoord2d {
	int16_t x;
	int16_t y;
	uint8_t state;
	uint8_t color;
} __attribute__((packed));

loadIldaFrameResult_t olLoadIldaPoints(FIL* fd, IldaFile* ild, IldaPoint* points, int maxPoints){
	int i;
	unsigned int bytesRead;
	struct icoord2d tmp2d;
	struct icoord3d tmp3d;

	// Set the start point to the next one
	ild->loadedStartPoint = ild->loadedStartPoint + ild->loadedPointCount;
	ild->loadedPointCount = 0;

	for(i=0; i < maxPoints && ild->loadedStartPoint + ild->loadedPointCount < ild->totalPoints; i++){
		switch(ild->frameType){
		case FRAME_2D:
			if(f_read(fd, &tmp2d, sizeof(struct icoord2d), &bytesRead) != 0 || bytesRead != sizeof(struct icoord2d)){
				//olLog("ILDA: error while reading frame\n");
				return FILE_ERROR;
			}
			points[i].x = (int16_t)swapshort(tmp2d.x);
			points[i].y = (int16_t)swapshort(tmp2d.y);
			points[i].z = 0;
			points[i].color = tmp2d.color;
			if(tmp2d.state & 0x40){ // isBlank
				points[i].color = 0;
			}
			break;

		case FRAME_3D:
			if(f_read(fd, &tmp3d, sizeof(struct icoord3d), &bytesRead) != 0 || bytesRead != sizeof(struct icoord3d)){
				//olLog("ILDA: error while reading frame\n");
				return FILE_ERROR;
			}
			points[i].x = (int16_t)swapshort(tmp3d.x);
			points[i].y = (int16_t)swapshort(tmp3d.y);
			points[i].z = (int16_t)swapshort(tmp3d.z);
			points[i].color = tmp3d.color;
			if(tmp3d.state & 0x40){ // isBlank
				points[i].color = 0;
			}
			break;
		}

		ild->loadedPointCount++;
	}

	return FRAME_LOADED;
}

loadIldaFrameResult_t olLoadIldaFrame(FIL* fd, IldaFile* ild, IldaPoint* points, int maxPoints){
	unsigned int bytesRead;
	struct ilda_hdr hdr;

	memset(ild, 0, sizeof(*ild));

	{ // Only execute once. Frames are loaded call-by-call
		if (f_read(fd, &hdr, sizeof(hdr), &bytesRead) != 0 || bytesRead != sizeof(hdr)){
			//olLog("ILDA: error while reading header\n");
			return FILE_ERROR;
		}

		if (hdr.magic != MAGIC) {
			//olLog("ILDA: Invalid magic 0x%08x\n", hdr.magic);
			return FILE_ERROR;
		}

		hdr.count = swapshort(hdr.count);
		hdr.frameno = swapshort(hdr.frameno);
		hdr.framecount = swapshort(hdr.framecount);

		ild->frameNumber = hdr.frameno;
		ild->totalPoints = hdr.count;

		switch (hdr.format) {
		case 0:
			//olLog("ILD: Got 3D frame, %d points\n", hdr.count);
			ild->frameType = FRAME_3D;
			break;

		case 1:
			//olLog("Got 2D frame, %d points\n", hdr.count);
			ild->frameType = FRAME_2D;
			break;

		case 2:
			//olLog("ILDA: Got color palette section, %d entries\n", hdr.count);
			//olLog("ILDA: NOT SUPPORTED\n");
			return FILE_ERROR; // Can't skip unless we learn how to skip...
		}

		if(olLoadIldaPoints(fd, ild, points, maxPoints) == FILE_ERROR){
			return FILE_ERROR;
		}
	}

	return FRAME_LOADED;
}
/*
void olDrawIlda(IldaFile *ild)
{
	if (!ild)
		return;
	IldaPoint *p = ild->points;
	int i;
	olBegin(OL_POINTS);
	for (i = 0; i < ild->count; i++) {
		//olLog("%f %f %f %d\n", p->x, p->y, p->z, p->is_blank);
		if (p->is_blank)
			olVertex(p->x, p->y, C_BLACK);
		else
			olVertex(p->x, p->y, C_WHITE);
		p++;
	}
	olEnd();
}

void olDrawIlda3D(IldaFile *ild)
{
	if (!ild)
		return;
	IldaPoint *p = ild->points;
	int i;
	olBegin(OL_POINTS);
	for (i = 0; i < ild->count; i++) {
		if (p->is_blank)
			olVertex3(p->x, p->y, p->z, C_BLACK);
		else
			olVertex3(p->x, p->y, p->z, C_WHITE);
		p++;
	}
	olEnd();
}

void olFreeIlda(IldaFile *ild)
{
	if(ild->points)
		free(ild->points);
	free(ild);
}
*/
