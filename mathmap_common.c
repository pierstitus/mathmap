/* -*- c -*- */

/*
 * mathmap_common.c
 *
 * MathMap
 *
 * Copyright (C) 1997-2004 Mark Probst
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "internals.h"
#include "tags.h"
#include "jump.h"
#include "scanner.h"
#include "postfix.h"
#include "cgen.h"
#include "mathmap.h"

mathmap_t *the_mathmap = 0;

/* from parser.y */
int yyparse (void);

void
unload_mathmap (mathmap_t *mathmap)
{
    if (mathmap->is_native && mathmap->module_info != 0)
    {
	unload_c_code(mathmap->module_info);
	mathmap->module_info = 0;
    }
}

void
free_mathmap (mathmap_t *mathmap)
{
    if (mathmap->variables != 0)
	free_variables(mathmap->variables);
    if (mathmap->exprtree != 0)
	free_exprtree(mathmap->exprtree);
    if (mathmap->userval_infos != 0)
	free_userval_infos(mathmap->userval_infos);
    if (!mathmap->is_native && mathmap->expression != 0)
	free(mathmap->expression);
    unload_mathmap(mathmap);

    free(mathmap);
}

void
free_invocation (mathmap_invocation_t *invocation)
{
    userval_info_t *info;

    if (invocation->internals != 0)
	free(invocation->internals);
    if (invocation->variables != 0)
	free(invocation->variables);
    if (invocation->stack_machine != 0)
	free_postfix_machine(invocation->stack_machine);
    if (invocation->uservals != 0)
    {
	for (info = invocation->mathmap->userval_infos; info != 0; info = info->next)
	{
	    switch (info->type)
	    {
		case USERVAL_CURVE :
		    free(invocation->uservals[info->index].v.curve.values);
		    break;

		case USERVAL_GRADIENT :
		    free(invocation->uservals[info->index].v.gradient.values);
		    break;

#ifdef GIMP
		case USERVAL_IMAGE :
		    if (invocation->uservals[info->index].v.image.index > 0)
			free_input_drawable(invocation->uservals[info->index].v.image.index);
		    break;
#endif
	    }
	}
	free(invocation->uservals);
    }
    if (invocation->xy_vars != 0)
	free(invocation->xy_vars);
    if (invocation->y_vars != 0)
	free(invocation->y_vars);
    free(invocation);
}

#define X_INTERNAL_INDEX         0
#define Y_INTERNAL_INDEX         1
#define R_INTERNAL_INDEX         2
#define A_INTERNAL_INDEX         3

void
init_internals (mathmap_t *mathmap)
{
    register_internal(&mathmap->internals, "x", CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "y", CONST_X | CONST_T);
    register_internal(&mathmap->internals, "r", CONST_T);
    register_internal(&mathmap->internals, "a", CONST_T);
    register_internal(&mathmap->internals, "t", CONST_X | CONST_Y);
    register_internal(&mathmap->internals, "X", CONST_X | CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "Y", CONST_X | CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "W", CONST_X | CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "H", CONST_X | CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "R", CONST_X | CONST_Y | CONST_T);
    register_internal(&mathmap->internals, "frame", CONST_X | CONST_Y);
}

int
does_mathmap_use_ra (mathmap_t *mathmap)
{
    internal_t *r_internal = lookup_internal(mathmap->internals, "r", 1);
    internal_t *a_internal = lookup_internal(mathmap->internals, "a", 1);

    assert(r_internal != 0 && a_internal != 0);

    return r_internal->is_used || a_internal->is_used;
}

int
does_mathmap_use_t (mathmap_t *mathmap)
{
    internal_t *t_internal = lookup_internal(mathmap->internals, "t", 1);

    assert(t_internal != 0);

    return t_internal->is_used;
}

int
check_mathmap (char *expression)
{
    static mathmap_t mathmap;

    mathmap.variables = 0;
    mathmap.userval_infos = 0;
    mathmap.internals = 0;
    mathmap.is_native = 0;
    mathmap.module_info = 0;

    mathmap.exprtree = 0;

    init_internals(&mathmap);

    the_mathmap = &mathmap;

    DO_JUMP_CODE {
	scanFromString(expression);
	yyparse();
	endScanningFromString();

	the_mathmap = 0;

	assert(mathmap.exprtree != 0);

	if (mathmap.exprtree->result.number != rgba_tag_number
	    || mathmap.exprtree->result.length != 4)
	{
	    free_exprtree(mathmap.exprtree);
	    mathmap.exprtree = 0;

	    sprintf(error_string, "The expression must have the result type rgba:4.");
	    JUMP(1);
	}
    } WITH_JUMP_HANDLER {
	the_mathmap = 0;
    } END_JUMP_HANDLER;

    if (mathmap.exprtree != 0)
    {
	free_exprtree(mathmap.exprtree);
	return 1;
    }
    return 0;
}

