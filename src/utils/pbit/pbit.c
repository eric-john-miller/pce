/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/utils/pbit/pbit.c                                        *
 * Created:     2012-01-31 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2012 Hampa Hug <hampa@hampa.ch>                          *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/


#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lib/getopt.h>

#include "pbit.h"

#include <drivers/pfdc/pfdc-img-pfdc.h>
#include <drivers/pfdc/pfdc.h>

#include <drivers/pbit/pbit.h>
#include <drivers/pbit/pbit-io.h>
#include <drivers/pbit/gcr-mac.h>
#include <drivers/pbit/mfm-ibm.h>


static const char    *arg0 = NULL;

static int           par_verbose = 0;

static int           par_list = 0;
static int           par_print_info = 0;

static unsigned      par_fmt_inp = PBIT_FORMAT_NONE;
static unsigned      par_fmt_out = PBIT_FORMAT_NONE;

static int           par_cyl_all = 1;
static unsigned long par_cyl[2];

static int           par_trk_all = 1;
static unsigned long par_trk[2];

static unsigned long par_data_rate = 500000;


static pce_option_t opts[] = {
	{ '?', 0, "help", NULL, "Print usage information" },
	{ 'c', 1, "cylinder", "c", "Select cylinders [all]" },
	{ 'e', 2, "edit", "what val", "Edit selected track attributes" },
	{ 'f', 0, "info", NULL, "Print image information [no]" },
	{ 'h', 1, "head", "h", "Select heads [all]" },
	{ 'i', 1, "input", "filename", "Load an input file" },
	{ 'I', 1, "input-format", "format", "Set the input format [auto]" },
	{ 'l', 0, "list-short", NULL, "List tracks (short) [no]" },
	{ 'L', 0, "list-long", NULL, "List tracks (long) [no]" },
	{ 'o', 1, "output", "filename", "Set the output file name [none]" },
	{ 'O', 1, "output-format", "format", "Set the output format [auto]" },
	{ 'p', 1, "operation", "name [...]", "Perform an operation" },
	{ 'r', 1, "data-rate", "rate", "Set the data rate [500000]" },
	{ 't', 2, "track", "c h", "Select tracks [all]" },
	{ 'v', 0, "verbose", NULL, "Verbose operation [no]" },
	{ 'V', 0, "version", NULL, "Print version information" },
	{  -1, 0, NULL, NULL, NULL }
};


static
void print_help (void)
{
	pce_getopt_help (
		"pbit: convert and modify PCE PBIT image files",
		"usage: pbit [options] [input] [options] [output]",
		opts
	);

	fputs (
		"\n"
		"operations are:\n"
		"  auto-align-gcr         Automatically align GCR tracks to the index\n"
		"  comment-add text       Add to the image comment\n"
		"  comment-load filename  Load the image comment from a file\n"
		"  comment-print          Print the image comment\n"
		"  comment-save filename  Save the image comment to a file\n"
		"  comment-set text       Set the image comment\n"
		"  decode <type> <file>   Decode tracks\n"
		"  delete                 Delete tracks\n"
		"  double-step            Remove odd numbered tracks\n"
		"  double-step-even       Remove even numbered tracks\n"
		"  encode <type> <file>   Encode tracks\n"
		"  info                   Print image information\n"
		"  new                    Create new tracks\n"
		"  rotate <bits>          Rotate tracks left\n"
		"  save <filename>        Save raw tracks\n"
		"\n"
		"decode types are:\n"
		"  raw, gcr, mfm, gcr-raw, mfm-raw\n"
		"\n"
		"encode types are:\n"
		"  gcr, mfm-dd-300, mfm-hd-300, mfm-hd-360\n"
		"\n"
		"file formats are:\n"
		"  pbit, tc\n"
		"\n"
		"track attributes are:\n"
		"  clock, data, size\n",
		stdout
	);

	fflush (stdout);
}

