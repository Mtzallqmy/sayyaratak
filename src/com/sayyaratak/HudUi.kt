package com.sayyaratak

import android.app.Activity
import android.graphics.Color
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView

class HudUi(val act: Activity, val cb: Callbacks) {
    interface Callbacks {
        fun onHorn()
        fun onLights()
        fun onInput(throttle: Float, brake: Float, steer: Float, handbrake: Boolean)
        fun onCamera()
        fun onExitToMenu()
    }

    lateinit var panel: FrameLayout
    lateinit var speedo: SpeedometerView
    lateinit var cockpit: CockpitView
    lateinit var scoreText: TextView
    lateinit var timerText: TextView
    lateinit var driftText: TextView
    lateinit var banner: TextView

    private var throttle = 0f
    private var brake = 0f
    private var steerL = false
    private var steerR = false
    private var wheelMode = false
    private var wheelSteer = 0f
    private lateinit var wheelView: SteerWheelView
    private lateinit var steerRow: android.widget.LinearLayout
    private var handbrake = false

    val districtNames = arrayOf("الساحة المركزية", "تعز", "صنعاء", "إب", "عدن", "حضرموت", "الصحراء المفتوحة")
    private var lastDistrict = -1

    fun build(root: FrameLayout) {
        val d = UiKit.dp(act, 1f)
        panel = FrameLayout(act)
        panel.setOnApplyWindowInsetsListener { v, insets ->
            var l = insets.systemWindowInsetLeft
            var t = insets.systemWindowInsetTop
            var r = insets.systemWindowInsetRight
            var b = insets.systemWindowInsetBottom
            if (android.os.Build.VERSION.SDK_INT >= 28) {
                insets.displayCutout?.let { dc ->
                    l = maxOf(l, dc.safeInsetLeft); t = maxOf(t, dc.safeInsetTop)
                    r = maxOf(r, dc.safeInsetRight); b = maxOf(b, dc.safeInsetBottom)
                }
            }
            v.setPadding(l, t, r, b)
            insets
        }

        speedo = SpeedometerView(act)
        speedo.setFont(UiKit.fontBold)
        panel.addView(speedo, UiKit.lp(UiKit.dp(act, 150f), UiKit.dp(act, 150f), Gravity.TOP or Gravity.CENTER_HORIZONTAL))

        scoreText = UiKit.label(act, "", 15f, Color.WHITE)
        scoreText.setBackgroundColor(Color.argb(110, 10, 12, 20))
        scoreText.setPadding(12 * d, 8 * d, 12 * d, 8 * d)
        val stp = UiKit.lp(-2, -2, Gravity.TOP or Gravity.LEFT)
        stp.setMargins(14 * d, 12 * d, 0, 0)
        panel.addView(scoreText, stp)

        timerText = UiKit.label(act, "", 17f, UiKit.GOLD)
        timerText.setBackgroundColor(Color.argb(110, 10, 12, 20))
        timerText.setPadding(12 * d, 8 * d, 12 * d, 8 * d)
        timerText.visibility = View.GONE
        val ttp = UiKit.lp(-2, -2, Gravity.TOP or Gravity.LEFT)
        ttp.setMargins(14 * d, 60 * d, 0, 0)
        panel.addView(timerText, ttp)

        banner = UiKit.label(act, "", 22f, UiKit.GOLD)
        banner.typeface = UiKit.fontBold
        banner.setBackgroundColor(Color.argb(90, 10, 12, 20))
        banner.setPadding(20 * d, 6 * d, 20 * d, 6 * d)
        banner.alpha = 0f
        val bp = UiKit.lp(-2, -2, Gravity.TOP or Gravity.CENTER_HORIZONTAL)
        bp.setMargins(0, 90 * d, 0, 0)
        panel.addView(banner, bp)

        driftText = UiKit.label(act, "", 20f, Color.rgb(255, 150, 60))
        driftText.typeface = UiKit.fontBold
        driftText.alpha = 0f
        panel.addView(driftText, UiKit.lp(-2, -2, Gravity.CENTER))

        // top-right buttons
        val topR = UiKit.row(act)
        val camB = UiKit.btn(act, "كاميرا", 13f)
        camB.setOnClickListener { cb.onCamera() }
        val menuB = UiKit.btn(act, "القائمة", 13f)
        menuB.setOnClickListener { cb.onExitToMenu() }
        topR.addView(camB, UiKit.margins(UiKit.vlp(UiKit.dp(act, 96f), UiKit.dp(act, 40f)), 4 * d, 0, 4 * d, 0))
        topR.addView(menuB, UiKit.margins(UiKit.vlp(UiKit.dp(act, 96f), UiKit.dp(act, 40f)), 4 * d, 0, 4 * d, 0))
        val trp = UiKit.lp(-2, -2, Gravity.TOP or Gravity.RIGHT)
        trp.setMargins(0, 12 * d, 22 * d, 0)
        panel.addView(topR, trp)

        // steering bottom-left (buttons, or drag wheel when wheelMode)
        steerRow = UiKit.row(act)
        val lb = holdButton("◀", 30f) { down -> steerL = down; sendInput() }
        val rb = holdButton("▶", 30f) { down -> steerR = down; sendInput() }
        steerRow.addView(lb, UiKit.margins(UiKit.vlp(UiKit.dp(act, 92f), UiKit.dp(act, 92f)), 8 * d, 0, 8 * d, 0))
        steerRow.addView(rb, UiKit.margins(UiKit.vlp(UiKit.dp(act, 92f), UiKit.dp(act, 92f)), 8 * d, 0, 8 * d, 0))
        val srp = UiKit.lp(-2, -2, Gravity.BOTTOM or Gravity.LEFT)
        srp.setMargins(22 * d, 0, 0, 22 * d)
        panel.addView(steerRow, srp)
        wheelView = SteerWheelView(act)
        wheelView.onChange = { v -> wheelSteer = v; sendInput() }
        wheelView.visibility = View.GONE
        val wvp = UiKit.lp(UiKit.dp(act, 170f), UiKit.dp(act, 170f), Gravity.BOTTOM or Gravity.LEFT)
        wvp.setMargins(20 * d, 0, 0, 18 * d)
        panel.addView(wheelView, wvp)

        // pedals bottom-right
        val pedalCol = UiKit.column(act)
        val hb = holdButton("فرامل يد", 13f) { down -> handbrake = down; sendInput() }
        pedalCol.addView(hb, UiKit.margins(UiKit.vlp(UiKit.dp(act, 220f), UiKit.dp(act, 38f)), 0, 0, 0, 8 * d))
        val pedalRow = UiKit.row(act)
        val brk = holdButton("فرامل", 15f) { down -> brake = if (down) 1f else 0f; sendInput() }
        val gas = holdButton("وقود", 15f) { down -> throttle = if (down) 1f else 0f; sendInput() }
        pedalRow.addView(brk, UiKit.margins(UiKit.vlp(UiKit.dp(act, 106f), UiKit.dp(act, 110f)), 4 * d, 0, 4 * d, 0))
        pedalRow.addView(gas, UiKit.margins(UiKit.vlp(UiKit.dp(act, 106f), UiKit.dp(act, 110f)), 4 * d, 0, 4 * d, 0))
        pedalCol.addView(pedalRow)
        val pcp = UiKit.lp(-2, -2, Gravity.BOTTOM or Gravity.RIGHT)
        pcp.setMargins(0, 0, 22 * d, 22 * d)
        panel.addView(pedalCol, pcp)

        // cockpit dashboard (interior camera)
        cockpit = CockpitView(act)
        cockpit.setFont(UiKit.fontBold)
        cockpit.visibility = View.GONE
        panel.addView(cockpit, UiKit.lp(-1, UiKit.dp(act, 220f), Gravity.BOTTOM))

        // horn + lights (left of the pedals)
        val miscRow = UiKit.row(act)
        val hornB = holdButton("بوق", 14f) { down -> if (down) cb.onHorn() }
        miscRow.addView(hornB, UiKit.margins(UiKit.vlp(UiKit.dp(act, 80f), UiKit.dp(act, 52f)), 6 * d, 0, 6 * d, 0))
        val lightsB = UiKit.btn(act, "أنوار", 14f)
        lightsB.setOnClickListener { cb.onLights() }
        miscRow.addView(lightsB, UiKit.margins(UiKit.vlp(UiKit.dp(act, 80f), UiKit.dp(act, 52f)), 6 * d, 0, 6 * d, 0))
        val mrp = UiKit.lp(-2, -2, Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL)
        mrp.setMargins(0, 0, 0, 24 * d)
        panel.addView(miscRow, mrp)

        panel.visibility = View.GONE
        root.addView(panel, UiKit.lp(-1, -1, 0))
    }

