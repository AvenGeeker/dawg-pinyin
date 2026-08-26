// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
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
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.avengeeker.dawgime.R

/**
 * Compose renderer for the WeChat-style keyboard: 44dp top bar (toolbar
 * when idle, candidate bar while composing), uppercase letter caps and the
 * five-key bottom row. [version] is bumped by the host service after every
 * session mutation to trigger recomposition.
 */
@Composable
fun KeyboardScreen(session: ImeSession, dark: Boolean, version: Int) {
    val theme = if (dark) KeyboardTheme.Dark else KeyboardTheme.Light

    BoxWithConstraints(Modifier.background(theme.background)) {
        val density = LocalDensity.current
        val widthDp = with(density) { constraints.maxWidth.toDp() }.value
        // The IME window wrap-contents the input view and key caps are
        // absolutely positioned via offset() (which does not contribute to
        // layout size), so the root must claim the spec height explicitly;
        // otherwise the window collapses to the top bar only.
        val heightDp = KeyLayout.totalHeight(widthDp)
        Box(Modifier.fillMaxWidth().height(heightDp.dp)) {
            val frames = remember(widthDp, session.page, version) {
                KeyLayout.compute(widthDp, session.page)
            }
            TopBar(session, theme, version)
            Box(Modifier.fillMaxSize()) {
                for (frame in frames) {
                    KeyButton(frame, session, theme, version)
                }
            }
        }
    }
}

@Composable
private fun TopBar(session: ImeSession, theme: KeyboardTheme, version: Int) {
    val composing = remember(version) { session.isComposing }
    if (composing) {
        CandidateBar(session, theme, version)
    } else {
        Toolbar(session, theme)
    }
}

/** Idle toolbar: brand mark left, IME switch + hide round buttons right. */
@Composable
private fun Toolbar(session: ImeSession, theme: KeyboardTheme) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(KeyMetrics.TOP_BAR_HEIGHT.dp)
            .padding(horizontal = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            modifier = Modifier
                .size(KeyMetrics.TOOLBAR_BUTTON_SIZE.dp)
                .clip(CircleShape)
                .background(theme.key),
            contentAlignment = Alignment.Center,
        ) {
            Text("D", color = theme.accent, fontWeight = FontWeight.Black,
                fontSize = 18.sp)
        }
        Spacer(Modifier.weight(1f))
        ToolbarButton(theme, R.drawable.ic_keyboard) {
            session.switchToNextKeyboard()
        }
        Spacer(Modifier.width(10.dp))
        ToolbarButton(theme, R.drawable.ic_chevron_down) { session.hideKeyboard() }
    }
}

@Composable
private fun ToolbarButton(
    theme: KeyboardTheme,
    icon: Int,
    onTap: () -> Unit,
) {
    KeyPressSurface(
        theme.key, theme.pressedKey,
        modifier = Modifier.size(KeyMetrics.TOOLBAR_BUTTON_SIZE.dp),
        cornerRadius = KeyMetrics.TOOLBAR_BUTTON_SIZE / 2,
        onTap = onTap,
    ) {
        Icon(
            painterResource(icon), contentDescription = null,
            tint = theme.text, modifier = Modifier.size(20.dp),
        )
    }
}

/** Composing candidate bar: first candidate highlighted WeChat-style. */
@Composable
private fun CandidateBar(session: ImeSession, theme: KeyboardTheme, version: Int) {
    val candidates = remember(version) { session.candidates }
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(KeyMetrics.TOP_BAR_HEIGHT.dp)
            .horizontalScroll(rememberScrollState()),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (candidates.isEmpty() && !session.engineReady) {
            Text(
                "词库加载失败，仅支持字母直输",
                fontSize = 13.sp,
                color = theme.text,
                modifier = Modifier.padding(horizontal = KeyMetrics.CANDIDATE_PADDING.dp),
            )
        }
        candidates.forEachIndexed { index, candidate ->
            if (index == 0) {
                KeyPressSurface(
                    theme.key, theme.pressedKey,
                    modifier = Modifier
                        .padding(horizontal = 6.dp)
                        .height(36.dp),
                    cornerRadius = 6f,
                    onTap = { session.selectCandidate(candidate) },
                ) {
                    Text(
                        candidate,
                        fontSize = KeyMetrics.CANDIDATE_FONT_SIZE.sp,
                        color = theme.accent,
                        modifier = Modifier.padding(horizontal = 12.dp),
                    )
                }
            } else {
                KeyPressSurface(
                    Color.Transparent, theme.pressedSpecialKey,
                    modifier = Modifier.height(KeyMetrics.TOP_BAR_HEIGHT.dp),
                    cornerRadius = 6f,
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
    val composing = remember(version) { session.isComposing }
    val chinese = remember(version) { session.chineseMode }
    val (base, pressed) = when {
        frame.kind == KeyKind.ACTION && !composing ->
            theme.accent to theme.pressedAccent
        frame.kind == KeyKind.SHIFT && shiftActive -> theme.key to theme.pressedKey
        frame.isSpecial -> theme.specialKey to theme.pressedSpecialKey
        else -> theme.key to theme.pressedKey
    }
    KeyPressSurface(
        base, pressed,
        modifier = Modifier
            .offset(x = frame.x.dp, y = frame.y.dp)
            .width(frame.width.dp)
            .height(frame.height.dp),
        onTap = { session.tap(frame) },
    ) {
        KeyLabel(frame, session, theme, composing, chinese)
    }
}

@Composable
private fun KeyLabel(
    frame: KeyFrame,
    session: ImeSession,
    theme: KeyboardTheme,
    composing: Boolean,
    chinese: Boolean,
) {
    when (frame.kind) {
        KeyKind.COMMA -> Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(if (chinese) "。" else ".", fontSize = 9.sp, color = theme.text)
            Text(if (chinese) "，" else ",", fontSize = 18.sp, color = theme.text)
        }
        KeyKind.LANG_TOGGLE -> Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy((-4).dp),
        ) {
            Text("中", fontSize = 13.sp,
                color = theme.text.copy(alpha = if (chinese) 1f else 0.4f))
            Text("英", fontSize = 13.sp,
                color = theme.text.copy(alpha = if (chinese) 0.4f else 1f))
        }
        KeyKind.ACTION -> Text(
            if (composing) "确定" else session.actionLabel(),
            fontSize = KeyMetrics.CANDIDATE_FONT_SIZE.sp,
            color = if (composing) theme.text else theme.textOnAccent,
        )
        KeyKind.SPACE -> Text("")
        KeyKind.TO_SYMBOLS1, KeyKind.TO_SYMBOLS2, KeyKind.TO_LETTERS -> Text(
            frame.label, fontSize = KeyMetrics.CANDIDATE_FONT_SIZE.sp,
            color = theme.text,
        )
        else -> Text(
            frame.label,
            fontSize = if (frame.isSpecial) KeyMetrics.SPECIAL_FONT_SIZE.sp
            else KeyMetrics.LETTER_FONT_SIZE.sp,
            color = theme.text,
        )
    }
}

@Composable
private fun KeyPressSurface(
    background: Color,
    pressedBackground: Color,
    modifier: Modifier = Modifier,
    cornerRadius: Float = KeyMetrics.CORNER_RADIUS,
    onTap: () -> Unit,
    content: @Composable () -> Unit,
) {
    var pressed by remember { mutableStateOf(false) }
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(cornerRadius.dp))
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