static
void print_version (void)
{
	fputs (
		"pbit version " PCE_VERSION_STR
		"\n\n"
		"Copyright (C) 2012 Hampa Hug <hampa@hampa.ch>\n",
		stdout
	);

	fflush (stdout);
}


static
int pbit_parse_range (const char *str, unsigned long *v1, unsigned long *v2, int *all)
{
	*v1 = 0;
	*v2 = 0;
	*all = 0;

	if (strcmp (str, "all") == 0) {
		*all = 1;
		return (0);
	}

	while ((*str >= '0') && (*str <= '9')) {
		*v1 = 10 * *v1 + (*str - '0');
		str += 1;
	}

	if (*str == '-') {
		str += 1;

		if (*str == 0) {
			*v2 = ~(unsigned long) 0;
			return (0);
		}

		while ((*str >= '0') && (*str <= '9')) {
			*v2 = 10 * *v2 + (*str - '0');
			str += 1;
		}
	}
	else {
		*v2 = *v1;
	}

	if (*str != 0) {
		return (1);
	}

	return (0);
}


static
int pbit_sel_match_track (unsigned c, unsigned h)
{
	if (par_cyl_all == 0) {
		if ((c < par_cyl[0]) || (c > par_cyl[1])) {
			return (0);
		}
	}

	if (par_trk_all == 0) {
		if ((h < par_trk[0]) || (h > par_trk[1])) {
			return (0);
		}
	}

	return (1);
}

int pbit_for_all_tracks (pbit_img_t *img, pbit_trk_cb fct, void *opaque)
{
	unsigned long c, h;
	pbit_cyl_t    *cyl;
	pbit_trk_t    *trk;

	for (c = 0; c < img->cyl_cnt; c++) {
		cyl = img->cyl[c];

		for (h = 0; h < cyl->trk_cnt; h++) {
			trk = cyl->trk[h];

			if (pbit_sel_match_track (c, h) == 0) {
				continue;
			}

			if (fct (img, trk, c, h, opaque)) {
				return (1);
			}
		}
	}

	return (0);
}


static
int pbit_align_gcr_track_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	unsigned      val, run;
	unsigned long bit;
	unsigned long syn1, syn2, syn_cnt;
	unsigned long max1, max2, max_cnt;

	pbit_trk_set_pos (trk, 0);

	syn1 = 0;
	syn2 = 0;
	syn_cnt = 0;

	max1 = 0;
	max2 = 0;
	max_cnt = 0;

	val = 0;
	run = 0;

	while (1) {
		pbit_trk_get_bits (trk, &bit, 1);

		val = (val << 1) | (bit & 1);
		run += 1;

		if ((val & 0x3ff) == 0xff) {
			syn2 = trk->idx;
			syn_cnt += 1;

			if (syn_cnt > max_cnt) {
				max1 = syn1;
				max2 = syn2;
				max_cnt = syn_cnt;
			}

			run = 0;
		}
		else if (run >= 18) {
			if (trk->wrap) {
				break;
			}

			syn1 = trk->idx;
			syn_cnt = 0;
		}
	}

	max1 = (max1 + (max2 - max1) / 2) % trk->size;

	pbit_trk_rotate (trk, max1);

	return (0);
}

static
int pbit_align_gcr_tracks (pbit_img_t *img)
{
	return (pbit_for_all_tracks (img, pbit_align_gcr_track_cb, NULL));
}


static
int pbit_comment_show (pbit_img_t *img)
{
	unsigned i;

	fputs ("comments:\n", stdout);

	for (i = 0; i < img->comment_size; i++) {
		fputc (img->comment[i], stdout);
	}

	fputs ("\n", stdout);

	return (0);
}

