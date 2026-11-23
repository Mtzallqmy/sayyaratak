package com.sayyaratak

object NativeBridge {
    init { System.loadLibrary("sayya") }

    external fun nativeInit()
    external fun nativeSurfaceCreated()
    external fun nativeSurfaceChanged(w: Int, h: Int)
    external fun nativeDrawFrame()
    external fun nativeGetState(out: FloatArray)
    external fun nativeHorn()
    external fun nativeRepair()
    external fun nativeRefuel()
    external fun nativeSetLights(on: Boolean)
    external fun nativeSetDriveMode(m: Int)
    external fun nativeSetInput(throttle: Float, brake: Float, steer: Float, handbrake: Boolean)
    external fun nativeCycleCamera(): Int
    external fun nativeSetMode(mode: Int)
    external fun nativeSetPaint(r: Float, g: Float, b: Float)
    external fun nativeSetWheels(style: Int)
    external fun nativeSetSpoiler(on: Boolean)
    external fun nativeSetNeon(on: Boolean)
    external fun nativeSetTint(on: Boolean)
    external fun nativeSetEngineProfile(p: Int)
    external fun nativeSetQuality(q: Int)
    external fun nativeSetAudioEnabled(on: Boolean)
    external fun nativeTeleport(district: Int)
    external fun nativeGetWorldData(): FloatArray
    external fun nativeUploadTexture(slot: Int, pixels: IntArray, w: Int, h: Int, mip: Boolean)
    external fun nativePreviewRev()
    external fun nativeGarageRotate(delta: Float)
    external fun nativeGetGlError(): String
    external fun nativeGetGlInfo(): String
    external fun nativeLoadModel(hero: ByteArray, lod: ByteArray)
    external fun nativeStartMission(city: Int, type: Int)
    external fun nativeAbortMission()
    external fun nativePauseAudio()
    external fun nativeResumeAudio()

    const val MODE_MENU = 0
    const val MODE_WORKSHOP = 1
    const val MODE_FREE = 2
    const val MODE_CHALLENGE = 3
    const val MODE_MISSION = 5

    // state array indices
    const val ST_SPEED = 0
    const val ST_SCORE = 1
    const val ST_DRIFT = 2
    const val ST_TIMELEFT = 3
    const val ST_HITS = 4
    const val ST_MODE = 5
    const val ST_FPS = 6
    const val ST_CAMERA = 7
    const val ST_DISTRICT = 8
    const val ST_STARS = 9
    const val ST_BEST = 10
    const val ST_OFFROAD = 11
    const val ST_STEER = 10
    const val ST_RPMGEAR = 16
    const val ST_DAMAGE = 17
    const val ST_SLIP = 18
    const val ST_DRIVEMODE = 19
    const val ST_FUEL = 20
    const val ST_TEMP = 21
    const val ST_CHALLENGE_BEST = 12

    // texture slots
    const val ST_MISSION_TYPE = 12
    const val ST_MISSION_TIME = 13
    const val ST_MISSION_PROG = 14
    const val ST_MISSION_STATE = 15
    const val TEX_GROUND = 0
    const val TEX_BLDG_MODERN = 1
    const val TEX_BLDG_SANAA = 2
    const val TEX_BLDG_MUD = 3
    const val TEX_BLDG_WHITE = 4
    const val TEX_PALM = 5
    const val TEX_PARTICLE = 6
    const val TEX_SKY = 8
    const val TEX_ASPHALT = 7
}
