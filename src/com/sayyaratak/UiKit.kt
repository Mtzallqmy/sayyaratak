package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView

object UiKit {
    var font: Typeface = Typeface.DEFAULT
    var fontBold: Typeface = Typeface.DEFAULT_BOLD

    fun dp(a: Activity, v: Float) = (v * a.resources.displayMetrics.density).toInt()

    val GOLD = Color.rgb(214, 178, 94)
    val DARK = Color.rgb(12, 16, 26)

    fun panelBg(a: Activity, alpha: Int = 215): GradientDrawable {
        val g = GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
            intArrayOf(Color.argb(alpha, 16, 20, 34), Color.argb(alpha, 8, 10, 18)))
        g.cornerRadius = dp(a, 18f).toFloat()
        g.setStroke(dp(a, 1f), Color.argb(120, 214, 178, 94))
        return g
    }

    fun btn(a: Activity, text: String, sizeSp: Float = 16f, accent: Boolean = false): Button {
        val b = Button(a)
        b.text = text
        b.textSize = sizeSp
        b.typeface = fontBold
        b.setTextColor(if (accent) DARK else Color.WHITE)
        val g = GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,
            if (accent) intArrayOf(Color.rgb(232, 196, 110), Color.rgb(198, 156, 70))
            else intArrayOf(Color.argb(200, 40, 48, 70), Color.argb(200, 26, 32, 50)))
        g.cornerRadius = dp(a, 14f).toFloat()
        g.setStroke(dp(a, 1f), if (accent) Color.rgb(255, 230, 160) else Color.argb(90, 255, 255, 255))
        b.background = g
        b.setPadding(dp(a, 8f), dp(a, 6f), dp(a, 8f), dp(a, 6f))
        b.minWidth = 0; b.minimumWidth = 0
        b.minHeight = 0; b.minimumHeight = 0
        b.isAllCaps = false
        b.maxLines = 1
        return b
    }

    fun label(a: Activity, text: String, sizeSp: Float = 14f, color: Int = Color.WHITE): TextView {
        val t = TextView(a)
        t.text = text
        t.textSize = sizeSp
        t.setTextColor(color)
        t.typeface = font
        return t
    }

    fun column(a: Activity, gap: Int = 10): LinearLayout {
        val l = LinearLayout(a)
        l.orientation = LinearLayout.VERTICAL
        l.gravity = Gravity.CENTER_HORIZONTAL
        return l
    }

    fun row(a: Activity): LinearLayout {
        val l = LinearLayout(a)
        l.orientation = LinearLayout.HORIZONTAL
        l.gravity = Gravity.CENTER
        return l
    }

    fun lp(w: Int, h: Int, gravity: Int): FrameLayout.LayoutParams {
        val p = FrameLayout.LayoutParams(w, h)
        p.gravity = gravity
        return p
    }

    fun vlp(w: Int, h: Int): LinearLayout.LayoutParams = LinearLayout.LayoutParams(w, h)

    fun margins(p: LinearLayout.LayoutParams, l: Int, t: Int, r: Int, b: Int): LinearLayout.LayoutParams {
        p.setMargins(l, t, r, b)
        return p
    }

    fun show(v: View) { v.visibility = View.VISIBLE }
    fun hide(v: View) { v.visibility = View.GONE }
}