static
int pbit_comment_add (pbit_img_t *img, const char *str)
{
	unsigned char       c;
	const unsigned char *tmp;

	if (img->comment_size > 0) {
		c = 0x0a;

		if (pbit_img_add_comment (img, &c, 1)) {
			return (1);
		}
	}

	tmp = (const unsigned char *) str;

	if (pbit_img_add_comment (img, tmp, strlen (str))) {
		return (1);
	}

	return (0);
}

static
int pbit_comment_set (pbit_img_t *img, const char *str)
{
	const unsigned char *tmp;

	if ((str == NULL) || (*str == 0)) {
		pbit_img_set_comment (img, NULL, 0);
		return (0);
	}

	tmp = (const unsigned char *) str;

	if (pbit_img_set_comment (img, tmp, strlen (str))) {
		return (1);
	}

	return (0);
}

static
int pbit_comment_save (pbit_img_t *img, const char *fname)
{
	unsigned cnt;
	FILE     *fp;

	fp = fopen (fname, "w");

	if (fp == NULL) {
		return (1);
	}

	cnt = img->comment_size;

	if (cnt > 0) {
		if (fwrite (img->comment, 1, cnt, fp) != cnt) {
			fclose (fp);
			return (1);
		}

		fputc (0x0a, fp);
	}

	fclose (fp);

	if (par_verbose) {
		fprintf (stderr, "%s: save comments to %s\n", arg0, fname);
	}

	return (0);
}

static
int pbit_comment_load (pbit_img_t *img, const char *fname)
{
	int           c, cr;
	unsigned      i, nl;
	FILE          *fp;
	unsigned char buf[256];

	fp = fopen (fname, "r");

	if (fp == NULL) {
		return (1);
	}

	pbit_img_set_comment (img, NULL, 0);

	cr = 0;
	nl = 0;
	i = 0;

	while (1) {
		c = fgetc (fp);

		if (c == EOF) {
			break;
		}

		if (c == 0x0d) {
			if (cr) {
				nl += 1;
			}

			cr = 1;
		}
		else if (c == 0x0a) {
			nl += 1;
			cr = 0;
		}
		else {
			if (cr) {
				nl += 1;
			}

			if (i > 0) {
				while (nl > 0) {
					buf[i++] = 0x0a;
					nl -= 1;

					if (i >= 256) {
						pbit_img_add_comment (img, buf, i);
						i = 0;
					}
				}
			}

			nl = 0;
			cr = 0;

			buf[i++] = c;

			if (i >= 256) {
				pbit_img_add_comment (img, buf, i);
				i = 0;
			}
		}
	}

	if (i > 0) {
		pbit_img_add_comment (img, buf, i);
		i = 0;
	}

	fclose (fp);

	if (par_verbose) {
		fprintf (stderr, "%s: load comments from %s\n", arg0, fname);
	}

	return (0);
}


static
int pbit_decode_raw_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	FILE *fp;

	fp = opaque;

	if (fwrite (trk->data, (trk->size + 7) / 8, 1, fp) != 1) {
		return (1);
	}

	return (0);
}

int pbit_decode_raw (pbit_img_t *img, const char *fname)
{
	int  r;
	FILE *fp;

	fp = fopen (fname, "wb");

	if (fp == NULL) {
		return (1);
	}

	r = pbit_for_all_tracks (img, pbit_decode_raw_cb, fp);

	fclose (fp);

	return (r);
}


static
int pbit_decode_mfm_raw_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	FILE          *fp;
	unsigned      outbuf, outcnt;
	unsigned      val, clk;
	unsigned long bit;

	fp = opaque;

	val = 0;
	clk = 0xffff;

	outbuf = 0;
	outcnt = 0;

	pbit_trk_set_pos (trk, 0);

	while (trk->wrap == 0) {
		pbit_trk_get_bits (trk, &bit, 1);

		val = (val << 1) | (bit & 1);
		clk = (clk << 1) | ((clk ^ 1) & 1);

		if ((val & 0xffff) == 0x4489) {
			if (outcnt > 0) {
				fputc (outbuf << (8 - outcnt), fp);
				outbuf = 0;
				outcnt = 0;
			}

			clk = 0xaaaa;
		}

		if ((clk & 0x8000) == 0) {
			outbuf = (outbuf << 1) | ((val >> 15) & 1);
			outcnt += 1;

			if (outcnt >= 8) {
				fputc (outbuf, fp);
				outbuf = 0;
				outcnt = 0;
			}
		}
	}

	return (0);
}

