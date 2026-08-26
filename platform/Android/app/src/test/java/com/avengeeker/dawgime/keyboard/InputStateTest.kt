// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/** Pins the shared three-platform interaction state machine. */
class InputStateTest {
    private class FakeSink : ImeSession.Sink {
        val inserted = StringBuilder()
        var backwardCount = 0
        var returnCount = 0
        var editorActionCount = 0
        var hideCount = 0
        var switchCount = 0
        var composing = ""
        var externalContext = ""

        override fun insertText(text: String) {
            inserted.append(text)
        }

        override fun deleteBackward() {
            backwardCount++
        }

        override fun sendReturn() {
            returnCount++
        }

        override fun contextBeforeInput(): String = externalContext

        override fun setComposingText(text: String) {
            composing = text
        }

        override fun editorActionLabel(): String = "搜索"

        override fun performEditorAction() {
            editorActionCount++
        }

        override fun hideKeyboard() {
            hideCount++
        }

        override fun switchToNextKeyboard() {
            switchCount++
        }
    }

    private fun frame(kind: KeyKind, label: String = "") =
        KeyFrame(kind, label, 0f, 0f, 10f, 10f, kind != KeyKind.CHAR)

    private fun letter(ch: String) = frame(KeyKind.CHAR, ch)

    private fun sessionWith(
        sink: FakeSink,
        results: List<String> = listOf("中国", "中"),
    ): ImeSession = ImeSession(
        search = { _, _ -> results },
        sink = sink,
    )

    @Test
    fun lettersAccumulateInBufferAndTriggerSearch() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(letter("z"))
        session.tap(letter("h"))
        session.tap(letter("o"))
        session.tap(letter("n"))
        session.tap(letter("g"))
        assertEquals("zhong", session.buffer)
        assertEquals(listOf("中国", "中"), session.candidates)
        assertEquals("", sink.inserted.toString())
    }

    @Test
    fun composingTextMirrorsBufferAndClearsOnCommit() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "zhong".forEach { session.tap(letter(it.toString())) }
        assertEquals("zhong", sink.composing)
        session.tap(frame(KeyKind.SPACE))
        assertEquals("", sink.composing)
    }

    @Test
    fun spaceCommitsFirstCandidate() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "zhong".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.SPACE, "空格"))
        assertEquals("中国", sink.inserted.toString())
        assertEquals("", session.buffer)
        assertTrue(session.candidates.isEmpty())
    }

    @Test
    fun tappingCandidateCommitsIt() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "zhong".forEach { session.tap(letter(it.toString())) }
        session.selectCandidate("中")
        assertEquals("中", sink.inserted.toString())
        assertEquals("", session.buffer)
    }

    @Test
    fun spaceWithoutBufferInsertsSpace() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.SPACE, "空格"))
        assertEquals(" ", sink.inserted.toString())
    }

    @Test
    fun backspaceEditsBufferBeforeHost() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(letter("a"))
        session.tap(letter("b"))
        session.tap(frame(KeyKind.BACKSPACE))
        assertEquals("a", session.buffer)
        assertEquals(0, sink.backwardCount)
        session.tap(frame(KeyKind.BACKSPACE))
        session.tap(frame(KeyKind.BACKSPACE))
        assertEquals(1, sink.backwardCount)
    }

    @Test
    fun returnKeyCommitsRawBuffer() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "zhong".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.RETURN))
        assertEquals("zhong", sink.inserted.toString())
        assertEquals(0, sink.returnCount)
    }

    @Test
    fun actionKeyCommitsRawBufferWhenComposing() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "mdj".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.ACTION))
        assertEquals("mdj", sink.inserted.toString())
        assertEquals(0, sink.editorActionCount)
    }

    @Test
    fun actionKeyPerformsEditorActionWhenIdle() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.ACTION))
        assertEquals(1, sink.editorActionCount)
        assertEquals("", sink.inserted.toString())
        assertEquals("搜索", session.actionLabel())
    }

    @Test
    fun commaKeyInsertsModeDependentComma() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.COMMA))
        assertEquals("，", sink.inserted.toString())
        session.tap(frame(KeyKind.LANG_TOGGLE))
        session.tap(frame(KeyKind.COMMA))
        assertEquals("，,", sink.inserted.toString())
    }

    @Test
    fun englishModeLettersInsertDirectly() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.LANG_TOGGLE))
        assertFalse(session.chineseMode)
        session.tap(letter("a"))
        session.tap(letter("b"))
        assertEquals("ab", sink.inserted.toString())
        assertEquals("", session.buffer)
        assertTrue(session.candidates.isEmpty())
    }

    @Test
    fun englishModeShiftUpperCasesNextLetterOnly() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.LANG_TOGGLE))
        session.tap(frame(KeyKind.SHIFT))
        session.tap(letter("a"))
        session.tap(letter("b"))
        assertEquals("Ab", sink.inserted.toString())
    }

    @Test
    fun langToggleCommitsPendingBufferFirst() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "zhong".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.LANG_TOGGLE))
        assertEquals("中国", sink.inserted.toString())
        assertFalse(session.chineseMode)
        assertEquals("", session.buffer)
    }

    @Test
    fun shiftUpperCasesNextLetterOnlyInChineseMode() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.SHIFT, "⇧"))
        assertTrue(session.shiftOn)
        session.tap(letter("a"))
        assertEquals("A", session.buffer)
        assertFalse(session.shiftOn)
    }

    @Test
    fun symbolCharsInsertDirectlyWithoutComposition() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(frame(KeyKind.TO_SYMBOLS1, "123"))
        assertEquals(KeyboardPage.SYMBOLS1, session.page)
        session.tap(frame(KeyKind.CHAR, "?"))
        assertEquals("?", sink.inserted.toString())
        assertEquals("", session.buffer)
    }

    @Test
    fun toolbarButtonsReachSink() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.hideKeyboard()
        session.switchToNextKeyboard()
        assertEquals(1, sink.hideCount)
        assertEquals(1, sink.switchCount)
    }

    @Test
    fun committedTextFeedsContext() {
        val sink = FakeSink()
        var seenContext = ""
        val session = ImeSession(
            search = { _, context ->
                seenContext = context
                listOf("中国")
            },
            sink = sink,
        )
        "zhong".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.SPACE, "空格"))
        assertEquals("中国", sink.inserted.toString())
        session.tap(letter("g"))
        assertEquals("中国", seenContext)
    }

    @Test
    fun externalContextWinsOverCommitted() {
        val sink = FakeSink()
        sink.externalContext = "你好世界欢迎"
        var seenContext = ""
        val session = ImeSession(
            search = { _, context ->
                seenContext = context
                listOf("国")
            },
            sink = sink,
        )
        session.tap(letter("g"))
        assertEquals("你好世界欢迎", seenContext)
    }

    @Test
    fun contextIsClampedToEightChars() {
        val sink = FakeSink()
        sink.externalContext = "一二三四五六七八九十"
        var seenContext = ""
        val session = ImeSession(
            search = { _, context ->
                seenContext = context
                emptyList()
            },
            sink = sink,
        )
        session.tap(letter("g"))
        assertEquals("三四五六七八九十", seenContext)
    }
}
