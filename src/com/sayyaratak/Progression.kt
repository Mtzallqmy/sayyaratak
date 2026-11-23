package com.sayyaratak

import android.content.Context

// Persistent progression: balance, level, mission results, workshop unlocks.
object Progression {
    private const val PREF = "sayyaratak_progress"

    fun balance(ctx: Context) = p(ctx).getInt("balance", 0)
    fun xp(ctx: Context) = p(ctx).getInt("xp", 0)
    fun level(ctx: Context) = (xp(ctx) / 500) + 1
    fun xpToNext(ctx: Context) = 500 - (xp(ctx) % 500)

    fun addEarning(ctx: Context, amount: Int, xpGain: Int = amount) {
        p(ctx).edit()
            .putInt("balance", balance(ctx) + amount)
            .putInt("xp", xp(ctx) + xpGain)
            .apply()
    }

    fun missionDone(ctx: Context, city: Int, type: Int) = p(ctx).getBoolean("m_${city}_${type}", false)
    fun missionBest(ctx: Context, city: Int, type: Int) = p(ctx).getInt("mb_${city}_${type}", 0)

    // returns the reward actually paid (full first time, 25% on repeats)
    fun completeMission(ctx: Context, city: Int, type: Int, reward: Int, best: Int): Int {
        val first = !missionDone(ctx, city, type)
        val pay = if (first) reward else reward / 4
        val e = p(ctx).edit().putBoolean("m_${city}_${type}", true)
        if (best > missionBest(ctx, city, type)) e.putInt("mb_${city}_${type}", best)
        e.apply()
        addEarning(ctx, pay)
        return pay
    }

    fun unlocked(ctx: Context, item: String) = p(ctx).getBoolean("u_$item", false)
    fun buy(ctx: Context, item: String, price: Int): Boolean {
        if (unlocked(ctx, item)) return true
        if (balance(ctx) < price) return false
        p(ctx).edit().putBoolean("u_$item", true).putInt("balance", balance(ctx) - price).apply()
        return true
    }

    fun resetAll(ctx: Context) = p(ctx).edit().clear().apply()

    private fun p(ctx: Context) = ctx.getSharedPreferences(PREF, Context.MODE_PRIVATE)

    val PRICES = mapOf(
        "paint4" to 150, "paint5" to 150, "paint6" to 150, "paint7" to 150,
        "wheels1" to 250, "wheels2" to 250,
        "spoiler" to 400, "neon" to 600, "tint" to 250,
        "engine0" to 200, "engine2" to 350
    )

    val MISSION_REWARDS = mapOf(1 to 300, 2 to 250, 3 to 350)
    val MISSION_NAMES = mapOf(1 to "سباق الحلقات", 2 to "التوصيل السريع", 3 to "تحدي الانجراف")
}