int pbit_decode_mfm_raw (pbit_img_t *img, const char *fname)
{
	int  r;
	FILE *fp;

	fp = fopen (fname, "wb");

	if (fp == NULL) {
		return (1);
	}

	r = pbit_for_all_tracks (img, pbit_decode_mfm_raw_cb, fp);

	fclose (fp);

	return (r);
}


static
int pbit_decode_gcr_raw_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	FILE          *fp;
	unsigned      val;
	unsigned long bit;

	fp = opaque;

	pbit_trk_set_pos (trk, 0);

	val = 0;

	while (trk->wrap == 0) {
		pbit_trk_get_bits (trk, &bit, 1);

		val = (val << 1) | (bit & 1);

		if (val & 0x80) {
			fputc (val, fp);
			val = 0;
		}
	}

	return (0);
}

int pbit_decode_gcr_raw (pbit_img_t *img, const char *fname)
{
	int  r;
	FILE *fp;

	fp = fopen (fname, "wb");

	if (fp == NULL) {
		return (1);
	}

	r = pbit_for_all_tracks (img, pbit_decode_gcr_raw_cb, fp);

	fclose (fp);

	return (r);
}


static
int pbit_decode (pbit_img_t *img, const char *type, const char *fname)
{
	int        r;
	FILE       *fp;
	pfdc_img_t *dimg;

	if (strcmp (type, "gcr-raw") == 0) {
		return (pbit_decode_gcr_raw (img, fname));
	}
	else if (strcmp (type, "mfm-raw") == 0) {
		return (pbit_decode_mfm_raw (img, fname));
	}
	else if (strcmp (type, "raw") == 0) {
		return (pbit_decode_raw (img, fname));
	}

	if (strcmp (type, "gcr") == 0) {
		dimg = pbit_decode_gcr (img);
	}
	else if (strcmp (type, "mfm") == 0) {
		dimg = pbit_decode_mfm (img);
	}
	else {
		dimg = NULL;
	}

	if (dimg == NULL) {
		return (1);
	}

	fp = fopen (fname, "wb");

	if (fp == NULL) {
		pfdc_img_del (dimg);
		return (1);
	}

	r = pfdc_save_pfdc (fp, dimg, -1);

	fclose (fp);

	pfdc_img_del (dimg);

	return (r);
}


static
int pbit_delete_track_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	pbit_img_del_track (img, c, h);

	return (0);
}

static
int pbit_delete_tracks (pbit_img_t *img)
{
	return (pbit_for_all_tracks (img, pbit_delete_track_cb, NULL));
}


static
int pbit_double_step (pbit_img_t *img, int even)
{
	unsigned   c, cn;
	pbit_cyl_t *cyl;

	cn = pbit_img_get_cyl_cnt (img);

	for (c = 0; c < cn; c++) {
		cyl = pbit_img_rmv_cylinder (img, c);

		if (((c & 1) && even) || (((c & 1) == 0) && !even)) {
			pbit_cyl_del (cyl);
		}
		else {
			pbit_img_set_cylinder (img, cyl, c / 2);
		}
	}

	return (0);
}


