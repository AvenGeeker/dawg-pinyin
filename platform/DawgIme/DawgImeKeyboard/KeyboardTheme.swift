// SPDX-License-Identifier: Apache-2.0
// WeChat-style palette shared by the three platform keyboards.

import SwiftUI

struct KeyboardTheme {
  let background: Color
  let key: Color
  let specialKey: Color
  let pressedKey: Color
  let pressedSpecialKey: Color
  let accent: Color
  let pressedAccent: Color
  let text: Color
  let textOnAccent: Color
  let separator: Color

  static func palette(for scheme: ColorScheme) -> KeyboardTheme {
    switch scheme {
    case .dark:
      let key = Color(hex: 0x494B53)
      let special = Color(hex: 0x2F3238)
      let accent = Color(hex: 0x07C160)  // WeChat green
      return KeyboardTheme(
        background: Color(hex: 0x1C1C1E),
        key: key,
        specialKey: special,
        pressedKey: key.darkened(0.10),
        pressedSpecialKey: special.darkened(0.10),
        accent: accent,
        pressedAccent: accent.darkened(0.10),
        text: .white,
        textOnAccent: .white,
        separator: Color(hex: 0x8A8F98).opacity(0.3))
    default:
      let key = Color(hex: 0xFFFFFF)
      let special = Color(hex: 0xABB2BC)
      let accent = Color(hex: 0x07C160)  // WeChat green
      return KeyboardTheme(
        background: Color(hex: 0xD1D5DB),
        key: key,
        specialKey: special,
        pressedKey: key.darkened(0.10),
        pressedSpecialKey: special.darkened(0.10),
        accent: accent,
        pressedAccent: accent.darkened(0.10),
        text: .black,
        textOnAccent: .white,
        separator: Color(hex: 0x8A8F98).opacity(0.4))
    }
  }
}

extension Color {
  init(hex: UInt32) {
    self.init(.sRGB,
              red: Double((hex >> 16) & 0xFF) / 255,
              green: Double((hex >> 8) & 0xFF) / 255,
              blue: Double(hex & 0xFF) / 255)
  }

  /// Overlay `fraction` of black (press feedback per spec).
  func darkened(_ fraction: Double) -> Color {
    let resolved = UIColor(self).cgColor.components ?? [0, 0, 0, 1]
    func ch(_ i: Int) -> Double { Double(resolved[i]) * (1 - fraction) }
    return Color(.sRGB, red: ch(0), green: ch(1), blue: ch(2))
  }
}
