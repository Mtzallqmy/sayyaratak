package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import android.widget.LinearLayout

class SettingsUi(val act: Activity, val cb: Callbacks) {
    interface Callbacks {
        fun onQuality(q: Int)
        fun onAudio(on: Boolean)
        fun onDefaultCamera(cam: Int)
        fun onResetProgress()
        fun onCloseSettings()
    }

    lateinit var panel: LinearLayout
    private var quality = 1
    private var audio = true
    private var camera = 0

    fun build(root: FrameLayout) {
        val d = UiKit.dp(act, 1f)
        panel = UiKit.column(act)
        panel.background = UiKit.panelBg(act)
        panel.setPadding(28 * d, 20 * d, 28 * d, 22 * d)

        val title = UiKit.label(act, "الإعدادات", 24f, UiKit.GOLD)
        title.typeface = UiKit.fontBold
        panel.addView(title)
        addSpacer(panel, 12)

        panel.addView(UiKit.label(act, "جودة الرسوم", 14f, Color.argb(220, 255, 255, 255)))
        val qr = UiKit.row(act)
        val qNames = listOf("منخفضة", "متوسطة", "عالية")
        for (i in 0..2) {
            val b = UiKit.btn(act, qNames[i], 13f)
            b.setOnClickListener { quality = i; cb.onQuality(i) }
            qr.addView(b, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 42f)).also { it.weight = 1f }, 3 * d, 4 * d, 3 * d, 0))
        }
        panel.addView(qr, UiKit.vlp(-1, -2))
        addSpacer(panel, 10)

        val audioBtn = UiKit.btn(act, "الصوت: يعمل", 14f)
        audioBtn.setOnClickListener {
            audio = !audio
            audioBtn.text = "الصوت: " + if (audio) "يعمل" else "متوقف"
            cb.onAudio(audio)
        }
        panel.addView(audioBtn, UiKit.margins(UiKit.vlp(-1, UiKit.dp(act, 44f)), 0, 4 * d, 0, 4 * d))

        panel.addView(UiKit.label(act, "الكاميرا الافتراضية", 14f, Color.argb(220, 255, 255, 255)))
        val cr = UiKit.row(act)
        val cNames = listOf("خلفية", "قريبة", "علوية")
        for (i in 0..2) {
            val b = UiKit.btn(act, cNames[i], 13f)
            b.setOnClickListener { camera = i; cb.onDefaultCamera(i) }
            cr.addView(b, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 42f)).also { it.weight = 1f }, 3 * d, 4 * d, 3 * d, 0))
        }
        panel.addView(cr, UiKit.vlp(-1, -2))
        addSpacer(panel, 10)

        val reset = UiKit.btn(act, "مسح التقدم المحفوظ", 13f)
        reset.setOnClickListener { cb.onResetProgress() }
        panel.addView(reset, UiKit.margins(UiKit.vlp(-1, UiKit.dp(act, 42f)), 0, 4 * d, 0, 4 * d))

        val close = UiKit.btn(act, "رجوع", 15f, true)
        close.setOnClickListener { cb.onCloseSettings() }
        panel.addView(close, UiKit.margins(UiKit.vlp(UiKit.dp(act, 180f), UiKit.dp(act, 46f)), 0, 10 * d, 0, 0))

        panel.visibility = View.GONE
        root.addView(panel, UiKit.lp(-2, -2, Gravity.CENTER))
    }

    fun sync(q: Int, a: Boolean, c: Int) { quality = q; audio = a; camera = c }

    private fun addSpacer(l: LinearLayout, dpv: Int) {
        val v = View(act)
        l.addView(v, UiKit.vlp(1, UiKit.dp(act, dpv.toFloat())))
    }
}
