/*
 * sdl_darw.c  SDL draw to surface
 *
 * Copyright (C) 2000-     Fumihiko Murata       <fmurata@p1.tcnet.ne.jp>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/
/* $Id: sdl_draw.c,v 1.13 2003/01/25 01:34:50 chikama Exp $ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "portab.h"
#include "system.h"
#include "sdl_core.h"
#include "sdl_private.h"
#include "font.h"
#include "ags.h"
#include "image.h"
#include "nact.h"
#include "input.h"

static int fadestep[256] =
{0,1,3,4,6,7,9,10,12,14,15,17,18,20,21,23,25,26,28,29,31,32,34,36,37,39,40,
 42,43,45,46,48,49,51,53,54,56,57,59,60,62,63,65,66,68,69,71,72,74,75,77,78,
 80,81,83,84,86,87,89,90,92,93,95,96,97,99,100,102,103,105,106,108,109,110,
 112,113,115,116,117,119,120,122,123,124,126,127,128,130,131,132,134,135,136,
 138,139,140,142,143,144,146,147,148,149,151,152,153,155,156,157,158,159,161,
 162,163,164,166,167,168,169,170,171,173,174,175,176,177,178,179,181,182,183,
 184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,
 203,204,205,206,207,208,209,210,211,211,212,213,214,215,216,217,217,218,219,
 220,221,221,222,223,224,225,225,226,227,227,228,229,230,230,231,232,232,233,
 234,234,235,235,236,237,237,238,238,239,239,240,241,241,242,242,243,243,244,
 244,244,245,245,246,246,247,247,247,248,248,249,249,249,250,250,250,251,251,
 251,251,252,252,252,252,253,253,253,253,254,254,254,254,254,254,255,255,255,
 255,255,255,255,255,255,255,255,255,255,255};

static SDL_Surface *s_fader;  /* fade in /out 用 work surface */

static void sdl_pal_check(void) {
	if (nact->sys_pal_changed) {
		nact->sys_pal_changed = FALSE;
		sdl_setPallet(nact->sys_pal, 0, 256);
	}
}

#ifdef VITA
extern int mousex;
extern int mousey;
extern float joydir_x;
extern float joydir_y;

static float joystick_dead_zone = 0.25;
static int joystick_speed = 16;
static SceUInt64 joy_time;
static boolean hide_cursor = TRUE;

SDL_Point sw_cursor_outline[8] = {
	{  0, 0  },
	{  0, 14 },
	{  3, 11 },
	{  7, 18 },
	{  9, 17 },
	{  6, 10 },
	{ 10, 10 },
	{  0, 0  }
};

SDL_Rect sw_cursor[11] = {
	{ .x = 1, .y = 2,  .w = 1, .h = 11 },
	{ .x = 2, .y = 3,  .w = 1, .h = 9  },
	{ .x = 3, .y = 4,  .w = 1, .h = 7  },
	{ .x = 4, .y = 5,  .w = 1, .h = 7 },
	{ .x = 5, .y = 6,  .w = 1, .h = 8 },
	{ .x = 6, .y = 7,  .w = 1, .h = 3 },
	{ .x = 7, .y = 8,  .w = 1, .h = 2 },
	{ .x = 8, .y = 9,  .w = 1, .h = 1 },
	{ .x = 6, .y = 12, .w = 1, .h = 4 },
	{ .x = 7, .y = 14, .w = 1, .h = 4 },
	{ .x = 8, .y = 16, .w = 1, .h = 2 }
};

static void render_sw_cursor(void)
{
	SDL_Rect cursor_buf[11];
	SDL_Point outline_buf[8];
	memcpy(cursor_buf, sw_cursor, sizeof(cursor_buf));
	memcpy(outline_buf, sw_cursor_outline, sizeof(outline_buf));
	for (int i = 0; i < 11; i++) {
		cursor_buf[i].x += renderoffset_x + mousex*renderscale;
		cursor_buf[i].y += renderoffset_y + mousey*renderscale;
	}
	for (int i = 0; i < 8; i++) {
		outline_buf[i].x += renderoffset_x + mousex*renderscale;
		outline_buf[i].y += renderoffset_y + mousey*renderscale;
	}
	SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRects(sdl_renderer, cursor_buf, 11);
	SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLines(sdl_renderer, outline_buf, 8);
}

