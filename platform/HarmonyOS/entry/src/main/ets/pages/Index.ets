// SPDX-License-Identifier: Apache-2.0
/** Setup guide page shown by the launcher app. */
@Entry
@Component
struct Index {
  private steps: string[] = [
    '打开系统「设置」→「系统和更新」→「语言和输入法」',
    '在「键盘」列表中选择 DawgIme Pinyin 并启用',
    '在任意输入框点击键盘切换按钮选择 DawgIme',
  ];

  build() {
    Column({ space: 16 }) {
      Text('启用 DawgIme 拼音键盘')
        .fontSize(22)
        .fontWeight(FontWeight.Bold)
      ForEach(this.steps, (step: string, index: number) => {
        Row({ space: 12 }) {
          Text(`${index + 1}`)
            .fontSize(16)
            .width(28)
            .height(28)
            .textAlign(TextAlign.Center)
            .borderRadius(14)
            .backgroundColor('#0A59F7')
            .fontColor(Color.White)
          Text(step)
            .fontSize(16)
            .layoutWeight(1)
        }
        .width('100%')
      }, (step: string, index: number) => `${index}:${step}`)
      Text('键盘无需网络权限；词库随应用内置，离线可用。')
        .fontSize(13)
        .fontColor('#888888')
    }
    .width('100%')
    .height('100%')
    .padding(24)
    .alignItems(HorizontalAlign.Start)
    .justifyContent(FlexAlign.Center)
  }
}
