/*
 *    rotate.c
 *
 *    Module for handling image rotation.
 *
 *    Copyright 2004-2005, Per Jonsson (per@pjd.nu)
 *
 *    This software is distributed under the GNU Public license
 *    Version 2.  See also the file 'COPYING'.
 *
 *    Image rotation is a feature of Motion that can be used when the
 *    camera is mounted upside-down or on the side. The module only
 *    supports rotation in multiples of 90 degrees. Using rotation
 *    increases the Motion CPU usage slightly.
 *
 *    Version history:
 *      v6 (29-Aug-2005) - simplified the code as Motion now requires
 *                         that width and height are multiples of 16
 *      v5 (3-Aug-2005)  - cleanup in code comments
 *                       - better adherence to coding standard
 *                       - fix for __bswap_32 macro collision
 *                       - fixed bug where initialization would be
 *                         incomplete for invalid degrees of rotation
 *                       - now uses MOTION_LOG for error reporting
 *      v4 (26-Oct-2004) - new fix for width/height from imgs/conf due to
 *                         earlier misinterpretation
 *      v3 (11-Oct-2004) - cleanup of width/height from imgs/conf
 *      v2 (26-Sep-2004) - separation of capture/internal dimensions
 *                       - speed optimization, including bswap
 *      v1 (28-Aug-2004) - initial version
 */
#include "ffmpeg_adapter_rotate.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  /* Swap bytes in 32 bit value. This is used as a fallback and for constants. */
#define rot__bswap_constant_32(x)                               \
  ((((x)& 0xff000000) >> 24) | (((x)& 0x00ff0000) >> 8) | \
  (((x)& 0x0000ff00) << 8) | (((x)& 0x000000ff) << 24))


  /* Finally define a macro with a more appropriate name, to be used below. */
