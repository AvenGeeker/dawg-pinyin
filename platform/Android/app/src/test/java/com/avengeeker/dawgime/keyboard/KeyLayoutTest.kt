// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** Pins the shared three-platform layout formulas at 390dp (iPhone-class). */
class KeyLayoutTest {
    private val width = 390f
    private val eps = 0.01f

    @Test
    fun letterKeyWidthMatchesSpec() {
        // (W − 2×3 − 9×6) / 10
        assertEquals(33.0f, KeyLayout.keyWidth(width), eps)
    }

    @Test
    fun specialKeyWidthMatchesSpec() {
        // (W − 6 − 7×keyW − 8×6) / 2 with keyW = 33
        assertEquals(52.5f, KeyLayout.specialWidth(width), eps)
    }

    @Test
    fun rowTwoIsIndentedHalfKeyPlusHalfGap() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val row2 = frames.filter { it.label in listOf("A", "S", "D") }
        val expectedX0 = 3f + (33f + 6f) / 2
        assertEquals(expectedX0, row2.first().x, eps)
    }

    @Test
    fun rowsHaveSpecVerticalPositions() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val y0 = KeyMetrics.TOP_BAR_HEIGHT + KeyMetrics.TOP_PADDING
        val q = frames.first { it.label == "Q" }
        val a = frames.first { it.label == "A" }
        val z = frames.first { it.label == "Z" }
        val oneTwoThree = frames.first { it.label == "123" }
        assertEquals(y0, q.y, eps)
        assertEquals(y0 + 54f, a.y, eps)
        assertEquals(y0 + 108f, z.y, eps)
        assertEquals(y0 + 162f, oneTwoThree.y, eps)
    }

    @Test
    fun letterCapsAreUppercase() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val labels = frames.map { it.label }
        assertTrue(labels.containsAll("QWERTYUIOPASDFGHJKLZXCVBNM".map { it.toString() }))
    }

    @Test
    fun letterPageHas33Keys() {
        // 10 + 9 + (shift + 7 + backspace) + (123 + comma + space + lang + action)
        assertEquals(33, KeyLayout.compute(width, KeyboardPage.LETTERS).size)
    }

    @Test
    fun symbolPagesHave32Keys() {
        // 10 + 10 + (page-switch + 5 chars + backspace) +
        // (ABC + comma + space + lang + action)
        assertEquals(32, KeyLayout.compute(width, KeyboardPage.SYMBOLS1).size)
        assertEquals(32, KeyLayout.compute(width, KeyboardPage.SYMBOLS2).size)
    }

    @Test
    fun totalHeightMatchesSpec() {
        // 44 + 8 + 4×42 + 3×12 + 6 = 262
        assertEquals(262f, KeyLayout.totalHeight(width), eps)
    }

    @Test
    fun bottomRowHasFiveKeysAndFillsWidthExactly() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val bottom = frames.filter { it.y > 200f }
        assertEquals(5, bottom.size)
        val total = bottom.sumOf { it.width.toDouble() } + 4 * KeyMetrics.H_GAP
        // Row spans edge-inset to edge-inset (3dp each side).
        assertEquals(width - 2 * KeyMetrics.EDGE_INSET.toDouble(), total, 0.05)
    }

    @Test
    fun bottomRowWidthsFollowWechatWeights() {
        // unit = (W − 2×3 − 4×6) / 10.7; widths = unit × [2, 1, 4.2, 1, 2.5]
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val bottom = frames.filter { it.y > 200f }
        val unit = (width - 2 * KeyMetrics.EDGE_INSET - 4 * KeyMetrics.H_GAP) / 10.7f
        assertEquals(2.0f * unit, bottom[0].width, eps)
        assertEquals(unit, bottom[1].width, eps)
        assertEquals(4.2f * unit, bottom[2].width, eps)
        assertEquals(unit, bottom[3].width, eps)
        assertEquals(2.5f * unit, bottom[4].width, eps)
    }

    @Test
    fun bottomRowKindsMatchWechatLayout() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val bottom = frames.filter { it.y > 200f }
        assertEquals(
            listOf(KeyKind.TO_SYMBOLS1, KeyKind.COMMA, KeyKind.SPACE,
                   KeyKind.LANG_TOGGLE, KeyKind.ACTION),
            bottom.map { it.kind },
        )
        // Comma and lang-toggle are white keys; 123 and action are special.
        assertTrue(bottom[0].isSpecial)
        assertTrue(!bottom[1].isSpecial)
        assertTrue(!bottom[2].isSpecial)
        assertTrue(!bottom[3].isSpecial)
        assertTrue(bottom[4].isSpecial)
    }

    @Test
    fun firstRowStartsAtEdgeInset() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val q = frames.first { it.label == "Q" }
        assertEquals(KeyMetrics.EDGE_INSET, q.x, eps)
        assertTrue(KeyLayout.keyWidth(width) > 0)
    }
}