static
int pbit_encode (pbit_img_t **img, const char *type, const char *fname)
{
	FILE       *fp;
	pfdc_img_t *simg;
	pbit_img_t *dimg;

	fp = fopen (fname, "rb");

	if (fp == NULL) {
		return (1);
	}

	simg = pfdc_load_pfdc (fp);

	fclose (fp);

	if (simg == NULL) {
		return (1);
	}

	if (strcmp (type, "gcr") == 0) {
		dimg = pbit_encode_gcr (simg);
	}
	else if (strcmp (type, "mfm") == 0) {
		dimg = pbit_encode_mfm_dd_300 (simg);
	}
	else if (strcmp (type, "mfm-dd-300") == 0) {
		dimg = pbit_encode_mfm_dd_300 (simg);
	}
	else if (strcmp (type, "mfm-hd-300") == 0) {
		dimg = pbit_encode_mfm_hd_300 (simg);
	}
	else if (strcmp (type, "mfm-hd-360") == 0) {
		dimg = pbit_encode_mfm_hd_360 (simg);
	}
	else {
		dimg = NULL;
	}

	if ((dimg != NULL) && (simg->comment_size > 0)) {
		pbit_img_set_comment (dimg, simg->comment, simg->comment_size);
	}

	pfdc_img_del (simg);

	if (dimg == NULL) {
		return (1);
	}

	pbit_img_del (*img);

	*img = dimg;

	return (0);
}


static
int pbit_list_track_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	double rpm;

	if ((trk->clock > 0) && (trk->size > 0)) {
		rpm = (60.0 * trk->clock) / trk->size;
	}
	else {
		rpm = 0.0;
	}

	printf ("%2lu/%lu: CLK: %lu  BITS: %lu  RPM: %.4f\n",
		c, h,
		pbit_trk_get_clock (trk),
		pbit_trk_get_size (trk),
		rpm
	);

	return (0);
}

static
int pbit_list_tracks (pbit_img_t *img)
{
	return (pbit_for_all_tracks (img, pbit_list_track_cb, NULL));
}


static
int pbit_new_tracks (pbit_img_t *img, pbit_cyl_t *cyl, unsigned c)
{
	unsigned   h, h0, h1;
	pbit_trk_t *trk;

	if (par_trk_all) {
		h0 = 0;
		h1 = cyl->trk_cnt;
	}
	else {
		h0 = par_trk[0];
		h1 = par_trk[1] + 1;
	}

	for (h = h0; h < h1; h++) {
		trk = pbit_img_get_track (img, c, h, 0);

		if (trk != NULL) {
			continue;
		}

		trk = pbit_img_get_track (img, c, h, 1);

		if (trk == NULL) {
			return (1);
		}

		pbit_trk_set_clock (trk, par_data_rate);
	}

	return (0);
}

static
int pbit_new_cylinders (pbit_img_t *img)
{
	unsigned   c, c0, c1;
	pbit_cyl_t *cyl;

	if (par_cyl_all) {
		c0 = 0;
		c1 = img->cyl_cnt;
	}
	else {
		c0 = par_cyl[0];
		c1 = par_cyl[1] + 1;
	}

	for (c = c0; c < c1; c++) {
		cyl = pbit_img_get_cylinder (img, c, 1);

		if (cyl == NULL) {
			return (1);
		}

		if (pbit_new_tracks (img, cyl, c)) {
			return (1);
		}
	}

	return (0);
}

static
int pbit_new (pbit_img_t *img)
{
	if (pbit_new_cylinders (img)) {
		fprintf (stderr, "%s: creating failed\n", arg0);
	}

	return (0);
}


static
void pbit_print_range (const char *str1, unsigned long v1, unsigned long v2, const char *str2)
{
	fputs (str1, stdout);

	if (v1 == v2) {
		printf ("%lu", v1);
	}
	else {
		printf ("%lu - %lu", v1, v2);
	}

	fputs (str2, stdout);
}

static
void pbit_print_range_float (const char *str1, double v1, double v2, const char *str2)
{
	fputs (str1, stdout);

	if (v1 == v2) {
		printf ("%.4f", v1);
	}
	else {
		printf ("%.4f - %.4f", v1, v2);
	}

	fputs (str2, stdout);
}