    private fun holdButton(text: String, sizeSp: Float, onHold: (Boolean) -> Unit): View {
        val b = UiKit.btn(act, text, sizeSp)
        b.setOnTouchListener { _, ev ->
            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> { onHold(true); true }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_POINTER_UP -> { onHold(false); true }
                else -> true
            }
        }
        return b
    }

    fun setSteerWheel(on: Boolean) {
        wheelMode = on
        if (::wheelView.isInitialized) wheelView.visibility = if (on) View.VISIBLE else View.GONE
        if (::steerRow.isInitialized) steerRow.visibility = if (on) View.GONE else View.VISIBLE
    }

    private fun sendInput() {
        val steer = if (wheelMode) wheelSteer else (if (steerL) -1f else 0f) + (if (steerR) 1f else 0f)
        cb.onInput(throttle, brake, steer, handbrake)
    }

    fun onDistrictChanged(d: Int) {
        if (d == lastDistrict) return
        lastDistrict = d
        banner.text = districtNames.getOrElse(d) { "" }
        banner.animate().cancel()
        banner.alpha = 1f
        banner.animate().alpha(0f).setStartDelay(1800).setDuration(900).start()
    }

    fun onDrift(active: Boolean) {
        if (active && driftText.alpha == 0f) {
            driftText.text = "انجراف!"
            driftText.alpha = 1f
        } else if (!active) {
            driftText.alpha = 0f
        }
    }

    fun setDashboard(visible: Boolean) {
        cockpit.visibility = if (visible) View.VISIBLE else View.GONE
    }

    fun resetInput() {
        throttle = 0f; brake = 0f; steerL = false; steerR = false; handbrake = false
        sendInput()
    }
}
