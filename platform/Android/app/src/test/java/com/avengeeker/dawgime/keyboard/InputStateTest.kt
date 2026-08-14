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
    fun backspacePopsBufferFirstThenDelegates() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        session.tap(letter("a"))
        session.tap(letter("b"))
        session.tap(frame(KeyKind.BACKSPACE, "⌫"))
        assertEquals("a", session.buffer)
        assertEquals(0, sink.backwardCount)
        session.tap(frame(KeyKind.BACKSPACE, "⌫"))
        assertEquals("", session.buffer)
        session.tap(frame(KeyKind.BACKSPACE, "⌫"))
        assertEquals(1, sink.backwardCount)
    }

    @Test
    fun returnCommitsRawBufferWithoutNewline() {
        val sink = FakeSink()
        val session = sessionWith(sink)
        "abc".forEach { session.tap(letter(it.toString())) }
        session.tap(frame(KeyKind.RETURN, "⏎"))
        assertEquals("abc", sink.inserted.toString())
        assertEquals(0, sink.returnCount)
        session.tap(frame(KeyKind.RETURN, "⏎"))
        assertEquals(1, sink.returnCount)
    }

    @Test
    fun shiftIsOneShot() {
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
