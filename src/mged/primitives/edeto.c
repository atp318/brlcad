/*                         E D E T O . C
 * BRL-CAD
 *
 * Copyright (c) 1996-2024 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file mged/primitives/edeto.c
 *
 */

#include "common.h"

#include <math.h>
#include <string.h>

#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "wdb.h"

#include "../mged.h"
#include "../sedit.h"
#include "../mged_dm.h"
#include "./mged_functab.h"
#include "./edeto.h"

static void
eto_ed(struct mged_state *s, int arg, int UNUSED(a), int UNUSED(b))
{
    es_menu = arg;
    mged_set_edflag(s, PSCALE);
    if (arg == MENU_ETO_ROT_C) {
	s->edit_state.edit_flag = ECMD_ETO_ROT_C;
	s->edit_state.solid_edit_rotate = 1;
	s->edit_state.solid_edit_translate = 0;
	s->edit_state.solid_edit_scale = 0;
	s->edit_state.solid_edit_pick = 0;
    }

    set_e_axes_pos(s, 1);
}
struct menu_item eto_menu[] = {
    { "ELL-TORUS MENU", NULL, 0 },
    { "Set r", eto_ed, MENU_ETO_R },
    { "Set D", eto_ed, MENU_ETO_RD },
    { "Set C", eto_ed, MENU_ETO_SCALE_C },
    { "Rotate C", eto_ed, MENU_ETO_ROT_C },
    { "", NULL, 0 }
};

struct menu_item *
mged_eto_menu_item(const struct bn_tol *UNUSED(tol))
{
    return eto_menu;
}

#define V3BASE2LOCAL(_pt) (_pt)[X]*base2local, (_pt)[Y]*base2local, (_pt)[Z]*base2local

void
mged_eto_write_params(
	struct bu_vls *p,
       	const struct rt_db_internal *ip,
       	const struct bn_tol *UNUSED(tol),
	fastf_t base2local)
{
    struct rt_eto_internal *eto = (struct rt_eto_internal *)ip->idb_ptr;
    RT_ETO_CK_MAGIC(eto);

    bu_vls_printf(p, "Vertex: %.9f %.9f %.9f\n", V3BASE2LOCAL(eto->eto_V));
    bu_vls_printf(p, "Normal: %.9f %.9f %.9f\n", V3BASE2LOCAL(eto->eto_N));
    bu_vls_printf(p, "Semi-major axis: %.9f %.9f %.9f\n", V3BASE2LOCAL(eto->eto_C));
    bu_vls_printf(p, "Semi-minor length: %.9f\n", eto->eto_rd * base2local);
    bu_vls_printf(p, "Radius of rotation: %.9f\n", eto->eto_r * base2local);
}

#define read_params_line_incr \
    lc = (ln) ? (ln + lcj) : NULL; \
    if (!lc) { \
	bu_free(wc, "wc"); \
	return BRLCAD_ERROR; \
    } \
    ln = strchr(lc, tc); \
    if (ln) *ln = '\0'; \
    while (lc && strchr(lc, ':')) lc++

int
mged_eto_read_params(
	struct rt_db_internal *ip,
	const char *fc,
	const struct bn_tol *UNUSED(tol),
	fastf_t local2base
	)
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    struct rt_eto_internal *eto = (struct rt_eto_internal *)ip->idb_ptr;
    RT_ETO_CK_MAGIC(eto);

    if (!fc)
	return BRLCAD_ERROR;

    // We're getting the file contents as a string, so we need to split it up
    // to process lines. See https://stackoverflow.com/a/17983619

    // Figure out if we need to deal with Windows line endings
    const char *crpos = strchr(fc, '\r');
    int crlf = (crpos && crpos[1] == '\n') ? 1 : 0;
    char tc = (crlf) ? '\r' : '\n';
    // If we're CRLF jump ahead another character.
    int lcj = (crlf) ? 2 : 1;

    char *ln = NULL;
    char *wc = bu_strdup(fc);
    char *lc = wc;

    // Set up initial line (Vertex)
    ln = strchr(lc, tc);
    if (ln) *ln = '\0';

    // Trim off prefixes, if user left them in
    while (lc && strchr(lc, ':')) lc++;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(eto->eto_V, a, b, c);
    VSCALE(eto->eto_V, eto->eto_V, local2base);

    // Set up Normal line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(eto->eto_N, a, b, c);
    VUNITIZE(eto->eto_N);

    // Set up Semi-major axis line
    read_params_line_incr;

    sscanf(lc, "%lf %lf %lf", &a, &b, &c);
    VSET(eto->eto_C, a, b, c);
    VSCALE(eto->eto_C, eto->eto_C, local2base);

    // Set up Semi-minor length line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    eto->eto_rd = a * local2base;

    // Set up Radius of rotation line
    read_params_line_incr;

    sscanf(lc, "%lf", &a);
    eto->eto_r = a * local2base;

    // Cleanup
    bu_free(wc, "wc");
    return BRLCAD_OK;
}

