// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime

import android.view.KeyEvent
import android.view.View
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.ComposeView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.LifecycleRegistry
import androidx.lifecycle.setViewTreeLifecycleOwner
import com.avengeeker.dawgime.engine.DawgEngine
import com.avengeeker.dawgime.keyboard.ImeSession
import com.avengeeker.dawgime.keyboard.KeyMetrics
import com.avengeeker.dawgime.keyboard.KeyboardScreen
import android.inputmethodservice.InputMethodService

/**
 * Pinyin IME entry point: hosts the shared-state-machine keyboard in a
 * ComposeView and adapts the InputConnection to the ImeSession sink.
 */
class DawgInputMethodService : InputMethodService(), LifecycleOwner {
    private val lifecycleRegistry = LifecycleRegistry(this)
    private var engine: DawgEngine? = null
    private lateinit var session: ImeSession

    override val lifecycle: Lifecycle
        get() = lifecycleRegistry

    override fun onCreate() {
        super.onCreate()
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_CREATE)
        engine = DawgEngine.create(applicationContext)
        session = ImeSession(
            search = { input, context ->
                engine?.search(input, context) ?: emptyList()
            },
            sink = ConnectionSink(),
        )
        session.setEngineReady(engine?.ready == true)
    }

    override fun onCreateInputView(): View {
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START)
        return ComposeView(this).apply {
            setViewTreeLifecycleOwner(this@DawgInputMethodService)
            setContent {
                var version by remember { mutableIntStateOf(0) }
                session.onChange = { version++ }
                KeyboardScreen(session, isSystemInDarkTheme(), version)
            }
        }
    }

    override fun onDestroy() {
        engine?.close()
        engine = null
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY)
        super.onDestroy()
    }

    private inner class ConnectionSink : ImeSession.Sink {
        override fun insertText(text: String) {
            currentInputConnection?.commitText(text, 1)
        }

        override fun deleteBackward() {
            currentInputConnection?.deleteSurroundingText(1, 0)
        }

        override fun sendReturn() {
            val connection = currentInputConnection ?: return
            connection.sendKeyEvent(
                KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER))
            connection.sendKeyEvent(
                KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER))
        }

        override fun contextBeforeInput(): String =
            currentInputConnection
                ?.getTextBeforeCursor(KeyMetrics.CONTEXT_CHARS, 0)
                ?.toString()
                ?: ""
    }
}
