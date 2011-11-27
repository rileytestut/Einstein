// ==============================
// File:			AndroidGlue.c
// Project:			Einstein
//
// Copyright 2011 by Matthias Melcher (einstein@matthiasm.com)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
// ==============================
// $Id$
// ==============================

#include <app/AndroidGlue.h>
#include <app/TAndroidApp.h>
#include <Emulator/Screen/TScreenManager.h>
#include <Platform/TPlatformManager.h>
#include <string.h>
#include <dlfcn.h>

#include <android/bitmap.h>


TAndroidApp *theApp = 0;

/*
 We need to find a way to stop all threads when the Java part of our app 
 vanishes by either getting closed, killed, or uninstalled. Maybe implementing
 these is helpful? It seems not, so we may have to implement a heartbeat :-(
 
 jint JNI_OnLoad(JavaVM* vm, void* reserved);
 void JNI_OnUnload(JavaVM* vm, void* reserved);
 
 also, what do these do?
 
 struct _JavaVM {
	jint AttachCurrentThread(JNIEnv** p_env, void* thr_args)
	...
 }
*/ 

#if 0
/* This function is called very early, as expected */
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGE("***** JNI_OnLoad called! ******");
	return JNI_VERSION_1_4;
}

/* this function is not called when deactivating or killing the app! */
void JNI_OnUnload(JavaVM*, void*)
{
	LOGE("***** JNI_OnUnload called! ******");
}
#endif


/*
 We need to call Java functions from C++. This should be the way:
 
 jclass GetObjectClass(jobject obj);
 jmethodID GetMethodID(jclass clazz, const char* name, const char* sig);
	sig is ()V for void fn(vioid) or (II)V for void fn(int, int), etc.
 void CallVoidMethod(jobject obj, jmethodID methodID, ...);
 void CallNonvirtualVoidMethod(jobject obj, jclass clazz, jmethodID methodID, ...)

 */

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. 
 */