/* scale radius 1 (r) of ETO */
void
menu_eto_r(struct mged_state *s)
{
    struct rt_eto_internal *eto =
	(struct rt_eto_internal *)s->edit_state.es_int.idb_ptr;
    fastf_t ch, cv, dh, newrad;
    vect_t Nu;

    RT_ETO_CK_MAGIC(eto);
    if (inpara) {
	/* take es_mat[15] (path scaling) into account */
	es_para[0] *= es_mat[15];
	newrad = es_para[0];
    } else {
	newrad = eto->eto_r * s->edit_state.es_scale;
    }
    if (newrad < SMALL) newrad = 4*SMALL;
    VMOVE(Nu, eto->eto_N);
    VUNITIZE(Nu);
    /* get horiz and vert components of C and Rd */
    cv = VDOT(eto->eto_C, Nu);
    ch = sqrt(VDOT(eto->eto_C, eto->eto_C) - cv * cv);
    /* angle between C and Nu */
    dh = eto->eto_rd * cv / MAGNITUDE(eto->eto_C);
    /* make sure revolved ellipse doesn't overlap itself */
    if (ch <= newrad && dh <= newrad)
	eto->eto_r = newrad;
}

/* scale Rd, ellipse semi-minor axis length, of ETO */
void
menu_eto_rd(struct mged_state *s)
{
    struct rt_eto_internal *eto =
	(struct rt_eto_internal *)s->edit_state.es_int.idb_ptr;
    fastf_t dh, newrad, work;
    vect_t Nu;

    RT_ETO_CK_MAGIC(eto);
    if (inpara) {
	/* take es_mat[15] (path scaling) into account */
	es_para[0] *= es_mat[15];
	newrad = es_para[0];
    } else {
	newrad = eto->eto_rd * s->edit_state.es_scale;
    }
    if (newrad < SMALL) newrad = 4*SMALL;
    work = MAGNITUDE(eto->eto_C);
    if (newrad <= work) {
	VMOVE(Nu, eto->eto_N);
	VUNITIZE(Nu);
	dh = newrad * VDOT(eto->eto_C, Nu) / work;
	/* make sure revolved ellipse doesn't overlap itself */
	if (dh <= eto->eto_r)
	    eto->eto_rd = newrad;
    }
}

/* scale vector C */
void
menu_eto_scale_c(struct mged_state *s)
{
    struct rt_eto_internal *eto =
	(struct rt_eto_internal *)s->edit_state.es_int.idb_ptr;
    fastf_t ch, cv;
    vect_t Nu, Work;

    RT_ETO_CK_MAGIC(eto);
    if (inpara) {
	/* take es_mat[15] (path scaling) into account */
	es_para[0] *= es_mat[15];
	s->edit_state.es_scale = es_para[0] / MAGNITUDE(eto->eto_C);
    }
    if (s->edit_state.es_scale * MAGNITUDE(eto->eto_C) >= eto->eto_rd) {
	VMOVE(Nu, eto->eto_N);
	VUNITIZE(Nu);
	VSCALE(Work, eto->eto_C, s->edit_state.es_scale);
	/* get horiz and vert comps of C and Rd */
	cv = VDOT(Work, Nu);
	ch = sqrt(VDOT(Work, Work) - cv * cv);
	if (ch <= eto->eto_r)
	    VMOVE(eto->eto_C, Work);
    }
}

