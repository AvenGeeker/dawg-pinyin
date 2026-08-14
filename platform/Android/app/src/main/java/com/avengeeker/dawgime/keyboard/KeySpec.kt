// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

/**
 * Apple-keyboard-spec metrics and pure layout math. The same numbers and
 * formulas are duplicated verbatim in KeySpec.swift (iOS) and
 * KeyboardPanel.ets (HarmonyOS); keep all three in sync. Units: dp.
 */
enum class KeyboardPage { LETTERS, SYMBOLS1, SYMBOLS2 }

enum class KeyKind {
    CHAR, SHIFT, BACKSPACE, SPACE, RETURN, TO_SYMBOLS1, TO_SYMBOLS2, TO_LETTERS,
}

data class KeyFrame(
    val kind: KeyKind,
    val label: String,
    val x: Float,
    val y: Float,
    val width: Float,
    val height: Float,
    val isSpecial: Boolean,
)

object KeyMetrics {
    const val CANDIDATE_BAR_HEIGHT = 44f
    const val TOP_PADDING = 8f
    const val KEY_HEIGHT = 42f
    const val ROW_GAP = 12f
    const val BOTTOM_PADDING = 6f
    const val H_GAP = 6f
    const val EDGE_INSET = 3f
    const val CORNER_RADIUS = 4f
    const val LETTER_FONT_SIZE = 22f
    const val SPECIAL_FONT_SIZE = 20f
    const val CANDIDATE_FONT_SIZE = 16f
    const val CANDIDATE_PADDING = 14f
    const val SEPARATOR_HEIGHT = 20f
    const val MAX_BUFFER_LEN = 64
    const val CONTEXT_CHARS = 8
}

object KeyLayout {
    /** Letter key width: (W - 2x3 - 9x6) / 10. */
    fun keyWidth(totalWidth: Float): Float =
        (totalWidth - 2 * KeyMetrics.EDGE_INSET - 9 * KeyMetrics.H_GAP) / 10

    /** Shift/backspace/page-switch width: (W - 6 - 7xkeyW - 8x6) / 2. */
    fun specialWidth(totalWidth: Float): Float {
        val keyW = keyWidth(totalWidth)
        return (totalWidth - 2 * KeyMetrics.EDGE_INSET - 7 * keyW -
            8 * KeyMetrics.H_GAP) / 2
    }

    /** Letter-area height below the candidate bar. */
    fun rowsHeight(): Float = KeyMetrics.TOP_PADDING + 4 * KeyMetrics.KEY_HEIGHT +
        3 * KeyMetrics.ROW_GAP + KeyMetrics.BOTTOM_PADDING

    fun totalHeight(totalWidth: Float): Float =
        KeyMetrics.CANDIDATE_BAR_HEIGHT + rowsHeight()

    fun compute(width: Float, page: KeyboardPage): List<KeyFrame> {
        val keyW = keyWidth(width)
        val ws = specialWidth(width)
        val frames = mutableListOf<KeyFrame>()
        val y0 = KeyMetrics.CANDIDATE_BAR_HEIGHT + KeyMetrics.TOP_PADDING
        val dy = KeyMetrics.KEY_HEIGHT + KeyMetrics.ROW_GAP
        val row2X0 = KeyMetrics.EDGE_INSET + (keyW + KeyMetrics.H_GAP) / 2

        fun addRow(entries: List<Triple<KeyKind, String, Float>>, x0: Float,
                   y: Float, special: (KeyKind) -> Boolean) {
            var x = x0
            for ((kind, label, w) in entries) {
                frames += KeyFrame(kind, label, x, y, w, KeyMetrics.KEY_HEIGHT,
                    special(kind))
                x += w + KeyMetrics.H_GAP
            }
        }

        fun charRow(chars: List<String>, x0: Float, y: Float) {
            addRow(chars.map { Triple(KeyKind.CHAR, it, keyW) }, x0, y) { false }
        }

        fun bottomRow(leftKind: KeyKind, leftLabel: String, y: Float) {
            val spaceW = width - 2 * KeyMetrics.EDGE_INSET - 2 * ws -
                2 * KeyMetrics.H_GAP
            addRow(
                listOf(
                    Triple(leftKind, leftLabel, ws),
                    Triple(KeyKind.SPACE, "空格", spaceW),
                    Triple(KeyKind.RETURN, "⏎", ws),
                ),
                KeyMetrics.EDGE_INSET, y,
            ) { it != KeyKind.CHAR }
        }

        when (page) {
            KeyboardPage.LETTERS -> {
                charRow("qwertyuiop".map { it.toString() }, KeyMetrics.EDGE_INSET, y0)
                charRow("asdfghjkl".map { it.toString() }, row2X0, y0 + dy)
                val row3 = mutableListOf(Triple(KeyKind.SHIFT, "⇧", ws))
                row3 += "zxcvbnm".map { Triple(KeyKind.CHAR, it.toString(), keyW) }
                row3 += Triple(KeyKind.BACKSPACE, "⌫", ws)
                addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy) { it != KeyKind.CHAR }
                bottomRow(KeyKind.TO_SYMBOLS1, "123", y0 + 3 * dy)
            }
            KeyboardPage.SYMBOLS1 -> {
                charRow(listOf("1", "2", "3", "4", "5", "6", "7", "8", "9", "0"),
                    KeyMetrics.EDGE_INSET, y0)
                charRow(listOf("-", "/", ":", ";", "(", ")", "¥", "&", "@", "\""),
                    KeyMetrics.EDGE_INSET, y0 + dy)
                addRow(
                    listOf(Triple(KeyKind.TO_SYMBOLS2, "#+=", ws)) +
                        listOf(".", ",", "?", "!", "'").map {
                            Triple(KeyKind.CHAR, it, keyW)
                        } +
                        Triple(KeyKind.BACKSPACE, "⌫", ws),
                    KeyMetrics.EDGE_INSET, y0 + 2 * dy,
                ) { it != KeyKind.CHAR }
                bottomRow(KeyKind.TO_LETTERS, "ABC", y0 + 3 * dy)
            }
            KeyboardPage.SYMBOLS2 -> {
                charRow(listOf("[", "]", "{", "}", "#", "%", "^", "*", "+", "="),
                    KeyMetrics.EDGE_INSET, y0)
                charRow(listOf("_", "\\", "|", "~", "<", ">", "€", "£", "$", "¥"),
                    KeyMetrics.EDGE_INSET, y0 + dy)
                addRow(
                    listOf(Triple(KeyKind.TO_SYMBOLS1, "123", ws)) +
                        listOf(".", ",", "?", "!", "'").map {
                            Triple(KeyKind.CHAR, it, keyW)
                        } +
                        Triple(KeyKind.BACKSPACE, "⌫", ws),
                    KeyMetrics.EDGE_INSET, y0 + 2 * dy,
                ) { it != KeyKind.CHAR }
                bottomRow(KeyKind.TO_LETTERS, "ABC", y0 + 3 * dy)
            }
        }
        return frames
    }
}
