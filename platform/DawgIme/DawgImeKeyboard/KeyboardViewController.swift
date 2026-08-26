// SPDX-License-Identifier: Apache-2.0
// Keyboard extension entry point: hosts the SwiftUI keyboard and adapts
// the text document proxy to the shared KeyboardModel state machine.

import SwiftUI
import UIKit

final class KeyboardViewController: UIInputViewController, KeyboardHost {
  private let model = KeyboardModel(engine: DawgEngine())
  private var hostingController: UIHostingController<KeyboardView>?
  private var heightConstraint: NSLayoutConstraint?

  override func viewDidLoad() {
    super.viewDidLoad()
    model.attach(host: self)
    let host = UIHostingController(rootView: KeyboardView(model: model))
    addChild(host)
    host.view.translatesAutoresizingMaskIntoConstraints = false
    host.view.backgroundColor = .clear
    view.addSubview(host.view)
    NSLayoutConstraint.activate([
      host.view.topAnchor.constraint(equalTo: view.topAnchor),
      host.view.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      host.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      host.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
    ])
    host.didMove(toParent: self)
    hostingController = host
    updateHeight()
  }

  override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()
    updateHeight()
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    updateHeight()
  }

  /// Portrait-only: pin the extension view to top bar + 4 key rows
  /// + bottom padding + home-indicator safe area.
  private func updateHeight() {
    let width = view.bounds.width > 0
      ? view.bounds.width
      : (view.window?.windowScene?.screen.bounds.width ?? 393)
    model.safeBottomInset = view.safeAreaInsets.bottom
    heightConstraint?.isActive = false
    let constraint = view.heightAnchor.constraint(
      equalToConstant: KeyLayout.totalHeight(totalWidth: width)
        + view.safeAreaInsets.bottom)
    constraint.isActive = true
    heightConstraint = constraint
  }

  // MARK: - KeyboardHost

  func insertText(_ text: String) {
    textDocumentProxy.insertText(text)
  }

  func deleteBackward() {
    textDocumentProxy.deleteBackward()
  }

  func sendReturn() {
    textDocumentProxy.insertText("\n")
  }

  func contextPrecedingInput() -> String {
    textDocumentProxy.documentContextBeforeInput ?? ""
  }

  func setComposingText(_ text: String) {
    if text.isEmpty {
      textDocumentProxy.setMarkedText("", selectedRange: NSMakeRange(0, 0))
    } else {
      let len = (text as NSString).length
      textDocumentProxy.setMarkedText(text,
                                      selectedRange: NSMakeRange(len, 0))
    }
  }

  func editorActionLabel() -> String {
    switch textDocumentProxy.returnKeyType {
    case .search: return "搜索"
    case .go: return "前往"
    case .send: return "发送"
    case .done: return "完成"
    case .next: return "下一步"
    case .join: return "加入"
    case .continue: return "继续"
    default: return "换行"
    }
  }

  func performEditorAction() {
    textDocumentProxy.insertText("\n")
  }

  func hideKeyboard() {
    dismissKeyboard()
  }

  func switchToNextKeyboard() {
    advanceToNextInputMode()
  }
}
