package com.sayyaratak

import android.content.Context
import android.graphics.*
import android.view.View

// Cockpit dashboard: rotating steering wheel + gauges driven by the simulation.
class CockpitView(ctx: Context) : View(ctx) {
    var kmh = 0f
    var rpm01 = 0.12f
    var gear = 1
    var steer = 0f          // radians, ~+-0.6
    var fuel = 100f
    var temp = 60f
    var damage = 0f

    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private val txt = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE; textAlign = Paint.Align.CENTER }
    private var tf: Typeface? = null
    fun setFont(t: Typeface) { tf = t; txt.typeface = t }

    private fun gauge(c: Canvas, cx: Float, cy: Float, r: Float, frac: Float, maxLabel: String, unit: String, redline: Float, value: String) {
        p.style = Paint.Style.FILL; p.color = Color.argb(190, 12, 14, 20)
        c.drawCircle(cx, cy, r, p)
        p.style = Paint.Style.STROKE; p.strokeWidth = r * 0.09f; p.shader = null
        val rect = RectF(cx - r * 0.82f, cy - r * 0.82f, cx + r * 0.82f, cy + r * 0.82f)
        p.color = Color.argb(80, 255, 255, 255)
        c.drawArc(rect, 135f, 270f, false, p)
        // redline zone
        p.color = Color.argb(160, 220, 50, 40)
        c.drawArc(rect, 135f + 270f * redline, 270f * (1f - redline), false, p)
        // value arc
        p.color = Color.rgb(255, 205, 80)
        c.drawArc(rect, 135f, 270f * frac.coerceIn(0f, 1f), false, p)
        // needle
        val a = Math.toRadians((135f + 270f * frac.coerceIn(0f, 1f)).toDouble())
        p.color = Color.rgb(255, 90, 60); p.strokeWidth = r * 0.05f; p.strokeCap = Paint.Cap.ROUND
        c.drawLine(cx, cy, cx + Math.cos(a).toFloat() * r * 0.70f, cy + Math.sin(a).toFloat() * r * 0.70f, p)
        p.strokeCap = Paint.Cap.BUTT
        txt.textSize = r * 0.34f; txt.color = Color.WHITE
        c.drawText(value, cx, cy + r * 0.50f, txt)
        txt.textSize = r * 0.17f; txt.color = Color.argb(200, 255, 255, 255)
        c.drawText("$maxLabel $unit", cx, cy + r * 0.72f, txt)
    }

    override fun onDraw(c: Canvas) {
        val w = width.toFloat(); val h = height.toFloat()
        // dash panel
        p.style = Paint.Style.FILL; p.shader = LinearGradient(0f, 0f, 0f, h,
            Color.rgb(16, 16, 20), Color.rgb(34, 30, 28), Shader.TileMode.CLAMP)
        c.drawRoundRect(RectF(0f, 0f, w, h), 30f, 30f, p)
        p.shader = null
        // trim strip
        p.color = Color.rgb(70, 58, 48); c.drawRect(0f, 0f, w, h * 0.06f, p)

        val gy = h * 0.52f; val gr = h * 0.40f
        // tachometer (left in RTL = right side), speedo (left side)
        gauge(c, w * 0.30f, gy, gr, kmh / 240f, "٢٤٠", "كم/س", 0.85f, kmh.toInt().toString())
        gauge(c, w * 0.70f, gy, gr, (rpm01 - 0.12f) / 0.88f, "٨", "لفة/د ×١٠٠٠", 0.82f, String.format("%.1f", rpm01 * 8f))

        // gear indicator between gauges
        txt.textSize = gr * 0.55f; txt.color = Color.rgb(120, 220, 255)
        c.drawText(if (kmh < 1f && rpm01 < 0.2f) "N" else gear.toString(), w * 0.5f, gy + gr * 0.2f, txt)
        txt.textSize = gr * 0.2f; txt.color = Color.argb(190, 255, 255, 255)
        c.drawText("الترس", w * 0.5f, gy + gr * 0.55f, txt)

        // steering wheel (center bottom), rotates with actual sim steering
        val wx = w * 0.5f; val wy = h * 1.28f; val wr = h * 0.78f
        c.save()
        c.rotate(Math.toDegrees(steer.toDouble()).toFloat() * 2.2f, wx, wy)
        p.style = Paint.Style.STROKE; p.strokeWidth = wr * 0.14f; p.color = Color.rgb(24, 24, 28)
        c.drawCircle(wx, wy, wr, p)
        p.strokeWidth = wr * 0.10f; p.color = Color.rgb(48, 48, 54)
        for (spoke in intArrayOf(90, 210, 330)) {
            val a = Math.toRadians(spoke.toDouble())
            c.drawLine(wx, wy, wx + Math.cos(a).toFloat() * wr, wy + Math.sin(a).toFloat() * wr, p)
        }
        p.style = Paint.Style.FILL; p.color = Color.rgb(30, 30, 36)
        c.drawCircle(wx, wy, wr * 0.22f, p)
        c.restore()

        // fuel + temp bars
        fun bar(x: Float, label: String, frac: Float, col: Int) {
            txt.textAlign = Paint.Align.RIGHT; txt.textSize = h * 0.10f; txt.color = Color.WHITE
            c.drawText(label, x + h * 0.42f, h * 0.16f, txt)
            txt.textAlign = Paint.Align.CENTER
            p.style = Paint.Style.FILL; p.color = Color.argb(120, 255, 255, 255)
            c.drawRoundRect(RectF(x, h * 0.20f, x + h * 0.42f, h * 0.26f), 6f, 6f, p)
            p.color = col
            c.drawRoundRect(RectF(x, h * 0.20f, x + h * 0.42f * frac.coerceIn(0f, 1f), h * 0.26f), 6f, 6f, p)
        }
        bar(w * 0.03f, "وقود", fuel / 100f, if (fuel < 20f) Color.rgb(230, 60, 40) else Color.rgb(90, 200, 110))
        bar(w * 0.88f, "حرارة", (temp - 40f) / 75f, if (temp > 105f) Color.rgb(230, 60, 40) else Color.rgb(90, 160, 230))
        if (damage > 5f) {
            txt.textSize = h * 0.11f; txt.color = Color.rgb(255, 120, 90)
            c.drawText("أضرار ${damage.toInt()}٪", w * 0.5f, h * 0.13f, txt)
        }
    }
}
