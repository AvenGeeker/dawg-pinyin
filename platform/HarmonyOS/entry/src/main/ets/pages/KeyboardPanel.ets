// SPDX-License-Identifier: Apache-2.0
import display from '@ohos.display';
import { KeyLayout, KeyMetrics, KeyFrame, KeyKind, KeyboardPage }
  from '../keyboard/KeySpec';
import { KeyboardState } from '../keyboard/KeyboardState';
import { InputBridge } from '../keyboard/InputBridge';

/** WeChat-style palette shared by the three platform keyboards. */
class KeyboardTheme {
  background: string;
  key: string;
  specialKey: string;
  pressed: string;
  accent: string;
  text: string;
  textOnAccent: string;
  separator: string;

  constructor(background: string, key: string, specialKey: string,
              pressed: string, accent: string, text: string,
              textOnAccent: string, separator: string) {
    this.background = background;
    this.key = key;
    this.specialKey = specialKey;
    this.pressed = pressed;
    this.accent = accent;
    this.text = text;
    this.textOnAccent = textOnAccent;
    this.separator = separator;
  }
}

const LIGHT_THEME = new KeyboardTheme('#D1D5DB', '#FFFFFF', '#ABB2BC',
  '#00000019', '#07C160', '#000000', '#FFFFFF', '#668A8F98');
const DARK_THEME = new KeyboardTheme('#1C1C1E', '#494B53', '#2F3238',
  '#00000019', '#07C160', '#FFFFFF', '#FFFFFF', '#4D8A8F98');

/**
 * Keyboard panel page hosted by the DawgIME Panel. Renders the WeChat-style
 * layout spec; the state machine is KeyboardState (three-platform parity).
 */
@Entry
@Component
struct KeyboardPanel {
  @State page: KeyboardPage = KeyboardPage.LETTERS;
  @State candidates: string[] = [];
  @State shiftOn: boolean = false;
  @State chineseMode: boolean = true;
  @State composing: boolean = false;
  @State version: number = 0;
  @StorageProp('currentColorMode') colorMode: number = 1;
  private state: KeyboardState = new KeyboardState(InputBridge.sink);
  private screenWidthVp: number =
    px2vp(display.getDefaultDisplaySync().width);
  private pressedKeyLabel: string = '';

  private theme(): KeyboardTheme {
    // ConfigurationConstant.COLOR_MODE_DARK = 0
    return this.colorMode === 0 ? DARK_THEME : LIGHT_THEME;
  }

  private syncFromState(): void {
    this.page = this.state.page;
    this.candidates = this.state.candidates.slice();
    this.shiftOn = this.state.shiftOn;
    this.chineseMode = this.state.chineseMode;
    this.composing = this.state.isComposing();
    this.version++;
  }

  private handleTap(frame: KeyFrame): void {
    this.state.tap(frame);
    this.pressedKeyLabel = '';
    this.syncFromState();
  }

  private keyBackground(frame: KeyFrame, theme: KeyboardTheme): string {
    if (frame.kind === KeyKind.ACTION && !this.composing) {
      return theme.accent;
    }
    if (frame.kind === KeyKind.SHIFT && this.shiftOn) {
      return theme.key;
    }
    return frame.isSpecial ? theme.specialKey : theme.key;
  }