static
int pbit_print_info (pbit_img_t *img)
{
	unsigned long c, h, cn, hn, tn;
	unsigned long h1, h2;
	unsigned long len;
	unsigned long clk, clk1, clk2;
	double        rpm, rpm1, rpm2;
	pbit_cyl_t    *cyl;
	pbit_trk_t    *trk;

	cn = pbit_img_get_cyl_cnt (img);
	tn = 0;

	h1 = 0;
	h2 = 0;

	clk1 = 0;
	clk2 = 0;

	rpm1 = 0.0;
	rpm2 = 0.0;

	for (c = 0; c < cn; c++) {
		cyl = pbit_img_get_cylinder (img, c, 0);

		if (cyl == NULL) {
			hn = 0;
		}
		else {
			hn = pbit_cyl_get_trk_cnt (cyl);
		}

		h1 = ((c == 0) || (hn < h1)) ? hn : h1;
		h2 = ((c == 0) || (hn > h2)) ? hn : h2;

		if (cyl == NULL) {
			continue;
		}

		for (h = 0; h < hn; h++) {
			trk = pbit_cyl_get_track (cyl, h, 0);

			if (trk == NULL) {
				clk = 0;
				len = 0;
			}
			else {
				clk = pbit_trk_get_clock (trk);
				len = pbit_trk_get_size (trk);
			}

			if (len > 0) {
				rpm = (60.0 * clk) / len;
			}
			else {
				rpm = 0.0;
			}

			clk1 = ((tn == 0) || (clk < clk1)) ? clk : clk1;
			clk2 = ((tn == 0) || (clk > clk2)) ? clk : clk2;

			rpm1 = ((tn == 0) || (rpm < rpm1)) ? rpm : rpm1;
			rpm2 = ((tn == 0) || (rpm > rpm2)) ? rpm : rpm2;

			tn += 1;
		}
	}

	printf ("cylinders: %lu\n", cn);
	pbit_print_range ("heads:     ", h1, h2, "\n");
	printf ("tracks:    %lu\n", tn);
	pbit_print_range ("clock:     ", clk1, clk2, "\n");
	pbit_print_range_float ("rpm:       ", rpm1, rpm2, "\n");

	if (img->comment_size > 0) {
		fputs ("\n", stdout);
		pbit_comment_show (img);
	}

	return (0);
}


static
int pbit_rotate_track_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *opaque)
{
	unsigned long *idx;

	idx = opaque;

	if (pbit_trk_rotate (trk, *idx)) {
		return (1);
	}

	return (0);
}

static
int pbit_rotate_tracks (pbit_img_t *img, unsigned long idx)
{
	return (pbit_for_all_tracks (img, pbit_rotate_track_cb, &idx));
}


static
int pbit_edit_clock_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *p)
{
	pbit_trk_set_clock (trk, *(unsigned long *) p);
	return (0);
}

static
int pbit_edit_data_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *p)
{
	pbit_trk_clear (trk, *(unsigned long *) p);
	return (0);
}

static
int pbit_edit_size_cb (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h, void *p)
{
	pbit_trk_set_size (trk, *(unsigned long *) p);
	return (0);
}

static
int pbit_edit_tracks (pbit_img_t *img, const char *what, const char *val)
{
	int           r;
	unsigned long v;
	pbit_trk_cb   fct;

	v = strtoul (val, NULL, 0);

	if (strcmp (what, "clock") == 0) {
		fct = pbit_edit_clock_cb;
	}
	else if (strcmp (what, "data") == 0) {
		fct = pbit_edit_data_cb;
	}
	else if (strcmp (what, "size") == 0) {
		fct = pbit_edit_size_cb;
	}
	else {
		fprintf (stderr, "%s: unknown field (%s)\n", arg0, what);
		return (1);
	}

	r = pbit_for_all_tracks (img, fct, &v);

	if (r) {
		fprintf (stderr, "%s: editing failed (%s = %lu)\n",
			arg0, what, v
		);
	}

	return (r);
}


