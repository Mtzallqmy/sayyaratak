package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView

class MenuUi(val act: Activity, val cb: Callbacks) {
    interface Callbacks {
        fun onStartDrive(district: Int)
        fun onStartChallenge()
        fun onOpenMissions()
        fun onOpenWorkshop()
        fun onOpenSettings()
    }

    lateinit var panel: LinearLayout
    lateinit var bestText: TextView
    lateinit var statsText: TextView

    fun build(root: android.widget.FrameLayout) {
        val d = UiKit.dp(act, 1f)
        panel = UiKit.column(act)
        panel.background = UiKit.panelBg(act)
        panel.layoutParams = android.widget.FrameLayout.LayoutParams(-1, -2)
        panel.setPadding(34 * d, 26 * d, 34 * d, 26 * d)

        val title = UiKit.label(act, "سيارتك", 46f, UiKit.GOLD)
        title.typeface = UiKit.fontBold
        title.setShadowLayer(10f, 0f, 3f, Color.argb(160, 0, 0, 0))
        val sub = UiKit.label(act, "ابنِ سيارتك… وقُدها في مدن اليمن", 15f, Color.argb(220, 255, 255, 255))
        sub.setPadding(0, 0, 0, 16 * d)
        panel.addView(title)
        panel.addView(sub)
        val dev = UiKit.label(act, "برمجة وتطوير معتز العلقمي – تعز – اليمن – 2026م", 11f, Color.argb(120, 255, 255, 255))
        dev.setPadding(0, 0, 0, 10 * d)
        panel.addView(dev)
        val cred = UiKit.label(act, "مبنية على مشاريع مفتوحة المصدر (MIT/CC0)", 9f, Color.argb(100, 255, 255, 255))
        cred.setPadding(0, 0, 0, 10 * d)
        panel.addView(cred)

        bestText = UiKit.label(act, "", 13f, Color.argb(190, 255, 230, 160))
        panel.addView(bestText)
        statsText = UiKit.label(act, "", 13f, Color.argb(220, 170, 220, 255))
        statsText.setPadding(0, 0, 0, 12 * d)
        panel.addView(statsText)

        val b1 = UiKit.btn(act, "قيادة حرة", 17f, true)
        val b2 = UiKit.btn(act, "تحدي السرعة", 17f)
        val b5 = UiKit.btn(act, "المهام", 17f)
        val b3 = UiKit.btn(act, "الورشة", 17f)
        val b4 = UiKit.btn(act, "الإعدادات", 17f)
        b5.setOnClickListener { cb.onOpenMissions() }
        b1.setOnClickListener { cb.onStartDrive(-1) }
        b2.setOnClickListener { cb.onStartChallenge() }
        b3.setOnClickListener { cb.onOpenWorkshop() }
        b4.setOnClickListener { cb.onOpenSettings() }
        for (b in listOf(b1, b2, b5, b3, b4)) {
            panel.addView(b, UiKit.margins(UiKit.vlp(UiKit.dp(act, 230f), UiKit.dp(act, 46f)), 0, 5 * d, 0, 5 * d))
        }

        val go = UiKit.label(act, "انطلق مباشرة إلى:", 13f, Color.argb(200, 255, 255, 255))
        go.setPadding(0, 14 * d, 0, 6 * d)
        panel.addView(go)
        val chips = UiKit.row(act)
        val cities = listOf("تعز", "صنعاء", "إب", "عدن", "حضرموت")
        for (i in cities.indices) {
            val chip = UiKit.btn(act, cities[i], 13f)
            chip.setPadding(10 * d, 4 * d, 10 * d, 4 * d)
            val idx = i + 1
            chip.setOnClickListener { cb.onStartDrive(idx) }
            chips.addView(chip, UiKit.margins(UiKit.vlp(-2, UiKit.dp(act, 38f)), 4 * d, 0, 4 * d, 0))
        }
        panel.addView(chips)

        val scroll = android.widget.ScrollView(act)
        scroll.addView(panel)
        val lp = UiKit.lp(UiKit.dp(act, 340f), -1, Gravity.CENTER)
        root.addView(scroll, lp)
    }

    fun setProgress(balance: Int, level: Int) {
        if (::statsText.isInitialized) {
            statsText.text = "المستوى $level • الرصيد: $balance"
        }
    }

    fun setBest(stars: Int, best: Int, challengeBest: Int) {
        bestText.text = "النجوم: $stars • أفضل انجراف: $best • أفضل تحدي: $challengeBest"
    }
}
