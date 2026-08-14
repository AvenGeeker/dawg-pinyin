// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

import androidx.compose.runtime.Immutable
import androidx.compose.ui.graphics.Color

/** Exact palette from the shared three-platform keyboard spec. */
@Immutable
data class KeyboardTheme(
    val background: Color,
    val key: Color,
    val specialKey: Color,
    val pressedKey: Color,
    val pressedSpecialKey: Color,
    val text: Color,
    val separator: Color,
) {
    companion object {
        val Light = KeyboardTheme(
            background = Color(0xFFD1D5DB),
            key = Color(0xFFFFFFFF),
            specialKey = Color(0xFFABB2BC),
            pressedKey = Color(0xFFE6E6E6), // white darkened 10%
            pressedSpecialKey = Color(0xFF9AA0A9),
            text = Color(0xFF000000),
            separator = Color(0x668A8F98), // 40% alpha
        )
        val Dark = KeyboardTheme(
            background = Color(0xFF1C1C1E),
            key = Color(0xFF494B53),
            specialKey = Color(0xFF2F3238),
            pressedKey = Color(0xFF42444B),
            pressedSpecialKey = Color(0xFF2A2D32),
            text = Color(0xFFFFFFFF),
            separator = Color(0x4D8A8F98), // 30% alpha
        )
    }
}
