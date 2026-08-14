// SPDX-License-Identifier: Apache-2.0
// SwiftUI renderer for the shared keyboard layout. Keys are absolutely
// positioned from KeyLayout.compute(); press feedback is a 10% darken
// overlay via the button style (no popups, per spec).

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
        candidateBar(theme: theme)
          .frame(width: geo.size.width,
                 height: KeyMetrics.candidateBarHeight)
        Rectangle()
          .fill(theme.separator)
          .frame(width: geo.size.width, height: 0.5)
          .position(x: geo.size.width / 2,
                    y: KeyMetrics.candidateBarHeight)
        ForEach(caps) { cap in
          keyButton(cap, theme: theme)
            .frame(width: cap.frame.width, height: cap.frame.height)
            .position(x: cap.frame.midX, y: cap.frame.midY)
        }
      }
    }
  }

  @ViewBuilder
  private func candidateBar(theme: KeyboardTheme) -> some View {
    if model.engineReady {
      ScrollView(.horizontal, showsIndicators: false) {
        HStack(spacing: 0) {
          ForEach(Array(model.candidates.enumerated()), id: \.offset) {
            index, candidate in
            Button { model.selectCandidate(candidate) } label: {
              Text(candidate)
                .font(.system(size: KeyMetrics.candidateFontSize))
                .foregroundStyle(theme.text)
                .padding(.horizontal, KeyMetrics.candidatePadding)
                .frame(height: KeyMetrics.candidateBarHeight)
            }
            if index < model.candidates.count - 1 {
              Rectangle()
                .fill(theme.separator)
                .frame(width: 1, height: KeyMetrics.separatorHeight)
            }
          }
        }
      }
    } else {
      Text("词库加载失败，仅支持字母直输")
        .font(.system(size: 13))
        .foregroundStyle(theme.text.opacity(0.6))
        .frame(maxWidth: .infinity,
               maxHeight: KeyMetrics.candidateBarHeight)
    }
  }

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
    case .space:
      Text(cap.label)
        .font(.system(size: KeyMetrics.candidateFontSize))
        .foregroundStyle(theme.text)
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
    if cap.kind == .shift && model.shiftOn { return theme.key }
    return cap.isSpecial ? theme.specialKey : theme.key
  }

  private func pressedColor(_ cap: KeyCap, theme: KeyboardTheme) -> Color {
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
