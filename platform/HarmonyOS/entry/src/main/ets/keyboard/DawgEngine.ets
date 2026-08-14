// SPDX-License-Identifier: Apache-2.0
import engine from 'libdawg_napi.so';

/**
 * Wrapper over the NAPI engine facade. Assets must live on real file paths
 * (mmap), so DawgIME copies the rawfile assets into filesDir before init().
 */
export class DawgEngineBridge {
  private static handle: number = 0;
  private static ready: boolean = false;

  static init(filesDir: string): void {
    const dawgPath = `${filesDir}/dawg_pinyin.dawg`;
    const clmPath = `${filesDir}/dawg_pinyin.clm`;
    DawgEngineBridge.handle = engine.create(dawgPath, clmPath, 32, 16, 8);
    DawgEngineBridge.ready = DawgEngineBridge.handle !== 0;
  }

  static isReady(): boolean {
    return DawgEngineBridge.ready;
  }

  static search(input: string, context: string): string[] {
    if (!DawgEngineBridge.ready || input.length === 0) {
      return [];
    }
    const count = engine.search(DawgEngineBridge.handle, input, context);
    const out: string[] = [];
    for (let i = 0; i < count; i++) {
      const text = engine.candidateText(DawgEngineBridge.handle, i);
      if (text !== null) {
        out.push(text);
      }
    }
    return out;
  }

  static destroy(): void {
    if (DawgEngineBridge.ready) {
      engine.destroy(DawgEngineBridge.handle);
      DawgEngineBridge.ready = false;
    }
  }
}
