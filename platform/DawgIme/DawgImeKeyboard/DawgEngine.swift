// SPDX-License-Identifier: Apache-2.0
// Swift wrapper over the extern "C" dawg engine facade (dawg_capi.h).

import Foundation

final class DawgEngine {
  private var handle: OpaquePointer?

  /// Loads dawg_pinyin.dawg (+ optional dawg_pinyin.clm) from the bundle.
  /// Returns nil when the lexicon asset is missing or unreadable.
  init?() {
    guard let dawgURL = Bundle.main.url(forResource: "dawg_pinyin",
                                        withExtension: "dawg") else {
      return nil
    }
    let lmURL = Bundle.main.url(forResource: "dawg_pinyin",
                                withExtension: "clm")
    var created: OpaquePointer?
    var err: Int32 = 0
    dawgURL.path.withCString { dawgPath in
      if let lmPath = lmURL?.path {
        lmPath.withCString { lmC in
          var options = DawgOptions(dawg_path: dawgPath, lm_path: lmC,
                                    max_candidates: 32, beam_width: 16,
                                    context_chars: 8)
          created = dawg_engine_create(&options, &err)
        }
      } else {
        var options = DawgOptions(dawg_path: dawgPath, lm_path: nil,
                                  max_candidates: 32, beam_width: 16,
                                  context_chars: 8)
        created = dawg_engine_create(&options, &err)
      }
    }
    guard let engine = created else {
      NSLog("DawgEngine create failed: \(err)")
      return nil
    }
    handle = engine
  }

  deinit {
    if let engine = handle {
      dawg_engine_destroy(engine)
    }
  }

  /// Ranked candidate texts for `input` conditioned on `context`.
  func search(_ input: String, context: String) -> [String] {
    guard let engine = handle, !input.isEmpty else { return [] }
    var texts: [String] = []
    input.withCString { inputC in
      context.withCString { contextC in
        var err: Int32 = 0
        let count = dawg_engine_search(engine, inputC, contextC, &err)
        guard count > 0 else { return }
        for index in 0..<count {
          var textPtr: UnsafePointer<CChar>?
          if dawg_candidate_get(engine, index, &textPtr, nil, nil, nil) == 1,
             let raw = textPtr {
            texts.append(String(cString: raw))
          }
        }
      }
    }
    return texts
  }
}