static void send_agsevent(int type, int code)
{
	if (!nact->ags.eventcb)
		return;
	agsevent_t agse;
	agse.type = type;
	agse.d1 = mousex;
	agse.d2 = mousey;
	agse.d3 = code;
	nact->ags.eventcb(&agse);
}

void sdl_updateScreen(void)
{
	SDL_Rect dst;

	// translate joystick to mouse movement
	// we do this here to ensure it runs exactly once per frame
	if (fabsf(joydir_x) > joystick_dead_zone || fabs(joydir_y) > joystick_dead_zone) {
		joy_time = sceKernelGetProcessTimeWide();
		mousex = max(0, min(view_w-1, mousex + joydir_x * joystick_speed));
		mousey = max(0, min(view_h-1, mousey + joydir_y * joystick_speed));
		send_agsevent(AGSEVENT_MOUSE_MOTION, 0);
		hide_cursor = FALSE;
		sdl_dirty = TRUE;
	} else if (!hide_cursor && sceKernelGetProcessTimeWide() - joy_time > 750000) {
		hide_cursor = TRUE;
		sdl_dirty = TRUE;
	}

	if (!sdl_dirty)
		return;
	SDL_UpdateTexture(sdl_texture, NULL, sdl_display->pixels, sdl_display->pitch);
	SDL_RenderClear(sdl_renderer);
	setRect(dst, renderoffset_x, renderoffset_y, view_w*renderscale, view_h*renderscale);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &dst);
	if (!hide_cursor)
		render_sw_cursor();
	SDL_RenderPresent(sdl_renderer);
	sdl_dirty = false;

}

#else

void sdl_updateScreen(void) {
	if (!sdl_dirty)
		return;
	SDL_UpdateTexture(sdl_texture, NULL, sdl_display->pixels, sdl_display->pitch);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);
	sdl_dirty = false;
}
#endif

