package com.cryptoscanner.network

import android.util.Log
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import java.math.BigDecimal
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * Bitcoin balance checker using three free public APIs (no key required).
 *
 * API priority:
 *  1. blockchain.info  – parses {"<addr>":{"final_balance": N}}
 *  2. mempool.space    – parses chain_stats.funded_txo_sum - chain_stats.spent_txo_sum
 *  3. blockstream.info – same structure as mempool.space
 *
 * All balances are in satoshis (Long).
 * 1 BTC = 100_000_000 satoshis.
 */
class BitcoinApi {

    companion object {
        private const val TAG = "BitcoinApi"
        private const val SATOSHIS_PER_BTC = 100_000_000L
    }

    private val client: OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(20, TimeUnit.SECONDS)
        .build()

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------

    /**
     * Returns the confirmed balance of [address] in satoshis.
     * Tries all three APIs via [ApiRotator].
     */
    suspend fun getBalance(address: String): Long = withContext(Dispatchers.IO) {
        val rotator = ApiRotator(
            endpoints = listOf(
                { fetchFromBlockchainInfo(address) },
                { fetchFromMempoolSpace(address) },
                { fetchFromBlockstream(address) }
            ),
            maxRetriesPerEndpoint = 2,
            baseDelayMs = 500
        )
        rotator.execute()
    }

    /**
     * Formats satoshis as a human-readable BTC string, e.g. "0.00123456 BTC".
     */
    fun formatBalance(satoshis: Long): String {
        val btc = BigDecimal(satoshis).divide(BigDecimal(SATOSHIS_PER_BTC))
        return "${btc.toPlainString()} BTC"
    }

    // ---------------------------------------------------------------------------
    // Private endpoint implementations
    // ---------------------------------------------------------------------------

    private fun fetchFromBlockchainInfo(address: String): Long {
        val url = "https://blockchain.info/balance?active=$address"
        Log.d(TAG, "blockchain.info → $url")
        val body = getBody(url)
        // Response: {"<address>":{"final_balance":N,...},...}
        val obj = JSONObject(body)
        val addrObj = obj.getJSONObject(address)
        return addrObj.getLong("final_balance")
    }

    private fun fetchFromMempoolSpace(address: String): Long {
        val url = "https://mempool.space/api/address/$address"
        Log.d(TAG, "mempool.space → $url")
        val body = getBody(url)
        return parseChainStats(body)
    }

    private fun fetchFromBlockstream(address: String): Long {
        val url = "https://blockstream.info/api/address/$address"
        Log.d(TAG, "blockstream.info → $url")
        val body = getBody(url)
        return parseChainStats(body)
    }

    /**
     * Parses mempool.space / blockstream.info format:
     * { "chain_stats": { "funded_txo_sum": N, "spent_txo_sum": M }, ... }
     */
    private fun parseChainStats(body: String): Long {
        val obj = JSONObject(body)
        val stats = obj.getJSONObject("chain_stats")
        val funded = stats.getLong("funded_txo_sum")
        val spent = stats.getLong("spent_txo_sum")
        return funded - spent
    }

    // ---------------------------------------------------------------------------
    // HTTP helper
    // ---------------------------------------------------------------------------

    private fun getBody(url: String): String {
        val request = Request.Builder()
            .url(url)
            .header("User-Agent", "CryptoScanner/1.0")
            .build()

        client.newCall(request).execute().use { response ->
            when {
                response.code == 429 ->
                    throw RateLimitException(
                        retryAfterMs = response.header("Retry-After")
                            ?.toLongOrNull()?.times(1000) ?: 2000
                    )
                response.code >= 500 ->
                    throw ApiException("Server error from $url", response.code)
                !response.isSuccessful ->
                    throw ApiException("HTTP ${response.code} from $url", response.code)
            }
            return response.body?.string()
                ?: throw ApiException("Empty body from $url")
        }
    }
}
