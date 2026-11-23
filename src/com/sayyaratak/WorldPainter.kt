package com.sayyaratak

import android.graphics.*
import java.util.Random

object WorldPainter {

    class WorldData(
        val size: Float, val seaX: Float,
        val roads: List<FloatArray>,   // x1,z1,x2,z2,width,type
        val tints: List<FloatArray>,   // x,z,w,h,style
        val buildings: List<FloatArray>, // x,z,w,d
        val palms: List<FloatArray>    // x,z,s
    )

    fun parse(d: FloatArray): WorldData {
        var i = 0
        val size = d[i++]; val seaX = d[i++]
        fun take(n: Int, rec: Int): List<FloatArray> {
            val out = ArrayList<FloatArray>(n)
            for (k in 0 until n) {
                val r = FloatArray(rec)
                for (j in 0 until rec) r[j] = d[i++]
                out.add(r)
            }
            return out
        }
        val roads = take(d[i++].toInt(), 6)
        val tints = take(d[i++].toInt(), 5)
        val buildings = take(d[i++].toInt(), 4)
        val palms = take(d[i++].toInt(), 3)
        return WorldData(size, seaX, roads, tints, buildings, palms)
    }

    fun groundPixels(d: FloatArray, res: Int): IntArray {
        val w = parse(d)
        val bmp = Bitmap.createBitmap(res, res, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        val rnd = Random(7)
        val scale = res / w.size
        fun X(x: Float) = (x + w.size / 2f) * scale
        fun Z(z: Float) = (z + w.size / 2f) * scale
        val p = Paint(Paint.ANTI_ALIAS_FLAG)

        // base sand, warm gradient
        p.shader = LinearGradient(0f, 0f, res.toFloat(), res.toFloat(),
            Color.rgb(214, 183, 128), Color.rgb(196, 158, 100), Shader.TileMode.CLAMP)
        c.drawRect(0f, 0f, res.toFloat(), res.toFloat(), p)
        p.shader = null
        // sand speckle
        for (k in 0 until 9000) {
            val a = 12 + rnd.nextInt(16)
            p.color = if (rnd.nextBoolean()) Color.argb(a, 120, 90, 50) else Color.argb(a, 255, 240, 200)
            val s = 1f + rnd.nextFloat() * 2.5f
            c.drawRect(rnd.nextFloat() * res, rnd.nextFloat() * res, rnd.nextFloat() * res + s, rnd.nextFloat() * res + s, p)
        }
        // district ground tints
        for (t in w.tints) {
            val col = when (t[4].toInt()) {
                1 -> Color.argb(70, 90, 150, 70)    // Ibb green
                2 -> Color.argb(70, 150, 105, 60)   // Hadramawt rock
                3 -> Color.argb(60, 235, 225, 200)  // Aden pale
                4 -> Color.argb(60, 170, 130, 85)   // Sanaa dusty
                5 -> Color.argb(55, 190, 140, 90)   // Taiz warm
                else -> Color.TRANSPARENT
            }
            p.color = col
            c.drawRect(X(t[0] - t[2] / 2), Z(t[1] - t[3] / 2), X(t[0] + t[2] / 2), Z(t[1] + t[3] / 2), p)
        }
        // Ibb vegetation patches
        for (t in w.tints) if (t[4].toInt() == 1) {
            for (k in 0 until 700) {
                p.color = Color.argb(30 + rnd.nextInt(40), 40 + rnd.nextInt(40), 110 + rnd.nextInt(60), 40)
                val px = X(t[0] - t[2] / 2 + rnd.nextFloat() * t[2])
                val pz = Z(t[1] - t[3] / 2 + rnd.nextFloat() * t[3])
                val s = 2f + rnd.nextFloat() * 6f
                c.drawCircle(px, pz, s, p)
            }
        }
        // sea
        val seaPx = X(w.seaX)
        p.shader = LinearGradient(seaPx, 0f, res.toFloat(), 0f,
            Color.rgb(28, 96, 140), Color.rgb(10, 48, 88), Shader.TileMode.CLAMP)
        c.drawRect(seaPx, 0f, res.toFloat(), res.toFloat(), p)
        p.shader = null
        // sand->wet band + foam
        p.color = Color.rgb(222, 205, 160)
        c.drawRect(seaPx - 8 * scale, 0f, seaPx, res.toFloat(), p)
        p.color = Color.argb(220, 255, 255, 255)
        c.drawRect(seaPx, 0f, seaPx + 1.6f * scale, res.toFloat(), p)
        p.color = Color.argb(90, 255, 255, 255)
        for (k in 0..3) c.drawRect(seaPx + (6 + k * 14) * scale, 0f, seaPx + (7 + k * 14) * scale, res.toFloat(), p)

        // building shadows (under roads is fine, drawn before roads look odd; draw now)
        for (b in w.buildings) {
            p.color = Color.argb(60, 40, 25, 10)
            val bx = X(b[0] - b[2] / 2) + 3f; val bz = Z(b[1] - b[3] / 2) + 3f
            c.drawRect(bx, bz, bx + b[2] * scale, bz + b[3] * scale, p)
        }
        // roads: sidewalk bed then asphalt then center line
        for (r in w.roads) {
            val hw = r[4]
            p.strokeCap = Paint.Cap.ROUND
            p.style = Paint.Style.STROKE
            p.color = Color.rgb(178, 168, 150) // sidewalk
            p.strokeWidth = (hw + 3.5f) * scale
            c.drawLine(X(r[0]), Z(r[1]), X(r[2]), Z(r[3]), p)
            p.color = if (r[5] > 0.5f) Color.rgb(52, 54, 60) else Color.rgb(60, 62, 68)
            p.strokeWidth = hw * scale
            c.drawLine(X(r[0]), Z(r[1]), X(r[2]), Z(r[3]), p)
            // center dashes
            p.color = if (r[5] > 0.5f) Color.rgb(240, 200, 70) else Color.rgb(225, 225, 225)
            p.strokeWidth = (if (r[5] > 0.5f) 0.7f else 0.45f) * scale
            p.pathEffect = DashPathEffect(floatArrayOf(6f * scale, 5f * scale), 0f)
            c.drawLine(X(r[0]), Z(r[1]), X(r[2]), Z(r[3]), p)
            p.pathEffect = null
            p.style = Paint.Style.FILL
        }
        // palms
        for (pl in w.palms) {
            p.color = Color.argb(160, 30, 80, 30)
            c.drawCircle(X(pl[0]), Z(pl[1]), pl[2] * 0.8f * scale, p)
            p.color = Color.argb(120, 70, 140, 60)
            c.drawCircle(X(pl[0]), Z(pl[1]), pl[2] * 0.4f * scale, p)
        }
        val px = IntArray(res * res)
        bmp.getPixels(px, 0, res, 0, 0, res, res)
        bmp.recycle()
        return px
    }

    // Building facade textures: style 0 modern, 1 sanaa tower, 2 mud (shibam), 3 white (aden)
    fun buildingPixels(style: Int, res: Int): IntArray {
        val bmp = Bitmap.createBitmap(res, res, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        val rnd = Random((style * 131 + 5).toLong())
        val p = Paint(Paint.ANTI_ALIAS_FLAG)
        val base = when (style) {
            1 -> Color.rgb(146, 104, 66)
            2 -> Color.rgb(158, 118, 74)
            3 -> Color.rgb(228, 220, 204)
            else -> Color.rgb(186, 172, 148)
        }
        c.drawColor(base)
        // subtle vertical gradient
        p.shader = LinearGradient(0f, 0f, 0f, res.toFloat(),
            Color.argb(30, 255, 255, 255), Color.argb(60, 20, 10, 0), Shader.TileMode.CLAMP)
        c.drawRect(0f, 0f, res.toFloat(), res.toFloat(), p)
        p.shader = null
        // grime noise
        for (k in 0 until 600) {
            p.color = Color.argb(8 + rnd.nextInt(10), 40, 25, 10)
            val s = 2f + rnd.nextFloat() * 8f
            c.drawRect(rnd.nextFloat() * res, rnd.nextFloat() * res, rnd.nextFloat() * res + s, rnd.nextFloat() * res + s, p)
        }
        val floors = when (style) { 1 -> 8; 2 -> 12; 3 -> 5; else -> 7 }
        val cols = when (style) { 2 -> 7; else -> 5 }
        val fh = res.toFloat() / floors
        val cw = res.toFloat() / cols
        for (f in 0 until floors) {
            // sanaa white gypsum bands every 3rd floor
            if (style == 1 && f % 3 == 2) {
                p.color = Color.argb(220, 240, 235, 225)
                c.drawRect(0f, f * fh, res.toFloat(), f * fh + fh * 0.16f, p)
            }
            if (style == 2 && f % 4 == 3) {
                p.color = Color.argb(60, 90, 60, 30)
                c.drawRect(0f, f * fh, res.toFloat(), f * fh + fh * 0.1f, p)
            }
            for (col in 0 until cols) {
                val wx = col * cw + cw * 0.22f
                val wy = f * fh + fh * 0.24f
                val ww = cw * 0.56f
                val wh = fh * 0.52f
                val lit = rnd.nextFloat() < 0.12f
                p.color = if (lit) Color.rgb(255, 214, 140) else when (style) {
                    3 -> Color.rgb(70, 110, 150)
                    1, 2 -> Color.rgb(45, 35, 28)
                    else -> Color.rgb(60, 84, 108)
                }
                if (style == 1) {
                    // arched window
                    val rf = RectF(wx, wy, wx + ww, wy + wh)
                    c.drawRoundRect(rf, ww * 0.5f, ww * 0.5f, p)
                    p.color = Color.argb(200, 245, 240, 230)
                    c.drawRoundRect(RectF(wx - 2, wy - 2, wx + ww + 2, wy + 2.5f), ww * 0.5f, ww * 0.5f, p)
                    p.color = if (lit) Color.rgb(255, 214, 140) else Color.rgb(45, 35, 28)
                    c.drawRoundRect(rf, ww * 0.5f, ww * 0.5f, p)
                } else {
                    c.drawRect(wx, wy, wx + ww, wy + wh, p)
                    if (style == 3) { // blue shutters
                        p.color = Color.rgb(60, 90, 140)
                        c.drawRect(wx - ww * 0.14f, wy, wx - ww * 0.02f, wy + wh, p)
                        c.drawRect(wx + ww * 1.02f, wy, wx + ww * 1.14f, wy + wh, p)
                    }
                }
                // glass shine
                p.color = Color.argb(40, 255, 255, 255)
                c.drawRect(wx, wy, wx + ww, wy + wh * 0.3f, p)
            }
        }
        val px = IntArray(res * res)
        bmp.getPixels(px, 0, res, 0, 0, res, res)
        bmp.recycle()
        return px
    }

    fun palmPixels(res: Int): IntArray {
        val bmp = Bitmap.createBitmap(res, res, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        val p = Paint(Paint.ANTI_ALIAS_FLAG)
        c.drawColor(Color.TRANSPARENT)
        val cx = res / 2f; val cy = res / 2f
        // radiating fronds, drooping curves
        for (i in 0 until 9) {
            val ang = i / 9.0 * 2 * Math.PI + 0.2
            val path = Path()
            path.moveTo(cx, cy)
            val mx = cx + Math.cos(ang).toFloat() * res * 0.28f
            val my = cy + Math.sin(ang).toFloat() * res * 0.28f
            val ex = cx + Math.cos(ang + 0.35).toFloat() * res * 0.46f
            val ey = cy + Math.sin(ang + 0.35).toFloat() * res * 0.46f + res * 0.06f
            path.quadTo(mx, my, ex, ey)
            p.style = Paint.Style.STROKE
            p.strokeWidth = res * 0.055f
            p.strokeCap = Paint.Cap.ROUND
            p.color = Color.rgb(34 + i * 3, 96 + i * 4, 40)
            c.drawPath(path, p)
            p.strokeWidth = res * 0.028f
            p.color = Color.rgb(60, 140, 60)
            c.drawPath(path, p)
        }
        p.style = Paint.Style.FILL
        p.color = Color.rgb(110, 80, 40)
        c.drawCircle(cx, cy, res * 0.05f, p)
        val px = IntArray(res * res)
        bmp.getPixels(px, 0, res, 0, 0, res, res)
        bmp.recycle()
        return px
    }

    fun particlePixels(res: Int): IntArray {
        val px = IntArray(res * res)
        val half = res / 2f
        for (y in 0 until res) for (x in 0 until res) {
            val dx = (x - half) / half; val dy = (y - half) / half
            val d = Math.sqrt((dx * dx + dy * dy).toDouble()).toFloat().coerceIn(0f, 1f)
            val a = ((1f - d) * (1f - d) * 255).toInt().coerceIn(0, 255)
            px[y * res + x] = (a shl 24) or 0xFFFFFF
        }
        return px
    }

    fun dashboardBitmap(w: Int, h: Int): Bitmap {
        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        val p = Paint(Paint.ANTI_ALIAS_FLAG)
        // leather dash gradient
        p.shader = LinearGradient(0f, 0f, 0f, h.toFloat(), Color.rgb(28, 26, 30), Color.rgb(8, 8, 10), Shader.TileMode.CLAMP)
        c.drawRect(0f, 0f, w.toFloat(), h.toFloat(), p)
        p.shader = null
        // top edge highlight
        p.color = Color.argb(80, 255, 235, 200)
        c.drawRect(0f, 0f, w.toFloat(), 3f, p)
        // gauges
        val gy = h * 0.52f; val gr = h * 0.34f
        for (gx in floatArrayOf(w * 0.38f, w * 0.62f)) {
            p.color = Color.rgb(14, 16, 22)
            c.drawCircle(gx, gy, gr, p)
            p.style = Paint.Style.STROKE
            p.strokeWidth = 4f
            p.color = Color.rgb(200, 170, 90)
            c.drawCircle(gx, gy, gr, p)
            p.color = Color.argb(160, 255, 255, 255)
            p.strokeWidth = 3f
            for (i in 0..8) {
                val a = Math.toRadians(150.0 + i * 30.0)
                c.drawLine(gx + Math.cos(a).toFloat() * (gr - 8f), gy + Math.sin(a).toFloat() * (gr - 8f),
                    gx + Math.cos(a).toFloat() * (gr - 18f), gy + Math.sin(a).toFloat() * (gr - 18f), p)
            }
            p.style = Paint.Style.FILL
        }
        // steering wheel bottom
        val wy = h * 1.15f; val wr = h * 0.62f
        p.style = Paint.Style.STROKE
        p.strokeWidth = h * 0.09f
        p.color = Color.rgb(20, 20, 24)
        c.drawCircle(w / 2f, wy, wr, p)
        p.strokeWidth = h * 0.02f
        p.color = Color.rgb(200, 170, 90)
        c.drawCircle(w / 2f, wy, wr, p)
        p.style = Paint.Style.FILL
        val px = IntArray(1); px[0] = 0
        return bmp
    }
}