JNIEXPORT jstring JNICALL Java_com_example_einstein_einstein_stringFromJNI( JNIEnv* env, jobject thiz )
{
    LOGI("Testing Android %s. Seems fine so far!", "NDK");
    return env->NewStringUTF("This is the Einstein native interface (5)!");
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_initEmulator( JNIEnv* env, jobject thiz )
{
	LOGI("initEmulator: start");
	theApp = new TAndroidApp();
	
	// TODO: this is just a rough test if we can find the audio library.
	// Android 2.2 does not have any support for audio from native code. Loading
	// libaudio the Unix way may give us audio support on many devices neverthelee.
	void *mLibHandle = dlopen("libaudio.so", 0);
	if (mLibHandle) {
		// http://source.android.com/porting/audio.html
		// http://source.android.com/porting/AudioHardwareInterface_8h-source.html
		// Galaxy Tab: "/sdcard/external_sd/" "/sdcard/sd/"
		LOGE("YAY, libaudio is here!\n");
		void *fn = dlsym(mLibHandle, "createAudioHardware");
		if (fn) {
			LOGE("YAY again: we have the right funciton ************");
		} else {
			LOGE("BOOHOO");
		}
	} else {
		LOGE("dlopen failed on libaudio.so %s\n", dlerror());
	}
	
	LOGI("initEmulator: done");
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_runEmulator( JNIEnv* env, jobject thiz, jstring dataPath )
{
	jboolean isCopy;
	const char *cDataPath = env->GetStringUTFChars(dataPath, &isCopy);
	LOGI("runEmulator: start (dataPath=%s)", cDataPath);
	theApp->Run(cDataPath);
	env->ReleaseStringUTFChars(dataPath, cDataPath);
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_stopEmulator( JNIEnv* env, jobject thiz )
{
	LOGI("stopEmulator");
	if (theApp) theApp->Stop();
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_powerOnEmulator( JNIEnv* env, jobject thiz )
{
	LOGI("powerOnEmulator");
	if (theApp) theApp->PowerOn();
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_powerOffEmulator( JNIEnv* env, jobject thiz )
{
	LOGI("powerOffEmulator");
	if (theApp) theApp->PowerOff();
}


JNIEXPORT void JNICALL Java_com_example_einstein_EinsteinView_penDown( JNIEnv* env, jobject thiz, jint x, jint y )
{
	if (theApp) {
		TScreenManager *tsm = theApp->getScreenManager();
		if (tsm) {
			if (!theApp->getPlatformManager()->IsPowerOn()) {
				// After five minutes, the MP switches itself off. On Android,
				// the host OS should decide when to put the device to sleep.
				// Newton OS nevertheless switches itself off. To continue work,
				// we'd have to pull the power switch. Since we have no power switch 
				// on Einstein/IOS, any screen touch will power the Newton back on.
				theApp->PowerOn();
				//theApp->getPlatformManager()->SendPowerSwitchEvent();
			}
			//LOGI("Sending pen down at %d, %d", x, y);
			tsm->PenDown(480-y, x);
		}
	}
}


JNIEXPORT void JNICALL Java_com_example_einstein_EinsteinView_penUp( JNIEnv* env, jobject thiz)
{
	if (theApp) {
		TScreenManager *tsm = theApp->getScreenManager();
		if (tsm) {
			tsm->PenUp();
		}
	}
}


/* -- this will be called by a thread in C++, which is probably not a good idea at all.
static jobject		tasmObject = 0;
static jclass		tasmClass = 0;
static jmethodID	tasmMethodInvalidate = 0;

void callScreenInvalidate() 
{
	if (tasmClass) {
		CallVoidMethod(tasmObject, tasmMethodInvalidate);
	}
}
*/

JNIEXPORT jint JNICALL Java_com_example_einstein_EinsteinView_renderEinsteinView(JNIEnv * env, jobject obj, jobject bitmap)
{
/*	
	if (!tasmClass) {
		tasmObject = obj;
		tasmClass = GetObjectClass(obj);
		tasmMethodInvalidate = GetMethodID(tasmClass, "invalidate", "()V");
	}
*/
	
    AndroidBitmapInfo	info;
    void				*pixels;
    int					ret = 0;
    static int			c = 0;

    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return ret;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
        LOGE("Bitmap format is not RGB_565 !");
        return ret;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return ret;
    }

    unsigned short *p = (unsigned short*)pixels;
	if (theApp)
	{
		ret = theApp->updateScreen(p);
	}
	
    //p[c] = 0xf00f;
    //p[(c+160)%320] = 0x0ff0;
    //c++;
    //if (c>320) c = 0;

    AndroidBitmap_unlockPixels(env, bitmap);
	
	return ret;
}

JNIEXPORT jint JNICALL Java_com_example_einstein_einstein_screenIsDirty( JNIEnv* env, jobject thiz )
{
	int ret = 0;
	if (theApp)
	{
		ret = theApp->screenIsDirty();
	}	
	return ret;
}


JNIEXPORT void JNICALL Java_com_example_einstein_einstein_setBacklight( JNIEnv* env, jobject thiz, jint v )
{
	if (theApp) {
		TScreenManager *tsm = theApp->getScreenManager();
		if (tsm) {
			tsm->SetBacklight(v);
		}
	}
}


JNIEXPORT jint JNICALL Java_com_example_einstein_einstein_backlightIsOn( JNIEnv* env, jobject thiz )
{
	if (theApp) {
		TScreenManager *tsm = theApp->getScreenManager();
		if (tsm) {
			return tsm->GetBacklight() ? 1 : 0;
		}
	}
	return -1;
}


// the font below was partially extracted from a Linux terminal
// screenshot and partially created by hand. Wonko.
unsigned char simple_font[128][13] = {
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, //
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
	{ 0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x10,0x00,0x00,0x00}, // '!'
	{ 0x00,0x24,0x24,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '"'
	{ 0x00,0x00,0x24,0x24,0x7e,0x24,0x7e,0x24,0x24,0x00,0x00,0x00,0x00}, // '#'
	{ 0x00,0x10,0x3c,0x50,0x50,0x38,0x14,0x14,0x78,0x10,0x00,0x00,0x00}, // '$'
	{ 0x00,0x22,0x52,0x24,0x08,0x08,0x10,0x24,0x2a,0x44,0x00,0x00,0x00}, // '%'
	{ 0x00,0x00,0x00,0x30,0x48,0x48,0x30,0x4a,0x44,0x3a,0x00,0x00,0x00}, // '&'
	{ 0x00,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '''
	{ 0x00,0x04,0x08,0x08,0x10,0x10,0x10,0x08,0x08,0x04,0x00,0x00,0x00}, // '('
	{ 0x00,0x20,0x10,0x10,0x08,0x08,0x08,0x10,0x10,0x20,0x00,0x00,0x00}, // ')'
	{ 0x00,0x00,0x00,0x24,0x18,0x7e,0x18,0x24,0x00,0x00,0x00,0x00,0x00}, // '*'
	{ 0x00,0x00,0x00,0x10,0x10,0x7c,0x10,0x10,0x00,0x00,0x00,0x00,0x00}, // '+'
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x30,0x40,0x00,0x00}, // ','
	{ 0x00,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '-'
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x38,0x10,0x00,0x00}, // '.'
	{ 0x00,0x02,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x80,0x00,0x00,0x00}, // '/'
	{ 0x00,0x18,0x24,0x42,0x42,0x42,0x42,0x42,0x24,0x18,0x00,0x00,0x00}, // '0'
	{ 0x00,0x10,0x30,0x50,0x10,0x10,0x10,0x10,0x10,0x7c,0x00,0x00,0x00}, // '1'
	{ 0x00,0x3c,0x42,0x42,0x02,0x04,0x18,0x20,0x40,0x7e,0x00,0x00,0x00}, // '2'
	{ 0x00,0x7e,0x02,0x04,0x08,0x1c,0x02,0x02,0x42,0x3c,0x00,0x00,0x00}, // '3'
	{ 0x00,0x04,0x0c,0x14,0x24,0x44,0x44,0x7e,0x04,0x04,0x00,0x00,0x00}, // '4'
	{ 0x00,0x7e,0x40,0x40,0x5c,0x62,0x02,0x02,0x42,0x3c,0x00,0x00,0x00}, // '5'
	{ 0x00,0x1c,0x20,0x40,0x40,0x5c,0x62,0x42,0x42,0x3c,0x00,0x00,0x00}, // '6'
	{ 0x00,0x7e,0x02,0x04,0x08,0x08,0x10,0x10,0x20,0x20,0x00,0x00,0x00}, // '7'
	{ 0x00,0x3c,0x42,0x42,0x42,0x3c,0x42,0x42,0x42,0x3c,0x00,0x00,0x00}, // '8'
	{ 0x00,0x3c,0x42,0x42,0x46,0x3a,0x02,0x02,0x04,0x38,0x00,0x00,0x00}, // '9'
	{ 0x00,0x00,0x00,0x10,0x38,0x10,0x00,0x00,0x10,0x38,0x10,0x00,0x00}, // ':'
	{ 0x00,0x00,0x00,0x10,0x38,0x10,0x00,0x00,0x38,0x30,0x40,0x00,0x00}, // ';'
	{ 0x00,0x04,0x08,0x10,0x20,0x40,0x20,0x10,0x08,0x04,0x00,0x00,0x00}, // '<'
	{ 0x00,0x00,0x00,0x00,0x7f,0x00,0x7f,0x00,0x00,0x00,0x00,0x00,0x00}, // '='
	{ 0x00,0x40,0x20,0x10,0x08,0x04,0x08,0x10,0x20,0x40,0x00,0x00,0x00}, // '>'
	{ 0x00,0x3c,0x42,0x42,0x02,0x04,0x08,0x08,0x00,0x08,0x00,0x00,0x00}, // '?'
	{ 0x00,0x3c,0x42,0x42,0x4e,0x52,0x56,0x4a,0x40,0x3c,0x00,0x00,0x00}, // '@'
	{ 0x00,0x18,0x24,0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0x00,0x00,0x00}, // 'A'
	{ 0x00,0x78,0x44,0x42,0x44,0x78,0x44,0x42,0x44,0x78,0x00,0x00,0x00}, // 'B'
	{ 0x00,0x3c,0x42,0x40,0x40,0x40,0x40,0x40,0x42,0x3c,0x00,0x00,0x00}, // 'C'
	{ 0x00,0x78,0x44,0x42,0x42,0x42,0x42,0x42,0x44,0x78,0x00,0x00,0x00}, // 'D'
	{ 0x00,0x7e,0x40,0x40,0x40,0x78,0x40,0x40,0x40,0x7e,0x00,0x00,0x00}, // 'E'
	{ 0x00,0x7e,0x40,0x40,0x40,0x78,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // 'F'
	{ 0x00,0x3c,0x42,0x40,0x40,0x40,0x4e,0x42,0x46,0x3a,0x00,0x00,0x00}, // 'G'
	{ 0x00,0x42,0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0x42,0x00,0x00,0x00}, // 'H'
	{ 0x00,0x7c,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x7c,0x00,0x00,0x00}, // 'I'
	{ 0x00,0x1e,0x04,0x04,0x04,0x04,0x04,0x04,0x44,0x38,0x00,0x00,0x00}, // 'J'
	{ 0x00,0x42,0x44,0x48,0x50,0x60,0x50,0x48,0x44,0x42,0x00,0x00,0x00}, // 'K'
	{ 0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7e,0x00,0x00,0x00}, // 'L'
	{ 0x00,0x82,0x82,0xc6,0xaa,0x92,0x92,0x82,0x82,0x82,0x00,0x00,0x00}, // 'M'
	{ 0x00,0x42,0x42,0x62,0x52,0x4a,0x46,0x42,0x42,0x42,0x00,0x00,0x00}, // 'N'
	{ 0x00,0x3c,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3c,0x00,0x00,0x00}, // 'O'
	{ 0x00,0x7c,0x42,0x42,0x42,0x7c,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // 'P'
	{ 0x00,0x3c,0x42,0x42,0x42,0x42,0x42,0x52,0x4a,0x3c,0x02,0x00,0x00}, // 'Q'
	{ 0x00,0x7c,0x42,0x42,0x42,0x7c,0x50,0x48,0x44,0x42,0x00,0x00,0x00}, // 'R'
	{ 0x00,0x3c,0x42,0x40,0x40,0x3c,0x02,0x02,0x42,0x3c,0x00,0x00,0x00}, // 'S'
	{ 0x00,0xfe,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00}, // 'T'
	{ 0x00,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3c,0x00,0x00,0x00}, // 'U'
	{ 0x00,0x82,0x82,0x44,0x44,0x44,0x28,0x28,0x28,0x10,0x00,0x00,0x00}, // 'V'
	{ 0x00,0x82,0x82,0x82,0x82,0x92,0x92,0x92,0xaa,0x44,0x00,0x00,0x00}, // 'W'
	{ 0x00,0x82,0x82,0x44,0x28,0x10,0x28,0x44,0x82,0x82,0x00,0x00,0x00}, // 'X'
	{ 0x00,0x82,0x82,0x44,0x28,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00}, // 'Y'
	{ 0x00,0x7e,0x02,0x04,0x08,0x10,0x20,0x40,0x40,0x7e,0x00,0x00,0x00}, // 'Z'
	{ 0x00,0x3c,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x3c,0x00,0x00,0x00}, // '['
	{ 0x00,0x80,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x02,0x00,0x00,0x00}, // '\'
	{ 0x00,0x78,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x78,0x00,0x00,0x00}, // ']'
	{ 0x00,0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '^'
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0x00,0x00}, // '_'
	{ 0x10,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '`'
	{ 0x00,0x00,0x00,0x00,0x3c,0x02,0x3e,0x42,0x46,0x3a,0x00,0x00,0x00}, // 'a'
	{ 0x00,0x40,0x40,0x40,0x5c,0x62,0x42,0x42,0x62,0x5c,0x00,0x00,0x00}, // 'b'
	{ 0x00,0x00,0x00,0x00,0x3c,0x42,0x40,0x40,0x42,0x3c,0x00,0x00,0x00}, // 'c'
	{ 0x00,0x02,0x02,0x02,0x3a,0x46,0x42,0x42,0x46,0x3a,0x00,0x00,0x00}, // 'd'
	{ 0x00,0x00,0x00,0x00,0x3c,0x42,0x7e,0x40,0x42,0x3c,0x00,0x00,0x00}, // 'e'
	{ 0x00,0x1c,0x22,0x20,0x20,0x7c,0x20,0x20,0x20,0x20,0x00,0x00,0x00}, // 'f'
	{ 0x00,0x00,0x00,0x00,0x3a,0x44,0x44,0x38,0x40,0x3c,0x42,0x3c,0x00}, // 'g'
	{ 0x00,0x40,0x40,0x40,0x5c,0x62,0x42,0x42,0x42,0x42,0x00,0x00,0x00}, // 'h'
	{ 0x00,0x00,0x10,0x00,0x30,0x10,0x10,0x10,0x10,0x7c,0x00,0x00,0x00}, // 'i'
	{ 0x00,0x00,0x04,0x00,0x0c,0x04,0x04,0x04,0x04,0x44,0x44,0x38,0x00}, // 'j'
	{ 0x00,0x40,0x40,0x40,0x44,0x48,0x70,0x48,0x44,0x42,0x00,0x00,0x00}, // 'k'
	{ 0x00,0x30,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x7c,0x00,0x00,0x00}, // 'l'
	{ 0x00,0x00,0x00,0x00,0xec,0x92,0x92,0x92,0x92,0x82,0x00,0x00,0x00}, // 'm'
	{ 0x00,0x00,0x00,0x00,0x5c,0x62,0x42,0x42,0x42,0x42,0x00,0x00,0x00}, // 'n'
	{ 0x00,0x00,0x00,0x00,0x3c,0x42,0x42,0x42,0x42,0x3c,0x00,0x00,0x00}, // 'o'
	{ 0x00,0x00,0x00,0x00,0x5c,0x62,0x42,0x62,0x5c,0x40,0x40,0x40,0x00}, // 'p'
	{ 0x00,0x00,0x00,0x00,0x3a,0x46,0x42,0x46,0x3a,0x02,0x02,0x02,0x00}, // 'q'
	{ 0x00,0x00,0x00,0x00,0x5c,0x22,0x20,0x20,0x20,0x20,0x00,0x00,0x00}, // 'r'
	{ 0x00,0x00,0x00,0x00,0x3c,0x42,0x30,0x0c,0x42,0x3c,0x00,0x00,0x00}, // 's'
	{ 0x00,0x00,0x20,0x20,0x7c,0x20,0x20,0x20,0x22,0x1c,0x00,0x00,0x00}, // 't'
	{ 0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x44,0x3a,0x00,0x00,0x00}, // 'u'
	{ 0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x28,0x28,0x10,0x00,0x00,0x00}, // 'v'
	{ 0x00,0x00,0x00,0x00,0x82,0x82,0x92,0x92,0xaa,0x44,0x00,0x00,0x00}, // 'w'
	{ 0x00,0x00,0x00,0x00,0x42,0x24,0x18,0x18,0x24,0x42,0x00,0x00,0x00}, // 'x'
	{ 0x00,0x00,0x00,0x00,0x42,0x42,0x42,0x46,0x3a,0x02,0x42,0x3c,0x00}, // 'y'
	{ 0x00,0x00,0x00,0x00,0x7e,0x04,0x08,0x10,0x20,0x7e,0x00,0x00,0x00}, // 'z'
	{ 0x00,0x0e,0x10,0x10,0x08,0x30,0x08,0x10,0x10,0x0e,0x00,0x00,0x00}, // '{'
	{ 0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00}, // '|'
	{ 0x00,0x70,0x08,0x08,0x10,0x0c,0x10,0x08,0x08,0x70,0x00,0x00,0x00}, // '}'
	{ 0x00,0x24,0x54,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '~'
	{ 0x00,0x7F,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x00,0x00}, // DEL
};


// ============================================================================ //
// Beware of the Turing Tar-pit in which everything is possible but nothing of  //
// interest is easy.                                                            //
// ============================================================================ //

