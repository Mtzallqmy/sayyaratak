package com.sayyaratak

import android.content.Context
import android.graphics.*
import android.view.View

class SpeedometerView(ctx: Context) : View(ctx) {
    var kmh = 0f
    var offroad = false

    private val arcPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE; strokeCap = Paint.Cap.ROUND }
    private val tickPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE; color = Color.WHITE; strokeWidth = 3f }
    private val needlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE; color = Color.rgb(255, 200, 60); strokeWidth = 6f; strokeCap = Paint.Cap.ROUND }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE; textAlign = Paint.Align.CENTER }
    private var typeface: Typeface? = null

    fun setFont(tf: Typeface) { typeface = tf; textPaint.typeface = tf }

    private fun ar(d: Int): String {
        val digits = charArrayOf('٠','١','٢','٣','٤','٥','٦','٧','٨','٩')
        return d.toString().map { if (it.isDigit()) digits[it - '0'] else it }.joinToString("")
    }

    override fun onDraw(c: Canvas) {
        val w = width.toFloat(); val h = height.toFloat()
        val cx = w / 2f; val cy = h / 2f; val r = Math.min(w, h) / 2f - 12f
        // background disc
        arcPaint.style = Paint.Style.FILL
        arcPaint.color = Color.argb(140, 10, 14, 24)
        c.drawCircle(cx, cy, r + 8f, arcPaint)
        arcPaint.style = Paint.Style.STROKE
        arcPaint.strokeWidth = 10f
        // base arc 135..405 (270 degrees), 0..240 kmh
        val rect = RectF(cx - r, cy - r, cx + r, cy + r)
        arcPaint.color = Color.argb(90, 255, 255, 255)
        c.drawArc(rect, 135f, 270f, false, arcPaint)
        // speed arc gradient gold->red
        val sweep = (kmh / 240f).coerceIn(0f, 1f) * 270f
        arcPaint.shader = SweepGradient(cx, cy, intArrayOf(Color.rgb(255, 214, 90), Color.rgb(255, 70, 40)), null)
        c.drawArc(rect, 135f, sweep, false, arcPaint)
        arcPaint.shader = null
        // ticks
        textPaint.textSize = r * 0.16f
        for (v in 0..240 step 30) {
            val a = Math.toRadians((135 + v / 240.0 * 270))
            val x1 = cx + Math.cos(a).toFloat() * (r - 14f); val y1 = cy + Math.sin(a).toFloat() * (r - 14f)
            val x2 = cx + Math.cos(a).toFloat() * (r - 26f); val y2 = cy + Math.sin(a).toFloat() * (r - 26f)
            c.drawLine(x1, y1, x2, y2, tickPaint)
            val tx = cx + Math.cos(a).toFloat() * (r - 46f); val ty = cy + Math.sin(a).toFloat() * (r - 46f)
            c.drawText(ar(v), tx, ty + textPaint.textSize * 0.35f, textPaint)
        }
        // needle
        val na = Math.toRadians((135.0 + (kmh.coerceIn(0f, 240f) / 240f) * 270.0))
        c.drawLine(cx, cy, cx + Math.cos(na).toFloat() * (r - 34f), cy + Math.sin(na).toFloat() * (r - 34f), needlePaint)
        // digital
        textPaint.textSize = r * 0.34f
        textPaint.color = Color.WHITE
        c.drawText(ar(kmh.toInt()), cx, cy + r * 0.55f, textPaint)
        textPaint.textSize = r * 0.13f
        textPaint.color = Color.argb(200, 255, 255, 255)
        c.drawText("كم/س", cx, cy + r * 0.72f, textPaint)
    }
}