static
int pbit_operation (pbit_img_t **img, const char *op, int argc, char **argv)
{
	int  r;
	char **optarg1, **optarg2;

	if (*img == NULL) {
		*img = pbit_img_new();

		if (*img == NULL) {
			return (1);
		}
	}

	r = 1;

	if (strcmp (op, "auto-align-gcr") == 0) {
		r = pbit_align_gcr_tracks (*img);
	}
	else if (strcmp (op, "comment-add") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			return (1);
		}

		r = pbit_comment_add (*img, optarg1[0]);
	}
	else if (strcmp (op, "comment-load") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			return (1);
		}

		r = pbit_comment_load (*img, optarg1[0]);
	}
	else if (strcmp (op, "comment-print") == 0) {
		r = pbit_comment_show (*img);
	}
	else if (strcmp (op, "comment-save") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			return (1);
		}

		r = pbit_comment_save (*img, optarg1[0]);
	}
	else if (strcmp (op, "comment-set") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			return (1);
		}

		r = pbit_comment_set (*img, optarg1[0]);
	}
	else if (strcmp (op, "decode") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			fprintf (stderr, "%s: missing decode type\n", arg0);
			return (1);
		}

		if (pce_getopt (argc, argv, &optarg2, NULL) != 0) {
			fprintf (stderr, "%s: missing file name\n", arg0);
			return (1);
		}

		r = pbit_decode (*img, optarg1[0], optarg2[0]);
	}
	else if (strcmp (op, "delete") == 0) {
		r = pbit_delete_tracks (*img);
	}
	else if (strcmp (op, "double-step") == 0) {
		r = pbit_double_step (*img, 1);
	}
	else if (strcmp (op, "double-step-even") == 0) {
		r = pbit_double_step (*img, 0);
	}
	else if (strcmp (op, "double-step-odd") == 0) {
		r = pbit_double_step (*img, 1);
	}
	else if (strcmp (op, "encode") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			fprintf (stderr, "%s: missing encode type\n", arg0);
			return (1);
		}

		if (pce_getopt (argc, argv, &optarg2, NULL) != 0) {
			fprintf (stderr, "%s: missing file name\n", arg0);
			return (1);
		}

		r = pbit_encode (img, optarg1[0], optarg2[0]);
	}
	else if (strcmp (op, "info") == 0) {
		r = pbit_print_info (*img);
	}
	else if (strcmp (op, "new") == 0) {
		r = pbit_new (*img);
	}
	else if (strcmp (op, "rotate") == 0) {
		unsigned long idx;

		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			fprintf (stderr, "%s: missing start position\n", arg0);
			return (1);
		}

		idx = strtoul (optarg1[0], NULL, 0);

		r = pbit_rotate_tracks (*img, idx);
	}
	else if (strcmp (op, "save") == 0) {
		if (pce_getopt (argc, argv, &optarg1, NULL) != 0) {
			fprintf (stderr, "%s: missing file name\n", arg0);
			return (1);
		}

		r = pbit_decode_raw (*img, optarg1[0]);
	}
	else {
		fprintf (stderr, "%s: unknown operation (%s)\n", arg0, op);
		return (1);
	}

	if (r) {
		fprintf (stderr, "%s: operation failed (%s)\n", arg0, op);
	}

	return (r);
}


static
pbit_img_t *pbit_load_image (const char *fname)
{
	pbit_img_t *img;

	if (par_verbose) {
		fprintf (stderr, "%s: loading image from %s\n", arg0, fname);
	}

	img = pbit_img_load (fname, par_fmt_inp);

	if (img == NULL) {
		fprintf (stderr, "%s: loading failed (%s)\n", arg0, fname);
		return (NULL);
	}

	if (par_list) {
		par_list = 0;
		pbit_list_tracks (img);
	}

	if (par_print_info) {
		par_print_info = 0;
		pbit_print_info (img);
	}

	return (img);
}

