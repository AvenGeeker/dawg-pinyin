import org.apache.tools.ant.taskdefs.condition.Os

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
}

// Repo root (platform/Android -> repo). Production IME assets live outside
// the git tree; when absent we fall back to the tiny testdata assets so any
// fresh clone still builds.
val repoRoot = rootDir.parentFile.parentFile
val prodAssets = repoRoot.resolve("assets")
val fallbackDawg = repoRoot.resolve("core/testdata/dict/tiny.dawg")
val fallbackClm = repoRoot.resolve("core/testdata/lm/tiny.clm")

val syncImeAssets by tasks.registering(Copy::class) {
    description = "Copies production (or tiny fallback) IME assets into app assets."
    into(layout.projectDirectory.dir("src/main/assets"))
    from(
        if (prodAssets.resolve("dawg_pinyin.dawg").exists()) {
            prodAssets.resolve("dawg_pinyin.dawg")
        } else {
            fallbackDawg
        },
    ) {
        rename { "dawg_pinyin.dawg" }
    }
    from(
        if (prodAssets.resolve("dawg_pinyin.clm").exists()) {
            prodAssets.resolve("dawg_pinyin.clm")
        } else {
            fallbackClm
        },
    ) {
        rename { "dawg_pinyin.clm" }
    }
}

android {
    namespace = "com.avengeeker.dawgime"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.avengeeker.dawgime"
        minSdk = 21
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
            }
        }
    }

    // The dawg core is consumed through an app-local CMake project that
    // add_subdirectory()s ../../../../core, keeping JNI glue out of core.
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        compose = true
    }
}

tasks.named("preBuild") {
    dependsOn(syncImeAssets)
}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.ui)
    implementation(libs.androidx.ui.graphics)
    implementation(libs.androidx.ui.tooling.preview)
    implementation(libs.androidx.material3)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.ui.test.junit4)
    debugImplementation(libs.androidx.ui.tooling)
    debugImplementation(libs.androidx.ui.test.manifest)
}
