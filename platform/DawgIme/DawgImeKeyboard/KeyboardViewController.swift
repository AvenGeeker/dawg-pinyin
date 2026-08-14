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

  /// Portrait-only: pin the extension view to candidate bar + 4 key rows
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

  func contextBeforeInput() -> String {
    textDocumentProxy.documentContextBeforeInput ?? ""
  }
}
