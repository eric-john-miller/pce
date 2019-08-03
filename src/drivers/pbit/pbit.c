/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/drivers/pbit/pbit.c                                      *
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pbit.h"


/*****************************************************************************
 * Clear all bits in the range i1 < i <= i2 in buf
 *****************************************************************************/
static
void pbit_clear_bits (unsigned char *buf, unsigned long i1, unsigned long i2)
{
	unsigned char m1, m2;

	if (i2 < i1) {
		return;
	}

	m1 = ~(0xff >> (i1 & 7));
	m2 = ~(0xff << (~i2 & 7));

	i1 >>= 3;
	i2 >>= 3;

	if (i1 == i2) {
		buf[i1] &= m1 | m2;
	}
	else {
		buf[i1++] &= m1;

		while (i1 < i2) {
			buf[i1++] = 0;
		}

		buf[i2] &= m2;
	}
}

/*****************************************************************************
 * Create a new track
 *
 * @param  size   The track size in bits
 * @param  clock  The bit rate in Herz
 *
 * @return The new track or NULL on error
 *****************************************************************************/
pbit_trk_t *pbit_trk_new (unsigned long size, unsigned long clock)
{
	pbit_trk_t *trk;

	trk = malloc (sizeof (pbit_trk_t));

	if (trk == NULL) {
		return (NULL);
	}

	trk->clock = clock;
	trk->size = size;
	trk->data = NULL;

	if (size > 0) {
		trk->data = malloc ((size + 7) / 8);

		if (trk->data == NULL) {
			free (trk);
			return (NULL);
		}
	}

	trk->idx = 0;
	trk->wrap = 0;

	return (trk);
}

/*****************************************************************************
 * Delete a track
 *****************************************************************************/
void pbit_trk_del (pbit_trk_t *trk)
{
	if (trk != NULL) {
		free (trk->data);
		free (trk);
	}
}

/*****************************************************************************
 * Create an exact copy of a track
 *****************************************************************************/
pbit_trk_t *pbit_trk_clone (const pbit_trk_t *trk)
{
	pbit_trk_t *ret;

	ret = pbit_trk_new (trk->size, trk->clock);

	if (ret == NULL) {
		return (NULL);
	}

	if (trk->size > 0) {
		memcpy (ret->data, trk->data, (trk->size + 7) / 8);
	}

	ret->idx = trk->idx;
	ret->wrap = trk->wrap;

	return (ret);
}

/*****************************************************************************
 * Clear a track
 *
 * @param val  An 8 bit value that is used to initialize the track
 *****************************************************************************/
void pbit_trk_clear (pbit_trk_t *trk, unsigned val)
{
	if (trk->size > 0) {
		memset (trk->data, val, (trk->size + 7) / 8);
		pbit_clear_bits (trk->data, trk->size, (trk->size - 1) | 7);
	}
}

/*****************************************************************************
 * Set the bit rate
 *
 * @param clock  The bit rate in Herz
 *****************************************************************************/
void pbit_trk_set_clock (pbit_trk_t *trk, unsigned long clock)
{
	trk->clock = clock;
}

/*****************************************************************************
 * Get the bit rate
 *
 * @return The bit rate in Herz
 *****************************************************************************/
unsigned long pbit_trk_get_clock (const pbit_trk_t *trk)
{
	return (trk->clock);
}

/*****************************************************************************
 * Get the track size
 *
 * @return The track size in bits
 *****************************************************************************/
unsigned long pbit_trk_get_size (const pbit_trk_t *trk)
{
	return (trk->size);
}

/*****************************************************************************
 * Set the track size
 *
 * @param  size  The new track size in bits
 *
 * If the new size is greater than the old size, the new bits are
 * initialized to 0.
 *****************************************************************************/
int pbit_trk_set_size (pbit_trk_t *trk, unsigned long size)
{
	unsigned long i1;
	unsigned char *tmp;

	if (trk->size == size) {
		return (0);
	}

	trk->idx = 0;
	trk->wrap = 0;

	if (size == 0) {
		free (trk->data);
		trk->size = 0;
		trk->data = NULL;
		return (0);
	}

	tmp = realloc (trk->data, (size + 7) / 8);

	if (tmp == NULL) {
		return (1);
	}

	if (size > 0) {
		i1 = (size < trk->size) ? size : trk->size;
		pbit_clear_bits (tmp, i1, (size - 1) | 7);
	}

	trk->size = size;
	trk->data = tmp;

	return (0);
}

