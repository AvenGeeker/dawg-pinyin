// SPDX-License-Identifier: Apache-2.0
// SwiftUI renderer for the WeChat-style keyboard: 44pt top bar (toolbar
// when idle, candidate bar while composing), uppercase letter caps and the
// five-key bottom row. Press feedback is a 10% darken overlay (no popups).

import SwiftUI

struct KeyboardView: View {
  var model: KeyboardModel
  @Environment(\.colorScheme) private var scheme

  var body: some View {
    let theme = KeyboardTheme.palette(for: scheme)
    GeometryReader { geo in
      let caps = KeyLayout.compute(width: geo.size.width, page: model.page)
      ZStack(alignment: .topLeading) {
        theme.background
        topBar(theme: theme)
          .frame(width: geo.size.width, height: KeyMetrics.topBarHeight)
        ForEach(caps) { cap in
          keyButton(cap, theme: theme)
            .frame(width: cap.frame.width, height: cap.frame.height)
            .position(x: cap.frame.midX, y: cap.frame.midY)
        }
      }
    }
  }

  // MARK: - Top bar

  @ViewBuilder
  private func topBar(theme: KeyboardTheme) -> some View {
    if model.isComposing {
      candidateBar(theme: theme)
    } else {
      toolbar(theme: theme)
    }
  }

  /// Idle toolbar: brand mark left, IME switch + hide round buttons right.
  private func toolbar(theme: KeyboardTheme) -> some View {
    HStack(spacing: 10) {
      ZStack {
        Circle().fill(theme.key)
        Text("D")
          .font(.system(size: 18, weight: .black))
          .foregroundStyle(theme.accent)
      }
      .frame(width: KeyMetrics.toolbarButtonSize,
             height: KeyMetrics.toolbarButtonSize)
      Spacer()
      roundButton("keyboard", theme: theme) { model.switchToNextKeyboard() }
      roundButton("chevron.down", theme: theme) { model.hideKeyboard() }
    }
    .padding(.horizontal, 8)
  }

  private func roundButton(_ systemName: String, theme: KeyboardTheme,
                           action: @escaping () -> Void) -> some View {
    Button(action: action) {
      Image(systemName: systemName)
        .font(.system(size: 16, weight: .medium))
        .foregroundStyle(theme.text)
        .frame(width: KeyMetrics.toolbarButtonSize,
               height: KeyMetrics.toolbarButtonSize)
        .background(Circle().fill(theme.key))
    }
    .buttonStyle(PressDimStyle())
  }

  /// Composing candidate bar: first candidate highlighted WeChat-style.
  @ViewBuilder
  private func candidateBar(theme: KeyboardTheme) -> some View {
    if model.engineReady {
      ScrollView(.horizontal, showsIndicators: false) {
        HStack(spacing: 0) {
          ForEach(Array(model.candidates.enumerated()), id: \.offset) {
            index, candidate in
            if index == 0 {
              Button { model.selectCandidate(candidate) } label: {
                Text(candidate)
                  .font(.system(size: KeyMetrics.candidateFontSize))
                  .foregroundStyle(theme.accent)
                  .padding(.horizontal, 12)
                  .frame(height: 36)
                  .background(
                    RoundedRectangle(cornerRadius: 6).fill(theme.key))
                  .padding(.horizontal, 6)
              }
              .buttonStyle(PressDimStyle())
            } else {
              Button { model.selectCandidate(candidate) } label: {
                Text(candidate)
                  .font(.system(size: KeyMetrics.candidateFontSize))
                  .foregroundStyle(theme.text)
                  .padding(.horizontal, KeyMetrics.candidatePadding)
                  .frame(height: KeyMetrics.topBarHeight)
              }
              .buttonStyle(PressDimStyle())
            }
          }
        }
      }
    } else {
      Text("词库加载失败，仅支持字母直输")
        .font(.system(size: 13))
        .foregroundStyle(theme.text.opacity(0.6))
        .frame(maxWidth: .infinity, maxHeight: KeyMetrics.topBarHeight)
    }
  }

  // MARK: - Keys

  private func keyButton(_ cap: KeyCap, theme: KeyboardTheme) -> some View {
    Button { model.tap(cap) } label: {
      keyLabel(cap, theme: theme)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    .buttonStyle(KeyButtonStyle(
      background: backgroundColor(cap, theme: theme),
      pressed: pressedColor(cap, theme: theme),
      cornerRadius: KeyMetrics.cornerRadius))
  }

  @ViewBuilder
  private func keyLabel(_ cap: KeyCap, theme: KeyboardTheme) -> some View {
    switch cap.kind {
    case .comma:
      VStack(spacing: -2) {
        Text(model.chineseMode ? "。" : ".")
          .font(.system(size: 9))
          .foregroundStyle(theme.text)
        Text(model.chineseMode ? "，" : ",")
          .font(.system(size: 18))
          .foregroundStyle(theme.text)
      }
    case .langToggle:
      VStack(spacing: -4) {
        Text("中")
          .font(.system(size: 13))
          .foregroundStyle(
            theme.text.opacity(model.chineseMode ? 1 : 0.4))
        Text("英")
          .font(.system(size: 13))
          .foregroundStyle(
            theme.text.opacity(model.chineseMode ? 0.4 : 1))
      }
    case .action:
      Text(model.isComposing ? "确定" : model.actionLabel())
        .font(.system(size: KeyMetrics.candidateFontSize))
        .foregroundStyle(model.isComposing ? theme.text
                                           : theme.textOnAccent)
    case .space:
      Text("")
    case .toSymbols1, .toSymbols2, .toLetters:
      Text(cap.label)
        .font(.system(size: KeyMetrics.candidateFontSize))
        .foregroundStyle(theme.text)
    default:
      Text(cap.label)
        .font(.system(size: cap.isSpecial ? KeyMetrics.specialFontSize
                                          : KeyMetrics.letterFontSize))
        .foregroundStyle(theme.text)
    }
  }

  private func backgroundColor(_ cap: KeyCap, theme: KeyboardTheme) -> Color {
    if cap.kind == .action && !model.isComposing { return theme.accent }
    if cap.kind == .shift && model.shiftOn { return theme.key }
    return cap.isSpecial ? theme.specialKey : theme.key
  }

  private func pressedColor(_ cap: KeyCap, theme: KeyboardTheme) -> Color {
    if cap.kind == .action && !model.isComposing { return theme.pressedAccent }
    if cap.kind == .shift && model.shiftOn { return theme.pressedKey }
    return cap.isSpecial ? theme.pressedSpecialKey : theme.pressedKey
  }
}

struct KeyButtonStyle: ButtonStyle {
  let background: Color
  let pressed: Color
  let cornerRadius: CGFloat

  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .background(configuration.isPressed ? pressed : background)
      .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
  }
}

/// Press feedback for borderless surfaces (toolbar / candidates).
struct PressDimStyle: ButtonStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.6 : 1)
  }
}
