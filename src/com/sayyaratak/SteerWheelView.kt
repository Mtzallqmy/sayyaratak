package com.sayyaratak

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import kotlin.math.max
import kotlin.math.min

// Drag-to-steer wheel: horizontal drag turns the wheel, release springs back.
class SteerWheelView(ctx: Context) : View(ctx) {
    var steer = 0f           // -1..1, written by touch, read by HudUi
    var onChange: ((Float) -> Unit)? = null
    private var typefaceSet = false
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)

    override fun onTouchEvent(ev: MotionEvent): Boolean {
        when (ev.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                val r = width * 0.5f
                val v = (ev.x - width * 0.5f) / (r * 0.9f)
                val nv = max(-1f, min(1f, v))
                if (nv != steer) { steer = nv; onChange?.invoke(steer); invalidate() }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                steer = 0f; onChange?.invoke(0f); invalidate()
            }
        }
        return true
    }

    override fun onDraw(c: Canvas) {
        val w = width.toFloat(); val h = height.toFloat()
        val cx = w / 2; val cy = h / 2; val r = min(w, h) * 0.46f
        val rot = steer * 150f
        c.save()
        c.rotate(rot, cx, cy)
        // rim
        p.style = Paint.Style.STROKE; p.strokeWidth = r * 0.16f
        p.color = Color.argb(220, 30, 32, 38)
        c.drawCircle(cx, cy, r, p)
        p.color = Color.argb(200, 120, 125, 135)
        p.strokeWidth = r * 0.05f
        c.drawCircle(cx, cy, r * 0.92f, p)
        // spokes
        p.color = Color.argb(220, 45, 48, 55)
        p.strokeWidth = r * 0.14f
        c.drawLine(cx, cy, cx - r * 0.85f, cy + r * 0.28f, p)
        c.drawLine(cx, cy, cx + r * 0.85f, cy + r * 0.28f, p)
        c.drawLine(cx, cy, cx, cy - r * 0.9f, p)
        // hub
        p.style = Paint.Style.FILL
        p.color = Color.argb(230, 60, 63, 72)
        c.drawCircle(cx, cy, r * 0.30f, p)
        p.color = Color.argb(255, 200, 165, 60)
        c.drawCircle(cx, cy, r * 0.12f, p)
        // top marker
        p.color = Color.argb(255, 200, 60, 50)
        c.drawRoundRect(RectF(cx - r * 0.07f, cy - r - r * 0.10f, cx + r * 0.07f, cy - r * 0.72f), r * 0.05f, r * 0.05f, p)
        c.restore()
    }
}
