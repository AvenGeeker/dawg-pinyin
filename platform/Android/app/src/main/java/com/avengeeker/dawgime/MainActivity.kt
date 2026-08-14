// SPDX-License-Identifier: Apache-2.0
package com.avengeeker.dawgime

import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

/** Setup guide for enabling the DawgIme input method. */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface(Modifier.fillMaxSize()) {
                    SetupGuide()
                }
            }
        }
    }
}

@Composable
private fun SetupGuide() {
    val context = LocalContext.current
    val steps = listOf(
        "点击下方按钮打开系统输入法设置",
        "在「屏幕键盘」中启用 DawgIme（中文拼音）",
        "在任意输入框点击键盘图标切换到 DawgIme",
    )
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("启用 DawgIme 拼音键盘", style = MaterialTheme.typography.headlineSmall)
        steps.forEachIndexed { index, step ->
            Text("${index + 1}. $step", style = MaterialTheme.typography.bodyLarge)
        }
        Text(
            "键盘无需网络权限；词库随应用内置，离线可用。",
            style = MaterialTheme.typography.bodyMedium,
        )
        Button(
            onClick = {
                context.startActivity(
                    Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
            },
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("打开输入法设置")
        }
    }
}
