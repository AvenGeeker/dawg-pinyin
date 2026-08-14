// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.engine

/** JNI bindings for the dawg C API (see app/src/main/cpp/dawg_jni.cc). */
object DawgEngineNative {
    init {
        System.loadLibrary("dawg_jni")
    }

    external fun nativeCreate(
        dawgPath: String,
        lmPath: String,
        maxCandidates: Int,
        beamWidth: Int,
        contextChars: Int,
        errOut: IntArray?,
    ): Long

    external fun nativeSearch(
        handle: Long,
        input: String,
        context: String,
        errOut: IntArray?,
    ): Int

    external fun nativeCandidateText(handle: Long, index: Int): String?

    external fun nativeDestroy(handle: Long)
}
