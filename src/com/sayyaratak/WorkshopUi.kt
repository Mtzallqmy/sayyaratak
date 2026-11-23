package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import android.widget.LinearLayout

class WorkshopUi(val act: Activity, val cb: Callbacks) {
    interface Callbacks {
        fun onPaint(r: Float, g: Float, b: Float)
        fun onWheels(style: Int)
        fun onSpoiler(on: Boolean)
        fun onNeon(on: Boolean)
        fun onTint(on: Boolean)
        fun onEngine(p: Int)
        fun onPreviewRev()
        fun onCloseWorkshop()
    }

    lateinit var panel: FrameLayout
    private lateinit var content: LinearLayout
    private var spoilerOn = true
    private var neonOn = false
    private var tintOn = true
    private lateinit var spoilerBtn: android.widget.Button
    private lateinit var neonBtn: android.widget.Button
    private lateinit var tintBtn: android.widget.Button

    val colors = listOf(
        Triple("أحمر قرمزي", 0.62f, 0.05f) to 0.06f,
        Triple("أزرق ليلي", 0.08f, 0.14f) to 0.38f,
        Triple("أبيض لؤلؤي", 0.92f, 0.90f) to 0.86f,
        Triple("أسود فحمي", 0.06f, 0.06f) to 0.08f,
        Triple("ذهبي", 0.80f, 0.62f) to 0.22f,
        Triple("أخضر زمردي", 0.05f, 0.38f) to 0.20f,
        Triple("برتقالي", 0.90f, 0.42f) to 0.08f,
        Triple("بنفسجي", 0.35f, 0.14f) to 0.50f
    )

