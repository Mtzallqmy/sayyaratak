package com.sayyaratak

import android.app.Activity
import android.app.Instrumentation
import android.content.Intent
import android.graphics.Rect
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.SystemClock
import android.os.ParcelFileDescriptor
import android.view.InputDevice
import android.view.MotionEvent
import android.view.View
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.sin

class DriveInstrumentation : Instrumentation() {
    private data class Point(val x: Float, val y: Float)
    private lateinit var activity: MainActivity
    private var downTime = 0L

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        try {
            runDrive()
            finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\nDRIVE_OK\n") })
        } catch (failure: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", "\n" + android.util.Log.getStackTraceString(failure))
            })
        }
    }

    private fun runDrive() {
        val launch = Intent(targetContext, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        activity = startActivitySync(launch) as MainActivity
        await("first rendered menu") { it.generation > 0 && it.frame > 0 && it.state[NativeBridge.ST_MODE] == 0f }
        val menu = bounds("free_drive")
        tap(menu.center())
        val start = await("free drive") { it.state[NativeBridge.ST_MODE] == NativeBridge.MODE_FREE.toFloat() }

        val gas = bounds("throttle").center()
        val wheelBounds = bounds("steering_wheel")
        val wheel = wheelBounds.center()
        val turned = Point(wheel.x + wheelBounds.width() * 0.24f, wheel.y)
        val brake = bounds("brake").center()

        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, gas)
        val driving = await("throttle input, positive speed and displacement") {
            it.state[NativeBridge.ST_THROTTLE_INPUT] == 1f &&
                it.state[NativeBridge.ST_SPEED] >= 14f && moved(start, it) >= 4f
        }

        val secondPointer = 1 shl MotionEvent.ACTION_POINTER_INDEX_SHIFT
        inject(MotionEvent.ACTION_POINTER_DOWN or secondPointer, gas, wheel)
        inject(MotionEvent.ACTION_MOVE, gas, turned)
        await("simultaneous steer and throttle change vehicle heading") {
            it.state[NativeBridge.ST_THROTTLE_INPUT] == 1f &&
                it.state[NativeBridge.ST_STEER_INPUT] >= 0.3f && turnedSince(driving, it) >= 0.10f
        }
        inject(MotionEvent.ACTION_POINTER_UP or secondPointer, gas, turned)
        val beforeBrake = await("released steering while throttle remains held") {
            it.state[NativeBridge.ST_STEER_INPUT] == 0f && it.state[NativeBridge.ST_THROTTLE_INPUT] == 1f &&
                it.state[NativeBridge.ST_SPEED] >= 14f
        }
        inject(MotionEvent.ACTION_UP, gas)

        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, brake)
        await("braking lowers speed to a stop") {
            it.state[NativeBridge.ST_BRAKE_INPUT] == 1f && it.state[NativeBridge.ST_THROTTLE_INPUT] == 0f &&
                it.state[NativeBridge.ST_SPEED] < 1.5f &&
                beforeBrake.state[NativeBridge.ST_SPEED] - it.state[NativeBridge.ST_SPEED] >= 10f
        }
        SystemClock.sleep(500)
        check(activity.driveSample().state[NativeBridge.ST_SPEED] < 1.5f) { "Holding brake moved the stopped car" }
        inject(MotionEvent.ACTION_UP, brake)
        await("brake released") { it.state[NativeBridge.ST_BRAKE_INPUT] == 0f }

        val gear = bounds("direction_gear").center()
        tap(gear)
        val reverseStart = await("reverse selected at a stop") { it.state[NativeBridge.ST_REVERSE] == 1f }
        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, gas)
        await("vehicle moves backward in reverse") {
            val heading = reverseStart.state[NativeBridge.ST_HEADING]
            val dx = it.state[NativeBridge.ST_X] - reverseStart.state[NativeBridge.ST_X]
            val dz = it.state[NativeBridge.ST_Z] - reverseStart.state[NativeBridge.ST_Z]
            dx * sin(heading) + dz * cos(heading) <= -1f
        }
        inject(MotionEvent.ACTION_UP, gas)
        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, brake)
        await("reverse braking stops the vehicle") { it.state[NativeBridge.ST_SPEED] < 0.5f }
        inject(MotionEvent.ACTION_UP, brake)
        tap(gear)
        val beforePause = await("forward gear restored") {
            it.state[NativeBridge.ST_REVERSE] == 0f && it.state[NativeBridge.ST_BRAKE_INPUT] == 0f
        }

        runOnMainSync {
            activity.findViewById<View>(android.R.id.content)
                .findViewWithTag<GLSurfaceView>("driving_surface").preserveEGLContextOnPause = false
        }
        shell("input keyevent KEYCODE_HOME")
        await("background lifecycle") { !activity.isDrivingActivityResumed }
        shell("am start -W -n com.sayyaratak/.MainActivity")
        val resumed = await("foreground creates a new EGL context and renders free drive") {
            activity.isDrivingActivityResumed && it.generation > beforePause.generation &&
                it.frame > beforePause.frame && it.state[NativeBridge.ST_MODE] == NativeBridge.MODE_FREE.toFloat()
        }
        check(moved(beforePause, resumed) <= 2f) { "Car advanced while app was suspended" }
        check(resumed.state[NativeBridge.ST_THROTTLE_INPUT] == 0f &&
            resumed.state[NativeBridge.ST_BRAKE_INPUT] == 0f &&
            resumed.state[NativeBridge.ST_STEER_INPUT] == 0f) { "Input stuck after resume" }
        val image = checkNotNull(uiAutomation.takeScreenshot()) { "Could not capture resumed scene" }
        try {
            val colors = mutableSetOf<Int>()
            for (row in 2..5) {
                for (column in 2..8) {
                    colors += image.getPixel(image.width * column / 10, image.height * row / 10)
                }
            }
            check(colors.size >= 4) { "Resumed scene did not render a varied image" }
        } finally {
            image.recycle()
        }

        val resumedGas = bounds("throttle").center()
        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, resumedGas)
        await("vehicle can drive again after EGL recreation") {
            it.state[NativeBridge.ST_SPEED] >= 10f && moved(resumed, it) >= 2f
        }
        inject(MotionEvent.ACTION_UP, resumedGas)
        android.util.Log.i("sayya", "Drive smoke passed: speed=${driving.state[NativeBridge.ST_SPEED]} km/h, " +
            "distance=${moved(start, driving)} m, EGL generation=${resumed.generation}")
    }

    private fun shell(command: String) {
        ParcelFileDescriptor.AutoCloseInputStream(uiAutomation.executeShellCommand(command)).use {
            it.readBytes()
        }
    }

    private fun await(label: String, condition: (MainActivity.DriveSample) -> Boolean): MainActivity.DriveSample {
        val deadline = SystemClock.uptimeMillis() + 15_000
        var sample: MainActivity.DriveSample
        do {
            sample = activity.driveSample()
            if (condition(sample)) return sample
            SystemClock.sleep(40)
        } while (SystemClock.uptimeMillis() < deadline)
        error("$label timed out. Frame=${sample.frame}, generation=${sample.generation}, " +
            "state=${sample.state.joinToString()}")
    }

    private fun bounds(tag: String): Rect {
        val bounds = Rect()
        runOnMainSync {
            val view = activity.findViewById<View>(android.R.id.content).findViewWithTag<View>(tag)
            val visible = Rect()
            check(view != null && view.isShown && view.getLocalVisibleRect(visible) && !visible.isEmpty) {
                "Control is missing or hidden: $tag"
            }
            val topLeft = IntArray(2)
            view.getLocationOnScreen(topLeft)
            bounds.set(visible)
            bounds.offset(topLeft[0], topLeft[1])
        }
        return bounds
    }

    private fun Rect.center() = Point(exactCenterX(), exactCenterY())

    private fun tap(point: Point) {
        downTime = SystemClock.uptimeMillis()
        inject(MotionEvent.ACTION_DOWN, point)
        inject(MotionEvent.ACTION_UP, point)
    }

    private fun inject(action: Int, vararg points: Point) {
        val properties = Array(points.size) { index ->
            MotionEvent.PointerProperties().apply { id = index; toolType = MotionEvent.TOOL_TYPE_FINGER }
        }
        val coordinates = Array(points.size) { index ->
            MotionEvent.PointerCoords().apply {
                x = points[index].x; y = points[index].y; pressure = 1f; size = 1f
            }
        }
        val event = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(), action, points.size,
            properties, coordinates, 0, 0, 1f, 1f, 0, 0, InputDevice.SOURCE_TOUCHSCREEN, 0)
        try {
            check(uiAutomation.injectInputEvent(event, true)) { "Touch injection failed for action $action" }
        } finally {
            event.recycle()
        }
    }

    private fun moved(from: MainActivity.DriveSample, to: MainActivity.DriveSample) =
        hypot(to.state[NativeBridge.ST_X] - from.state[NativeBridge.ST_X],
            to.state[NativeBridge.ST_Z] - from.state[NativeBridge.ST_Z])

    private fun turnedSince(from: MainActivity.DriveSample, to: MainActivity.DriveSample): Float {
        val change = to.state[NativeBridge.ST_HEADING] - from.state[NativeBridge.ST_HEADING]
        return abs(atan2(sin(change), cos(change)))
    }
}