mathmap_t*
parse_mathmap (char *expression)
{
    static mathmap_t *mathmap;	/* this is static to avoid problems with longjmp.  */
    userval_info_t *info;

    mathmap = (mathmap_t*)malloc(sizeof(mathmap_t));

    mathmap->variables = 0;
    mathmap->userval_infos = 0;
    mathmap->internals = 0;
    mathmap->is_native = 0;
    mathmap->module_info = 0;
    mathmap->expression = 0;

    mathmap->exprtree = 0;

    init_internals(mathmap);

    the_mathmap = mathmap;

    DO_JUMP_CODE {
	scanFromString(expression);
	yyparse();
	endScanningFromString();

	the_mathmap = 0;

	assert(mathmap->exprtree != 0);

	if (mathmap->exprtree->result.number != rgba_tag_number
	    || mathmap->exprtree->result.length != 4)
	{
	    free_mathmap(mathmap);
	    mathmap = 0;

	    sprintf(error_string, "The expression must have the result type rgba:4.");
	    JUMP(1);
	}
    } WITH_JUMP_HANDLER {
	the_mathmap = 0;

	free_mathmap(mathmap);
	mathmap = 0;
    } END_JUMP_HANDLER;

    if (mathmap != 0)
    {
	mathmap->num_uservals = 0;
	for (info = mathmap->userval_infos; info != 0; info = info->next)
	    ++mathmap->num_uservals;
    }

    return mathmap;
}

mathmap_t*
compile_mathmap (char *expression, FILE *template, char *opmacros_filename)
{
    static mathmap_t *mathmap;	/* this is static to avoid problems with longjmp.  */
    static int try_compiler = 1;

    DO_JUMP_CODE {
	mathmap = parse_mathmap(expression);

	if (mathmap == 0)
	{
	    JUMP(1);
	}

	if (try_compiler)
	    mathmap->initfunc = gen_and_load_c_code(mathmap, &mathmap->module_info, template, opmacros_filename);

	if (!try_compiler || mathmap->initfunc == 0)
	{
	    mathmap->expression = make_postfix(mathmap->exprtree, &mathmap->exprlen);
	    output_postfix(mathmap->expression, mathmap->exprlen);

	    mathmap->is_native = 0;

	    if (try_compiler)
		fprintf(stderr, "falling back to interpreter\n");

	    try_compiler = 0;
	}
	else
	    mathmap->is_native = 1;
    } WITH_JUMP_HANDLER {
	the_mathmap = 0;

	if (mathmap != 0)
	{
	    free_mathmap(mathmap);
	    mathmap = 0;
	}
    } END_JUMP_HANDLER;

    return mathmap;
}

static void
init_invocation (mathmap_invocation_t *invocation)
{
    if (invocation->mathmap->is_native)
	invocation->mathfuncs = invocation->mathmap->initfunc(invocation);
}

mathmap_invocation_t*
invoke_mathmap (mathmap_t *mathmap, mathmap_invocation_t *template, int img_width, int img_height)
{
    mathmap_invocation_t *invocation = (mathmap_invocation_t*)malloc(sizeof(mathmap_invocation_t));

    invocation->mathmap = mathmap;

    invocation->mathfuncs.init_frame = 0;
    invocation->mathfuncs.calc_lines = 0;

    invocation->uservals = instantiate_uservals(mathmap->userval_infos);
#ifdef GIMP
    {
	userval_info_t *info;

	for (info = mathmap->userval_infos; info != 0; info = info->next)
	    if (info->type == USERVAL_IMAGE
		&& strcmp(info->name, INPUT_IMAGE_USERVAL_NAME) == 0)
	    {
		/* this is the original image */
		invocation->uservals[info->index].v.image.index = 0;
		break;
	    }
    }
#endif

    invocation->variables = instantiate_variables(mathmap->variables);

    invocation->internals = instantiate_internals(mathmap->internals);

    invocation->antialiasing = 0;
    invocation->supersampling = 0;

    invocation->output_bpp = 4;

    invocation->origin_x = invocation->origin_y = 0;

    invocation->img_width = img_width;
    invocation->img_height = img_height;

    invocation->image_W = img_width;
    invocation->image_H = img_height;

    invocation->middle_x = img_width / 2.0;
    invocation->middle_y = img_height / 2.0;

    if (invocation->middle_x > img_width - invocation->middle_x)
	invocation->image_X = invocation->middle_x;
    else
	invocation->image_X = img_width - invocation->middle_x;

    if (invocation->middle_y > img_height - invocation->middle_y)
	invocation->image_Y = invocation->middle_y;
    else
	invocation->image_Y = img_height - invocation->middle_y;
    
    invocation->image_R = hypot(invocation->image_X, invocation->image_Y);

    invocation->scale_x = invocation->scale_y = 1.0;

    invocation->current_r = invocation->current_a = 0.0;

    if (template != 0)
	carry_over_uservals_from_template(invocation, template);

    invocation->row_stride = img_width * 4;
    invocation->num_rows_finished = 0;

    if (!mathmap->is_native)
	invocation->stack_machine = make_postfix_machine();
    else
	invocation->stack_machine = 0;

    invocation->xy_vars = 0;
    invocation->y_vars = 0;

    invocation->do_debug = 0;

    init_invocation(invocation);

    return invocation;
}

