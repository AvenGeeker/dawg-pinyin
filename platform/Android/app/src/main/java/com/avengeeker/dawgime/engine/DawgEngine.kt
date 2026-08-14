// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.engine

import android.content.Context
import java.io.File

/**
 * Owns one native engine handle. The mmap-backed assets must live on real
 * file paths, so [create] first copies the bundled assets out of the APK.
 */
class DawgEngine private constructor(private val handle: Long) : AutoCloseable {

    val ready: Boolean get() = handle != 0L

    fun search(input: String, context: String): List<String> {
        if (!ready || input.isEmpty()) return emptyList()
        val count = DawgEngineNative.nativeSearch(handle, input, context, null)
        if (count <= 0) return emptyList()
        return buildList(count) {
            for (i in 0 until count) {
                DawgEngineNative.nativeCandidateText(handle, i)?.let { add(it) }
            }
        }
    }

    override fun close() {
        if (handle != 0L) DawgEngineNative.nativeDestroy(handle)
    }

    companion object {
        private const val DAWG_ASSET = "dawg_pinyin.dawg"
        private const val CLM_ASSET = "dawg_pinyin.clm"

        fun create(context: Context): DawgEngine {
            val dawgFile = materialize(context, DAWG_ASSET) ?: return DawgEngine(0L)
            val clmFile = materialize(context, CLM_ASSET)
            val err = IntArray(1)
            val handle = DawgEngineNative.nativeCreate(
                dawgFile.absolutePath,
                clmFile?.absolutePath ?: "",
                maxCandidates = 32,
                beamWidth = 16,
                contextChars = 8,
                errOut = err,
            )
            return DawgEngine(handle)
        }

        private fun materialize(context: Context, name: String): File? {
            return try {
                val dest = File(context.filesDir, name)
                if (!dest.exists()) {
                    context.assets.open(name).use { input ->
                        dest.outputStream().use { output -> input.copyTo(output) }
                    }
                }
                dest
            } catch (_: Exception) {
                null
            }
        }
    }
}
