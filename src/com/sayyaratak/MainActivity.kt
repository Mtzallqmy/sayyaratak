package com.sayyaratak

import android.app.Activity
import android.content.Context
import android.graphics.Typeface
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.opengl.GLSurfaceView
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
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
    private lateinit var missions: MissionUi
    private lateinit var workshop: WorkshopUi
    private lateinit var settings: SettingsUi
    private lateinit var hud: HudUi
    private val handler = Handler(Looper.getMainLooper())
    private val state = FloatArray(NativeBridge.STATE_SIZE)
    private val renderState = FloatArray(NativeBridge.STATE_SIZE)
    private val renderLock = Any()
    private var sceneGeneration = 0
    private var frameSample = 0
    private var nextSampleAt = 0L
    private var initialized = false
    @Volatile internal var isDrivingActivityResumed = false
        private set

    internal data class DriveSample(val generation: Int, val frame: Int, val state: FloatArray)

    internal fun driveSample(): DriveSample = synchronized(renderLock) {
        DriveSample(sceneGeneration, frameSample, renderState.copyOf())
    }
    private var mode = NativeBridge.MODE_MENU
    private var lightsOn = false
    private var challengeEnded = false
    private var missionEnded = false
    private var missionCity = 0
    private var missionType = 0
    @Volatile private var frameFailed = false

    private val prefs by lazy { getSharedPreferences("sayyaratak", Context.MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        ErrorTrap.install(this)
        try {
            onCreateSafe()
        } catch (e: Throwable) {
            ErrorTrap.show(this, "تعذّر بدء اللعبة", ErrorTrap.stackOf(e))
        }
    }

    private fun onCreateSafe() {
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
                val hero = assets.open("models/car.bin").use { it.readBytes() }
                val lod = assets.open("models/car_lod.bin").use { it.readBytes() }
                NativeBridge.nativeLoadModel(hero, lod)
            } catch (e: java.io.IOException) {
                Log.w("sayya", "Bundled car model unavailable; using procedural mesh", e)
            }
        } catch (e: Throwable) {
            ErrorTrap.show(this, "تعذّر تحميل محرك اللعبة", ErrorTrap.stackOf(e))
            return
        }

        root = FrameLayout(this).apply { isMotionEventSplittingEnabled = true }
        glView = GLSurfaceView(this).apply { tag = "driving_surface" }
        glView.setEGLContextClientVersion(3)
        glView.setEGLConfigChooser(MsaaChooser())
        glView.preserveEGLContextOnPause = true
        glView.setRenderer(object : GLSurfaceView.Renderer {
            override fun onSurfaceCreated(unused: GL10?, config: EGLConfig?) {
                frameFailed = false
                try {
                    val info = NativeBridge.nativeGetGlInfo()
                    Log.i("sayya", "EGL context: $info")
                    NativeBridge.nativeSurfaceCreated()
                    val error = NativeBridge.nativeGetGlError()
                    check(error.isEmpty()) { "$error\n$info" }
                    uploadTextures()
                    val textureError = NativeBridge.nativeGetGlError()
                    check(textureError.isEmpty()) { "$textureError\n$info" }
                    synchronized(renderLock) { sceneGeneration++ }
                    nextSampleAt = 0L
                } catch (e: Throwable) {
                    reportFrameFailure("خطأ في الرسوم", e)
                }
            }

            override fun onSurfaceChanged(unused: GL10?, w: Int, h: Int) {
                NativeBridge.nativeSurfaceChanged(w, h)
            }

            override fun onDrawFrame(unused: GL10?) {
                if (frameFailed) return
                try {
                    NativeBridge.nativeDrawFrame()
                    val now = SystemClock.uptimeMillis()
                    if (now >= nextSampleAt) {
                        synchronized(renderLock) {
                            NativeBridge.nativeGetState(renderState)
                            frameSample++
                        }
                        nextSampleAt = now + 50
                    }
                } catch (e: Throwable) {
                    reportFrameFailure("خطأ أثناء الرسم", e)
                }
            }
        })
        root.addView(glView)

        hud = HudUi(this, object : HudUi.Callbacks {
            override fun onHorn() { glView.queueEvent { NativeBridge.nativeHorn() } }
            override fun onLights() {
                lightsOn = !lightsOn
                glView.queueEvent { NativeBridge.nativeSetLights(lightsOn) }
            }
            override fun onInput(t: Float, b: Float, s: Float, hb: Boolean) {
                glView.queueEvent { NativeBridge.nativeSetInput(t, b, s, hb) }
            }
            override fun onCamera() { glView.queueEvent { NativeBridge.nativeCycleCamera() } }
            override fun onGear() { glView.queueEvent { NativeBridge.nativeToggleReverse() } }
            override fun onExitToMenu() { showMenu() }
        })
        hud.build(root)
        hud.setSteerWheel(prefs.getBoolean("steerwheel", true))

        menu = MenuUi(this, object : MenuUi.Callbacks {
            override fun onStartDrive(district: Int) { startDrive(district) }
            override fun onStartChallenge() { startChallenge() }
            override fun onOpenMissions() { showMissions() }
            override fun onOpenWorkshop() { showWorkshop() }
            override fun onOpenSettings() { showSettings() }
        })
        menu.build(root)

        missions = MissionUi(this, object : MissionUi.Callbacks {
            override fun onStartMission(city: Int, type: Int) { startMission(city, type) }
            override fun onCloseMissions() { showMenu() }
        })
        missions.build(root)

        workshop = WorkshopUi(this, object : WorkshopUi.Callbacks {
            override fun onPaint(r: Float, g: Float, b: Float) {
                val idx = paintIndex(r, g, b)
                if (idx >= 4 && !gate("paint$idx")) return@onPaint
                prefs.edit().putFloat("pr", r).putFloat("pg", g).putFloat("pb", b).apply()
                glView.queueEvent { NativeBridge.nativeSetPaint(r, g, b) }
            }
            override fun onWheels(style: Int) {
                if (style > 0 && !gate("wheels$style")) return@onWheels
                prefs.edit().putInt("wheels", style).apply()
                glView.queueEvent { NativeBridge.nativeSetWheels(style) }
            }
            override fun onSpoiler(on: Boolean) {
                if (on && !gate("spoiler")) return@onSpoiler
                prefs.edit().putBoolean("spoiler", on).apply()
                glView.queueEvent { NativeBridge.nativeSetSpoiler(on) }
            }
            override fun onNeon(on: Boolean) {
                if (on && !gate("neon")) return@onNeon
                prefs.edit().putBoolean("neon", on).apply()
                glView.queueEvent { NativeBridge.nativeSetNeon(on) }
            }
            override fun onTint(on: Boolean) {
                if (on && !gate("tint")) return@onTint
                prefs.edit().putBoolean("tint", on).apply()
                glView.queueEvent { NativeBridge.nativeSetTint(on) }
            }
            override fun onEngine(p: Int) {
                if (p != 1 && !gate("engine$p")) return@onEngine
                prefs.edit().putInt("engine", p).apply()
                glView.queueEvent { NativeBridge.nativeSetEngineProfile(p) }
            }
            override fun onPreviewRev() { glView.queueEvent { NativeBridge.nativePreviewRev() } }
            override fun onRepairCost(): Int = (state[NativeBridge.ST_DAMAGE] * 300f).toInt()
            override fun onRepair(): Boolean {
                val cost = onRepairCost()
                if (cost <= 0) return true
                if (!Progression.spend(this@MainActivity, cost)) return false
                glView.queueEvent { NativeBridge.nativeRepair() }
                refreshBest()
                return true
            }
            override fun onRefuel(): Boolean {
                if (state[NativeBridge.ST_FUEL] > 0.98f) return true
                if (!Progression.spend(this@MainActivity, 50)) return false
                glView.queueEvent { NativeBridge.nativeRefuel() }
                refreshBest()
                return true
            }
            override fun onDriveMode(m: Int): Boolean {
                if (m == 1 && !Progression.unlocked(this@MainActivity, "awd")) {
                    if (!Progression.buy(this@MainActivity, "awd", 800)) return false
                }
                if (m == 2 && !Progression.unlocked(this@MainActivity, "awd")) return false
                prefs.edit().putInt("drivemode", m).apply()
                glView.queueEvent { NativeBridge.nativeSetDriveMode(m) }
                refreshBest()
                return true
            }
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
            override fun onSteerWheel(on: Boolean) {
                prefs.edit().putBoolean("steerwheel", on).apply()
                hud.setSteerWheel(on)
            }
            override fun onResetProgress() {
                prefs.edit().putInt("stars", 0).putInt("best", 0).putInt("cbest", 0).apply()
                refreshBest()
            }
            override fun onCloseSettings() { showMenu() }
        })
        settings.build(root)

        setContentView(root)
        root.setOnApplyWindowInsetsListener { v, ins ->
            var l = ins.systemWindowInsetLeft; var t = ins.systemWindowInsetTop
            var r = ins.systemWindowInsetRight; var b = ins.systemWindowInsetBottom
            if (Build.VERSION.SDK_INT >= 28) ins.displayCutout?.let { cut ->
                l = maxOf(l, cut.safeInsetLeft); t = maxOf(t, cut.safeInsetTop)
                r = maxOf(r, cut.safeInsetRight); b = maxOf(b, cut.safeInsetBottom)
            }
            v.setPadding(l, t, r, b)
            (v as? android.view.ViewGroup)?.clipToPadding = false
            ins
        }
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
        initialized = true
    }

    private fun reportFrameFailure(title: String, failure: Throwable) {
        frameFailed = true
        Log.e("sayya", title, failure)
        runOnUiThread { if (!isFinishing && !isDestroyed) ErrorTrap.show(this, title, ErrorTrap.stackOf(failure)) }
    }

    private fun applyPrefs() {
        val r = prefs.getFloat("pr", 0.62f); val g = prefs.getFloat("pg", 0.05f); val b = prefs.getFloat("pb", 0.06f)
        workshop.syncState(prefs.getBoolean("spoiler", true), prefs.getBoolean("neon", false), prefs.getBoolean("tint", true))
        settings.sync(prefs.getInt("quality", 1), prefs.getBoolean("audio", true), prefs.getInt("camera", 0))
        settings.syncWheel(prefs.getBoolean("steerwheel", true))
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
        NativeBridge.nativeUploadTexture(NativeBridge.TEX_ASPHALT, WorldPainter.roadPixels(256, asphaltBmp), 256, 256, true)
        assetBmp("sky.jpg")?.let { sky ->
            val s2048 = android.graphics.Bitmap.createScaledBitmap(sky, 1024, 512, true)
            val px = IntArray(1024 * 512)
            s2048.getPixels(px, 0, 1024, 0, 0, 1024, 512)
            NativeBridge.nativeUploadTexture(NativeBridge.TEX_SKY, px, 1024, 512, true)
        }
    }

    private fun setMode(m: Int) {
        mode = m
        glView.queueEvent { NativeBridge.nativeSetMode(m) }
    }

    private fun gate(item: String): Boolean {
        val price = Progression.PRICES[item] ?: 0
        if (price == 0 || Progression.unlocked(this, item)) return true
        if (Progression.buy(this, item, price)) {
            android.widget.Toast.makeText(this, "تم الشراء! -$price رصيد", android.widget.Toast.LENGTH_SHORT).show()
            return true
        }
        android.widget.Toast.makeText(this, "رصيدك ما يكفي (تحتاج $price - أكمل مهاماً)", android.widget.Toast.LENGTH_LONG).show()
        workshop.syncState(prefs.getBoolean("spoiler", true), prefs.getBoolean("neon", false), prefs.getBoolean("tint", true))
        return false
    }

    private fun paintIndex(r: Float, g: Float, b: Float): Int {
        val paints = arrayOf(
            floatArrayOf(0.62f,0.05f,0.06f), floatArrayOf(0.08f,0.14f,0.38f),
            floatArrayOf(0.92f,0.90f,0.86f), floatArrayOf(0.06f,0.06f,0.08f),
            floatArrayOf(0.80f,0.62f,0.22f), floatArrayOf(0.05f,0.38f,0.20f),
            floatArrayOf(0.90f,0.42f,0.08f), floatArrayOf(0.35f,0.14f,0.50f))
        for (i in paints.indices) {
            val p = paints[i]
            if (kotlin.math.abs(p[0]-r) < 0.02f && kotlin.math.abs(p[1]-g) < 0.02f && kotlin.math.abs(p[2]-b) < 0.02f) return i
        }
        return 0
    }

    private fun startMission(city: Int, type: Int) {
        UiKit.hide(menu.panel); UiKit.hide(settings.panel); UiKit.hide(workshop.panel); UiKit.hide(missions.panel)
        UiKit.show(hud.panel)
        hud.resetInput()
        missionEnded = false
        missionCity = city; missionType = type
        hud.timerText.visibility = View.VISIBLE
        mode = NativeBridge.MODE_MISSION
        glView.queueEvent { NativeBridge.nativeStartMission(city, type) }
    }

    private fun showMissions() {
        UiKit.hide(menu.panel)
        missions.refresh()
        UiKit.show(missions.panel)
    }

    private fun startDrive(district: Int) {
        UiKit.hide(menu.panel); UiKit.hide(settings.panel); UiKit.hide(workshop.panel); UiKit.hide(missions.panel)
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
        if (mode == NativeBridge.MODE_MISSION) glView.queueEvent { NativeBridge.nativeAbortMission() }
        UiKit.hide(hud.panel); UiKit.hide(settings.panel); UiKit.hide(workshop.panel); UiKit.hide(missions.panel)
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
        menu.setProgress(Progression.balance(this), Progression.level(this))
    }

    private val hudTick = object : Runnable {
        override fun run() {
            try {
            synchronized(renderLock) { renderState.copyInto(state) }
            if (mode >= NativeBridge.MODE_FREE) hud.setReverse(state[NativeBridge.ST_REVERSE] == 1f)
            if (mode == NativeBridge.MODE_MISSION) {
                hud.speedo.kmh = state[NativeBridge.ST_SPEED]
                hud.speedo.invalidate()
                hud.onDistrictChanged(state[NativeBridge.ST_DISTRICT].toInt())
                hud.onDrift(state[NativeBridge.ST_DRIFT] > 0.5f)
                hud.setDashboard(state[NativeBridge.ST_CAMERA] == 1f)
                val cp = hud.cockpit
                cp.kmh = state[NativeBridge.ST_SPEED]
                val rp = state[NativeBridge.ST_RPMGEAR]
                cp.rpm01 = kotlin.math.floor(rp) / 10f
                cp.gear = ((rp - kotlin.math.floor(rp)) * 100f + 0.5f).toInt()
                cp.reverse = state[NativeBridge.ST_REVERSE] == 1f
                cp.steer = state[NativeBridge.ST_STEER] / 100f
                cp.fuel = state[NativeBridge.ST_FUEL]
                cp.temp = state[NativeBridge.ST_TEMP]
                cp.damage = state[NativeBridge.ST_DAMAGE]
                cp.invalidate()
                val packed = state[NativeBridge.ST_MISSION_PROG].toInt()
                val prog = packed / 10000; val goal = packed % 10000
                val spacked = state[NativeBridge.ST_MISSION_STATE].toInt()
                val mstate = spacked / 100000; val reward = spacked % 100000
                val mt = state[NativeBridge.ST_MISSION_TYPE].toInt()
                val progLabel = when (mt) { 1 -> "الحلقات"; 2 -> "الوصول"; else -> "النقاط" }
                hud.scoreText.text = "${Progression.MISSION_NAMES[mt]}: $progLabel $prog/$goal"
                hud.timerText.text = "الوقت: ${state[NativeBridge.ST_MISSION_TIME].toInt()}"
                if ((mstate == 2 || mstate == 3) && !missionEnded) {
                    missionEnded = true
                    if (mstate == 2) {
                        val best = if (mt == 3) prog else (state[NativeBridge.ST_MISSION_TIME].toInt())
                        val paid = Progression.completeMission(this@MainActivity, missionCity, missionType, reward, best)
                        hud.banner.text = "مهمة مكتملة! +$paid رصيد"
                    } else {
                        hud.banner.text = "انتهى الوقت - حاول مرة أخرى"
                    }
                    hud.banner.animate().cancel()
                    hud.banner.alpha = 1f
                    hud.banner.animate().alpha(0f).setStartDelay(2800).setDuration(900).start()
                    mode = NativeBridge.MODE_FREE
                }
            }
            if (mode == NativeBridge.MODE_FREE || mode == NativeBridge.MODE_CHALLENGE) {
                hud.speedo.kmh = state[NativeBridge.ST_SPEED]
                hud.speedo.invalidate()
                hud.scoreText.text = "النجوم: ${state[NativeBridge.ST_STARS].toInt()}   •   الانجراف: ${state[NativeBridge.ST_SCORE].toInt()}"
                hud.onDistrictChanged(state[NativeBridge.ST_DISTRICT].toInt())
                hud.onDrift(state[NativeBridge.ST_DRIFT] > 0.5f)
                hud.setDashboard(state[NativeBridge.ST_CAMERA] == 1f)
                val cp = hud.cockpit
                cp.kmh = state[NativeBridge.ST_SPEED]
                val rp = state[NativeBridge.ST_RPMGEAR]
                cp.rpm01 = kotlin.math.floor(rp) / 10f
                cp.gear = ((rp - kotlin.math.floor(rp)) * 100f + 0.5f).toInt()
                cp.reverse = state[NativeBridge.ST_REVERSE] == 1f
                cp.steer = state[NativeBridge.ST_STEER] / 100f
                cp.fuel = state[NativeBridge.ST_FUEL]
                cp.temp = state[NativeBridge.ST_TEMP]
                cp.damage = state[NativeBridge.ST_DAMAGE]
                cp.invalidate()
                // persist bests
                val best = prefs.getInt("best", 0)
                if (state[NativeBridge.ST_SCORE] > best) prefs.edit().putInt("best", state[NativeBridge.ST_SCORE].toInt()).apply()
                val stars = prefs.getInt("stars", 0)
                if (state[NativeBridge.ST_STARS] > stars) {
                    val gained = state[NativeBridge.ST_STARS].toInt() - stars
                    prefs.edit().putInt("stars", state[NativeBridge.ST_STARS].toInt()).apply()
                    Progression.addEarning(this@MainActivity, 25 * gained)
                }
                if (mode == NativeBridge.MODE_CHALLENGE) {
                    val tl = state[NativeBridge.ST_TIMELEFT]
                    hud.timerText.text = "الوقت: ${tl.toInt()}   •   الحلقات: ${state[NativeBridge.ST_HITS].toInt()}"
                    if (tl <= 0f && !challengeEnded) {
                        challengeEnded = true
                        val hits = state[NativeBridge.ST_HITS].toInt()
                        val cb = prefs.getInt("cbest", 0)
                        if (hits > cb) prefs.edit().putInt("cbest", hits).apply()
                        if (hits > 0) Progression.addEarning(this@MainActivity, hits * 20)
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
        if (hasFocus) hideSystemUi() else if (initialized) hud.resetInput()
    }

    override fun onPause() {
        super.onPause()
        isDrivingActivityResumed = false
        if (!initialized) return
        handler.removeCallbacks(hudTick)
        hud.resetInput()
        glView.onPause()
        NativeBridge.nativePauseAudio()
    }

    override fun onResume() {
        super.onResume()
        if (!initialized) return
        glView.queueEvent { NativeBridge.nativeResetClock() }
        glView.onResume()
        NativeBridge.nativeResumeAudio()
        hideSystemUi()
        isDrivingActivityResumed = true
        handler.post(hudTick)
    }

    override fun onBackPressed() {
        if (initialized && mode != NativeBridge.MODE_MENU) showMenu() else super.onBackPressed()
    }

    private class MsaaChooser : GLSurfaceView.EGLConfigChooser {
        override fun chooseConfig(egl: EGL10, display: EGLDisplay): EGLConfig {
            val eglOpenGles3Bit = 0x0040
            val configs = arrayOfNulls<EGLConfig>(1)
            val count = IntArray(1)
            for ((depth, samples) in listOf(24 to 4, 24 to 0, 16 to 0)) {
                val attributes = intArrayOf(
                    EGL10.EGL_RED_SIZE, 8, EGL10.EGL_GREEN_SIZE, 8, EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, depth, EGL10.EGL_RENDERABLE_TYPE, eglOpenGles3Bit,
                    EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT,
                    EGL10.EGL_SAMPLE_BUFFERS, if (samples > 0) 1 else 0,
                    EGL10.EGL_SAMPLES, samples, EGL10.EGL_NONE
                )
                if (egl.eglChooseConfig(display, attributes, configs, 1, count) && count[0] > 0) {
                    requireNotNull(configs[0]).let { return it }
                }
            }
            error("No OpenGL ES 3 window configuration with a depth buffer")
        }
    }
}
