// SPDX-License-Identifier: Apache-2.0
import { KeyboardPage, KeyFrame, KeyKind, KeyMetrics } from './KeySpec';
import { DawgEngineBridge } from './DawgEngine';

/** Platform text-editing sink supplied by the keyboard container. */
export interface TextSink {
  insertText(text: string): void;
  deleteBackward(): void;
  sendReturn(): void;
}

/**
 * Platform-agnostic input state machine. Mirrors KeyboardModel.swift (iOS)
 * and ImeSession.kt (Android); keep behaviors aligned.
 */
export class KeyboardState {
  page: KeyboardPage = KeyboardPage.LETTERS;
  buffer: string = '';
  candidates: string[] = [];
  shiftOn: boolean = false;
  engineReady: boolean;
  /**
   * API 9's InputClient cannot supply preceding text, so context is tracked
   * internally from this IME's committed output only (last 8 chars).
   */
  committedContext: string = '';

  private sink: TextSink;

  constructor(sink: TextSink) {
    this.sink = sink;
    this.engineReady = DawgEngineBridge.isReady();
  }

  tap(frame: KeyFrame): void {
    if (frame.kind === KeyKind.CHAR) {
      if (this.page === KeyboardPage.LETTERS) {
        this.appendLetter(frame.label);
      } else {
        this.commit(frame.label); // symbols insert directly
      }
    } else if (frame.kind === KeyKind.SHIFT) {
      if (this.page === KeyboardPage.LETTERS) {
        this.shiftOn = !this.shiftOn;
      }
    } else if (frame.kind === KeyKind.BACKSPACE) {
      if (this.buffer.length > 0) {
        this.buffer = this.buffer.slice(0, -1);
        this.refresh();
      } else {
        this.sink.deleteBackward();
      }
    } else if (frame.kind === KeyKind.SPACE) {
      if (this.buffer.length > 0) {
        this.commit(this.candidates.length > 0 ? this.candidates[0]
                                               : this.buffer);
      } else {
        this.commit(' ');
      }
    } else if (frame.kind === KeyKind.RETURN) {
      if (this.buffer.length > 0) {
        this.commit(this.buffer); // commit raw letters, no newline
      } else {
        this.sink.sendReturn();
      }
    } else if (frame.kind === KeyKind.TO_SYMBOLS1) {
      this.page = KeyboardPage.SYMBOLS1;
      this.shiftOn = false;
    } else if (frame.kind === KeyKind.TO_SYMBOLS2) {
      this.page = KeyboardPage.SYMBOLS2;
    } else if (frame.kind === KeyKind.TO_LETTERS) {
      this.page = KeyboardPage.LETTERS;
    }
  }

  selectCandidate(text: string): void {
    this.commit(text);
  }

  private appendLetter(letter: string): void {
    if (this.buffer.length >= KeyMetrics.MAX_BUFFER_LEN) {
      return;
    }
    this.buffer += this.shiftOn ? letter.toUpperCase() : letter.toLowerCase();
    if (this.shiftOn) {
      this.shiftOn = false; // one-shot shift
    }
    this.refresh();
  }

  private commit(text: string): void {
    if (text.length === 0) {
      return;
    }
    this.sink.insertText(text);
    this.buffer = '';
    this.candidates = [];
    const joined = this.committedContext + text;
    this.committedContext = joined.length > KeyMetrics.CONTEXT_CHARS
      ? joined.slice(-KeyMetrics.CONTEXT_CHARS)
      : joined;
  }

  private refresh(): void {
    if (this.engineReady && this.buffer.length > 0) {
      const context = this.committedContext.length > KeyMetrics.CONTEXT_CHARS
        ? this.committedContext.slice(-KeyMetrics.CONTEXT_CHARS)
        : this.committedContext;
      this.candidates = DawgEngineBridge.search(this.buffer.toLowerCase(),
                                                context);
    } else {
      this.candidates = [];
    }
  }
}
