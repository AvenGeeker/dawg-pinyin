// SPDX-License-Identifier: Apache-2.0
/** NAPI surface of libdawg_napi.so (see ../dawg_napi.cc). */
export interface DawgNapi {
  create(dawgPath: string, lmPath: string, maxCandidates: number,
         beamWidth: number, contextChars: number): number;
  search(handle: number, input: string, context: string): number;
  candidateText(handle: number, index: number): string | null;
  destroy(handle: number): void;
}

declare const engine: DawgNapi;
export default engine;
