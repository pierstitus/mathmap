/* -*- c -*- */

/*
 * convolve.c
 *
 * MathMap
 *
 * Copyright (C) 2008 Mark Probst
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <complex.h>
#include <fftw3.h>

#include "../drawable.h"
#include "../lispreader/pools.h"

#include "native-filters.h"

static void
copy (double *dest, float *src, int n)
{
    int i;

    for (i = 0; i < n; ++i)
	dest[i] = src[i * 4];
}

static double
copy_and_add (double *dest, float *src, int n)
{
    int half;

    if (n <= 0)
	return 0.0;
    if (n == 1)
	return dest[0] = src[0];
    if (n == 2)
    {
	double d1, d2;

	d1 = dest[0] = src[0];
	d2 = dest[1] = src[NUM_FLOATMAP_CHANNELS];

	return d1 + d2;
    }

    half = n / 2;
    return copy_and_add(dest, src, half)
	+ copy_and_add(dest + half, src + half * NUM_FLOATMAP_CHANNELS, n - half);
}

CALLBACK_SYMBOL
image_t*
native_filter_convolve (mathmap_invocation_t *invocation, userval_t *args, pools_t *pools)
{
    image_t *in_image = args[0].v.image;
    image_t *filter_image = args[1].v.image;
    gboolean normalize = args[2].v.bool_const != 0.0;
    image_t *out_image;
    double *fftw_in;
    fftw_complex *image_out, *filter_out;
    fftw_plan in_plan, filter_plan, inverse_plan;
    int i, n, nhalf, cn, channel;

    if (in_image->type != IMAGE_FLOATMAP)
	in_image = render_image(invocation, in_image,
				invocation->render_width, invocation->render_height, pools, TRUE);
    if (filter_image->type != IMAGE_FLOATMAP
	|| filter_image->pixel_width != in_image->pixel_width
	|| filter_image->pixel_height != in_image->pixel_height)
	filter_image = render_image(invocation, filter_image,
				    in_image->pixel_width, in_image->pixel_height, pools, TRUE);

    out_image = floatmap_alloc(in_image->pixel_width, in_image->pixel_height, pools);

    n = in_image->pixel_height * in_image->pixel_width;
    nhalf = in_image->pixel_width * (in_image->pixel_height / 2) + in_image->pixel_width / 2;
    cn = in_image->pixel_height * (in_image->pixel_width / 2 + 1);

    fftw_in = fftw_malloc(sizeof(double) * n);
    image_out = fftw_malloc(sizeof(fftw_complex) * cn);
    filter_out = fftw_malloc(sizeof(fftw_complex) * cn);

    in_plan = fftw_plan_dft_r2c_2d(in_image->pixel_height, in_image->pixel_width,
				    fftw_in, image_out,
				    FFTW_ESTIMATE);
    filter_plan = fftw_plan_dft_r2c_2d(in_image->pixel_height, in_image->pixel_width,
					fftw_in, filter_out,
					FFTW_ESTIMATE);
    inverse_plan = fftw_plan_dft_c2r_2d(in_image->pixel_height, in_image->pixel_width,
					 image_out, fftw_in,
					 FFTW_ESTIMATE);

    for (channel = 0; channel < 3; ++channel)
    {
	/* FFT of input image */
	for (i = 0; i < n; ++i)
	    fftw_in[i] = in_image->v.floatmap.data[i * 4 + channel];
	fftw_execute(in_plan);

	/* FFT of kernel image */
	if (normalize)
	{
	    double d1 = copy_and_add(fftw_in,
				     filter_image->v.floatmap.data + channel + (n - nhalf) * NUM_FLOATMAP_CHANNELS,
				     nhalf);
	    double d2 = copy_and_add(fftw_in + nhalf,
				     filter_image->v.floatmap.data + channel,
				     n - nhalf);
	    double factor = 1.0 / (d1 + d2);

	    for (i = 0; i < n; ++i)
		fftw_in[i] *= factor;
	}
	else
	{
	    copy(fftw_in, filter_image->v.floatmap.data + channel + (n - nhalf) * NUM_FLOATMAP_CHANNELS, nhalf);
	    copy(fftw_in + nhalf, filter_image->v.floatmap.data + channel, n - nhalf);
	}
	fftw_execute(filter_plan);

	// multiply in frequency domain
	for (i = 0; i < cn; ++i)
	    image_out[i] *= filter_out[i];

	// reverse FFT
	fftw_execute(inverse_plan);
	for (i = 0; i < n; ++i)
	    out_image->v.floatmap.data[i * 4 + channel] = fftw_in[i] / n;
    }

    /* copy alpha channel */
    for (i = 0; i < n; ++i)
	out_image->v.floatmap.data[i * 4 + 3] = in_image->v.floatmap.data[i * 4 + 3];

    fftw_destroy_plan(in_plan);
    fftw_destroy_plan(filter_plan);
    fftw_destroy_plan(inverse_plan);

    fftw_free(fftw_in);
    fftw_free(image_out);
    fftw_free(filter_out);

    return out_image;
}