void
enable_debugging (mathmap_invocation_t *invocation)
{
    invocation->do_debug = 1;
}

void
disable_debugging (mathmap_invocation_t *invocation)
{
    invocation->do_debug = 0;
}

static void
update_pixel_internals (mathmap_invocation_t *invocation, float x, float y, float r, float a)
{
    invocation->internals[X_INTERNAL_INDEX].data[0] = x;
    invocation->internals[Y_INTERNAL_INDEX].data[0] = y;

    invocation->internals[R_INTERNAL_INDEX].data[0] = r;
    invocation->internals[A_INTERNAL_INDEX].data[0] = a;
}

static void
write_tuple_to_pixel (tuple_t *tuple, guchar *dest, int output_bpp)
{
    float redf,
	greenf,
	bluef,
	alphaf;

    tuple_to_color(tuple, &redf, &greenf, &bluef, &alphaf);

    if (output_bpp == 1 || output_bpp == 2)
	dest[0] = (0.299 * redf + 0.587 * greenf + 0.114 * bluef) * 255;
    else if (output_bpp == 3 || output_bpp == 4)
    {
	dest[0] = redf * 255;
	dest[1] = greenf * 255;
	dest[2] = bluef * 255;
    }
    else
	assert(0);

    if (output_bpp == 2 || output_bpp == 4)
	dest[output_bpp - 1] = alphaf * 255;
}

void
init_frame (mathmap_invocation_t *invocation)
{
    if (invocation->mathmap->is_native)
	invocation->mathfuncs.init_frame(invocation);
}

static void
calc_lines (mathmap_invocation_t *invocation, int first_row, int last_row, unsigned char *q)
{
    if (invocation->mathmap->is_native)
	invocation->mathfuncs.calc_lines(invocation, first_row, last_row, q);
    else
    {
	int row, col;
	int output_bpp = invocation->output_bpp;
	int origin_x = invocation->origin_x, origin_y = invocation->origin_y;
	float middle_x = invocation->middle_x, middle_y = invocation->middle_y;
	float scale_x = invocation->scale_x, scale_y = invocation->scale_y;
	int uses_ra = does_mathmap_use_ra(invocation->mathmap);

	for (row = first_row; row < last_row; ++row)
	{
	    float y = middle_y - (float)(row + origin_y) * scale_y;
	    unsigned char *p = q;

	    for (col = 0; col < invocation->img_width; ++col)
	    {
		float x = (float)(col + origin_x) * scale_x - middle_x;
		float r, a;

		if (uses_ra)
		{
		    r = hypot(x, y);
		    if (r == 0.0)
			a = 0.0;
		    else
			a = acos(x / r);

		    if (y < 0)
			a = 2 * M_PI - a;
		}

		update_pixel_internals(invocation, x, y, r, a);

		write_tuple_to_pixel(eval_postfix(invocation), p, output_bpp);

		p += output_bpp;
	    }

	    q += invocation->row_stride;

	    if (!invocation->supersampling)
		invocation->num_rows_finished = row + 1;
	}
    }
}