/*****************************************************************************
 * Set the track position
 *
 * @param  pos  The new track position in bits
 *****************************************************************************/
void pbit_trk_set_pos (pbit_trk_t *trk, unsigned long pos)
{
	if (trk->size > 0) {
		trk->idx = pos % trk->size;
		trk->wrap = 0;
	}
}

/*****************************************************************************
 * Get bits from the current position
 *
 * @param  val  The bits, in the low order cnt bits
 * @param  cnt  The number of bits (cnt <= 32)
 *
 * @return Non-zero if the current position wrapped around
 *****************************************************************************/
int pbit_trk_get_bits (pbit_trk_t *trk, unsigned long *val, unsigned cnt)
{
	unsigned long       v;
	unsigned char       m;
	const unsigned char *p;

	p = trk->data + (trk->idx / 8);
	m = 0x80 >> (trk->idx & 7);
	v = 0;

	while (cnt > 0) {
		v = (v << 1) | ((*p & m) != 0);

		m >>= 1;
		trk->idx += 1;

		if (trk->idx >= trk->size) {
			p = trk->data;
			m = 0x80;
			trk->idx = 0;
			trk->wrap = 1;
		}
		else if (m == 0) {
			p += 1;
			m = 0x80;
		}

		cnt -= 1;
	}

	*val = v;

	return (trk->wrap);
}

/*****************************************************************************
 * Set bits at the current position
 *
 * @param  val  The bits, in the low order cnt bits
 * @param  cnt  The number of bits (cnt <= 32)
 *
 * @return Non-zero if the current position wrapped around
 *****************************************************************************/
int pbit_trk_set_bits (pbit_trk_t *trk, unsigned long val, unsigned cnt)
{
	unsigned char m;
	unsigned char *p;

	p = trk->data + (trk->idx / 8);
	m = 0x80 >> (trk->idx & 7);

	while (cnt > 0) {
		cnt -= 1;

		if ((val >> cnt) & 1) {
			*p |= m;
		}
		else {
			*p &= ~m;
		}

		m >>= 1;
		trk->idx += 1;

		if (trk->idx >= trk->size) {
			p = trk->data;
			m = 0x80;
			trk->idx = 0;
			trk->wrap = 1;
		}
		else if (m == 0) {
			p += 1;
			m = 0x80;
		}
	}

	return (trk->wrap);
}

int pbit_trk_rotate (pbit_trk_t *trk, unsigned long idx)
{
	unsigned long i;
	unsigned char dm, sm;
	unsigned char *sp, *dp, *tmp;

	if (idx >= trk->size) {
		return (1);
	}

	if (idx == 0) {
		return (0);
	}

	tmp = malloc ((trk->size + 7) / 8);

	if (tmp == NULL) {
		return (1);
	}

	memset (tmp, 0, (trk->size + 7) / 8);

	sp = trk->data + (idx >> 3);
	sm = 0x80 >> (idx & 7);

	dp = tmp;
	dm = 0x80;

	for (i = 0; i < trk->size; i++) {
		if (*sp & sm) {
			*dp |= dm;
		}

		dm >>= 1;

		if (dm == 0) {
			dp += 1;
			dm = 0x80;
		}

		idx += 1;

		if (idx >= trk->size) {
			idx = 0;
			sp = trk->data;
			sm = 0x80;
		}
		else {
			sm >>= 1;

			if (sm == 0) {
				sp += 1;
				sm = 0x80;
			}
		}

	}

	free (trk->data);
	trk->data = tmp;

	return (0);
}


pbit_cyl_t *pbit_cyl_new (void)
{
	pbit_cyl_t *cyl;

	cyl = malloc (sizeof (pbit_cyl_t));

	if (cyl == NULL) {
		return (NULL);
	}

	cyl->trk_cnt = 0;
	cyl->trk = NULL;

	return (cyl);
}

