// SPDX-License-Identifier: Apache-2.0
// Apple-keyboard-spec metrics and pure layout math. The same numbers and
// formulas are duplicated verbatim in KeySpec.swift (iOS) and KeySpec.kt
// (Android); keep all three in sync. Units: vp.

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
  static readonly CANDIDATE_BAR_HEIGHT: number = 44;
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
  static readonly SEPARATOR_HEIGHT: number = 20;
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

  /** Letter-area height below the candidate bar. */
  static rowsHeight(): number {
    return KeyMetrics.TOP_PADDING + 4 * KeyMetrics.KEY_HEIGHT +
      3 * KeyMetrics.ROW_GAP + KeyMetrics.BOTTOM_PADDING;
  }

  static totalHeight(): number {
    return KeyMetrics.CANDIDATE_BAR_HEIGHT + KeyLayout.rowsHeight();
  }

  static compute(width: number, page: KeyboardPage): KeyFrame[] {
    const keyW = KeyLayout.keyWidth(width);
    const ws = KeyLayout.specialWidth(width);
    const frames: KeyFrame[] = [];
    const y0 = KeyMetrics.CANDIDATE_BAR_HEIGHT + KeyMetrics.TOP_PADDING;
    const dy = KeyMetrics.KEY_HEIGHT + KeyMetrics.ROW_GAP;
    const row2X0 = KeyMetrics.EDGE_INSET + (keyW + KeyMetrics.H_GAP) / 2;

    const addRow = (entries: Array<[KeyKind, string, number]>, x0: number,
                    y: number): void => {
      let x = x0;
      for (const entry of entries) {
        frames.push({
          kind: entry[0], label: entry[1], x: x, y: y,
          width: entry[2], height: KeyMetrics.KEY_HEIGHT,
          isSpecial: entry[0] !== KeyKind.CHAR,
        });
        x += entry[2] + KeyMetrics.H_GAP;
      }
    };
    const charRow = (chars: string[], x0: number, y: number): void => {
      addRow(chars.map((c): [KeyKind, string, number] =>
        [KeyKind.CHAR, c, keyW]), x0, y);
    };
    const bottomRow = (leftKind: KeyKind, leftLabel: string, y: number): void => {
      const spaceW = width - 2 * KeyMetrics.EDGE_INSET - 2 * ws -
        2 * KeyMetrics.H_GAP;
      addRow([
        [leftKind, leftLabel, ws],
        [KeyKind.SPACE, '空格', spaceW],
        [KeyKind.RETURN, '⏎', ws],
      ], KeyMetrics.EDGE_INSET, y);
    };

    if (page === KeyboardPage.LETTERS) {
      charRow(['q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l'], row2X0, y0 + dy);
      const row3: Array<[KeyKind, string, number]> = [[KeyKind.SHIFT, '⇧', ws]];
      for (const c of ['z', 'x', 'c', 'v', 'b', 'n', 'm']) {
        row3.push([KeyKind.CHAR, c, keyW]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_SYMBOLS1, '123', y0 + 3 * dy);
    } else if (page === KeyboardPage.SYMBOLS1) {
      charRow(['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['-', '/', ':', ';', '(', ')', '¥', '&', '@', '"'],
        KeyMetrics.EDGE_INSET, y0 + dy);
      const row3: Array<[KeyKind, string, number]> =
        [[KeyKind.TO_SYMBOLS2, '#+=', ws]];
      for (const c of ['.', ',', '?', '!', "'"]) {
        row3.push([KeyKind.CHAR, c, keyW]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_LETTERS, 'ABC', y0 + 3 * dy);
    } else {
      charRow(['[', ']', '{', '}', '#', '%', '^', '*', '+', '='],
        KeyMetrics.EDGE_INSET, y0);
      charRow(['_', '\\', '|', '~', '<', '>', '€', '£', '$', '¥'],
        KeyMetrics.EDGE_INSET, y0 + dy);
      const row3: Array<[KeyKind, string, number]> =
        [[KeyKind.TO_SYMBOLS1, '123', ws]];
      for (const c of ['.', ',', '?', '!', "'"]) {
        row3.push([KeyKind.CHAR, c, keyW]);
      }
      row3.push([KeyKind.BACKSPACE, '⌫', ws]);
      addRow(row3, KeyMetrics.EDGE_INSET, y0 + 2 * dy);
      bottomRow(KeyKind.TO_LETTERS, 'ABC', y0 + 3 * dy);
    }
    return frames;
  }
}