void
call_invocation (mathmap_invocation_t *invocation, int first_row, int last_row, unsigned char *q)
{
    if (invocation->supersampling)
    {
	guchar *line1, *line2, *line3;
	float middle_x = invocation->middle_x, middle_y = invocation->middle_y;
	int row, col;
	int img_width = invocation->img_width;

	line1 = (guchar*)malloc((img_width + 1) * invocation->output_bpp);
	line2 = (guchar*)malloc(img_width * invocation->output_bpp);
	line3 = (guchar*)malloc((img_width + 1) * invocation->output_bpp);

	invocation->img_width = img_width + 1;
	calc_lines(invocation, first_row, first_row + 1, line1);

	for (row = first_row; row < last_row; ++row)
	{
	    char *p = q;

	    invocation->img_width = img_width;
	    invocation->middle_x = middle_x - 0.5;
	    invocation->middle_y = middle_y - 0.5;
	    calc_lines(invocation, row, row + 1, line2);

	    invocation->img_width = img_width + 1;
	    invocation->middle_x = middle_x;
	    invocation->middle_y = middle_y;
	    calc_lines(invocation, row + 1, row + 2, line3);
	    
	    for (col = 0; col < img_width; ++col)
	    {
		int i;

		for (i = 0; i < invocation->output_bpp; ++i)
		    p[i] = (line1[col*invocation->output_bpp+i]
			    + line1[(col+1)*invocation->output_bpp+i]
			    + 2*line2[col*invocation->output_bpp+i]
			    + line3[col*invocation->output_bpp+i]
			    + line3[(col+1)*invocation->output_bpp+i]) / 6;
		p += invocation->output_bpp;
	    }
	    memcpy(line1, line3, (img_width + 1) * invocation->output_bpp);

	    q += invocation->row_stride;

	    invocation->num_rows_finished = row + 1;
	}

	free(line1);
	free(line2);
	free(line3);
    }
    else
	calc_lines(invocation, first_row, last_row, q);
}

void
update_image_internals (mathmap_invocation_t *invocation)
{
    internal_t *internal;

    internal = lookup_internal(invocation->mathmap->internals, "t", 1);
    invocation->internals[internal->index].data[0] = invocation->current_t;

    internal = lookup_internal(invocation->mathmap->internals, "X", 1);
    invocation->internals[internal->index].data[0] = invocation->image_X;
    internal = lookup_internal(invocation->mathmap->internals, "Y", 1);
    invocation->internals[internal->index].data[0] = invocation->image_Y;
    
    internal = lookup_internal(invocation->mathmap->internals, "W", 1);
    invocation->internals[internal->index].data[0] = invocation->image_W;
    internal = lookup_internal(invocation->mathmap->internals, "H", 1);
    invocation->internals[internal->index].data[0] = invocation->image_H;
    
    internal = lookup_internal(invocation->mathmap->internals, "R", 1);
    invocation->internals[internal->index].data[0] = invocation->image_R;

    internal = lookup_internal(invocation->mathmap->internals, "frame", 1);
    invocation->internals[internal->index].data[0] = invocation->current_frame;
}

void
carry_over_uservals_from_template (mathmap_invocation_t *invocation, mathmap_invocation_t *template)
{
    userval_info_t *info;

    for (info = invocation->mathmap->userval_infos; info != 0; info = info->next)
    {
	userval_info_t *template_info = lookup_matching_userval(template->mathmap->userval_infos, info);

	if (template_info != 0)
	    copy_userval(&invocation->uservals[info->index], &template->uservals[template_info->index], info->type);
    }
}

#define MAX_TEMPLATE_VAR_LENGTH       64
#define is_word_character(c)          (isalnum((c)) || (c) == '_')

void
process_template_file (mathmap_t *mathmap, FILE *template, FILE *out, template_processor_func_t template_processor)
{
    int c;

    while ((c = fgetc(template)) != EOF)
    {
	if (c == '$')
	{
	    c = fgetc(template);
	    assert(c != EOF);

	    if (!is_word_character(c))
		putc(c, out);
	    else
	    {
		char name[MAX_TEMPLATE_VAR_LENGTH + 1];
		int length = 1;

		name[0] = c;

		do
		{
		    c = fgetc(template);

		    if (is_word_character(c))
		    {
			assert(length < MAX_TEMPLATE_VAR_LENGTH);
			name[length++] = c;
		    }
		    else
			if (c != EOF)
			    ungetc(c, template);
		} while (is_word_character(c));

		assert(length > 0 && length <= MAX_TEMPLATE_VAR_LENGTH);

		name[length] = '\0';

		if (!template_processor(mathmap, name, out))
		    assert(0);
	    }
	}
	else
	    putc(c, out);
    }
}
