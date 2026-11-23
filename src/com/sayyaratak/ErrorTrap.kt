package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.io.File
import java.io.PrintWriter
import java.io.StringWriter

object ErrorTrap {
    private var activity: Activity? = null
    private val ui = Handler(Looper.getMainLooper())

    fun install(a: Activity) {
        activity = a
        val prev = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { _, e ->
            try {
                val trace = stackOf(e)
                File(a.filesDir, "crash.txt").writeText(trace)
                ui.post { show(a, "تعذّر التشغيل", trace) }
                Thread.sleep(60000)
            } catch (_: Throwable) {}
            prev?.uncaughtException(Thread.currentThread(), e)
            android.os.Process.killProcess(android.os.Process.myPid())
        }
    }

    fun stackOf(e: Throwable): String {
        val sw = StringWriter()
        e.printStackTrace(PrintWriter(sw))
        return e.javaClass.simpleName + ": " + (e.message ?: "") + "\n\n" + sw.toString().take(3500)
    }

    fun crashFile(a: Activity): File = File(a.filesDir, "crash.txt")

    fun showPreviousIfAny(a: Activity, root: FrameLayout): Boolean {
        val f = crashFile(a)
        if (!f.exists()) return false
        val txt = f.readText()
        show(a, "تقرير خطأ سابق (صوّر الشاشة وأرسلها)", txt)
        f.delete()
        return true
    }

    fun show(a: Activity, title: String, trace: String) {
        try {
            val root = a.window.decorView.findViewById<FrameLayout>(android.R.id.content)
            val d = UiKit.dp(a, 1f)
            val wrap = LinearLayout(a)
            wrap.orientation = LinearLayout.VERTICAL
            wrap.setBackgroundColor(Color.argb(245, 12, 14, 22))
            wrap.setPadding(22 * d, 22 * d, 22 * d, 22 * d)

            val t = UiKit.label(a, title, 20f, UiKit.GOLD)
            t.typeface = UiKit.fontBold
            wrap.addView(t)
            val hint = UiKit.label(a, "صوّر هذه الشاشة وأرسلها للمطوّر لإصلاح الخطأ", 13f, Color.argb(220, 255, 255, 255))
            hint.setPadding(0, 6 * d, 0, 10 * d)
            wrap.addView(hint)

            val sv = ScrollView(a)
            val tv = TextView(a)
            tv.text = trace
            tv.textSize = 12f
            tv.setTextColor(Color.rgb(255, 210, 200))
            tv.typeface = android.graphics.Typeface.MONOSPACE
            tv.setTextIsSelectable(true)
            sv.addView(tv)
            wrap.addView(sv, UiKit.vlp(-1, 0).also { it.weight = 1f })

            val row = UiKit.row(a)
            val retry = UiKit.btn(a, "إعادة المحاولة", 14f, true)
            retry.setOnClickListener {
                crashFile(a).delete()
                a.recreate()
            }
            val exit = UiKit.btn(a, "خروج", 14f)
            exit.setOnClickListener { a.finish() }
            row.addView(retry, UiKit.margins(UiKit.vlp(0, UiKit.dp(a, 46f)).also { it.weight = 1f }, 4 * d, 8 * d, 4 * d, 0))
            row.addView(exit, UiKit.margins(UiKit.vlp(0, UiKit.dp(a, 46f)).also { it.weight = 1f }, 4 * d, 8 * d, 4 * d, 0))
            wrap.addView(row, UiKit.vlp(-1, -2))

            root.addView(wrap, FrameLayout.LayoutParams(-1, -1))
        } catch (_: Throwable) {}
    }
}
