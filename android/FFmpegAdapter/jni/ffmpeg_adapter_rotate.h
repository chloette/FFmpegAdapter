/*
 *    rotate.h
 *
 *    Include file for handling image rotation.
 *
 *    Copyright 2004-2005, Per Jonsson (per@pjd.nu)
 *
 *    This software is distributed under the GNU Public license
 *    Version 2.  See also the file 'COPYING'.
 */
#ifndef __INCLUDE_FFMPEG_ADAPTER_ROTATE_H__
#define __INCLUDE_FFMPEG_ADAPTER_ROTATE_H__

#include "ffmpeg_adapter_common.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  /**
   * yuv420p_rotate
   *
   *  Rotates the image stored in map according to the rotation data
   *  available in cnt. Rotation is performed clockwise. Supports 90,
   *  180 and 270 degrees rotation. 180 degrees rotation is performed
   *  in-place by simply reversing the image data, which is a very
   *  fast operation. 90 and 270 degrees rotation are performed using
   *  a temporary buffer and a somewhat more complicated algorithm,
   *  which makes them slower.
   *
   *  Note that to the caller, all rotations will seem as they are
   *  performed in-place.
   *
   * Parameters:
   *
   *   src - the image data to rotate
   *   dst - the image data to save target
   *   deg - the degrees to rotate
   *   width - width of src
   *   height - height of src
   *
   * Returns:
   *
   *   0  - success
   *   -1 - failure (rare, shouldn't happen)
   */
  int YUV420P_rotate(unsigned char *src, unsigned char* dst, int deg, int width, int height);

  int YUV420SP_rotate(unsigned char *src, unsigned char* dst, int deg, int width, int height);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__INCLUDE_FFMPEG_ADAPTER_ROTATE_H__
