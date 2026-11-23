package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class MissionUi(val act: Activity, val cb: Callbacks) {
    interface Callbacks {
        fun onStartMission(city: Int, type: Int)
        fun onCloseMissions()
    }

    lateinit var panel: FrameLayout
    private lateinit var listCol: LinearLayout
    private lateinit var balanceText: TextView
    val cities = arrayOf("", "تعز", "صنعاء", "إب", "عدن", "حضرموت")

    fun build(root: FrameLayout) {
        panel = FrameLayout(act)
        panel.setBackgroundColor(Color.argb(235, 12, 14, 22))

        val scroll = ScrollView(act)
        listCol = UiKit.column(act)
        listCol.setPadding(UiKit.dp(act, 16f), UiKit.dp(act, 16f), UiKit.dp(act, 16f), UiKit.dp(act, 30f))
        scroll.addView(listCol)
        panel.addView(scroll, UiKit.lp(-1, -1, Gravity.TOP))

        balanceText = UiKit.label(act, "", 15f, UiKit.GOLD)
        val bp = UiKit.lp(-2, -2, Gravity.TOP or Gravity.RIGHT)
        bp.setMargins(0, UiKit.dp(act, 10f), UiKit.dp(act, 18f), 0)
        panel.addView(balanceText, bp)

        val back = UiKit.btn(act, "رجوع", 14f)
        back.setOnClickListener { cb.onCloseMissions() }
        val bb = UiKit.lp(-2, -2, Gravity.BOTTOM or Gravity.LEFT)
        bb.setMargins(UiKit.dp(act, 16f), 0, 0, UiKit.dp(act, 14f))
        panel.addView(back, bb)

        panel.visibility = View.GONE
        root.addView(panel)
    }

    fun refresh() {
        balanceText.text = "الرصيد: ${Progression.balance(act)}   •   المستوى: ${Progression.level(act)}"
        listCol.removeAllViews()
        val d = UiKit.dp(act, 1f)
        val title = UiKit.label(act, "المهام", 24f, UiKit.GOLD)
        title.typeface = UiKit.fontBold
        listCol.addView(title, UiKit.margins(UiKit.vlp(-2, -2), 0, 0, 0, 12 * d))
        for (city in 1..5) {
            val ct = UiKit.label(act, cities[city], 19f, Color.WHITE)
            ct.typeface = UiKit.fontBold
            listCol.addView(ct, UiKit.margins(UiKit.vlp(-2, -2), 0, 8 * d, 0, 4 * d))
            for (type in 1..3) {
                val row = UiKit.row(act)
                row.setBackgroundColor(Color.argb(60, 30, 34, 48))
                row.setPadding(10 * d, 6 * d, 10 * d, 6 * d)
                val done = Progression.missionDone(act, city, type)
                val best = Progression.missionBest(act, city, type)
                val info = UiKit.label(act,
                    "${if (done) "✓ " else ""}${Progression.MISSION_NAMES[type]}  •  ${Progression.MISSION_REWARDS[type]} رصيد" +
                    (if (best > 0) "  •  أفضل: $best" else ""),
                    13f, if (done) Color.rgb(120, 220, 130) else Color.rgb(220, 220, 228))
                row.addView(info, UiKit.margins(UiKit.vlp(0, -2).also { it.weight = 1f }, 0, 0, 0, 0))
                val go = UiKit.btn(act, "ابدأ", 13f, !done)
                go.setOnClickListener { cb.onStartMission(city, type) }
                row.addView(go, UiKit.margins(UiKit.vlp(UiKit.dp(act, 80f), UiKit.dp(act, 38f)), 8 * d, 0, 0, 0))
                listCol.addView(row, UiKit.margins(UiKit.vlp(-1, -2), 0, 0, 0, 6 * d))
            }
        }
    }
}
