// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime.keyboard

/**
 * Platform-agnostic input state machine (WeChat-style behaviors). Mirrors
 * KeyboardModel.swift (iOS) and KeyboardState.ets (HarmonyOS); keep
 * behaviors aligned.
 */
class ImeSession(
    private val search: (input: String, context: String) -> List<String>,
    private val sink: Sink,
) {
    /** Platform text-editing sink supplied by the keyboard container. */
    interface Sink {
        fun insertText(text: String)
        fun deleteBackward()
        fun sendReturn()
        fun contextBeforeInput(): String
        /** Mirrors the pinyin buffer into the host as composing/marked text. */
        fun setComposingText(text: String)
        /** Localized editor-action label shown on the idle action key. */
        fun editorActionLabel(): String
        fun performEditorAction()
        fun hideKeyboard()
        fun switchToNextKeyboard()
    }

    var page: KeyboardPage = KeyboardPage.LETTERS
        private set
    var buffer: String = ""
        private set
    var candidates: List<String> = emptyList()
        private set
    var shiftOn: Boolean = false
        private set
    var chineseMode: Boolean = true
        private set
    var engineReady: Boolean = true
        private set

    /** Invoked after every mutation so hosts can trigger recomposition. */
    var onChange: (() -> Unit)? = null

    fun setEngineReady(ready: Boolean) {
        engineReady = ready
    }

    /**
     * Fallback context when the platform cannot supply preceding text;
     * the sink-provided context wins when non-empty.
     */
    var committedContext: String = ""
        private set

    private val context: String
        get() {
            val source = sink.contextBeforeInput().ifEmpty { committedContext }
            return source.takeLast(KeyMetrics.CONTEXT_CHARS)
        }

    val isComposing: Boolean get() = buffer.isNotEmpty()

    fun actionLabel(): String = sink.editorActionLabel()

    fun tap(frame: KeyFrame) {
        dispatch(frame)
        onChange?.invoke()
    }

    fun hideKeyboard() {
        sink.hideKeyboard()
        onChange?.invoke()
    }

    fun switchToNextKeyboard() {
        sink.switchToNextKeyboard()
        onChange?.invoke()
    }

    private fun dispatch(frame: KeyFrame) {
        when (frame.kind) {
            KeyKind.CHAR -> when {
                page != KeyboardPage.LETTERS -> commit(frame.label)
                chineseMode -> appendLetter(frame.label)
                else -> {
                    val text = if (shiftOn) frame.label.uppercase()
                    else frame.label.lowercase()
                    shiftOn = false // one-shot shift
                    commit(text)
                }
            }
            KeyKind.COMMA -> commit(if (chineseMode) "，" else ",")
            KeyKind.LANG_TOGGLE -> toggleLanguage()
            KeyKind.ACTION ->
                if (buffer.isNotEmpty()) {
                    commit(buffer) // 确定: commit raw letters
                } else {
                    sink.performEditorAction()
                }
            KeyKind.SHIFT ->
                if (page == KeyboardPage.LETTERS) shiftOn = !shiftOn
            KeyKind.BACKSPACE ->
                if (buffer.isNotEmpty()) {
                    buffer = buffer.dropLast(1)
                    refresh()
                } else {
                    sink.deleteBackward()
                }
            KeyKind.SPACE ->
                if (buffer.isNotEmpty()) {
                    commit(candidates.firstOrNull() ?: buffer)
                } else {
                    commit(" ")
                }
            KeyKind.RETURN ->
                if (buffer.isNotEmpty()) {
                    commit(buffer) // commit raw letters, no newline
                } else {
                    sink.sendReturn()
                }
            KeyKind.TO_SYMBOLS1 -> {
                page = KeyboardPage.SYMBOLS1
                shiftOn = false
            }
            KeyKind.TO_SYMBOLS2 -> page = KeyboardPage.SYMBOLS2
            KeyKind.TO_LETTERS -> page = KeyboardPage.LETTERS
        }
    }

    fun selectCandidate(text: String) {
        commit(text)
        onChange?.invoke()
    }

    private fun toggleLanguage() {
        // Committing the pending buffer first matches WeChat behavior.
        if (buffer.isNotEmpty()) {
            commit(candidates.firstOrNull() ?: buffer)
        }
        chineseMode = !chineseMode
    }

    private fun appendLetter(letter: String) {
        if (buffer.length >= KeyMetrics.MAX_BUFFER_LEN) return
        buffer += if (shiftOn) letter.uppercase() else letter.lowercase()
        if (shiftOn) shiftOn = false // one-shot shift
        refresh()
    }

    private fun commit(text: String) {
        if (text.isEmpty()) return
        sink.setComposingText("")
        sink.insertText(text)
        buffer = ""
        candidates = emptyList()
        committedContext = (committedContext + text).takeLast(KeyMetrics.CONTEXT_CHARS)
    }

    private fun refresh() {
        candidates = if (engineReady && buffer.isNotEmpty()) {
            search(buffer.lowercase(), context)
        } else {
            emptyList()
        }
        sink.setComposingText(buffer)
    }
}
