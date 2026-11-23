package com.sayyaratak

import android.app.Activity
import android.content.Context
import android.graphics.Typeface
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.WindowManager
import android.widget.FrameLayout
import javax.microedition.khronos.egl.EGL10
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.egl.EGLDisplay
import javax.microedition.khronos.opengles.GL10

class MainActivity : Activity() {
    private lateinit var root: FrameLayout
    private lateinit var glView: GLSurfaceView
    private lateinit var menu: MenuUi
    private lateinit var workshop: WorkshopUi
    private lateinit var settings: SettingsUi
    private lateinit var hud: HudUi
    private val handler = Handler(Looper.getMainLooper())
    private val state = FloatArray(16)
    private var mode = NativeBridge.MODE_MENU
    private var challengeEnded = false
    @Volatile private var frameFailed = false

    private val prefs by lazy { getSharedPreferences("sayyaratak", Context.MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        ErrorTrap.install(this)
        try {
            onCreateSafe(savedInstanceState)
        } catch (e: Throwable) {
            ErrorTrap.show(this, "تعذّر بدء اللعبة", ErrorTrap.stackOf(e))
        }
    }

    private fun onCreateSafe(savedInstanceState: Bundle?) {
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        hideSystemUi()

        try {
            UiKit.font = Typeface.createFromAsset(assets, "fonts/Tajawal-Regular.ttf")
            UiKit.fontBold = Typeface.createFromAsset(assets, "fonts/Tajawal-Bold.ttf")
        } catch (e: Throwable) {
            UiKit.font = Typeface.DEFAULT
            UiKit.fontBold = Typeface.DEFAULT_BOLD
        }

        try {
            NativeBridge.nativeInit()
        try {
            val hero = assets.open("models/car.bin").readBytes()
            val lod = assets.open("models/car_lod.bin").readBytes()
            NativeBridge.nativeLoadModel(hero, lod)
        } catch (t: Throwable) { /* fall back to procedural car */ }
        } catch (e: Throwable) {
            ErrorTrap.show(this, "تعذّر تحميل محرك اللعبة", ErrorTrap.stackOf(e))
            return
        }

        root = FrameLayout(this)
        glView = GLSurfaceView(this)
        glView.setEGLContextClientVersion(3)
        glView.setEGLConfigChooser(MsaaChooser())
        glView.preserveEGLContextOnPause = true
        glView.setRenderer(object : GLSurfaceView.Renderer {
            override fun onSurfaceCreated(unused: GL10?, config: EGLConfig?) {
                try {
                    NativeBridge.nativeSurfaceCreated()
                    uploadTextures()
                    val glErr = NativeBridge.nativeGetGlError()
                    if (glErr.isNotEmpty()) {
                        val info = NativeBridge.nativeGetGlInfo()
                        runOnUiThread { ErrorTrap.show(this@MainActivity, "تعذّر تجهيز الرسوم (صوّر الشاشة)", glErr + "\n" + info) }
                    }
                } catch (e: Throwable) {
                    runOnUiThread { ErrorTrap.show(this@MainActivity, "خطأ في الرسوم", ErrorTrap.stackOf(e)) }
                }
            }
            override fun onSurfaceChanged(unused: GL10?, w: Int, h: Int) {
                NativeBridge.nativeSurfaceChanged(w, h)
            }
            override fun onDrawFrame(unused: GL10?) {
                if (frameFailed) return
                try {
                    NativeBridge.nativeDrawFrame()
                } catch (e: Throwable) {
                    frameFailed = true
                    runOnUiThread { ErrorTrap.show(this@MainActivity, "خطأ أثناء الرسم", ErrorTrap.stackOf(e)) }
                }
            }
        })
        root.addView(glView)

        hud = HudUi(this, object : HudUi.Callbacks {
            override fun onInput(t: Float, b: Float, s: Float, hb: Boolean) {
                glView.queueEvent { NativeBridge.nativeSetInput(t, b, s, hb) }
            }
            override fun onCamera() { glView.queueEvent { NativeBridge.nativeCycleCamera() } }
            override fun onExitToMenu() { showMenu() }
        })
        hud.build(root)

        menu = MenuUi(this, object : MenuUi.Callbacks {
            override fun onStartDrive(district: Int) { startDrive(district) }
            override fun onStartChallenge() { startChallenge() }
            override fun onOpenWorkshop() { showWorkshop() }
            override fun onOpenSettings() { showSettings() }
        })
        menu.build(root)

        workshop = WorkshopUi(this, object : WorkshopUi.Callbacks {
            override fun onPaint(r: Float, g: Float, b: Float) {
                prefs.edit().putFloat("pr", r).putFloat("pg", g).putFloat("pb", b).apply()
                glView.queueEvent { NativeBridge.nativeSetPaint(r, g, b) }
            }
            override fun onWheels(style: Int) {
                prefs.edit().putInt("wheels", style).apply()
                glView.queueEvent { NativeBridge.nativeSetWheels(style) }
            }
            override fun onSpoiler(on: Boolean) {
                prefs.edit().putBoolean("spoiler", on).apply()
                glView.queueEvent { NativeBridge.nativeSetSpoiler(on) }
            }
            override fun onNeon(on: Boolean) {
                prefs.edit().putBoolean("neon", on).apply()
                glView.queueEvent { NativeBridge.nativeSetNeon(on) }
            }
            override fun onTint(on: Boolean) {
                prefs.edit().putBoolean("tint", on).apply()
                glView.queueEvent { NativeBridge.nativeSetTint(on) }
            }
            override fun onEngine(p: Int) {
                prefs.edit().putInt("engine", p).apply()
                glView.queueEvent { NativeBridge.nativeSetEngineProfile(p) }
            }
            override fun onPreviewRev() { glView.queueEvent { NativeBridge.nativePreviewRev() } }
            override fun onCloseWorkshop() { showMenu() }
        })
        workshop.build(root)

        settings = SettingsUi(this, object : SettingsUi.Callbacks {
            override fun onQuality(q: Int) {
                prefs.edit().putInt("quality", q).apply()
                glView.queueEvent { NativeBridge.nativeSetQuality(q) }
            }
            override fun onAudio(on: Boolean) {
                prefs.edit().putBoolean("audio", on).apply()
                glView.queueEvent { NativeBridge.nativeSetAudioEnabled(on) }
            }
            override fun onDefaultCamera(cam: Int) {
                prefs.edit().putInt("camera", cam).apply()
            }
            override fun onResetProgress() {
                prefs.edit().putInt("stars", 0).putInt("best", 0).putInt("cbest", 0).apply()
                refreshBest()
            }
            override fun onCloseSettings() { showMenu() }
        })
        settings.build(root)

        setContentView(root)
        ErrorTrap.showPreviousIfAny(this, root)

        // garage drag to rotate (workshop/menu)
        var lastX = 0f
        glView.setOnTouchListener { _, ev ->
            if (mode == NativeBridge.MODE_WORKSHOP || mode == NativeBridge.MODE_MENU) {
                when (ev.actionMasked) {
                    android.view.MotionEvent.ACTION_DOWN -> lastX = ev.x
                    android.view.MotionEvent.ACTION_MOVE -> {
                        val dx = ev.x - lastX; lastX = ev.x
                        glView.queueEvent { NativeBridge.nativeGarageRotate(dx * 0.01f) }
                    }
                }
            }
            false
        }

        applyPrefs()
        showMenu()
        handler.post(hudTick)
    }

    private fun applyPrefs() {
        val r = prefs.getFloat("pr", 0.62f); val g = prefs.getFloat("pg", 0.05f); val b = prefs.getFloat("pb", 0.06f)
        workshop.syncState(prefs.getBoolean("spoiler", true), prefs.getBoolean("neon", false), prefs.getBoolean("tint", true))
        settings.sync(prefs.getInt("quality", 1), prefs.getBoolean("audio", true), prefs.getInt("camera", 0))
        glView.queueEvent {
            NativeBridge.nativeSetPaint(r, g, b)
            NativeBridge.nativeSetWheels(prefs.getInt("wheels", 0))
            NativeBridge.nativeSetSpoiler(prefs.getBoolean("spoiler", true))
            NativeBridge.nativeSetNeon(prefs.getBoolean("neon", false))
            NativeBridge.nativeSetTint(prefs.getBoolean("tint", true))
            NativeBridge.nativeSetEngineProfile(prefs.getInt("engine", 1))
            NativeBridge.nativeSetQuality(prefs.getInt("quality", 1))
            NativeBridge.nativeSetAudioEnabled(prefs.getBoolean("audio", true))
        }
    }

    private fun uploadTextures() {
        val data = NativeBridge.nativeGetWorldData()
        fun assetBmp(name: String): Bitmap? = try {
            assets.open("tex/" + name).use { BitmapFactory.decodeStream(it) }
        } catch (t: Throwable) { null }
        val sandBmp = assetBmp("sand.jpg")
        val asphaltBmp = assetBmp("asphalt.jpg")
        val plasterBmp = assetBmp("plaster.jpg")
        val ground = WorldPainter.groundPixels(data, 2048, sandBmp, asphaltBmp)
        NativeBridge.nativeUploadTexture(NativeBridge.TEX_GROUND, ground, 2048, 2048, true)
        for (s in 0..3) {
            val px = WorldPainter.buildingPixels(s, 256, plasterBmp)
            NativeBridge.nativeUploadTexture(NativeBridge.TEX_BLDG_MODERN + s, px, 256, 256, true)
        }
        NativeBridge.nativeUploadTexture(NativeBridge.TEX_PALM, WorldPainter.palmPixels(256), 256, 256, true)
        NativeBridge.nativeUploadTexture(NativeBridge.TEX_PARTICLE, WorldPainter.particlePixels(64), 64, 64, false)
    }

    private fun setMode(m: Int) {
        mode = m
        glView.queueEvent { NativeBridge.nativeSetMode(m) }
    }

    private fun startDrive(district: Int) {
        UiKit.hide(menu.panel); UiKit.hide(settings.panel); UiKit.hide(workshop.panel)
        UiKit.show(hud.panel)
        hud.resetInput()
        challengeEnded = false
        hud.timerText.visibility = View.GONE
        setMode(NativeBridge.MODE_FREE)
        if (district >= 0) glView.queueEvent { NativeBridge.nativeTeleport(district) }
    }

    private fun startChallenge() {
        startDrive(-1)
        hud.timerText.visibility = View.VISIBLE
        setMode(NativeBridge.MODE_CHALLENGE)
    }

    private fun showMenu() {
        UiKit.hide(hud.panel); UiKit.hide(settings.panel); UiKit.hide(workshop.panel)
        UiKit.show(menu.panel)
        hud.resetInput()
        refreshBest()
        setMode(NativeBridge.MODE_MENU)
    }

    private fun showWorkshop() {
        UiKit.hide(menu.panel); UiKit.show(workshop.panel)
        setMode(NativeBridge.MODE_WORKSHOP)
    }

    private fun showSettings() {
        UiKit.hide(menu.panel); UiKit.show(settings.panel)
    }

    private fun refreshBest() {
        menu.setBest(prefs.getInt("stars", 0), prefs.getInt("best", 0), prefs.getInt("cbest", 0))
    }

    private val hudTick = object : Runnable {
        override fun run() {
            try {
            NativeBridge.nativeGetState(state)
            if (mode == NativeBridge.MODE_FREE || mode == NativeBridge.MODE_CHALLENGE) {
                hud.speedo.kmh = state[NativeBridge.ST_SPEED]
                hud.speedo.invalidate()
                hud.scoreText.text = "النجوم: ${state[NativeBridge.ST_STARS].toInt()}   •   الانجراف: ${state[NativeBridge.ST_SCORE].toInt()}"
                hud.onDistrictChanged(state[NativeBridge.ST_DISTRICT].toInt())
                hud.onDrift(state[NativeBridge.ST_DRIFT] > 0.5f)
                hud.setDashboard(state[NativeBridge.ST_CAMERA] == 1f)
                // persist bests
                val best = prefs.getInt("best", 0)
                if (state[NativeBridge.ST_SCORE] > best) prefs.edit().putInt("best", state[NativeBridge.ST_SCORE].toInt()).apply()
                val stars = prefs.getInt("stars", 0)
                if (state[NativeBridge.ST_STARS] > stars) prefs.edit().putInt("stars", state[NativeBridge.ST_STARS].toInt()).apply()
                if (mode == NativeBridge.MODE_CHALLENGE) {
                    val tl = state[NativeBridge.ST_TIMELEFT]
                    hud.timerText.text = "الوقت: ${tl.toInt()}   •   الحلقات: ${state[NativeBridge.ST_HITS].toInt()}"
                    if (tl <= 0f && !challengeEnded) {
                        challengeEnded = true
                        val hits = state[NativeBridge.ST_HITS].toInt()
                        val cb = prefs.getInt("cbest", 0)
                        if (hits > cb) prefs.edit().putInt("cbest", hits).apply()
                        hud.banner.text = "انتهى التحدي! حلقات: $hits"
                        hud.banner.animate().cancel()
                        hud.banner.alpha = 1f
                        hud.banner.animate().alpha(0f).setStartDelay(2600).setDuration(900).start()
                        setMode(NativeBridge.MODE_FREE)
                    }
                }
            }
            } catch (_: Throwable) {}
            handler.postDelayed(this, 100)
        }
    }

    private fun hideSystemUi() {
        window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            or View.SYSTEM_UI_FLAG_FULLSCREEN
            or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION)
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) hideSystemUi()
    }

    override fun onPause() {
        super.onPause()
        glView.onPause()
        NativeBridge.nativePauseAudio()
    }

    override fun onResume() {
        super.onResume()
        glView.onResume()
        NativeBridge.nativeResumeAudio()
        hideSystemUi()
    }

    override fun onBackPressed() {
        if (mode != NativeBridge.MODE_MENU) showMenu() else super.onBackPressed()
    }

    private class MsaaChooser : GLSurfaceView.EGLConfigChooser {
        override fun chooseConfig(egl: EGL10, display: EGLDisplay): EGLConfig {
            val configs = arrayOfNulls<EGLConfig>(1)
            val num = IntArray(1)
            val spec = intArrayOf(
                EGL10.EGL_RED_SIZE, 8, EGL10.EGL_GREEN_SIZE, 8, EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8, EGL10.EGL_DEPTH_SIZE, 16,
                EGL10.EGL_RENDERABLE_TYPE, 4,
                EGL10.EGL_SAMPLE_BUFFERS, 1, EGL10.EGL_SAMPLES, 4,
                EGL10.EGL_NONE)
            if (egl.eglChooseConfig(display, spec, configs, 1, num) && num[0] > 0) return configs[0]!!
            // fallback without MSAA
            val spec2 = intArrayOf(
                EGL10.EGL_RED_SIZE, 8, EGL10.EGL_GREEN_SIZE, 8, EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 16, EGL10.EGL_RENDERABLE_TYPE, 4, EGL10.EGL_NONE)
            egl.eglChooseConfig(display, spec2, configs, 1, num)
            return configs[0]!!
        }
    }
}