#define swap_bytes(x) rot__bswap_constant_32(x)

  /**
   * reverse_inplace_quad
   *
   *  Reverses a block of memory in-place, 4 bytes at a time. This function
   *  requires the __uint32 type, which is 32 bits wide.
   *
   * Parameters:
   *
   *   src  - the memory block to reverse
   *   size - the size (in bytes) of the memory block
   *
   * Returns: nothing
   */
  static void reverse_inplace_quad(unsigned char *src, int size)
  {
    uint32_t *nsrc = (uint32_t *)src;              /* first quad */
    uint32_t *ndst = (uint32_t *)(src + size - 4); /* last quad */
    register uint32_t tmp;

    while (nsrc < ndst) {
      tmp = swap_bytes(*ndst);
      *ndst-- = swap_bytes(*nsrc);
      *nsrc++ = tmp;
    }
  }

  /**
   * rot90cw
   *
   *  Performs a 90 degrees clockwise rotation of the memory block pointed to
   *  by src. The rotation is NOT performed in-place; dst must point to a
   *  receiving memory block the same size as src.
   *
   * Parameters:
   *
   *   src    - pointer to the memory block (image) to rotate clockwise
   *   dst    - where to put the rotated memory block
   *   size   - the size (in bytes) of the memory blocks (both src and dst)
   *   width  - the width of the memory block when seen as an image
   *   height - the height of the memory block when seen as an image
   *
   * Returns: nothing
   */
  static void rot90cw(unsigned char *src, register unsigned char *dst, int size,
    int width, int height)
  {
    unsigned char *endp;
    register unsigned char *base;
    int j;

    endp = src + size;
    for (base = endp - width; base < endp; base++) {
      src = base;
      for (j = 0; j < height; j++, src -= width)
        *dst++ = *src;

    }
  }

  /**
   * rot90ccw
   *
   *  Performs a 90 degrees counterclockwise rotation of the memory block pointed
   *  to by src. The rotation is not performed in-place; dst must point to a
   *  receiving memory block the same size as src.
   *
   * Parameters:
   *
   *   src    - pointer to the memory block (image) to rotate counterclockwise
   *   dst    - where to put the rotated memory block
   *   size   - the size (in bytes) of the memory blocks (both src and dst)
   *   width  - the width of the memory block when seen as an image
   *   height - the height of the memory block when seen as an image
   *
   * Returns: nothing
   */
  static inline void rot90ccw(unsigned char *src, register unsigned char *dst,
    int size, int width, int height)
  {
    unsigned char *endp;
    register unsigned char *base;
    int j;

    endp = src + size;
    dst = dst + size - 1;
    for (base = endp - width; base < endp; base++) {
      src = base;
      for (j = 0; j < height; j++, src -= width)
        *dst-- = *src;

    }
  }

  int YUV420P_rotate(unsigned char *src, unsigned char* dst, int deg, int width, int height)
  {
    /*
     * The image format is either YUV 4:2:0 planar, in which case the pixel
     * data is divided in three parts:
     *    Y - width x height bytes
     *    U - width x height / 4 bytes
     *    V - as U
     * or, it is in greyscale, in which case the pixel data simply consists
     * of width x height bytes.
     */
    int wh, wh4 = 0, w2 = 0, h2 = 0;  /* width * height, width * height / 4 etc. */
    int size;

    /*
     * Pre-calculate some stuff:
     *  wh   - size of the Y plane, or the entire greyscale image
     *  size - size of the entire memory block
     *  wh4  - size of the U plane, and the V plane
     *  w2   - width of the U plane, and the V plane
     *  h2   - as w2, but height instead
     */
    wh = width * height;
    //if (YUV420P) {
    size = wh * 3 / 2;
    wh4 = wh / 4;
    w2 = width / 2;
    h2 = height / 2;
    //}

    switch (deg) {
    case 90:
      /* First do the Y part */
      rot90cw(src, dst, wh, width, height);
      //if (YUV420P) {
      /* Then do U and V */
      rot90cw(src + wh, dst + wh, wh4, w2, h2);
      rot90cw(src + wh + wh4, dst + wh + wh4,
        wh4, w2, h2);
      //}

      /* Then copy back from the temp buffer to src. */
      memcpy(src, dst, size);
      break;

    case 180:
      /*
       * 180 degrees is easy - just reverse the data within
       * Y, U and V.
       */
      reverse_inplace_quad(src, wh);
      //if (YUV420P) {
      reverse_inplace_quad(src + wh, wh4);
      reverse_inplace_quad(src + wh + wh4, wh4);
      //}
      break;

    case 270:

      /* First do the Y part */
      rot90ccw(src, dst, wh, width, height);
      //if (YUV420P) {
      /* Then do U and V */
      rot90ccw(src + wh, dst + wh, wh4, w2, h2);
      rot90ccw(src + wh + wh4, dst + wh + wh4,
        wh4, w2, h2);
      //}

      /* Then copy back from the temp buffer to src. */
      memcpy(src, dst, size);
      break;

    default:
      /* Invalid */
      return -1;
    }

    return 0;
  }


  int YUV420SP_rotate(unsigned char *src, unsigned char* dst, int deg, int width, int height)
  {
	  /*
	  * The image format is either YUV 4:2:0 planar, in which case the pixel
	  * data is divided in three parts:
	  *    Y - width x height bytes
	  *    U - width x height / 4 bytes
	  *    V - as U
	  * or, it is in greyscale, in which case the pixel data simply consists
	  * of width x height bytes.
	  */
	  int wh, wh4 = 0, w2 = 0, h2 = 0;  /* width * height, width * height / 4 etc. */
    int i, k, j, a, b;
	  int size;

	  /*
	  * Pre-calculate some stuff:
	  *  wh   - size of the Y plane, or the entire greyscale image
	  *  size - size of the entire memory block
	  *  wh4  - size of the U plane, and the V plane
	  *  w2   - width of the U plane, and the V plane
	  *  h2   - as w2, but height instead
	  */
	  wh = width * height;
	  //if (YUV420P) {
	  size = wh * 3 / 2;
	  wh4 = wh / 4;
	  w2 = width / 2;
	  h2 = height / 2;
	  //}

	  switch (deg) {
	  case 90:
		  /* First do the Y part */
		  rot90cw(src, dst, wh, width, height);
		  //if (YUV420P) {
		  /* Then do U and V */
		  //    rot90cw(src + wh, dst + wh, wh4, w2, h2);
		  //   rot90cw(src + wh + wh4, dst + wh + wh4,
		  //     wh4, w2, h2);
		  //}

		  for (i = 0, j = height + h2 - 1; i < height; i += 2, j--) {
			  for (k = 0, a = i + wh, b = j * width; k < w2; k++, a += height, b += 2) {
				  dst[a] = src[b];
				  dst[a + 1] = src[b + 1];
			  }
		  }

		  /* Then copy back from the temp buffer to src. */
		  // memcpy(src, dst, size);
		  break;

	  case 180:
		  /*
		  * 180 degrees is easy - just reverse the data within
		  * Y, U and V.
		  */
		  reverse_inplace_quad(src, wh);
		  //if (YUV420P) {
		  reverse_inplace_quad(src + wh, wh4);
		  reverse_inplace_quad(src + wh + wh4, wh4);
		  //}
		  break;

	  case 270:

		  /* First do the Y part */
		  rot90ccw(src, dst, wh, width, height);
		  //if (YUV420P) {
		  /* Then do U and V */
		  //rot90ccw(src + wh, dst + wh, wh4, w2, h2);
		  //rot90ccw(src + wh + wh4, dst + wh + wh4,
		  //  wh4, w2, h2);
		  //}

		  for (i = 0, j = wh + width; i < height; i += 2, j += width) {
			  for (k = 0, a = i + wh, b = j - 2; k < w2; k++, a += height, b -= 2) {
				  dst[a] = src[b];
				  dst[a + 1] = src[b + 1];
			  }
		  }

		  /* Then copy back from the temp buffer to src. */
		  //  memcpy(src, dst, size);
		  break;

	  default:
		  /* Invalid */
		  return -1;
	  }

	  return 0;
  }


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
