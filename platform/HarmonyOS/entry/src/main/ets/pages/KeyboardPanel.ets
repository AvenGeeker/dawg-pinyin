// SPDX-License-Identifier: Apache-2.0
import display from '@ohos.display';
import { KeyLayout, KeyMetrics, KeyFrame, KeyKind, KeyboardPage }
  from '../keyboard/KeySpec';
import { KeyboardState } from '../keyboard/KeyboardState';
import { InputBridge } from '../keyboard/InputBridge';

/** Exact palette from the shared three-platform keyboard spec. */
class KeyboardTheme {
  background: string;
  key: string;
  specialKey: string;
  pressed: string;
  text: string;
  separator: string;

  constructor(background: string, key: string, specialKey: string,
              pressed: string, text: string, separator: string) {
    this.background = background;
    this.key = key;
    this.specialKey = specialKey;
    this.pressed = pressed;
    this.text = text;
    this.separator = separator;
  }
}

const LIGHT_THEME = new KeyboardTheme('#D1D5DB', '#FFFFFF', '#ABB2BC',
  '#00000019', '#000000', '#668A8F98');
const DARK_THEME = new KeyboardTheme('#1C1C1E', '#494B53', '#2F3238',
  '#00000019', '#FFFFFF', '#4D8A8F98');

/**
 * Keyboard panel page hosted by the DawgIME Panel. Renders the shared
 * layout spec; the state machine is KeyboardState (three-platform parity).
 */
@Entry
@Component
struct KeyboardPanel {
  @State page: KeyboardPage = KeyboardPage.LETTERS;
  @State candidates: string[] = [];
  @State shiftOn: boolean = false;
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
    this.version++;
  }

  private handleTap(frame: KeyFrame): void {
    this.state.tap(frame);
    this.pressedKeyLabel = '';
    this.syncFromState();
  }

  private keyBackground(frame: KeyFrame, theme: KeyboardTheme): string {
    if (frame.kind === KeyKind.SHIFT && this.shiftOn) {
      return theme.key;
    }
    return frame.isSpecial ? theme.specialKey : theme.key;
  }

  private keyFontSize(frame: KeyFrame): number {
    if (frame.kind === KeyKind.SPACE || frame.kind === KeyKind.TO_SYMBOLS1 ||
        frame.kind === KeyKind.TO_SYMBOLS2 || frame.kind === KeyKind.TO_LETTERS) {
      return KeyMetrics.CANDIDATE_FONT_SIZE;
    }
    return frame.isSpecial ? KeyMetrics.SPECIAL_FONT_SIZE
                           : KeyMetrics.LETTER_FONT_SIZE;
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      // Candidate bar.
      Row() {
        if (this.state.engineReady) {
          Scroll() {
            Row() {
              ForEach(this.candidates, (candidate: string, index: number) => {
                Text(candidate)
                  .fontSize(KeyMetrics.CANDIDATE_FONT_SIZE)
                  .fontColor(this.theme().text)
                  .padding({ left: KeyMetrics.CANDIDATE_PADDING,
                             right: KeyMetrics.CANDIDATE_PADDING })
                  .height(KeyMetrics.CANDIDATE_BAR_HEIGHT)
                  .onClick(() => {
                    this.state.selectCandidate(candidate);
                    this.syncFromState();
                  })
                if (index < this.candidates.length - 1) {
                  Divider()
                    .vertical(true)
                    .height(KeyMetrics.SEPARATOR_HEIGHT)
                    .color(this.theme().separator)
                }
              }, (candidate: string, index: number) => `${this.version}:${index}:${candidate}`)
            }
            .height(KeyMetrics.CANDIDATE_BAR_HEIGHT)
          }
          .scrollable(ScrollDirection.Horizontal)
          .scrollBar(BarState.Off)
          .layoutWeight(1)
        } else {
          Text('词库加载失败，仅支持字母直输')
            .fontSize(13)
            .fontColor(this.theme().text)
            .opacity(0.6)
        }
      }
      .width('100%')
      .height(KeyMetrics.CANDIDATE_BAR_HEIGHT)
      .position({ x: 0, y: 0 })

      // Keys.
      ForEach(KeyLayout.compute(this.screenWidthVp, this.page),
        (frame: KeyFrame) => {
          Text(frame.label)
            .fontSize(this.keyFontSize(frame))
            .fontColor(this.theme().text)
            .textAlign(TextAlign.Center)
            .width(frame.width)
            .height(frame.height)
            .borderRadius(KeyMetrics.CORNER_RADIUS)
            .backgroundColor(this.keyBackground(frame, this.theme()))
            .position({ x: frame.x, y: frame.y })
            .onTouch((event: TouchEvent) => {
              if (event.type === TouchType.Down) {
                this.pressedKeyLabel = frame.label;
              } else if (event.type === TouchType.Up) {
                this.handleTap(frame);
              }
            })
            .opacity(this.pressedKeyLabel === frame.label ? 0.85 : 1.0)
        },
        (frame: KeyFrame) =>
          `${this.page}:${frame.label}:${frame.x.toFixed(1)}:${frame.y.toFixed(1)}`)
    }
    .width('100%')
    .height(KeyLayout.totalHeight())
    .backgroundColor(this.theme().background)
  }
}