static
int pbit_set_format (const char *name, unsigned *val)
{
	if (strcmp (name, "pbit") == 0) {
		*val = PBIT_FORMAT_PBIT;
	}
	else if (strcmp (name, "tc") == 0) {
		*val = PBIT_FORMAT_TC;
	}
	else {
		fprintf (stderr, "%s: unknown format (%s)\n", arg0, name);
		*val = PBIT_FORMAT_NONE;
		return (1);
	}

	return (0);
}

int main (int argc, char **argv)
{
	int        r;
	char       **optarg;
	pbit_img_t *img;
	const char *out;

	arg0 = argv[0];

	img = NULL;
	out = NULL;

	while (1) {
		r = pce_getopt (argc, argv, &optarg, opts);

		if (r == GETOPT_DONE) {
			break;
		}

		if (r < 0) {
			return (1);
		}

		switch (r) {
		case '?':
			print_help();
			return (0);

		case 'V':
			print_version();
			return (0);

		case 'c':
			if (pbit_parse_range (optarg[0], &par_cyl[0], &par_cyl[1], &par_cyl_all)) {
				return (1);
			}
			break;

		case 'e':
			if (img != NULL) {
				if (pbit_edit_tracks (img, optarg[0], optarg[1])) {
					return (1);
				}
			}
			break;

		case 'f':
			if (img != NULL) {
				pbit_print_info (img);
			}
			else {
				par_print_info = 1;
			}
			break;

		case 'h':
			if (pbit_parse_range (optarg[0], &par_trk[0], &par_trk[1], &par_trk_all)) {
				return (1);
			}
			break;

		case 'i':
			if (img != NULL) {
				pbit_img_del (img);
			}

			img = pbit_load_image (optarg[0]);

			if (img == NULL) {
				return (1);
			}
			break;

		case 'I':
			if (pbit_set_format (optarg[0], &par_fmt_inp)) {
				return (1);
			}
			break;

		case 'l':
			if (img != NULL) {
				pbit_list_tracks (img);
			}
			else {
				par_list = 1;
			}
			break;

		case 'o':
			out = optarg[0];
			break;

		case 'O':
			if (pbit_set_format (optarg[0], &par_fmt_out)) {
				return (1);
			}
			break;

		case 'p':
			if (pbit_operation (&img, optarg[0], argc, argv)) {
				return (1);
			}
			break;

		case 'r':
			par_data_rate = strtoul (optarg[0], NULL, 0);

			if (par_data_rate <= 1000) {
				par_data_rate *= 1000;
			}
			break;

		case 't':
			if (pbit_parse_range (optarg[0], &par_cyl[0], &par_cyl[1], &par_cyl_all)) {
				return (1);
			}
			if (pbit_parse_range (optarg[1], &par_trk[0], &par_trk[1], &par_trk_all)) {
				return (1);
			}
			break;

		case 'v':
			par_verbose = 1;
			break;

		case 0:
			if (img == NULL) {
				img = pbit_load_image (optarg[0]);

				if (img == NULL) {
					return (1);
				}
			}
			else if (out == NULL) {
				out = optarg[0];
			}
			else {
				fprintf (stderr, "%s: unknown option (%s)\n",
					arg0, optarg[0]
				);

				return (1);
			}
			break;

		default:
			return (1);
		}
	}

	if (out != NULL) {
		if (img == NULL) {
			img = pbit_img_new();
		}

		if (img == NULL) {
			return (1);
		}

		if (par_verbose) {
			fprintf (stderr, "%s: save image to %s\n", arg0, out);
		}

		r = pbit_img_save (out, img, par_fmt_out);

		if (r) {
			fprintf (stderr, "%s: saving failed (%s)\n",
				argv[0], out
			);
			return (1);
		}
	}

	return (0);
}