void sdl_sleep(int msec) {
	sdl_updateScreen();

#ifdef __EMSCRIPTEN__
	emscripten_sleep(msec);
#else
	SDL_Delay(msec);
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(void, wait_vsync, (void), {
	Asyncify.handleSleep(function(wakeUp) {
		window.requestAnimationFrame(function() {
			wakeUp();
		});
	});
});
#endif

void sdl_wait_vsync() {
	sdl_updateScreen();
#ifdef __EMSCRIPTEN__
	wait_vsync();
#else
	SDL_Delay(16);
#endif
}

/* off-screen の指定領域を Main Window へ転送 */
void sdl_updateArea(MyRectangle *src, MyPoint *dst) {
	SDL_Rect rect_s, rect_d;
	
	setRect(rect_s, src->x, src->y, src->width, src->height);
	setRect(rect_d, winoffset_x + dst->x, winoffset_y + dst->y, src->width, src->height);
	
	SDL_BlitSurface(sdl_dib, &rect_s, sdl_display, &rect_d);
	
	sdl_dirty = TRUE;
}

/* 全画面更新 */
static void sdl_updateAll() {
	SDL_Rect rect;

	setRect(rect, winoffset_x, winoffset_y, view_w, view_h);
	
	SDL_BlitSurface(sdl_dib, &sdl_view, sdl_display, &rect);

	sdl_dirty = TRUE;
}

/* Color の複数個指定 */
void sdl_setPallet(Pallet256 *pal, int src, int cnt) {
	int i;
	
	for (i = 0; i < cnt; i++) {
		sdl_col[src + i].r = pal->red  [src + i];
		sdl_col[src + i].g = pal->green[src + i];
		sdl_col[src + i].b = pal->blue [src + i];
	}
	
	if (sdl_dib->format->BitsPerPixel == 8) {
		SDL_SetPaletteColors(sdl_dib->format->palette, sdl_col, src, cnt);
	}
}

/* 矩形の描画 */
void sdl_drawRectangle(int x, int y, int w, int h, int c) {
	SDL_Rect rect;
	
	sdl_pal_check();
	
	if (c < 256 && sdl_dib->format->BitsPerPixel > 8)
		c = SDL_MapRGB(sdl_dib->format, sdl_col[c].r, sdl_col[c].g, sdl_col[c].b);

	setRect(rect,x,y,w,1);
	SDL_FillRect(sdl_dib, &rect, c);
	
	setRect(rect,x,y,1,h);
	SDL_FillRect(sdl_dib, &rect, c);
	
	setRect(rect,x,y+h-1,w,1);
	SDL_FillRect(sdl_dib, &rect, c);
	
	setRect(rect,x+w-1,y,1,h);
	SDL_FillRect(sdl_dib, &rect, c);
}

/* 矩形塗りつぶし */
void sdl_fillRectangle(int x, int y, int w, int h, unsigned long c) {
	SDL_Rect rect;
	
	sdl_pal_check();
	
	setRect(rect,x,y,w,h);
	
	if (c < 256 && sdl_dib->format->BitsPerPixel > 8)
		c = SDL_MapRGB(sdl_dib->format, sdl_col[c].r, sdl_col[c].g, sdl_col[c].b);
	
	SDL_FillRect(sdl_dib, &rect, c);
}

/* 領域コピー */
void sdl_copyArea(int sx, int sy, int w, int h, int dx, int dy) {
	if (sx == dx && sy == dy)
		return;

	SDL_Rect r_src, r_dst;
	setRect(r_src, sx, sy, w, h);
	setRect(r_dst, dx, dy, w, h);

	SDL_Rect intersect;
	if (SDL_IntersectRect(&r_src, &r_dst, &intersect)) {
		void* region = sdl_saveRegion(sx, sy, w, h);
		sdl_restoreRegion(region, dx, dy);
	} else {
		SDL_BlitSurface(sdl_dib, &r_src, sdl_dib, &r_dst);
	}
}

/*
 * dib に指定のパレット sp を抜いてコピー
 */
void sdl_copyAreaSP(int sx, int sy, int w, int h, int dx, int dy, int sp) {
	SDL_Rect r_src, r_dst;

	sdl_pal_check();
	
	if (sdl_dib->format->BitsPerPixel > 8 && sp < 256) {
		sp = SDL_MapRGB(sdl_dib->format,
				sdl_col[sp].r & 0xf8,
				sdl_col[sp].g & 0xfc,
				sdl_col[sp].b & 0xf8);
	}
	
	SDL_SetColorKey(sdl_dib, SDL_TRUE, sp);
	
	setRect(r_src, sx, sy, w, h);
	setRect(r_dst, dx, dy, w, h);
	
	SDL_BlitSurface(sdl_dib, &r_src, sdl_dib, &r_dst);
	SDL_SetColorKey(sdl_dib, SDL_FALSE, 0);
}

void sdl_drawImage8_fromData(cgdata *cg, int dx, int dy, int w, int h) {
	SDL_Surface *s;
	SDL_Rect r_src, r_dst;
	
	s = SDL_AllocSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);

	SDL_LockSurface(s);

#if 0  /* for broken cg */
	if (s->pitch == s->w) {
		memcpy(s->pixels, cg->pic, w * h);
	} else 
#endif
	{
		int i = h;
		BYTE *p_src = (cg->pic + cg->data_offset), *p_dst = s->pixels;
		
		while (i--) {
			memcpy(p_dst, p_src, w);
			p_dst += s->pitch;
			p_src += cg->width;
		}
	}
	
	SDL_UnlockSurface(s);
	
	sdl_pal_check();
	
	if (sdl_dib->format->BitsPerPixel > 8 && cg->pal) {
		int i, i_st = 0, i_end = 256;
		SDL_Color *c = s->format->palette->colors;
		BYTE *r = cg->pal->red, *g = cg->pal->green, *b = cg->pal->blue;
		
		if (cg->type == ALCG_VSP) {
			i_st  = (cg->vsp_bank << 4);
			i_end = i_st + 16;
			c += i_st;
		}
		for (i = i_st; i < i_end; i++) {
			c->r = *(r++);
			c->g = *(g++);
			c->b = *(b++);
			c++;
		}
	} else {
		memcpy(s->format->palette->colors, sdl_col, sizeof(SDL_Color) * 256);
	}
	
	if (cg->spritecolor != -1) {
		SDL_SetColorKey(s, SDL_TRUE, cg->spritecolor);
	}
	
	setRect(r_src,  0,  0, w, h);
	setRect(r_dst, dx, dy, w, h);
	
	SDL_BlitSurface(s, &r_src, sdl_dib, &r_dst);
	SDL_FreeSurface(s);
}

/* 直線描画 */
void sdl_drawLine(int x1, int y1, int x2, int y2, unsigned long cl) {

	sdl_pal_check();
	
	if (sdl_dib->format->BitsPerPixel > 8 && cl < 256) {
		cl = SDL_MapRGB(sdl_dib->format,
				sdl_col[cl].r, sdl_col[cl].g, sdl_col[cl].b);
	}
	
	SDL_LockSurface(sdl_dib);

	image_drawLine(sdl_dibinfo, x1, y1, x2, y2, cl);
	
	SDL_UnlockSurface(sdl_dib);
	
}

static agsurface_t* surface2com(SDL_Surface *src) {
	agsurface_t *dst = malloc(sizeof(agsurface_t));
	
	dst->depth           = src->format->BitsPerPixel;
	dst->bytes_per_pixel = src->format->BytesPerPixel;
	dst->bytes_per_line  = src->pitch;
	dst->pixel   = src->pixels;
	dst->width  = src->w;
	dst->height = src->h;
	
	return dst;
}

static SDL_Surface *com2surface(agsurface_t *src) {
	SDL_Surface *s;
	int y;
	BYTE *sp, *dp;
	
	s = SDL_AllocSurface(SDL_SWSURFACE, src->width, src->height, src->depth, 0, 0, 0, 0);
	
	SDL_LockSurface(s);
	
	sp = s->pixels;
	dp = src->pixel;
	
	for (y = 0; y < src->height; y++) {
		memcpy(sp, dp, src->width);
		sp += s->pitch;
		dp += src->bytes_per_line;
	}
	
	SDL_UnlockSurface(s);
	return s;
}

static SDL_Surface *com2alphasurface(agsurface_t *src, int cl) {
	SDL_Surface *s;
	int x,y;
	BYTE *sp, *dp;
	SDL_Rect r_src;
	
	s = SDL_CreateRGBSurfaceWithFormat(0, src->width, src->height, 32, SDL_PIXELFORMAT_ARGB8888);
	
	setRect(r_src, 0, 0, src->width, src->height);
	SDL_FillRect(s, &r_src, RGB_PIX24(sdl_col[cl].r, sdl_col[cl].g, sdl_col[cl].b));

	SDL_LockSurface(s);
	
	for (y = 0; y < src->height; y++) {
		sp = src->pixel + y * src->bytes_per_line;
		dp = s->pixels + y * s->pitch;
#ifndef WORDS_BIGENDIAN
		dp += s->format->BytesPerPixel -1;
#endif
		
		for (x = 0; x < src->width; x++) {
			*dp = *sp;
			sp++;
			dp += s->format->BytesPerPixel;
		}
	}
	
	SDL_UnlockSurface(s);
	return s;
}

int sdl_nearest_color(int r, int g, int b) {
	int i, col, mind = INT_MAX;
	for (i = 0; i < 256; i++) {
		int dr = r - sdl_col[i].r;
		int dg = g - sdl_col[i].g;
		int db = b - sdl_col[i].b;
		int d = dr*dr*30 + dg*dg*59 + db*db*11;
		if (d < mind) {
			mind = d;
			col = i;
		}
	}
	return col;
}

static void sdl_drawAntiAlias_8bpp(int dstx, int dsty, agsurface_t *src, unsigned long col) {
	int x, y;
	SDL_LockSurface(sdl_dib);

	Uint8 cache[256*7];
	memset(cache, 0, 256);

	for (y = 0; y < src->height; y++) {
		BYTE* sp = src->pixel + y * src->bytes_per_line;
		BYTE* dp = sdl_dib->pixels + (dsty + y) * sdl_dib->pitch + dstx;
		for (x = 0; x < src->width; x++) {
			int alpha = *sp >> 5;
			if (alpha == 0) {
				// Transparent, do nothing
			} else if (alpha == 7) {
				*dp = col;
			} else if (cache[*dp] & 1 << alpha) {
				*dp = cache[alpha << 8 | *dp];
			} else {
				cache[*dp] |= 1 << alpha;
				int c = sdl_nearest_color(
					(sdl_col[col].r * alpha + sdl_col[*dp].r * (7 - alpha)) / 7,
					(sdl_col[col].g * alpha + sdl_col[*dp].g * (7 - alpha)) / 7,
					(sdl_col[col].b * alpha + sdl_col[*dp].b * (7 - alpha)) / 7);
				cache[alpha << 8 | *dp] = c;
				*dp = c;
			}
			sp++;
			dp++;
		}
	}

	SDL_UnlockSurface(sdl_dib);
}

int sdl_drawString(int x, int y, const char *msg, unsigned long col) {
	int w;

	sdl_pal_check();
	
	if (sdl_font->self_drawable()) {
		w = sdl_font->draw_glyph(x, y, msg, col);
	} else {
		agsurface_t *glyph = sdl_font->get_glyph(msg);
		SDL_Rect r_src, r_dst;
		
		if (glyph == NULL) return 0;
		setRect(r_src, 0, 0, glyph->width, glyph->height);
		setRect(r_dst, x, y, glyph->width, glyph->height);
		if (!sdl_font->antialiase_on) {
			int i;
			SDL_Surface *src = com2surface(glyph);
			for (i = 1; i < 256; i++) { 
				memcpy(src->format->palette->colors + i, &sdl_col[col],
				       sizeof(SDL_Color));
			}
			SDL_SetColorKey(src, SDL_TRUE, 0);
			SDL_BlitSurface(src, &r_src, sdl_dib, &r_dst);
			SDL_FreeSurface(src);
		} else if (sdl_dib->format->BitsPerPixel == 8) {
			sdl_drawAntiAlias_8bpp(x, y, glyph, col);
		} else {
			SDL_Surface *src = com2alphasurface(glyph, col);

			SDL_BlitSurface(src, &r_src, sdl_dib, &r_dst);
			SDL_FreeSurface(src);
		}
		w = glyph->width;
	}
	
	return w;
}

void sdl_Mosaic(int sx, int sy, int w, int h, int dx, int dy, int slice) {
	
	SDL_LockSurface(sdl_dib);

	image_Mosaic(sdl_dibinfo, sx, sy, w, h, dx, dy, slice); 

	SDL_UnlockSurface(sdl_dib);
}

static void setBligtness(SDL_Surface *s, int val) {
	int i;
	Pallet256 *pal = nact->sys_pal;
	Uint8 *r = pal->red, *g = pal->green, *b = pal->blue;
	SDL_Color *cl = sdl_col;
	
	for (i = 0; i < 256; i++) {
		cl->r = (val * (*(r++))) / 255;
		cl->g = (val * (*(g++))) / 255;
		cl->b = (val * (*(b++))) / 255;
		cl++;
	}
	SDL_SetPaletteColors(s->format->palette, sdl_col, 0, 256);
}

static void setWhiteness(SDL_Surface *s, int val) {
	int i;
	Pallet256 *pal = nact->sys_pal;
	Uint8 *r = pal->red, *g = pal->green, *b = pal->blue;
	SDL_Color *cl = sdl_col;
	
	for (i = 0; i < 256; i++) {
		cl->r = (((255- *r) * val) / 256) + *r; r++;
		cl->g = (((255- *g) * val) / 256) + *g; g++;
		cl->b = (((255- *b) * val) / 256) + *b; b++;
		cl++;
	}
	SDL_SetPaletteColors(s->format->palette, sdl_col, 0, 256);
}

static void fader_in(int n) {
	static agsurface_t *work, *disp;
	
	if (n == 0) {
		SDL_Rect r_src, r_dst;
		
		s_fader = SDL_AllocSurface(sdl_dib->flags, sdl_display->w, sdl_display->h,
					   sdl_display->format->BitsPerPixel, 0, 0, 0, 0);
		
		if (sdl_display->format->BitsPerPixel == 8) {
			memcpy(s_fader->format->palette->colors,
			       sdl_display->format->palette->colors,
			       sizeof(SDL_Color) * 256);
		}
		setRect(r_src, view_x, view_y, view_w, view_h);
		setRect(r_dst, winoffset_x, winoffset_y, view_w, view_h);
		SDL_BlitSurface(sdl_dib, &r_src, s_fader, &r_dst);
		
		work = surface2com(s_fader);
		disp = surface2com(sdl_display);
	}
	
	if (n == 255) {
		SDL_FreeSurface(s_fader);
		sdl_updateAll();
		free(work);
		free(disp);
		return;
	}
	
	SDL_LockSurface(s_fader);
	SDL_LockSurface(sdl_display);
	
	image_fadeIn(work, disp, n / 16);
	
	SDL_UnlockSurface(sdl_display);
	SDL_UnlockSurface(s_fader);
	sdl_dirty = TRUE;
}

static void fader_out(int n,Uint32 c) {
	static agsurface_t *disp;

	if (n == 0) {
		disp = surface2com(sdl_display);
	}
	
	if (n == 255) {
		SDL_FillRect(sdl_display, NULL, c);
		free(disp);
		return;
	}
	
	SDL_LockSurface(sdl_display);
	
	image_fadeOut(disp, (255 - n) / 16, c);
	
	SDL_UnlockSurface(sdl_display);
	
	sdl_dirty = TRUE;
}

static __inline void sdl_fade_blit(void) {
	SDL_Rect r_dst;
	setRect(r_dst, winoffset_x, winoffset_y, view_w, view_h);

	SDL_BlitSurface(sdl_dib, &sdl_view, sdl_display, &r_dst);
	sdl_dirty = TRUE;
}

void sdl_fadeIn(int step) {
	if (sdl_dib->format->BitsPerPixel == 8) {
		setBligtness(sdl_dib, fadestep[step]);
		sdl_fade_blit();
	} else {
		fader_in(step);
	}
}

void sdl_fadeOut(int step) {
	if (sdl_dib->format->BitsPerPixel == 8) {
		setBligtness(sdl_dib, fadestep[255 - step]);
		sdl_fade_blit();
	} else {
		fader_out(step, SDL_MapRGB(sdl_display->format, 0, 0, 0));
	}
}

void sdl_whiteIn(int step) {
	if (sdl_dib->format->BitsPerPixel == 8) {
		setWhiteness(sdl_dib, fadestep[255 - step]); /* ??? */
		sdl_fade_blit();
	} else {
		fader_in(step);
	}
}

void sdl_whiteOut(int step) {
	if (sdl_dib->format->BitsPerPixel == 8) {
		setWhiteness(sdl_dib, fadestep[step]); /* ??? */
		sdl_fade_blit();
	} else {
		fader_out(step, SDL_MapRGB(sdl_display->format, 255, 255, 255));
	}
}

/*
 * 指定範囲にパレット col を rate の割合で重ねる CK1
 */
void sdl_wrapColor(int sx, int sy, int w, int h, int cl, int rate) {
	SDL_Surface *s;
	SDL_Rect r_src,r_dst;

	s = SDL_AllocSurface(SDL_SWSURFACE, w, h,
			     sdl_dib->format->BitsPerPixel, 0, 0, 0, 0);
	
	if (s->format->BitsPerPixel == 8) {
		memcpy(s->format->palette->colors, sdl_dib->format->palette->colors,
		       sizeof(SDL_Color)*256);
	} else {
		cl = (cl < 256) ? SDL_MapRGB(sdl_dib->format, sdl_col[cl].r, sdl_col[cl].g, sdl_col[cl].b) : cl;
	}
	
	setRect(r_src, 0, 0, w, h);
	SDL_FillRect(s, &r_src, cl);
	
	SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(s, rate);
	setRect(r_dst, sx, sy, w, h);
	SDL_BlitSurface(s, &r_src, sdl_dib, &r_dst);
	SDL_FreeSurface(s);
}

/* mask update まだ */
void sdl_maskupdate(int sx, int sy, int w, int h, int dx, int dy, int func, int step) {

	if (step == 256) {
		ags_copyArea(sx, sy, w, h, dx, dy);
		ags_updateArea(dx, dy, w, h);
	}
}
