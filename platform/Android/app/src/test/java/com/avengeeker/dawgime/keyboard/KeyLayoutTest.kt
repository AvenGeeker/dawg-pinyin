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
        val row2 = frames.filter { it.label in listOf("a", "s", "d") }
        val expectedX0 = 3f + (33f + 6f) / 2
        assertEquals(expectedX0, row2.first().x, eps)
    }

    @Test
    fun rowsHaveSpecVerticalPositions() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val y0 = KeyMetrics.CANDIDATE_BAR_HEIGHT + KeyMetrics.TOP_PADDING
        val q = frames.first { it.label == "q" }
        val a = frames.first { it.label == "a" }
        val z = frames.first { it.label == "z" }
        val oneTwoThree = frames.first { it.label == "123" }
        assertEquals(y0, q.y, eps)
        assertEquals(y0 + 54f, a.y, eps)
        assertEquals(y0 + 108f, z.y, eps)
        assertEquals(y0 + 162f, oneTwoThree.y, eps)
    }

    @Test
    fun letterPageHas31Keys() {
        // 10 + 9 + (shift + 7 + backspace) + (123 + space + return)
        assertEquals(31, KeyLayout.compute(width, KeyboardPage.LETTERS).size)
    }

    @Test
    fun symbolPagesHave30Keys() {
        // 10 + 10 + (page-switch + 5 chars + backspace) + (ABC + space + return)
        assertEquals(30, KeyLayout.compute(width, KeyboardPage.SYMBOLS1).size)
        assertEquals(30, KeyLayout.compute(width, KeyboardPage.SYMBOLS2).size)
    }

    @Test
    fun totalHeightMatchesSpec() {
        // 44 + 8 + 4×42 + 3×12 + 6 = 262
        assertEquals(262f, KeyLayout.totalHeight(width), eps)
    }

    @Test
    fun bottomRowFillsWidthExactly() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val bottom = frames.filter { it.y > 200f }
        assertEquals(3, bottom.size)
        val total = bottom.sumOf { it.width.toDouble() } + 2 * KeyMetrics.H_GAP
        // Row spans edge-inset to edge-inset (3dp each side).
        assertEquals(width - 2 * KeyMetrics.EDGE_INSET.toDouble(), total, 0.05)
    }

    @Test
    fun firstRowStartsAtEdgeInset() {
        val frames = KeyLayout.compute(width, KeyboardPage.LETTERS)
        val q = frames.first { it.label == "q" }
        assertEquals(KeyMetrics.EDGE_INSET, q.x, eps)
        assertTrue(KeyLayout.keyWidth(width) > 0)
    }
}
