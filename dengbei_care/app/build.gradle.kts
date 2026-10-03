plugins {
    alias(libs.plugins.androidApplication)
    alias(libs.plugins.jetbrainsKotlinAndroid)
}

android {
    namespace = "com.jinyuni.dengbei_care"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.jinyuni.dengbei_care"
        minSdk = 26
        targetSdk = 34
        versionCode = 8
        versionName = "1.7"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }
    buildFeatures {
        viewBinding = true
    }
    packaging {
        resources {
            excludes += listOf(
                "META-INF/DEPENDENCIES",
                "META-INF/LICENSE",
                "META-INF/LICENSE.txt",
                "META-INF/license.txt",
                "META-INF/NOTICE",
                "META-INF/NOTICE.txt",
                "META-INF/notice.txt",
                "META-INF/ASL2.0"
            )
        }
    }
    testOptions {
        // 宿主机单测（:app:testDebugUnitTest）中加载含 android.util.Log 的云上传类时，
        // 让 android.jar 桩返回默认值而非抛 “not mocked”（AGP 标准做法，仅影响单元测试）。
        unitTests.isReturnDefaultValues = true
    }
}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.lifecycle.livedata.ktx)
    implementation(libs.androidx.lifecycle.viewmodel.ktx)
    implementation(libs.androidx.navigation.fragment.ktx)
    implementation(libs.androidx.navigation.ui.ktx)
    implementation(libs.androidx.legacy.support.v4)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    implementation(libs.mpandroidchart)
    implementation(libs.okhttp)
    implementation(libs.espressif)
    implementation(libs.localbroadcast)
    implementation("androidx.annotation:annotation:1.6.0")
    implementation(libs.work.runtime)
//    implementation(libs.paho.service)
    implementation(libs.retrofit )
    implementation(libs.convertergson)
    implementation(libs.lifecycle)
//    implementation(libs.paho.service)
    implementation("com.google.zxing:core:3.5.3")
    implementation(libs.zxingandroidembedded)
    implementation(libs.paho.mqtt){
        exclude( group = "com.google.zxing", module = "core")
    }
}