/* rotate ellipse semi-major axis vector */
int
ecmd_eto_rot_c(struct mged_state *s)
{
    struct rt_eto_internal *eto =
	(struct rt_eto_internal *)s->edit_state.es_int.idb_ptr;
    mat_t mat;
    mat_t mat1;
    mat_t edit;

    RT_ETO_CK_MAGIC(eto);
    if (inpara) {
	if (inpara != 3) {
	    Tcl_AppendResult(s->interp, "ERROR: three arguments needed\n", (char *)NULL);
	    inpara = 0;
	    return TCL_ERROR;
	}

	static mat_t invsolr;
	/*
	 * Keyboard parameters:  absolute x, y, z rotations,
	 * in degrees.  First, cancel any existing rotations,
	 * then perform new rotation
	 */
	bn_mat_inv(invsolr, acc_rot_sol);

	/* Build completely new rotation change */
	MAT_IDN(modelchanges);
	bn_mat_angles(modelchanges,
		es_para[0],
		es_para[1],
		es_para[2]);
	/* Borrow incr_change matrix here */
	bn_mat_mul(incr_change, modelchanges, invsolr);
	MAT_COPY(acc_rot_sol, modelchanges);

	/* Apply new rotation to solid */
	/* Clear out solid rotation */
	MAT_IDN(modelchanges);
    } else {
	/* Apply incremental changes already in incr_change */
    }

    if (mged_variables->mv_context) {
	/* calculate rotations about keypoint */
	bn_mat_xform_about_pnt(edit, incr_change, es_keypoint);

	/* We want our final matrix (mat) to xform the original solid
	 * to the position of this instance of the solid, perform the
	 * current edit operations, then xform back.
	 * mat = es_invmat * edit * es_mat
	 */
	bn_mat_mul(mat1, edit, es_mat);
	bn_mat_mul(mat, es_invmat, mat1);

	MAT4X3VEC(eto->eto_C, mat, eto->eto_C);
    } else {
	MAT4X3VEC(eto->eto_C, incr_change, eto->eto_C);
    }

    MAT_IDN(incr_change);

    return 0;
}

static int
mged_eto_pscale(struct mged_state *s, int mode)
{
    if (inpara > 1) {
	Tcl_AppendResult(s->interp, "ERROR: only one argument needed\n", (char *)NULL);
	inpara = 0;
	return TCL_ERROR;
    }

    if (es_para[0] <= 0.0) {
	Tcl_AppendResult(s->interp, "ERROR: SCALE FACTOR <= 0\n", (char *)NULL);
	inpara = 0;
	return TCL_ERROR;
    }

    /* must convert to base units */
    es_para[0] *= s->dbip->dbi_local2base;
    es_para[1] *= s->dbip->dbi_local2base;
    es_para[2] *= s->dbip->dbi_local2base;

    switch (mode) {
	case MENU_ETO_R:
	    menu_eto_r(s);
	    break;
	case MENU_ETO_RD:
	    menu_eto_rd(s);
	    break;
	case MENU_ETO_SCALE_C:
	    menu_eto_scale_c(s);
	    break;
    };

    return 0;
}

int
mged_eto_edit(struct mged_state *s, int edflag)
{
    switch (edflag) {
	case SSCALE:
	    /* scale the solid uniformly about its vertex point */
	    return mged_generic_sscale(s, &s->edit_state.es_int);
	case STRANS:
	    /* translate solid */
	    mged_generic_strans(s, &s->edit_state.es_int);
	    break;
	case SROT:
	    /* rot solid about vertex */
	    mged_generic_srot(s, &s->edit_state.es_int);
	    break;
	case PSCALE:
	    return mged_eto_pscale(s, es_menu);
	case ECMD_ETO_ROT_C:
	    return ecmd_eto_rot_c(s);
    }
    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */