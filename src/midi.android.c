/*
 * Copyright (C) 2020 <KichikuouChrome@gmail.com>
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

#include <SDL.h>
#include <jni.h>
#include "portab.h"
#include "midi.h"

static int midi_initilize(char *pname, int subdev);
static int midi_exit();
static int midi_start(int no, int loop, char *data, int datalen);
static int midi_stop();
static int midi_pause(void);
static int midi_unpause(void);
static int midi_get_playing_info(midiplaystate *st);
static int midi_getflag(int mode, int index);
static int midi_setflag(int mode, int index, int val);
static int midi_setvol(int vol);
static int midi_getvol();
static int midi_fadestart(int time, int volume, int stop);
static boolean midi_fading();

#define midi midi_android
mididevice_t midi = {
	midi_initilize,
	midi_exit,
	midi_start,
	midi_stop,
	midi_pause,
	midi_unpause,
	midi_get_playing_info,
	midi_getflag,
	midi_setflag,
	midi_setvol,
	midi_getvol,
	midi_fadestart,
	midi_fading
};

static int midino;

static int midi_initilize(char *pname, int subdev) {
	return OK;
}

static int midi_exit() {
	midi_stop();
	return OK;
}

static int midi_start(int no, int loop, char *data, int datalen) {
	if (midino == no)
		return OK;

	JNIEnv *env = SDL_AndroidGetJNIEnv();

	jbyteArray array = (*env)->NewByteArray(env, datalen);
	if (!array)
		return NG;
	jbyte *buf = (*env)->GetByteArrayElements(env, array, NULL);
	memcpy(buf, data, datalen);
	(*env)->ReleaseByteArrayElements(env, array, buf, 0);

	jobject context = SDL_AndroidGetActivity();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
										"midiStart", "([BI)V");
	(*env)->CallVoidMethod(env, context, mid, array, loop);
	(*env)->DeleteLocalRef(env, array);
	(*env)->DeleteLocalRef(env, context);

	midino = no;
	return OK;
}

static int midi_stop() {
	JNIEnv *env = SDL_AndroidGetJNIEnv();
	jobject context = SDL_AndroidGetActivity();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
										"midiStop", "()V");
	(*env)->CallVoidMethod(env, context, mid);
	(*env)->DeleteLocalRef(env, context);

	midino = 0;
	return OK;
}

static int midi_pause(void) {
	return NG; // FIXME
}

static int midi_unpause(void) {
	return NG; // FIXME
}

static int midi_get_playing_info(midiplaystate *st) {
	st->in_play = FALSE;
	st->play_no = 0;
	st->loc_ms  = 0;
	if (midino == 0)
		return OK;

	JNIEnv *env = SDL_AndroidGetJNIEnv();
	jobject context = SDL_AndroidGetActivity();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
										"midiCurrentPosition", "()I");
	int pos = (*env)->CallIntMethod(env, context, mid);
	(*env)->DeleteLocalRef(env, context);

	if (pos >= 0) {
		st->in_play = TRUE;
		st->play_no = midino;
		st->loc_ms = pos;
	}
	return OK;
}

static int midi_getflag(int mode, int index) {
	return 0;
}

static int midi_setflag(int mode, int index, int val) {
	return NG;
}

static int midi_setvol(int vol) {
	return NG; // FIXME
}

static int midi_getvol() {
	return 100; // FIXME
}

static int midi_fadestart(int time, int volume, int stop) {
	return NG; // FIXME
}

static boolean midi_fading() {
	return FALSE; // FIXME
}
