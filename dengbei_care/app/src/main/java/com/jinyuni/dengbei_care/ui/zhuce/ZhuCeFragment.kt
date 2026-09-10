package com.jinyuni.dengbei_care.ui.zhuce

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.CountDownTimer
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.MenuHost
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.navigation.fragment.findNavController
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.databinding.FragmentZhuceBinding
import java.util.concurrent.TimeUnit

class ZhuCeFragment : Fragment(){
    private var _binding: FragmentZhuceBinding? = null
    private val binding get() = _binding!!
    private var countDownTimer: CountDownTimer? = null
    private var isCountDownStarted = false
    private val viewModel: ZhuCeViewModel by viewModels()
    private lateinit var denglucheck: CheckBox
    private lateinit var zhucecheck: CheckBox
    private var clickCount = 0 // 点击次数


    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {  // 修改这里
        _binding = FragmentZhuceBinding.inflate(inflater, container, false)
        (activity as AppCompatActivity).supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setupMenu()

        val privacyPolicyTextView = binding.privacyPolicyTextView


        val termsOfServiceTextView = binding.termsOfServiceTextView



        privacyPolicyTextView.setOnClickListener {
            val intent = Intent(requireContext(), PrivacyPolicyActivity::class.java)
            startActivity(intent)
        }

        termsOfServiceTextView.setOnClickListener {
            val intent = Intent(requireContext(), TermsOfServiceActivity::class.java)
            startActivity(intent)
        }

        denglucheck = binding.checkBox
        zhucecheck = binding.checkBox2



        denglucheck.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                zhucecheck.isChecked = false
                binding.btnGetVerificationCode.isEnabled = false
                binding.etVerificationCode.isEnabled = false
                binding.btnRegister.isEnabled = false
                binding.etPasswordConfirm.isEnabled = false

                binding.button.isEnabled = true
                binding.button2.isEnabled = true
                binding.button3.isEnabled = true
                binding.button4.isEnabled = true
            }
        }

        zhucecheck.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                denglucheck.isChecked = false

                binding.btnGetVerificationCode.isEnabled = true
                binding.etVerificationCode.isEnabled = true
                binding.btnRegister.isEnabled = true
                binding.etPasswordConfirm.isEnabled = true

                binding.button.isEnabled = false
                binding.button2.isEnabled = false
                binding.button3.isEnabled = false
                binding.button4.isEnabled = false
            }
        }

        binding.button2.setOnClickListener{
            Toast.makeText(requireContext(), "忘记或者修改密码，请用相同手机号重新注册", Toast.LENGTH_SHORT).show()
        }

        binding.button5.setOnClickListener{
            Toast.makeText(requireContext(), "请联系:snack_yx@126.com", Toast.LENGTH_LONG).show()
        }

        denglucheck.isChecked = true

        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnGetVerificationCode.setOnClickListener {
            val phoneNumber = binding.etPhone.text.toString()

            // 验证手机号格式
            if (!isValidPhoneNumber(phoneNumber)) {
                Toast.makeText(context, "请输入正确的手机号", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            // 获取验证码间隔
            if (isCountDownStarted) {
                return@setOnClickListener
            }

            // 从云平台获取验证码
            fetchVerificationCode(phoneNumber)

            // 开始倒计时
//            startCountDown()
        }
        binding.btnRegister.setOnClickListener {
//            val username = binding.etUsername.text.toString()
            val phoneNumber = binding.etPhone.text.toString()
            val verificationCode = binding.etVerificationCode.text.toString()
            val password = binding.etPassword.text.toString()
            val passwordConfirm = binding.etPasswordConfirm.text.toString()

            // 验证用户名、手机号、验证码、密码和确认密码
//            if (!isValidUsername(username)) {
//                Toast.makeText(context, "用户名格式不正确", Toast.LENGTH_SHORT).show()
//                return@setOnClickListener
//            }
            if (!isValidPhoneNumber(phoneNumber)) {
                Toast.makeText(context, "请输入正确的手机号", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (!isValidVerificationCode(verificationCode)) {
                Toast.makeText(context, "请输入有效的验证码", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (!isValidPassword(password)) {
                Toast.makeText(context, "密码格式不正确,6-16个字母或符号", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (!isValidPasswordConfirm(password, passwordConfirm)) {
                Toast.makeText(context, "两次输入的密码不一致", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            // 注册用户
            registerUser( phoneNumber, verificationCode, password)
        }


        binding.button.setOnClickListener {
//            val username = binding.etUsername.text.toString()
            val phoneNumber = binding.etPhone.text.toString()
//            val verificationCode = binding.etVerificationCode.text.toString()
            val password = binding.etPassword.text.toString()
//            val passwordConfirm = binding.etPasswordConfirm.text.toString()

            // 验证用户名、手机号、验证码、密码和确认密码
//            if (!isValidUsername(username)) {
//                Toast.makeText(context, "用户名格式不正确", Toast.LENGTH_SHORT).show()
//                return@setOnClickListener
//            }
            if (!isValidPhoneNumber(phoneNumber)) {
                Toast.makeText(context, "请输入正确的手机号", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
//            if (!isValidVerificationCode(verificationCode)) {
//                Toast.makeText(context, "请输入有效的验证码", Toast.LENGTH_SHORT).show()
//                return@setOnClickListener
//            }
            if (!isValidPassword(password)) {
                Toast.makeText(context, "密码格式不正确,6-16个字母或符号", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
//            if (!isValidPasswordConfirm(password, passwordConfirm)) {
//                Toast.makeText(context, "两次输入的密码不一致", Toast.LENGTH_SHORT).show()
//                return@setOnClickListener
//            }

            val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
            val registerstatee = prefs.getString("registerphone", "")
            if ((registerstatee != "")&&(registerstatee != null)) {
                Toast.makeText(context, "您已经登录了，请勿再次登录", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            var loginCount = prefs.getInt("LOGIN_CNT", 0) // 获取当前的登录次数，默认为 0
            loginCount++ // 递增登录次数
            prefs.edit().putInt("LOGIN_CNT", loginCount).apply()
            if (loginCount > 30) {
                Toast.makeText(context, "您登录/退出次数太多，需要重新安装软件", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            // 登录
            loginserver( phoneNumber, password)
        }

        binding.button4.setOnClickListener{
            logoutserver()
        }

        binding.button3.setOnClickListener{
            clickCount++
            if(clickCount > 3) {
                val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
                val registerstatee = prefs.getString("registerphone", "")
                if ((registerstatee != "") && (registerstatee != null)) {
                    deleteUser(registerstatee)
                } else {
                    Toast.makeText(context, "注销前请登录", Toast.LENGTH_SHORT).show()
                }
            }else{
                Toast.makeText(context, "请点击3次注销", Toast.LENGTH_SHORT).show()
            }
        }

    }

    // 开始倒计时
    private fun startCountDown() {
        isCountDownStarted = true
        binding.btnGetVerificationCode.isEnabled = false
        countDownTimer = object : CountDownTimer(60 * 1000, 1000) {
            override fun onTick(millisUntilFinished: Long) {
                val seconds = TimeUnit.MILLISECONDS.toSeconds(millisUntilFinished)
                binding.btnGetVerificationCode.text = getString(R.string.send_again_text, seconds)
            }

            override fun onFinish() {
                isCountDownStarted = false
                binding.btnGetVerificationCode.isEnabled = true
                binding.btnGetVerificationCode.text = "获取验证码"
            }
        }.start()
    }

    // 从云平台获取验证码
    private fun fetchVerificationCode(phoneNumber: String) {
        // 这里需要调用你的云平台 API 获取验证码，例如使用 Retrofit 或 Volley 等网络请求库
        // ...
        // 如果获取验证码成功，可以显示提示信息
//        Toast.makeText(context, "验证码已发送", Toast.LENGTH_SHORT).show()

//        lifecycleScope.launch { // 或 lifecycleScope.launch {  如果你在 Fragment 中
//            try {
//                val response = viewModel.sendVerificationCode(phoneNumber)
//                if (response.isSuccessful) {
//                    // 验证码发送成功
//                    Toast.makeText(context, "验证码已发送", Toast.LENGTH_SHORT).show()
//                } else {
//                    // 验证码发送失败
//                    val errorMessage = response.errorBody()?.string() ?: "验证码发送失败"
//                    Toast.makeText(context, errorMessage, Toast.LENGTH_SHORT).show()
//                }
//            } catch (e: Exception) {
//                // 网络错误或其他异常
//                Toast.makeText(context, "网络错误", Toast.LENGTH_SHORT).show()
//                Log.e("Verification Code Error", "Error fetching verification code", e) // 打印错误日志
//            }

        viewModel.sendVerificationCode(phoneNumber).observe(viewLifecycleOwner) { success ->
            if (success) {
                Toast.makeText(context, "验证码已发送，有效期5分钟", Toast.LENGTH_SHORT).show()
                startCountDown() // 开始倒计时
            } else {
                // 处理错误，例如显示错误信息
                Toast.makeText(context, "验证码发送失败", Toast.LENGTH_SHORT).show()
            }
        }
//        }
    }


    private fun loginserver(phoneNumber: String,password: String) {
        // 这里需要调用你的云平台 API 获取验证码，例如使用 Retrofit 或 Volley 等网络请求库
        // ...
        // 如果获取验证码成功，可以显示提示信息

        viewModel.login(phoneNumber,password).observe(viewLifecycleOwner) { success ->
            if (success) {
                Toast.makeText(context, "登录成功", Toast.LENGTH_SHORT).show()
                requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                    .putString("registerphone", phoneNumber)
                    .apply()
                findNavController().navigate(R.id.action_zhuCeFragment_to_notificationsFragment)
            } else {
                // 处理错误，例如显示错误信息
                Toast.makeText(context, "登录失败，请检查手机号或密码是否正确", Toast.LENGTH_SHORT).show()
                requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                    .putString("registerphone", "")
                    .apply()
            }
        }
    }


    private fun logoutserver() {
        // 这里需要调用你的云平台 API 获取验证码，例如使用 Retrofit 或 Volley 等网络请求库
        // ...
        // 如果获取验证码成功，可以显示提示信息

        val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        val registerstatee = prefs.getString("registerphone", "")
        if ((registerstatee != "")&&(registerstatee != null)) {
            val editor = prefs.edit()
            editor.putString("registerphone", "")
            editor.apply()
            Toast.makeText(context, "用户已退出", Toast.LENGTH_SHORT).show()
        }else{
            Toast.makeText(context, "用户未登录，不用退出", Toast.LENGTH_SHORT).show()
        }
        val navController = findNavController()
        val currentDestination = navController.currentDestination?.id
        if (currentDestination == R.id.zhuCeFragment) {
            findNavController().navigate(R.id.action_zhuCeFragment_to_notificationsFragment)
        }
    }

    // 注册用户
    private fun registerUser(
        phoneNumber: String,
        verificationCode: String,
        password: String
    ) {
        // 这里需要调用你的云平台 API 注册用户，例如使用 Retrofit 或 Volley 等网络请求库
        // ...
        // 如果注册成功，可以跳转到登录界面或其他界面
        // ...
        // 如果注册失败，可以显示错误信息
        // ...
        viewModel.verifyCode(phoneNumber,verificationCode).observe(viewLifecycleOwner) { success ->
            if (success) {
                viewModel.register(phoneNumber,password).observe(viewLifecycleOwner) { success ->
                    if (success) {
                        Toast.makeText(context, "注册成功,已登录。", Toast.LENGTH_SHORT).show()
                        requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                            .putString("registerphone", phoneNumber)
                            .apply()
                        findNavController().navigate(R.id.action_zhuCeFragment_to_notificationsFragment)
                    } else {
                        // 处理错误，例如显示错误信息
                        Toast.makeText(context, "注册失败", Toast.LENGTH_SHORT).show()
                        requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                            .putString("registerphone", "")
                            .apply()
                    }
                }

            } else {
                // 处理错误，例如显示错误信息
                Toast.makeText(context, "验证码错误或者超时", Toast.LENGTH_SHORT).show()
                requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                    .putString("registerphone", "")
                    .apply()
            }
        }



    }


    //删除用户，这里是改了用户的密码，没有删掉账号
    private fun deleteUser(
        phoneNumber: String
    ) {
        // 这里需要调用你的云平台 API 注册用户，例如使用 Retrofit 或 Volley 等网络请求库
        // ...
        // 如果注册成功，可以跳转到登录界面或其他界面
        // ...
        // 如果注册失败，可以显示错误信息
        // ...
        viewModel.register(phoneNumber,"jiEwql!@:zA>").observe(viewLifecycleOwner) { success ->
            if (success) {
                Toast.makeText(context, "注销成功", Toast.LENGTH_SHORT).show()
                requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                    .putString("registerphone", "")
                    .apply()
                val navController = findNavController()
                val currentDestination = navController.currentDestination?.id
                if (currentDestination == R.id.zhuCeFragment) {
                    findNavController().navigate(R.id.action_zhuCeFragment_to_notificationsFragment)
                }
            } else {
                // 处理错误，例如显示错误信息
                Toast.makeText(context, "注销失败,请联系管理员", Toast.LENGTH_SHORT).show()
                requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                    .putString("registerphone", "")
                    .apply()
            }
        }

    }


    // 验证用户名是否合法
//    private fun isValidUsername(username: String): Boolean {
//        return username.length in 4..16 && username.matches(Regex("[a-zA-Z0-9_]{4,16}"))
//    }

    // 验证手机号是否合法
    private fun isValidPhoneNumber(phoneNumber: String): Boolean {
        return phoneNumber.matches(Regex("^1[3-9]\\d{9}$"))
    }

    // 验证验证码是否合法
    private fun isValidVerificationCode(verificationCode: String): Boolean {
        // 这里需要根据你的验证码规则进行验证
        return verificationCode.length == 6 && verificationCode.matches(Regex("[0-9]{6}"))
    }

    // 验证密码是否合法
    private fun isValidPassword(password: String): Boolean {
        return password.length in 6..16 && password.matches(Regex("[a-zA-Z0-9!@#$%^&*()_+,./<>?;':\"{}|\\[\\]\\-=`~]{6,16}"))
    }

    // 验证确认密码是否与密码一致
    private fun isValidPasswordConfirm(password: String, passwordConfirm: String): Boolean {
        return password == passwordConfirm
    }


    private fun setupMenu() {
        val menuHost: MenuHost = requireActivity()
        menuHost.addMenuProvider(object : MenuProvider {
            override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {
                // No need to inflate menu for now
            }

            override fun onMenuItemSelected(menuItem: MenuItem): Boolean {
                return when (menuItem.itemId) {
                    android.R.id.home -> {
                        findNavController().navigateUp()
                        true
                    }
                    else -> false
                }
            }
        }, viewLifecycleOwner, Lifecycle.State.RESUMED)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
        countDownTimer?.cancel()
        countDownTimer = null
        isCountDownStarted = false
    }
}