    fun build(root: FrameLayout) {
        val d = UiKit.dp(act, 1f)
        panel = FrameLayout(act)

        val topBar = UiKit.row(act)
        topBar.gravity = Gravity.RIGHT
        val back = UiKit.btn(act, "رجوع", 14f)
        back.setOnClickListener { cb.onCloseWorkshop() }
        val title = UiKit.label(act, "الورشة", 24f, UiKit.GOLD)
        title.typeface = UiKit.fontBold
        title.setPadding(16 * d, 0, 0, 0)
        topBar.addView(title, UiKit.vlp(0, -2).also { it.weight = 1f })
        topBar.addView(back, UiKit.vlp(UiKit.dp(act, 90f), UiKit.dp(act, 42f)))
        topBar.setPadding(14 * d, 8 * d, 14 * d, 0)
        panel.addView(topBar, UiKit.lp(-1, -2, Gravity.TOP))

        val card = UiKit.column(act)
        card.background = UiKit.panelBg(act)
        card.setPadding(16 * d, 12 * d, 16 * d, 14 * d)

        val tabs = UiKit.row(act)
        val tabDefs = listOf("اللون", "الجنوط", "الإضافات", "الصوت")
        for (i in tabDefs.indices) {
            val t = UiKit.btn(act, tabDefs[i], 13f)
            t.setOnClickListener { showTab(i) }
            tabs.addView(t, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 40f)).also { it.weight = 1f }, 3 * d, 0, 3 * d, 0))
        }
        card.addView(tabs, UiKit.vlp(-1, -2))
        content = UiKit.column(act)
        content.gravity = Gravity.CENTER
        card.addView(content, UiKit.margins(UiKit.vlp(-1, UiKit.dp(act, 96f)), 0, 10 * d, 0, 0))

        panel.addView(card, UiKit.lp(-1, -2, Gravity.BOTTOM))
        panel.visibility = View.GONE
        root.addView(panel)
        showTab(0)
    }

    private fun swatch(r: Float, g: Float, b: Float): View {
        val v = View(act)
        val gd = GradientDrawable()
        gd.setColor(Color.rgb((r * 255).toInt(), (g * 255).toInt(), (b * 255).toInt()))
        gd.cornerRadius = UiKit.dp(act, 10f).toFloat()
        gd.setStroke(UiKit.dp(act, 2f), Color.argb(200, 255, 255, 255))
        v.background = gd
        v.setOnClickListener { cb.onPaint(r, g, b) }
        return v
    }

    private fun showTab(i: Int) {
        content.removeAllViews()
        val d = UiKit.dp(act, 1f)
        when (i) {
            0 -> {
                val r1 = UiKit.row(act)
                for (k in 0..3) {
                    val c = colors[k]; r1.addView(swatch(c.first.second, c.first.third, c.second),
                        UiKit.margins(UiKit.vlp(UiKit.dp(act, 56f), UiKit.dp(act, 56f)), 5 * d, 5 * d, 5 * d, 5 * d))
                }
                val r2 = UiKit.row(act)
                for (k in 4..7) {
                    val c = colors[k]; r2.addView(swatch(c.first.second, c.first.third, c.second),
                        UiKit.margins(UiKit.vlp(UiKit.dp(act, 56f), UiKit.dp(act, 56f)), 5 * d, 5 * d, 5 * d, 5 * d))
                }
                content.addView(r1); content.addView(r2)
            }
            1 -> {
                val names = listOf("كلاسيك فضي", "رياضي أسود", "ذهبي فاخر")
                val r = UiKit.row(act)
                for (k in 0..2) {
                    val b = UiKit.btn(act, names[k], 13f)
                    b.setOnClickListener { cb.onWheels(k) }
                    r.addView(b, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 48f)).also { it.weight = 1f }, 4 * d, 0, 4 * d, 0))
                }
                content.addView(r, UiKit.vlp(-1, -2))
            }
            2 -> {
                val r = UiKit.row(act)
                spoilerBtn = UiKit.btn(act, "جناح خلفي: " + if (spoilerOn) "نعم" else "لا", 13f)
                neonBtn = UiKit.btn(act, "نيون أرضي: " + if (neonOn) "نعم" else "لا", 13f)
                tintBtn = UiKit.btn(act, "زجاج داكن: " + if (tintOn) "نعم" else "لا", 13f)
                spoilerBtn.setOnClickListener { spoilerOn = !spoilerOn; spoilerBtn.text = "جناح خلفي: " + if (spoilerOn) "نعم" else "لا"; cb.onSpoiler(spoilerOn) }
                neonBtn.setOnClickListener { neonOn = !neonOn; neonBtn.text = "نيون أرضي: " + if (neonOn) "نعم" else "لا"; cb.onNeon(neonOn) }
                tintBtn.setOnClickListener { tintOn = !tintOn; tintBtn.text = "زجاج داكن: " + if (tintOn) "نعم" else "لا"; cb.onTint(tintOn) }
                for (b in listOf(spoilerBtn, neonBtn, tintBtn))
                    r.addView(b, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 48f)).also { it.weight = 1f }, 4 * d, 0, 4 * d, 0))
                content.addView(r, UiKit.vlp(-1, -2))
            }
            3 -> {
                val names = listOf("هادئ", "رياضي", "وحش")
                val r = UiKit.row(act)
                for (k in 0..2) {
                    val b = UiKit.btn(act, names[k], 13f)
                    b.setOnClickListener { cb.onEngine(k) }
                    r.addView(b, UiKit.margins(UiKit.vlp(0, UiKit.dp(act, 48f)).also { it.weight = 1f }, 4 * d, 0, 4 * d, 0))
                }
                content.addView(r, UiKit.vlp(-1, -2))
                val rev = UiKit.btn(act, "تجربة الصوت", 13f, true)
                rev.setOnClickListener { cb.onPreviewRev() }
                content.addView(rev, UiKit.margins(UiKit.vlp(UiKit.dp(act, 200f), UiKit.dp(act, 42f)), 0, 8 * d, 0, 0))
            }
        }
    }

    fun syncState(spoiler: Boolean, neon: Boolean, tint: Boolean) {
        spoilerOn = spoiler; neonOn = neon; tintOn = tint
    }
}
