package com.jinyuni.dengbei_care.ui.zhuce

import android.os.Bundle
import android.webkit.WebViewClient
import androidx.appcompat.app.AppCompatActivity
import androidx.navigation.ui.AppBarConfiguration
import com.jinyuni.dengbei_care.databinding.ActivityTermsOfServiceBinding

class TermsOfServiceActivity : AppCompatActivity() {

    private lateinit var appBarConfiguration: AppBarConfiguration
    private lateinit var binding: ActivityTermsOfServiceBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityTermsOfServiceBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val webView = binding.termsOfServiceWebView

        webView.settings.javaScriptEnabled = true
        webView.webViewClient = WebViewClient()
        webView.loadUrl("file:///android_asset/terms_of_service.html")
    }

}