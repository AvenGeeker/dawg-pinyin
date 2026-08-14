// SPDX-License-Identifier: Apache-2.0
// Apple-keyboard-spec metrics and pure layout math. The same numbers and
// formulas are duplicated verbatim on Android (KeyLayout.kt) and HarmonyOS
// (KeyboardPanel.ets); keep all three in sync.

import CoreGraphics

enum KeyboardPage {
  case letters
  case symbols1
  case symbols2
}

enum KeyKind: Equatable {
  case char(String)
  case shift
  case backspace
  case space
  case returnKey
  case toSymbols1
  case toSymbols2
  case toLetters
}

struct KeyCap: Identifiable {
  let id: Int
  let kind: KeyKind
  let label: String
  let frame: CGRect
  let isSpecial: Bool
}

enum KeyMetrics {
  static let candidateBarHeight: CGFloat = 44
  static let topPadding: CGFloat = 8
  static let keyHeight: CGFloat = 42
  static let rowGap: CGFloat = 12
  static let bottomPadding: CGFloat = 6
  static let hGap: CGFloat = 6
  static let edgeInset: CGFloat = 3
  static let cornerRadius: CGFloat = 4
  static let letterFontSize: CGFloat = 22
  static let specialFontSize: CGFloat = 20
  static let candidateFontSize: CGFloat = 16
  static let candidatePadding: CGFloat = 14
  static let separatorHeight: CGFloat = 20
  static let maxBufferLen = 64
  static let contextChars = 8
}

enum KeyLayout {
  /// Letter key width: (W − 2×3 − 9×6) / 10.
  static func keyWidth(totalWidth: CGFloat) -> CGFloat {
    (totalWidth - 2 * KeyMetrics.edgeInset - 9 * KeyMetrics.hGap) / 10
  }

  /// Shift/backspace/page-switch width: (W − 6 − 7×keyW − 8×6) / 2.
  static func specialWidth(totalWidth: CGFloat) -> CGFloat {
    let keyW = keyWidth(totalWidth: totalWidth)
    return (totalWidth - 2 * KeyMetrics.edgeInset - 7 * keyW
            - 8 * KeyMetrics.hGap) / 2
  }

  /// Letter-area height below the candidate bar.
  static func rowsHeight() -> CGFloat {
    KeyMetrics.topPadding + 4 * KeyMetrics.keyHeight
      + 3 * KeyMetrics.rowGap + KeyMetrics.bottomPadding
  }

  static func totalHeight(totalWidth: CGFloat) -> CGFloat {
    _ = totalWidth  // height is width-independent in portrait
    return KeyMetrics.candidateBarHeight + rowsHeight()
  }

  static func compute(width: CGFloat, page: KeyboardPage) -> [KeyCap] {
    let keyW = keyWidth(totalWidth: width)
    let ws = specialWidth(totalWidth: width)
    var caps: [KeyCap] = []
    var nextId = 0

    func addRow(_ entries: [(KeyKind, String, CGFloat, Bool)],
                x0: CGFloat, y: CGFloat) {
      var x = x0
      for (kind, label, w, special) in entries {
        caps.append(KeyCap(id: nextId, kind: kind, label: label,
                           frame: CGRect(x: x, y: y, width: w,
                                         height: KeyMetrics.keyHeight),
                           isSpecial: special))
        nextId += 1
        x += w + KeyMetrics.hGap
      }
    }

    func charRow(_ chars: [String], x0: CGFloat, y: CGFloat) {
      addRow(chars.map { (.char($0), $0, keyW, false) }, x0: x0, y: y)
    }

    func bottomRow(_ leftKind: KeyKind, _ leftLabel: String, y: CGFloat) {
      let spaceW = width - 2 * KeyMetrics.edgeInset - 2 * ws
        - 2 * KeyMetrics.hGap
      addRow([
        (leftKind, leftLabel, ws, true),
        (.space, "空格", spaceW, true),
        (.returnKey, "⏎", ws, true),
      ], x0: KeyMetrics.edgeInset, y: y)
    }

    let y0 = KeyMetrics.candidateBarHeight + KeyMetrics.topPadding
    let dy = KeyMetrics.keyHeight + KeyMetrics.rowGap
    let row2X0 = KeyMetrics.edgeInset + (keyW + KeyMetrics.hGap) / 2

    switch page {
    case .letters:
      charRow("qwertyuiop".map(String.init), x0: KeyMetrics.edgeInset, y: y0)
      charRow("asdfghjkl".map(String.init), x0: row2X0, y: y0 + dy)
      let letters3 = "zxcvbnm".map(String.init)
      var row3: [(KeyKind, String, CGFloat, Bool)] = [(.shift, "⇧", ws, true)]
      row3 += letters3.map { (.char($0), $0, keyW, false) }
      row3.append((.backspace, "⌫", ws, true))
      addRow(row3, x0: KeyMetrics.edgeInset, y: y0 + 2 * dy)
      bottomRow(.toSymbols1, "123", y: y0 + 3 * dy)
    case .symbols1:
      charRow(["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
              x0: KeyMetrics.edgeInset, y: y0)
      charRow(["-", "/", ":", ";", "(", ")", "¥", "&", "@", "\""],
              x0: KeyMetrics.edgeInset, y: y0 + dy)
      addRow([(.toSymbols2, "#+=", ws, true)]
             + [".", ",", "?", "!", "'"].map { (.char($0), $0, keyW, false) }
             + [(.backspace, "⌫", ws, true)],
             x0: KeyMetrics.edgeInset, y: y0 + 2 * dy)
      bottomRow(.toLetters, "ABC", y: y0 + 3 * dy)
    case .symbols2:
      charRow(["[", "]", "{", "}", "#", "%", "^", "*", "+", "="],
              x0: KeyMetrics.edgeInset, y: y0)
      charRow(["_", "\\", "|", "~", "<", ">", "€", "£", "$", "¥"],
              x0: KeyMetrics.edgeInset, y: y0 + dy)
      addRow([(.toSymbols1, "123", ws, true)]
             + [".", ",", "?", "!", "'"].map { (.char($0), $0, keyW, false) }
             + [(.backspace, "⌫", ws, true)],
             x0: KeyMetrics.edgeInset, y: y0 + 2 * dy)
      bottomRow(.toLetters, "ABC", y: y0 + 3 * dy)
    }
    return caps
  }
}
