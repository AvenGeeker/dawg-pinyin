// SPDX-License-Identifier: Apache-2.0
// Platform-agnostic input state machine. Mirrors ImeSession.kt (Android)
// and the ArkTS session in DawgIME.ets (HarmonyOS); keep behaviors aligned.

import Foundation
import Observation

/// Platform text-editing sink supplied by the keyboard container.
protocol KeyboardHost: AnyObject {
  func insertText(_ text: String)
  func deleteBackward()
  func sendReturn()
  func contextBeforeInput() -> String
}

@Observable
final class KeyboardModel {
  private(set) var page: KeyboardPage = .letters
  private(set) var buffer = ""
  private(set) var candidates: [String] = []
  private(set) var shiftOn = false
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

  private var context: String {
    let source = host?.contextBeforeInput() ?? committedContext
    return String(source.suffix(KeyMetrics.contextChars))
  }

  // MARK: - Actions

  func tap(_ cap: KeyCap) {
    switch cap.kind {
    case .char(let text):
      if page == .letters {
        appendLetter(text)
      } else {
        commit(text)  // symbols insert directly, no composition
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

  // MARK: - Internals

  private func appendLetter(_ letter: String) {
    guard buffer.count < KeyMetrics.maxBufferLen else { return }
    buffer += shiftOn ? letter.uppercased() : letter.lowercased()
    if shiftOn { shiftOn = false }  // one-shot shift
    search()
  }

  private func commit(_ text: String) {
    guard !text.isEmpty else { return }
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
  }
}
