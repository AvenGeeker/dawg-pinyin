// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime

import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.FrameLayout
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
import androidx.savedstate.SavedStateRegistry
import androidx.savedstate.SavedStateRegistryController
import androidx.savedstate.SavedStateRegistryOwner
import androidx.savedstate.setViewTreeSavedStateRegistryOwner
import com.avengeeker.dawgime.engine.DawgEngine
import com.avengeeker.dawgime.keyboard.ImeSession
import com.avengeeker.dawgime.keyboard.KeyMetrics
import com.avengeeker.dawgime.keyboard.KeyboardScreen
import android.inputmethodservice.InputMethodService

/**
 * Pinyin IME entry point: hosts the shared-state-machine keyboard in a
 * ComposeView and adapts the InputConnection to the ImeSession sink.
 */
class DawgInputMethodService :
    InputMethodService(), LifecycleOwner, SavedStateRegistryOwner
{
    private val lifecycleRegistry = LifecycleRegistry(this)
    private val savedStateRegistryController =
        SavedStateRegistryController.create(this)
    private var engine: DawgEngine? = null
    private lateinit var session: ImeSession

    override val lifecycle: Lifecycle
        get() = lifecycleRegistry

    override val savedStateRegistry: SavedStateRegistry
        get() = savedStateRegistryController.savedStateRegistry

    override fun onCreate() {
        super.onCreate()
        // Services have no saved instance state; restore with null so the
        // registry is attached before Compose asks for it.
        savedStateRegistryController.performRestore(null)
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
        // Compose's WindowRecomposer resolves the LifecycleOwner and
        // SavedStateRegistryOwner by walking up from the window's content root
        // ("ViewTreeLifecycleOwner not found" / "doesn't propagate
        // ViewTreeSavedStateRegistryOwner" crashes), so both owners must be
        // published on the window root before the ComposeView attaches.
        // ComposeView is final and attach listeners fire after
        // onAttachedToWindow(), neither of which helps here; instead host the
        // ComposeView in a FrameLayout whose onAttachedToWindow() publishes the
        // owners first — parents attach before their children, so this always
        // runs ahead of the ComposeView's own attach hook.
        return object : FrameLayout(this) {
            override fun onAttachedToWindow() {
                rootView.setViewTreeLifecycleOwner(this@DawgInputMethodService)
                rootView.setViewTreeSavedStateRegistryOwner(
                    this@DawgInputMethodService
                )
                super.onAttachedToWindow()
            }
        }.apply {
            addView(
                ComposeView(context).apply {
                    setViewTreeLifecycleOwner(this@DawgInputMethodService)
                    setViewTreeSavedStateRegistryOwner(
                        this@DawgInputMethodService
                    )
                    setContent {
                        var version by remember { mutableIntStateOf(0) }
                        session.onChange = { version++ }
                        KeyboardScreen(session, isSystemInDarkTheme(), version)
                    }
                },
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                ),
            )
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

        override fun setComposingText(text: String) {
            currentInputConnection?.setComposingText(text, 1)
        }

        override fun editorActionLabel(): String = when (
            currentInputEditorInfo?.imeOptions
                ?.and(EditorInfo.IME_MASK_ACTION)) {
            EditorInfo.IME_ACTION_SEARCH -> "搜索"
            EditorInfo.IME_ACTION_GO -> "前往"
            EditorInfo.IME_ACTION_SEND -> "发送"
            EditorInfo.IME_ACTION_DONE -> "完成"
            EditorInfo.IME_ACTION_NEXT -> "下一步"
            else -> "换行"
        }

        override fun performEditorAction() {
            val connection = currentInputConnection ?: return
            val action = currentInputEditorInfo?.imeOptions
                ?.and(EditorInfo.IME_MASK_ACTION)
            if (action == null || action == EditorInfo.IME_ACTION_NONE ||
                action == EditorInfo.IME_ACTION_UNSPECIFIED) {
                sendReturn()
            } else {
                connection.performEditorAction(action)
            }
        }

        override fun hideKeyboard() {
            requestHideSelf(0)
        }

        override fun switchToNextKeyboard() {
            switchToNextInputMethod(false)
        }
    }
}
