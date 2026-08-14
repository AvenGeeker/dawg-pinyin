// SPDX-License-Identifier: Apache-2.0
import InputMethodExtensionAbility from '@ohos.InputMethodExtensionAbility';
import inputMethodEngine from '@ohos.inputMethodEngine';
import fs from '@ohos.file.fs';
import type Want from '@ohos.app.ability.Want';
import { DawgEngineBridge } from '../keyboard/DawgEngine';
import { InputBridge } from '../keyboard/InputBridge';

const DAWG_ASSET = 'dawg_pinyin.dawg';
const CLM_ASSET = 'dawg_pinyin.clm';
const TAG = 'DawgIME';

/**
 * Input method extension: materializes the mmap assets, boots the engine
 * and shows the keyboard panel.
 *
 * NOTE: Panel/KeyboardController panel management below uses the input
 * method panel API surface (Panel + kbController show/hide). The local
 * DevEco 3.1 SDK is API 9 which predates `Panel`; compile this module with
 * an API 10+ SDK (see platform/HarmonyOS/README.md).
 */
export default class DawgIME extends InputMethodExtensionAbility {
  private panel: inputMethodEngine.Panel | null = null;

  onCreate(want: Want): void {
    console.info(`${TAG} onCreate`);
    this.copyAsset(DAWG_ASSET);
    this.copyAsset(CLM_ASSET);
    DawgEngineBridge.init(this.context.filesDir);
    const ability = inputMethodEngine.getInputMethodAbility();
    ability.on('inputStart',
      (kbController: inputMethodEngine.KeyboardController,
       inputClient: inputMethodEngine.InputClient): void => {
        InputBridge.setClient(inputClient);
        if (this.panel === null) {
          this.panel = new inputMethodEngine.Panel('pages/KeyboardPanel', {
            type: inputMethodEngine.TYPE_PANEL,
            flag: inputMethodEngine.FLAG_FULLSCREEN,
          });
          kbController.on('show', (): void => {
            if (this.panel !== null) {
              this.panel.show();
            }
          });
          kbController.on('hide', (): void => {
            if (this.panel !== null) {
              this.panel.hide();
            }
          });
        }
        if (this.panel !== null) {
          this.panel.show();
        }
      });
  }

  onDestroy(): void {
    DawgEngineBridge.destroy();
  }

  /** rawfile -> filesDir so the engine can mmap real file paths. */
  private copyAsset(name: string): void {
    const dest = `${this.context.filesDir}/${name}`;
    if (fs.accessSync(dest)) {
      return;
    }
    try {
      const data = this.context.resourceManager.getRawFileContentSync(name);
      const file = fs.openSync(
        dest,
        fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      fs.writeSync(file.fd, data.buffer);
      fs.closeSync(file);
    } catch (err) {
      console.error(`${TAG} copyAsset ${name} failed: ${JSON.stringify(err)}`);
    }
  }
}