  private keyFontSize(frame: KeyFrame): number {
    if (frame.kind === KeyKind.SPACE || frame.kind === KeyKind.TO_SYMBOLS1 ||
        frame.kind === KeyKind.TO_SYMBOLS2 || frame.kind === KeyKind.TO_LETTERS ||
        frame.kind === KeyKind.ACTION) {
      return KeyMetrics.CANDIDATE_FONT_SIZE;
    }
    return frame.isSpecial ? KeyMetrics.SPECIAL_FONT_SIZE
                           : KeyMetrics.LETTER_FONT_SIZE;
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      // Top bar: toolbar when idle, candidate bar while composing.
      if (this.composing) {
        Row() {
          if (!this.state.engineReady) {
            Text('词库加载失败，仅支持字母直输')
              .fontSize(13)
              .fontColor(this.theme().text)
              .opacity(0.6)
          }
          Scroll() {
            Row() {
              ForEach(this.candidates, (candidate: string, index: number) => {
                if (index === 0) {
                  Text(candidate)
                    .fontSize(KeyMetrics.CANDIDATE_FONT_SIZE)
                    .fontColor(this.theme().accent)
                    .padding({ left: 12, right: 12 })
                    .height(36)
                    .borderRadius(6)
                    .backgroundColor(this.theme().key)
                    .margin({ left: 6, right: 6 })
                    .onClick(() => {
                      this.state.selectCandidate(candidate);
                      this.syncFromState();
                    })
                } else {
                  Text(candidate)
                    .fontSize(KeyMetrics.CANDIDATE_FONT_SIZE)
                    .fontColor(this.theme().text)
                    .padding({ left: KeyMetrics.CANDIDATE_PADDING,
                               right: KeyMetrics.CANDIDATE_PADDING })
                    .height(KeyMetrics.TOP_BAR_HEIGHT)
                    .onClick(() => {
                      this.state.selectCandidate(candidate);
                      this.syncFromState();
                    })
                }
              }, (candidate: string, index: number) =>
                `${this.version}:${index}:${candidate}`)
            }
            .height(KeyMetrics.TOP_BAR_HEIGHT)
          }
          .scrollable(ScrollDirection.Horizontal)
          .scrollBar(BarState.Off)
          .layoutWeight(1)
        }
        .width('100%')
        .height(KeyMetrics.TOP_BAR_HEIGHT)
        .position({ x: 0, y: 0 })
      } else {
        Row() {
          Stack() {
            Circle()
              .width(KeyMetrics.TOOLBAR_BUTTON_SIZE)
              .height(KeyMetrics.TOOLBAR_BUTTON_SIZE)
              .fill(this.theme().key)
            Text('D')
              .fontSize(18)
              .fontWeight(FontWeight.Black)
              .fontColor(this.theme().accent)
          }
          .width(KeyMetrics.TOOLBAR_BUTTON_SIZE)
          .height(KeyMetrics.TOOLBAR_BUTTON_SIZE)

          Blank()

          Text('⌨')
            .fontSize(16)
            .fontColor(this.theme().text)
            .textAlign(TextAlign.Center)
            .width(KeyMetrics.TOOLBAR_BUTTON_SIZE)
            .height(KeyMetrics.TOOLBAR_BUTTON_SIZE)
            .borderRadius(KeyMetrics.TOOLBAR_BUTTON_SIZE / 2)
            .backgroundColor(this.theme().key)
            .onClick(() => {
              this.state.switchKeyboard();
              this.syncFromState();
            })

          Text('∨')
            .fontSize(16)
            .fontColor(this.theme().text)
            .textAlign(TextAlign.Center)
            .width(KeyMetrics.TOOLBAR_BUTTON_SIZE)
            .height(KeyMetrics.TOOLBAR_BUTTON_SIZE)
            .borderRadius(KeyMetrics.TOOLBAR_BUTTON_SIZE / 2)
            .backgroundColor(this.theme().key)
            .margin({ left: 10 })
            .onClick(() => {
              this.state.hideKeyboard();
              this.syncFromState();
            })
        }
        .width('100%')
        .height(KeyMetrics.TOP_BAR_HEIGHT)
        .padding({ left: 8, right: 8 })
        .position({ x: 0, y: 0 })
      }

      // Keys.
      ForEach(KeyLayout.compute(this.screenWidthVp, this.page),
        (frame: KeyFrame) => {
          Stack() {
            if (frame.kind === KeyKind.COMMA) {
              Column() {
                Text(this.chineseMode ? '。' : '.')
                  .fontSize(9)
                  .fontColor(this.theme().text)
                Text(this.chineseMode ? '，' : ',')
                  .fontSize(18)
                  .fontColor(this.theme().text)
              }
            } else if (frame.kind === KeyKind.LANG_TOGGLE) {
              Column() {
                Text('中')
                  .fontSize(13)
                  .fontColor(this.theme().text)
                  .opacity(this.chineseMode ? 1 : 0.4)
                Text('英')
                  .fontSize(13)
                  .fontColor(this.theme().text)
                  .opacity(this.chineseMode ? 0.4 : 1)
              }
              .margin({ top: -4 })
            } else if (frame.kind === KeyKind.ACTION) {
              Text(this.composing ? '确定' : this.state.actionLabel())
                .fontSize(KeyMetrics.CANDIDATE_FONT_SIZE)
                .fontColor(this.composing ? this.theme().text
                                          : this.theme().textOnAccent)
            } else {
              Text(frame.label)
                .fontSize(this.keyFontSize(frame))
                .fontColor(this.theme().text)
            }
          }
          .width(frame.width)
          .height(frame.height)
          .borderRadius(KeyMetrics.CORNER_RADIUS)
          .backgroundColor(this.keyBackground(frame, this.theme()))
          .position({ x: frame.x, y: frame.y })
          .onTouch((event: TouchEvent) => {
            if (event.type === TouchType.Down) {
              this.pressedKeyLabel = frame.label + frame.x.toString();
            } else if (event.type === TouchType.Up) {
              this.handleTap(frame);
            }
          })
          .opacity(this.pressedKeyLabel === frame.label + frame.x.toString()
            ? 0.85 : 1.0)
        },
        (frame: KeyFrame) =>
          `${this.page}:${frame.kind}:${frame.x.toFixed(1)}:${frame.y.toFixed(1)}`)
    }
    .width('100%')
    .height(KeyLayout.totalHeight())
    .backgroundColor(this.theme().background)
  }
}
