// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * Compose renderer for the shared keyboard layout. Keys are absolutely
 * positioned from KeyLayout.compute(); press feedback is a 10% darken
 * overlay (no popups, per spec). [version] is bumped by the host service
 * after every session mutation to trigger recomposition.
 */
@Composable
fun KeyboardScreen(session: ImeSession, dark: Boolean, version: Int) {
    val theme = if (dark) KeyboardTheme.Dark else KeyboardTheme.Light

    BoxWithConstraints(Modifier.background(theme.background)) {
        val density = LocalDensity.current
        val widthDp = with(density) { constraints.maxWidth.toDp() }.value
        val frames = remember(widthDp, session.page, version) {
            KeyLayout.compute(widthDp, session.page)
        }
        CandidateBar(session, theme, version)
        Box(Modifier.fillMaxWidth()) {
            for (frame in frames) {
                KeyButton(frame, session, theme, version)
            }
        }
    }
}

@Composable
private fun CandidateBar(session: ImeSession, theme: KeyboardTheme, version: Int) {
    val candidates = remember(version) { session.candidates }
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(KeyMetrics.CANDIDATE_BAR_HEIGHT.dp)
            .horizontalScroll(rememberScrollState()),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        candidates.forEachIndexed { index, candidate ->
            KeyPressSurface(
                theme.background, theme.pressedSpecialKey,
                modifier = Modifier.height(KeyMetrics.CANDIDATE_BAR_HEIGHT.dp),
                onTap = { session.selectCandidate(candidate) },
            ) {
                Text(
                    candidate,
                    fontSize = KeyMetrics.CANDIDATE_FONT_SIZE.sp,
                    color = theme.text,
                    modifier = Modifier.padding(
                        horizontal = KeyMetrics.CANDIDATE_PADDING.dp),
                )
            }
            if (index < candidates.size - 1) {
                Box(
                    Modifier
                        .width(1.dp)
                        .height(KeyMetrics.SEPARATOR_HEIGHT.dp)
                        .background(theme.separator),
                )
            }
        }
    }
}

@Composable
private fun KeyButton(
    frame: KeyFrame,
    session: ImeSession,
    theme: KeyboardTheme,
    version: Int,
) {
    val shiftActive = remember(version) { session.shiftOn }
    val base = when {
        frame.kind == KeyKind.SHIFT && shiftActive -> theme.key
        frame.isSpecial -> theme.specialKey
        else -> theme.key
    }
    val pressed = when {
        frame.kind == KeyKind.SHIFT && shiftActive -> theme.pressedKey
        frame.isSpecial -> theme.pressedSpecialKey
        else -> theme.pressedKey
    }
    val fontSize = when (frame.kind) {
        KeyKind.SPACE, KeyKind.TO_SYMBOLS1, KeyKind.TO_SYMBOLS2, KeyKind.TO_LETTERS ->
            KeyMetrics.CANDIDATE_FONT_SIZE
        else -> if (frame.isSpecial) {
            KeyMetrics.SPECIAL_FONT_SIZE
        } else {
            KeyMetrics.LETTER_FONT_SIZE
        }
    }
    KeyPressSurface(
        base, pressed,
        modifier = Modifier
            .offset(x = frame.x.dp, y = frame.y.dp)
            .width(frame.width.dp)
            .height(frame.height.dp),
        onTap = { session.tap(frame) },
    ) {
        Text(frame.label, fontSize = fontSize.sp, color = theme.text)
    }
}

@Composable
private fun KeyPressSurface(
    background: Color,
    pressedBackground: Color,
    modifier: Modifier = Modifier,
    onTap: () -> Unit,
    content: @Composable () -> Unit,
) {
    var pressed by remember { mutableStateOf(false) }
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(KeyMetrics.CORNER_RADIUS.dp))
            .background(if (pressed) pressedBackground else background)
            .pointerInput(Unit) {
                detectTapGestures(
                    onPress = {
                        pressed = true
                        try {
                            awaitRelease()
                            onTap()
                        } finally {
                            pressed = false
                        }
                    },
                )
            },
        contentAlignment = Alignment.Center,
    ) {
        content()
    }
}