void pbit_cyl_del (pbit_cyl_t *cyl)
{
	unsigned long i;

	if (cyl != NULL) {
		for (i = 0; i < cyl->trk_cnt; i++) {
			pbit_trk_del (cyl->trk[i]);
		}

		free (cyl->trk);
		free (cyl);
	}
}

unsigned long pbit_cyl_get_trk_cnt (const pbit_cyl_t *cyl)
{
	return (cyl->trk_cnt);
}

pbit_trk_t *pbit_cyl_get_track (pbit_cyl_t *cyl, unsigned long h, int alloc)
{
	pbit_trk_t *trk;

	if ((h < cyl->trk_cnt) && (cyl->trk[h] != NULL)) {
		return (cyl->trk[h]);
	}

	if (alloc == 0) {
		return (NULL);
	}

	trk = pbit_trk_new (0, 0);

	if (trk == NULL) {
		return (NULL);
	}

	if (pbit_cyl_set_track (cyl, trk, h)) {
		pbit_trk_del (trk);
		return (NULL);
	}

	return (trk);
}

int pbit_cyl_set_track (pbit_cyl_t *cyl, pbit_trk_t *trk, unsigned long h)
{
	unsigned long i;
	pbit_trk_t    **tmp;

	if (h < cyl->trk_cnt) {
		pbit_trk_del (cyl->trk[h]);

		cyl->trk[h] = trk;

		return (0);
	}

	tmp = realloc (cyl->trk, (h + 1) * sizeof (pbit_trk_t *));

	if (tmp == NULL) {
		return (1);
	}

	for (i = cyl->trk_cnt; i < h; i++) {
		tmp[i] = NULL;
	}

	tmp[h] = trk;

	cyl->trk = tmp;
	cyl->trk_cnt = h + 1;

	return (0);
}

int pbit_cyl_add_track (pbit_cyl_t *cyl, pbit_trk_t *trk)
{
	return (pbit_cyl_set_track (cyl, trk, cyl->trk_cnt));
}

int pbit_cyl_del_track (pbit_cyl_t *cyl, unsigned long h)
{
	if ((h >= cyl->trk_cnt) || (cyl->trk[h] == NULL)) {
		return (1);
	}

	pbit_trk_del (cyl->trk[h]);

	cyl->trk[h] = NULL;

	while ((cyl->trk_cnt > 0) && (cyl->trk[cyl->trk_cnt - 1] == NULL)) {
		cyl->trk_cnt -= 1;
	}

	return (0);
}


pbit_img_t *pbit_img_new (void)
{
	pbit_img_t *img;

	img = malloc (sizeof (pbit_img_t));

	if (img == NULL) {
		return (NULL);
	}

	img->cyl_cnt = 0;
	img->cyl = NULL;

	img->comment_size = 0;
	img->comment = NULL;

	return (img);
}

void pbit_img_del (pbit_img_t *img)
{
	unsigned long i;

	if (img != NULL) {
		for (i = 0; i < img->cyl_cnt; i++) {
			pbit_cyl_del (img->cyl[i]);
		}

		free (img->comment);
		free (img->cyl);
		free (img);
	}
}

unsigned long pbit_img_get_cyl_cnt (const pbit_img_t *img)
{
	return (img->cyl_cnt);
}

unsigned long pbit_img_get_trk_cnt (const pbit_img_t *img, unsigned long c)
{
	if ((c >= img->cyl_cnt) || (img->cyl[c] == NULL)) {
		return (0);
	}

	return (img->cyl[c]->trk_cnt);
}

static
void pbit_img_fix_cyl (pbit_img_t *img)
{
	while ((img->cyl_cnt > 0) && (img->cyl[img->cyl_cnt - 1] == NULL)) {
		img->cyl_cnt -= 1;
	}
}

