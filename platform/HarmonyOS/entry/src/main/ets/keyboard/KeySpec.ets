// SPDX-License-Identifier: Apache-2.0
// WeChat-style keyboard metrics and pure layout math. The same numbers and
// formulas are duplicated verbatim in KeySpec.swift (iOS) and KeySpec.kt
// (Android); keep all three in sync. Units: vp.
//
// Layout: 44vp top bar (toolbar when idle, candidate bar while composing),
// four key rows with uppercase letter caps, and a five-key bottom row
// [123/ABC] [comma] [space] [CN/EN] [action].

export enum KeyboardPage {
  LETTERS,
  SYMBOLS1,
  SYMBOLS2,
}

export enum KeyKind {
  CHAR,
  SHIFT,
  BACKSPACE,
  SPACE,
  RETURN,
  TO_SYMBOLS1,
  TO_SYMBOLS2,
  TO_LETTERS,
  COMMA,
  LANG_TOGGLE,
  ACTION,
}

export interface KeyFrame {
  kind: KeyKind;
  label: string;
  x: number;
  y: number;
  width: number;
  height: number;
  isSpecial: boolean;
}

export class KeyMetrics {
  static readonly TOP_BAR_HEIGHT: number = 44;
  static readonly TOP_PADDING: number = 8;
  static readonly KEY_HEIGHT: number = 42;
  static readonly ROW_GAP: number = 12;
  static readonly BOTTOM_PADDING: number = 6;
  static readonly H_GAP: number = 6;
  static readonly EDGE_INSET: number = 3;
  static readonly CORNER_RADIUS: number = 4;
  static readonly LETTER_FONT_SIZE: number = 22;
  static readonly SPECIAL_FONT_SIZE: number = 20;
  static readonly CANDIDATE_FONT_SIZE: number = 16;
  static readonly CANDIDATE_PADDING: number = 14;
  static readonly TOOLBAR_BUTTON_SIZE: number = 34;
  static readonly MAX_BUFFER_LEN: number = 64;
  static readonly CONTEXT_CHARS: number = 8;
}

export class KeyLayout {
  /** Letter key width: (W - 2x3 - 9x6) / 10. */
  static keyWidth(totalWidth: number): number {
    return (totalWidth - 2 * KeyMetrics.EDGE_INSET - 9 * KeyMetrics.H_GAP) / 10;
  }

  /** Shift/backspace/page-switch width: (W - 6 - 7xkeyW - 8x6) / 2. */
  static specialWidth(totalWidth: number): number {
    const keyW = KeyLayout.keyWidth(totalWidth);
    return (totalWidth - 2 * KeyMetrics.EDGE_INSET - 7 * keyW -
      8 * KeyMetrics.H_GAP) / 2;
  }

  /**
   * Bottom-row widths [num, comma, space, lang, action] as
   * unit x [2, 1, 4.2, 1, 2.5] with unit = (W - 2x3 - 4x6) / 10.7.
   */
  static bottomWidths(totalWidth: number): number[] {
    const unit = (totalWidth - 2 * KeyMetrics.EDGE_INSET -
      4 * KeyMetrics.H_GAP) / 10.7;
    return [2 * unit, unit, 4.2 * unit, unit, 2.5 * unit];
  }

  /** Letter-area height below the top bar. */
  static rowsHeight(): number {
    return KeyMetrics.TOP_PADDING + 4 * KeyMetrics.KEY_HEIGHT +
      3 * KeyMetrics.ROW_GAP + KeyMetrics.BOTTOM_PADDING;
  }

  static totalHeight(): number {
    return KeyMetrics.TOP_BAR_HEIGHT + KeyLayout.rowsHeight();
  }

  static compute(width: number, page: KeyboardPage): KeyFrame[] {
    const keyW = KeyLayout.keyWidth(width);
    const ws = KeyLayout.specialWidth(width);
    const frames: KeyFrame[] = [];
    const y0 = KeyMetrics.TOP_BAR_HEIGHT + KeyMetrics.TOP_PADDING;
    const dy = KeyMetrics.KEY_HEIGHT + KeyMetrics.ROW_GAP;
    const row2X0 = KeyMetrics.EDGE_INSET + (keyW + KeyMetrics.H_GAP) / 2;

    const addRow = (entries: Array<[KeyKind, string, number, boolean]>,
                    x0: number, y: number): void => {
      let x = x0;
      for (const entry of entries) {
        frames.push({
          kind: entry[0], label: entry[1], x: x, y: y,
          width: entry[2], height: KeyMetrics.KEY_HEIGHT,
          isSpecial: entry[3],
        });
        x += entry[2] + KeyMetrics.H_GAP;
      }
    };
    const charRow = (chars: string[], x0: number, y: number): void => {
      addRow(chars.map((c): [KeyKind, string, number, boolean] =>
        [KeyKind.CHAR, c, keyW, false]), x0, y);
    };
    const bottomRow = (leftKind: KeyKind, leftLabel: string,
                       y: number): void => {
      const bw = KeyLayout.bottomWidths(width);
      addRow([
        [leftKind, leftLabel, bw[0], true],
        [KeyKind.COMMA, '', bw[1], false],
        [KeyKind.SPACE, '', bw[2], false],
        [KeyKind.LANG_TOGGLE, '', bw[3], false],
        [KeyKind.ACTION, '', bw[4], true],
      ], KeyMetrics.EDGE_INSET, y);
    };

    if (page === KeyboardPage.LETTERS) {
      charRow(['Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'], row2X0, y0 + dy);
      const row3: Array<[KeyKind, string, number, boolean]> =
        [[KeyKind.SHIFT, '⇧', ws, true]];
      for (const c of ['Z', 'X', 'C', 'V', 'B', 'N', 'M']) {
        row3.push([KeyKind.CHAR, c, keyW, false]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws, true]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_SYMBOLS1, '123', y0 + 3 * dy);
    } else if (page === KeyboardPage.SYMBOLS1) {
      charRow(['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['-', '/', ':', ';', '(', ')', '¥', '&', '@', '"'],
        KeyMetrics.EDGE_INSET, y0 + dy);
      const row3: Array<[KeyKind, string, number, boolean]> =
        [[KeyKind.TO_SYMBOLS2, '#+=', ws, true]];
      for (const c of ['.', ',', '?', '!', "'"]) {
        row3.push([KeyKind.CHAR, c, keyW, false]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws, true]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_LETTERS, 'ABC', y0 + 3 * dy);
    } else {
      charRow(['[', ']', '{', '}', '#', '%', '^', '*', '+', '='],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['_', '\\', '|', '~', '<', '>', '€', '£', '$', '¥'],
        KeyMetrics.EDGE_INSET, y0 + dy);
      const row3: Array<[KeyKind, string, number, boolean]> =
        [[KeyKind.TO_SYMBOLS1, '123', ws, true]];
      for (const c of ['.', ',', '?', '!', "'"]) {
        row3.push([KeyKind.CHAR, c, keyW, false]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws, true]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_LETTERS, 'ABC', y0 + 3 * dy);
    }
    return frames;
  }
}
