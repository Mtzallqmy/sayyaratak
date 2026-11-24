package com.sayyaratak

import android.content.Context
import android.graphics.*
import android.view.View

class SpeedometerView(ctx: Context) : View(ctx) {
    var kmh = 0f
    var offroad = false

    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE; textAlign = Paint.Align.CENTER }
    private var typeface: Typeface? = null

    fun setFont(tf: Typeface) { typeface = tf; textPaint.typeface = tf }

    private fun ar(d: Int): String {
        val digits = charArrayOf('٠','١','٢','٣','٤','٥','٦','٧','٨','٩')
        return d.toString().map { if (it.isDigit()) digits[it - '0'] else it }.joinToString("")
    }

    override fun onDraw(c: Canvas) {
        val w = width.toFloat(); val h = height.toFloat()
        val cx = w / 2f; val cy = h / 2f; val r = Math.min(w, h) / 2f - 10f
        val rect = RectF(cx - r, cy - r, cx + r, cy + r)

        // outer metallic bezel with soft shadow
        p.style = Paint.Style.FILL
        p.shader = RadialGradient(cx, cy - r * 0.5f, r * 2.1f,
            intArrayOf(Color.rgb(150, 155, 168), Color.rgb(58, 62, 74), Color.rgb(16, 18, 26)),
            floatArrayOf(0f, 0.82f, 1f), Shader.TileMode.CLAMP)
        c.drawCircle(cx, cy, r + 6f, p)
        // dark glass face
        p.shader = RadialGradient(cx, cy - r * 0.4f, r * 1.35f,
            intArrayOf(Color.rgb(52, 60, 82), Color.rgb(18, 22, 36), Color.rgb(8, 10, 18)),
            floatArrayOf(0f, 0.72f, 1f), Shader.TileMode.CLAMP)
        c.drawCircle(cx, cy, r - 4f, p)
        p.shader = null

        // redline zone 200-240
        p.style = Paint.Style.STROKE; p.strokeWidth = 9f; p.strokeCap = Paint.Cap.BUTT
        p.color = Color.argb(220, 210, 40, 30)
        c.drawArc(RectF(cx - r + 12f, cy - r + 12f, cx + r - 12f, cy + r - 12f), 135f + 270f * (200f / 240f), 270f * (40f / 240f), false, p)
        // base arc
        p.strokeWidth = 5f
        p.color = Color.argb(70, 255, 255, 255)
        c.drawArc(RectF(cx - r + 12f, cy - r + 12f, cx + r - 12f, cy + r - 12f), 135f, 270f * (200f / 240f), false, p)
        // live speed arc gold->red
        val sweep = (kmh / 240f).coerceIn(0f, 1f) * 270f
        if (sweep > 0.5f) {
            p.strokeWidth = 9f; p.strokeCap = Paint.Cap.ROUND
            p.shader = SweepGradient(cx, cy, intArrayOf(Color.rgb(255, 214, 90), Color.rgb(255, 70, 40), Color.rgb(255, 214, 90)), floatArrayOf(0.1f, 0.55f, 0.9f))
            c.drawArc(RectF(cx - r + 12f, cy - r + 12f, cx + r - 12f, cy + r - 12f), 135f, sweep, false, p)
            p.shader = null; p.strokeCap = Paint.Cap.BUTT
        }

        // ticks: minor every 15, major every 30 with labels
        for (v in 0..240 step 15) {
            val major = v % 30 == 0
            val a = Math.toRadians((135 + v / 240.0 * 270))
            val co = Math.cos(a).toFloat(); val si = Math.sin(a).toFloat()
            p.color = if (v >= 200) Color.rgb(255, 90, 70) else Color.argb(if (major) 235 else 110, 255, 255, 255)
            p.strokeWidth = if (major) 4f else 2f
            val inner = if (major) 30f else 22f
            c.drawLine(cx + co * (r - 16f), cy + si * (r - 16f), cx + co * (r - inner), cy + si * (r - inner), p)
            if (major) {
                textPaint.textSize = r * 0.145f
                textPaint.color = if (v >= 200) Color.rgb(255, 120, 100) else Color.WHITE
                c.drawText(ar(v), cx + co * (r - 50f), cy + si * (r - 50f) + textPaint.textSize * 0.35f, textPaint)
            }
        }

        // needle: tapered gold blade with dark spine
        val na = Math.toRadians((135.0 + (kmh.coerceIn(0f, 240f) / 240f) * 270.0))
        val nx = Math.cos(na).toFloat(); val ny = Math.sin(na).toFloat()
        val tip = Path()
        tip.moveTo(cx + nx * (r - 40f), cy + ny * (r - 40f))
        tip.lineTo(cx - ny * 5f - nx * 18f, cy + nx * 5f - ny * 18f)
        tip.lineTo(cx + ny * 5f - nx * 18f, cy - nx * 5f - ny * 18f)
        tip.close()
        p.style = Paint.Style.FILL
        p.color = Color.rgb(255, 196, 60)
        c.drawPath(tip, p)
        p.style = Paint.Style.STROKE; p.strokeWidth = 1.5f; p.color = Color.argb(160, 60, 30, 0)
        c.drawPath(tip, p)
        // center cap
        p.style = Paint.Style.FILL; p.color = Color.rgb(24, 28, 42)
        c.drawCircle(cx, cy, 15f, p)
        p.style = Paint.Style.STROKE; p.strokeWidth = 3f; p.color = Color.rgb(214, 178, 94)
        c.drawCircle(cx, cy, 15f, p)

        // digital readout pill
        textPaint.textSize = r * 0.30f
        val label = ar(kmh.toInt())
        val tw = textPaint.measureText(label)
        p.style = Paint.Style.FILL; p.color = Color.argb(170, 6, 8, 14)
        val pill = RectF(cx - tw / 2f - 18f, cy + r * 0.34f, cx + tw / 2f + 18f, cy + r * 0.34f + textPaint.textSize * 1.45f)
        c.drawRoundRect(pill, 14f, 14f, p)
        p.style = Paint.Style.STROKE; p.strokeWidth = 1.5f; p.color = Color.argb(90, 214, 178, 94)
        c.drawRoundRect(pill, 14f, 14f, p)
        textPaint.color = Color.WHITE
        c.drawText(label, cx, cy + r * 0.34f + textPaint.textSize * 1.12f, textPaint)
        textPaint.textSize = r * 0.115f
        textPaint.color = Color.argb(200, 255, 255, 255)
        c.drawText("كم/س", cx, cy + r * 0.34f + textPaint.textSize * 3.4f, textPaint)
    }
}
