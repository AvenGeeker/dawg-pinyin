// SPDX-License-Identifier: Apache-2.0
// Platform-agnostic input state machine (WeChat-style behaviors). Mirrors
// ImeSession.kt (Android) and KeyboardState.ets (HarmonyOS); keep behaviors
// aligned.

import Foundation
import Observation

/// Platform text-editing sink supplied by the keyboard container.
protocol KeyboardHost: AnyObject {
  func insertText(_ text: String)
  func deleteBackward()
  func sendReturn()
  func contextPrecedingInput() -> String
  /// Mirrors the pinyin buffer into the host as marked text.
  func setComposingText(_ text: String)
  /// Localized editor-action label shown on the idle action key.
  func editorActionLabel() -> String
  func performEditorAction()
  func hideKeyboard()
  func switchToNextKeyboard()
}

@Observable
final class KeyboardModel {
  private(set) var page: KeyboardPage = .letters
  private(set) var buffer = ""
  private(set) var candidates: [String] = []
  private(set) var shiftOn = false
  private(set) var chineseMode = true
  private(set) var engineReady = false
  /// Fallback context when the platform cannot supply preceding text
  /// (HarmonyOS API 9); on iOS the host context wins.
  private(set) var committedContext = ""
  var safeBottomInset: CGFloat = 0

  private let engine: DawgEngine?
  private weak var host: KeyboardHost?

  init(engine: DawgEngine?) {
    self.engine = engine
    self.engineReady = engine != nil
  }

  func attach(host: KeyboardHost) {
    self.host = host
  }

  var isComposing: Bool { !buffer.isEmpty }

  func actionLabel() -> String {
    host?.editorActionLabel() ?? "换行"
  }

  private var context: String {
    let source = host?.contextPrecedingInput() ?? committedContext
    return String(source.suffix(KeyMetrics.contextChars))
  }

  // MARK: - Actions

  func tap(_ cap: KeyCap) {
    switch cap.kind {
    case .char(let text):
      if page != .letters {
        commit(text)  // symbols insert directly, no composition
      } else if chineseMode {
        appendLetter(text)
      } else {
        // English mode: direct insert, one-shot shift.
        commit(shiftOn ? text.uppercased() : text.lowercased())
        shiftOn = false
      }
    case .comma:
      commit(chineseMode ? "，" : ",")
    case .langToggle:
      toggleLanguage()
    case .action:
      if !buffer.isEmpty {
        commit(buffer)  // 确定: commit raw letters
      } else {
        host?.performEditorAction()
      }
    case .shift:
      guard page == .letters else { return }
      shiftOn.toggle()
    case .backspace:
      if !buffer.isEmpty {
        buffer.removeLast()
        search()
      } else {
        host?.deleteBackward()
      }
    case .space:
      if !buffer.isEmpty {
        commit(candidates.first ?? buffer)
      } else {
        commit(" ")
      }
    case .returnKey:
      if !buffer.isEmpty {
        commit(buffer)  // commit raw letters, no newline
      } else {
        host?.sendReturn()
      }
    case .toSymbols1:
      page = .symbols1
      shiftOn = false
    case .toSymbols2:
      page = .symbols2
    case .toLetters:
      page = .letters
    }
  }

  func selectCandidate(_ text: String) {
    commit(text)
  }

  func hideKeyboard() {
    host?.hideKeyboard()
  }

  func switchToNextKeyboard() {
    host?.switchToNextKeyboard()
  }

  // MARK: - Internals

  private func toggleLanguage() {
    // Committing the pending buffer first matches WeChat behavior.
    if !buffer.isEmpty {
      commit(candidates.first ?? buffer)
    }
    chineseMode.toggle()
  }

  private func appendLetter(_ letter: String) {
    guard buffer.count < KeyMetrics.maxBufferLen else { return }
    buffer += shiftOn ? letter.uppercased() : letter.lowercased()
    if shiftOn { shiftOn = false }  // one-shot shift
    search()
  }

  private func commit(_ text: String) {
    guard !text.isEmpty else { return }
    host?.setComposingText("")
    host?.insertText(text)
    buffer = ""
    candidates = []
    committedContext = String((committedContext + text)
      .suffix(KeyMetrics.contextChars))
  }

  private func search() {
    guard engineReady, let engine, !buffer.isEmpty else {
      candidates = []
      return
    }
    candidates = engine.search(buffer.lowercased(), context: context)
    host?.setComposingText(buffer)
  }
}
