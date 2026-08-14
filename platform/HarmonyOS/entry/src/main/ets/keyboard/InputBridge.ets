// SPDX-License-Identifier: Apache-2.0
import inputMethodEngine from '@ohos.inputMethodEngine';
import { TextSink } from './KeyboardState';

/**
 * Adapts the framework InputClient to the state machine's TextSink.
 * NOTE (API 9): InputClient exposes no way to read the text before the
 * cursor, so KeyboardState tracks committed context internally only.
 */
export class InputBridge {
  private static client: inputMethodEngine.InputClient | null = null;

  static setClient(client: inputMethodEngine.InputClient): void {
    InputBridge.client = client;
  }

  static readonly sink: TextSink = {
    insertText(text: string): void {
      const client = InputBridge.client;
      if (client !== null) {
        client.insertText(text);
      }
    },
    deleteBackward(): void {
      const client = InputBridge.client;
      if (client !== null) {
        client.deleteBackward(1);
      }
    },
    sendReturn(): void {
      const client = InputBridge.client;
      if (client !== null) {
        client.sendKeyFunction(0); // ENTER_KEY_TYPE_UNSPECIFIED
      }
    },
  };
}
