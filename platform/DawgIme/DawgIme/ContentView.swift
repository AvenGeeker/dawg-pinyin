// SPDX-License-Identifier: Apache-2.0
// Setup guide for enabling the DawgIme keyboard.

import SwiftUI

struct ContentView: View {
  private let steps = [
    "打开系统「设置」→「通用」→「键盘」→「键盘」→「添加新键盘…」",
    "在第三方键盘中选择「DawgIme」",
    "返回输入界面，长按左下角地球键切换到 DawgIme",
  ]

  var body: some View {
    NavigationStack {
      List {
        Section {
          ForEach(Array(steps.enumerated()), id: \.offset) { index, step in
            HStack(alignment: .top, spacing: 12) {
              Text("\(index + 1)")
                .font(.headline)
                .frame(width: 24, height: 24)
                .background(Circle().fill(.tint.opacity(0.15)))
              Text(step)
            }
            .padding(.vertical, 4)
          }
        } header: {
          Text("启用 DawgIme 拼音键盘")
        } footer: {
          Text("键盘无需网络权限；词库随应用内置，离线可用。")
        }
      }
      .navigationTitle("DawgIme")
    }
  }
}

#Preview {
  ContentView()
}