pbit_cyl_t *pbit_img_get_cylinder (pbit_img_t *img, unsigned long c, int alloc)
{
	pbit_cyl_t *cyl;

	if ((c < img->cyl_cnt) && (img->cyl[c] != NULL)) {
		return (img->cyl[c]);
	}

	if (alloc == 0) {
		return (NULL);
	}

	cyl = pbit_cyl_new();

	if (cyl == NULL) {
		return (NULL);
	}

	if (pbit_img_set_cylinder (img, cyl, img->cyl_cnt)) {
		pbit_cyl_del (cyl);
		return (NULL);
	}

	return (cyl);
}

pbit_cyl_t *pbit_img_rmv_cylinder (pbit_img_t *img, unsigned long c)
{
	pbit_cyl_t *cyl;

	if ((c >= img->cyl_cnt) || (img->cyl[c] == NULL)) {
		return (NULL);
	}

	cyl = img->cyl[c];

	img->cyl[c] = NULL;

	pbit_img_fix_cyl (img);

	return (cyl);
}

int pbit_img_set_cylinder (pbit_img_t *img, pbit_cyl_t *cyl, unsigned long c)
{
	unsigned long i;
	pbit_cyl_t    **tmp;

	if (c < img->cyl_cnt) {
		pbit_cyl_del (img->cyl[c]);
		img->cyl[c] = cyl;
		pbit_img_fix_cyl (img);

		return (0);
	}

	tmp = realloc (img->cyl, (c + 1) * sizeof (pbit_cyl_t *));

	if (tmp == NULL) {
		return (1);
	}

	for (i = img->cyl_cnt; i < c; i++) {
		tmp[i] = NULL;
	}

	tmp[c] = cyl;

	img->cyl = tmp;
	img->cyl_cnt = c + 1;

	pbit_img_fix_cyl (img);

	return (0);
}

int pbit_img_add_cylinder (pbit_img_t *img, pbit_cyl_t *cyl)
{
	return (pbit_img_set_cylinder (img, cyl, img->cyl_cnt));
}

int pbit_img_del_cylinder (pbit_img_t *img, unsigned long c)
{
	if ((c >= img->cyl_cnt) || (img->cyl[c] == NULL)) {
		return (1);
	}

	pbit_cyl_del (img->cyl[c]);

	img->cyl[c] = NULL;

	pbit_img_fix_cyl (img);

	return (0);
}

pbit_trk_t *pbit_img_get_track (pbit_img_t *img, unsigned long c, unsigned long h, int alloc)
{
	pbit_cyl_t *cyl;
	pbit_trk_t *trk;

	cyl = pbit_img_get_cylinder (img, c, alloc);

	if (cyl == NULL) {
		return (NULL);
	}

	trk = pbit_cyl_get_track (cyl, h, alloc);

	if (trk == NULL) {
		return (NULL);
	}

	return (trk);
}

int pbit_img_set_track (pbit_img_t *img, pbit_trk_t *trk, unsigned long c, unsigned long h)
{
	pbit_cyl_t *cyl;

	cyl = pbit_img_get_cylinder (img, c, 1);

	if (cyl == NULL) {
		return (1);
	}

	if (pbit_cyl_set_track (cyl, trk, h)) {
		return (1);
	}

	return (0);
}

int pbit_img_del_track (pbit_img_t *img, unsigned long c, unsigned long h)
{
	if ((c >= img->cyl_cnt) || (img->cyl[c] == NULL)) {
		return (1);
	}

	if (pbit_cyl_del_track (img->cyl[c], h)) {
		return (1);
	}

	return (0);
}

int pbit_img_add_comment (pbit_img_t *img, const unsigned char *buf, unsigned cnt)
{
	unsigned char *tmp;

	tmp = realloc (img->comment, img->comment_size + cnt);

	if (tmp == NULL) {
		return (1);
	}

	memcpy (tmp + img->comment_size, buf, cnt);

	img->comment_size += cnt;
	img->comment = tmp;

	return (0);
}

int pbit_img_set_comment (pbit_img_t *img, const unsigned char *buf, unsigned cnt)
{
	free (img->comment);

	img->comment_size = 0;
	img->comment = NULL;

	if ((buf == NULL) || (cnt == 0)) {
		return (0);
	}

	if (pbit_img_add_comment (img, buf, cnt)) {
		return (1);
	}

	return (0);
}
