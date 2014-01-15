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

#ifndef ILD_H
#define ILD_H

#include <stdint.h>
#include "ff.h"

typedef enum {FRAME_3D = 0, FRAME_2D = 1} frameType_t;

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t color;
} IldaPoint;

typedef struct {
	int totalPoints;
	int loadedStartPoint;
	int loadedPointCount;

	int frameNumber;
	int maxFrames;
	int displayCount;

	frameType_t frameType;
} IldaFile;

typedef enum{
	FRAME_LOADED, // Frame loaded, but possibly not totally. Check IldaFile struct
	FRAME_SKIPPED, // Frame skipped. Try again
	FILE_ERROR // Formatting error. File cannot be processed further
} loadIldaFrameResult_t;

loadIldaFrameResult_t olLoadIldaPoints(FIL* fd, IldaFile* ild, IldaPoint* points, int maxPoints);
loadIldaFrameResult_t olLoadIldaFrame(FIL* fd, IldaFile* ild, IldaPoint* points, int maxPoints);

#endif
